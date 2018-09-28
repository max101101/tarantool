/*
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "tarantool.h"
#include "tarantool/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <pwd.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <sysexits.h>
#if defined(TARGET_OS_LINUX) && defined(HAVE_PRCTL_H)
# include <sys/prctl.h>
#endif
#include <admin.h>
#include <replication.h>
#include <fiber.h>
#include <coeio.h>
#include <iproto.h>
#include "mutex.h"
#include <recovery.h>
#include <crc32.h>
#include <palloc.h>
#include <salloc.h>
#include <say.h>
#include <stat.h>
#include <limits.h>
#include "tarantool/util.h"
extern "C" {
#include <cfg/warning.h>
#include <cfg/tarantool_box_cfg.h>
#include <third_party/gopt/gopt.h>
} /* extern "C" */
#include "tt_pthread.h"
#include "lua/init.h"
#include "memcached.h"
#include "session.h"
#include "box/box.h"
#include "scoped_guard.h"


static pid_t master_pid;
const char *cfg_filename = NULL;
char *cfg_filename_fullpath = NULL;
char *binary_filename;
char *custom_proc_title;
char status[64] = "unknown";
char **main_argv;
int main_argc;
static void *main_opt = NULL;
struct tarantool_cfg cfg;
/** Signals handled after start as part of the event loop. */
static ev_signal ev_sigs[4];
static const int ev_sig_count = sizeof(ev_sigs)/sizeof(*ev_sigs);

int snapshot_pid = 0; /* snapshot processes pid */
uint32_t snapshot_version = 0;

extern const void *opt_def;

/* defined in third_party/proctitle.c */
extern "C" {
char **init_set_proc_title(int argc, char **argv);
void free_proc_title(int argc, char **argv);
void set_proc_title(const char *format, ...);
} /* extern "C" */

static int
core_check_config(struct tarantool_cfg *conf)
{
	if (strindex(wal_mode_STRS, conf->wal_mode,
		     WAL_MODE_MAX) == WAL_MODE_MAX) {
		out_warning(CNF_OK, "wal_mode %s is not recognized", conf->wal_mode);
		return -1;
	}
	return 0;
}

void
title(const char *role, const char *fmt, ...)
{
	(void) role;

	va_list ap;
	char buf[256], *bufptr = buf, *bufend = buf + sizeof(buf);
	char *statusptr = status, *statusend = status + sizeof(status);
	statusptr += snprintf(statusptr, statusend - statusptr, "%s", role);
	bufptr += snprintf(bufptr, bufend - bufptr, "%s%s", role,
			    custom_proc_title);

	if (fmt != NULL) {
		const char *s = statusptr;
		statusptr += snprintf(statusptr, statusend - statusptr, "/");
		va_start(ap, fmt);
		statusptr += vsnprintf(statusptr, statusend - statusptr,
				       fmt, ap);
		va_end(ap);
		bufptr += snprintf(bufptr, bufend - bufptr, "%s", s);
	}

	int ports[] = { cfg.primary_port, cfg.secondary_port,
			cfg.memcached_port, cfg.admin_port,
			cfg.replication_port };
	int *pptr = ports;
	const char *names[] = { "pri", "sec", "memc", "adm", "rpl", NULL };
	const char **nptr = names;

	for (; *nptr; nptr++, pptr++)
		if (*pptr)
			bufptr += snprintf(bufptr, bufend - bufptr,
					   " %s: %i", *nptr, *pptr);
	if (strlen(cfg.bind_ipaddr)) {
		bufptr += snprintf(bufptr, bufend - bufptr,
				   ", ip: %s", cfg.bind_ipaddr);
	}

	set_proc_title(buf);
}

static int
load_cfg(struct tarantool_cfg *conf, int32_t check_rdonly)
{
	FILE *f;
	int32_t n_accepted, n_skipped, n_ignored;

	tbuf_reset(cfg_out);

	if (cfg_filename_fullpath != NULL)
		f = fopen(cfg_filename_fullpath, "r");
	else
		f = fopen(cfg_filename, "r");

	if (f == NULL) {
		out_warning(CNF_OK, "can't open config `%s'", cfg_filename);
		return -1;
	}

	int syntax = parse_cfg_file_tarantool_cfg(conf, f, check_rdonly,
						  &n_accepted,
						  &n_skipped,
						  &n_ignored);
	fclose(f);

	if (syntax != 0)
		return -1;

	if (check_cfg_tarantool_cfg(conf) != 0)
		return -1;

	if (n_skipped != 0)
		return -1;

	if (n_accepted == 0) {
		out_warning(CNF_OK, "empty configuration file '%s'", cfg_filename);
		return -1;
	}

	if (core_check_config(conf) != 0)
		return -1;

	if (replication_check_config(conf) != 0)
		return -1;

	return box_check_config(conf);
}

static int
core_reload_config(const struct tarantool_cfg *old_conf,
		   const struct tarantool_cfg *new_conf)
{
	if (strcasecmp(old_conf->wal_mode, new_conf->wal_mode) != 0 ||
	    old_conf->wal_fsync_delay != new_conf->wal_fsync_delay) {

		double new_delay = new_conf->wal_fsync_delay;

		/* Mode has changed: */
		if (strcasecmp(old_conf->wal_mode, new_conf->wal_mode)) {
			if (strcasecmp(old_conf->wal_mode, "fsync") == 0 ||
			    strcasecmp(new_conf->wal_mode, "fsync") == 0) {
				out_warning(CNF_OK, "wal_mode cannot switch to/from fsync");
				return -1;
			}
		}

		/*
		 * Unless wal_mode=fsync_delay, wal_fsync_delay is
		 * irrelevant and must be 0.
		 */
		if (strcasecmp(new_conf->wal_mode, "fsync_delay") != 0)
			new_delay = 0.0;


		recovery_update_mode(recovery_state, new_conf->wal_mode, new_delay);
	}

	if (old_conf->snap_io_rate_limit != new_conf->snap_io_rate_limit)
		recovery_update_io_rate_limit(recovery_state, new_conf->snap_io_rate_limit);

	if (old_conf->io_collect_interval != new_conf->io_collect_interval)
		ev_set_io_collect_interval(new_conf->io_collect_interval);

	return 0;
}

int
reload_cfg(struct tbuf *out)
{
	static struct mutex *mutex = NULL;
	struct tarantool_cfg new_cfg, aux_cfg;

	if (mutex == NULL) {
		mutex = (struct mutex *) palloc(eter_pool, sizeof(*mutex));
		mutex_create(mutex);
	}

	if (mutex_trylock(mutex) == true) {
		out_warning(CNF_OK, "Could not reload configuration: it is being reloaded right now");
		tbuf_append(out, cfg_out->data, cfg_out->size);

		return -1;
	}


	auto scoped_guard = make_scoped_guard([&] {
		destroy_tarantool_cfg(&aux_cfg);
		destroy_tarantool_cfg(&new_cfg);

		if (cfg_out->size != 0)
			tbuf_append(out, cfg_out->data, cfg_out->size);

		mutex_unlock(mutex);
	});

	init_tarantool_cfg(&new_cfg);
	init_tarantool_cfg(&aux_cfg);

	/*
	  Prepare a copy of the original config file
	   for confetti, so that it can compare the new
	  file with the old one when loading the new file.
	  Load the new file and return an error if it
	  contains a different value for some read-only
	  parameter.
	*/
	if (dup_tarantool_cfg(&aux_cfg, &cfg) != 0 ||
	    load_cfg(&aux_cfg, 1) != 0)
		return -1;
	/*
	  Load the new configuration file, but
	  skip the check for read only parameters.
	  new_cfg contains only defaults and
	  new settings.
	*/
	if (fill_default_tarantool_cfg(&new_cfg) != 0 ||
	    load_cfg(&new_cfg, 0) != 0)
		return -1;

	/* Check that no default value has been changed. */
	char *diff = cmp_tarantool_cfg(&aux_cfg, &new_cfg, 1);
	if (diff != NULL) {
		out_warning(CNF_OK, "Could not accept read only '%s' option", diff);
		return -1;
	}

	/* Process wal-writer-related changes. */
	if (core_reload_config(&cfg, &new_cfg) != 0)
		return -1;

	/* Now pass the config to the module, to take action. */
	if (box_reload_config(&cfg, &new_cfg) != 0)
		return -1;
	/* All OK, activate the config. */
	swap_tarantool_cfg(&cfg, &new_cfg);
	tarantool_lua_load_cfg(tarantool_L, &cfg);

	return 0;
}

/** Print the configuration file in YAML format. */
void
show_cfg(struct tbuf *out)
{
	tarantool_cfg_iterator_t *i;
	char *key, *value;

	tbuf_printf(out, "configuration:" CRLF);
	i = tarantool_cfg_iterator_init();
	while ((key = tarantool_cfg_iterator_next(i, &cfg, &value)) != NULL) {
		if (value) {
			tbuf_printf(out, "  %s: \"%s\"" CRLF, key, value);
			free(value);
		} else {
			tbuf_printf(out, "  %s: (null)" CRLF, key);
		}
	}
}

const char *
tarantool_version(void)
{
	return PACKAGE_VERSION;
}

static double start_time;

double
tarantool_uptime(void)
{
	return ev_now() - start_time;
}

int
snapshot(void)
{
	if (snapshot_pid)
		return EINPROGRESS;


	pid_t p = fork();
	if (p < 0) {
		say_syserror("fork");
		return -1;
	}
	if (p > 0) {
		snapshot_pid = p;
		/* increment snapshot version */
		snapshot_version++;
		int status = wait_for_child(p);
		snapshot_pid = 0;
		return (WIFSIGNALED(status) ? EINTR : WEXITSTATUS(status));
	}

	salloc_protect();

	title("dumper", "%" PRIu32, getppid());
	fiber_set_name(fiber_ptr, status);

	/*
	 * Safety: make sure we don't double-write
	 * parent stdio buffers at exit().
	 */
	close_all_xcpt(1, sayfd);
	/*
	 * We must avoid double destruction of tuples on exit.
	 * Since there is no way to remove existing handlers
	 * registered in the master process, and snapshot_save()
	 * may call exit(), push a top-level handler which will do
	 * _exit() for us.
	 */
	snapshot_save(recovery_state, box_snapshot);

	exit(EXIT_SUCCESS);
	return 0;
}


/**
* Create snapshot from signal handler (SIGUSR1)
*/
static void
sig_snapshot(struct ev_signal *w, int revents)
{
	(void) w;
	(void) revents;

	if (snapshot_pid) {
		say_warn("Snapshot process is already running,"
			" the signal is ignored");
		return;
	}
	fiber_call(fiber_new("snapshot", (fiber_func)snapshot));
}

static void
signal_cb(struct ev_signal *w, int revents)
{
	(void) w;
	(void) revents;

	/* Terminate the main event loop */
	ev_unloop(EV_A_ EVUNLOOP_ALL);
}

/** Try to log as much as possible before dumping a core.
 *
 * Core files are not aways allowed and it takes an effort to
 * extract useful information from them.
 *
 * *Recursive invocation*
 *
 * Unless SIGSEGV is sent by kill(), Linux
 * resets the signal a default value before invoking
 * the handler.
 *
 * Despite that, as an extra precaution to avoid infinite
 * recursion, we count invocations of the handler, and
 * quietly _exit() when called for a second time.
 */
static void
sig_fatal_cb(int signo)
{
	static volatile sig_atomic_t in_cb = 0;
	int fd = STDERR_FILENO;
	struct sigaction sa;

	/* Got a signal while running the handler. */
	if (in_cb) {
		fdprintf(fd, "Fatal %d while backtracing", signo);
		goto end;
	}

	in_cb = 1;

	if (signo == SIGSEGV)
		fdprintf(fd, "Segmentation fault\n");
	else
		fdprintf(fd, "Got a fatal signal %d\n", signo);

	fdprintf(fd, "Current time: %u\n", (unsigned) time(0));
	fdprintf(fd,
		 "Please file a bug at http://github.com/tarantool/tarantool/issues\n"
		 "Attempting backtrace... Note: since the server has "
		 "already crashed, \nthis may fail as well\n");
#ifdef ENABLE_BACKTRACE
	print_backtrace();
#endif
end:
	/* Try to dump core. */
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sigaction(SIGABRT, &sa, NULL);

	abort();
}

/**
 * This SIGTERM handler is only used before the main event loop started to
 * cleanup server pid file. The handler is replaced by ev_signal after the boot.
 * @sa signal_start
 */
static void
sig_term_cb(int signo)
{
	psignal(signo, "");
	/* unlink pidfile. */
	if (cfg.pid_file != NULL)
		unlink(cfg.pid_file);

	_exit(EXIT_SUCCESS);
}

static void
signal_free(void)
{
	int i;
	for (i = 0; i < ev_sig_count; i++)
		ev_signal_stop(&ev_sigs[i]);
}

static void
signal_start(void)
{
	for (int i = 0; i < ev_sig_count; i++)
		ev_signal_start(&ev_sigs[i]);
}

/** Make sure the child has a default signal disposition. */
static void
signal_reset()
{
	struct sigaction sa;

	/* Reset all signals to their defaults. */
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;

	if (sigaction(SIGUSR1, &sa, NULL) == -1 ||
	    sigaction(SIGINT, &sa, NULL) == -1 ||
	    sigaction(SIGTERM, &sa, NULL) == -1 ||
	    sigaction(SIGHUP, &sa, NULL) == -1 ||
	    sigaction(SIGSEGV, &sa, NULL) == -1 ||
	    sigaction(SIGFPE, &sa, NULL) == -1)
		say_syserror("sigaction");

	/* Unblock any signals blocked by libev. */
	sigset_t sigset;
	sigfillset(&sigset);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) == -1)
		say_syserror("sigprocmask");
}


/**
 * Adjust the process signal mask and add handlers for signals.
 */
static void
signal_init(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGPIPE, &sa, 0) == -1)
		panic_syserror("sigaction");

	sa.sa_handler = sig_fatal_cb;

	if (sigaction(SIGSEGV, &sa, 0) == -1 ||
	    sigaction(SIGFPE, &sa, 0) == -1) {
		panic_syserror("sigaction");
	}

	sa.sa_handler = sig_term_cb;
	if (sigaction(SIGUSR1, &sa, 0) == -1 ||
	    sigaction(SIGINT, &sa, 0) == -1  ||
	    sigaction(SIGTERM, &sa, 0) == -1 ||
	    sigaction(SIGHUP, &sa, 0) == -1) {
		panic_syserror("sigaction");
	}

	ev_signal_init(&ev_sigs[0], sig_snapshot, SIGUSR1);
	ev_signal_init(&ev_sigs[1], signal_cb, SIGINT);
	ev_signal_init(&ev_sigs[2], signal_cb, SIGTERM);
	ev_signal_init(&ev_sigs[3], signal_cb, SIGHUP);

	(void) tt_pthread_atfork(NULL, NULL, signal_reset);
}

static void
create_pid(void)
{
	FILE *f;
	char buf[16] = { 0 };
	pid_t pid;

	if (cfg.pid_file == NULL)
		return;

	f = fopen(cfg.pid_file, "a+");
	if (f == NULL)
		panic_syserror("can't open pid file");
	/*
	 * fopen() is not guaranteed to set the seek position to
	 * the beginning of file according to ANSI C (and, e.g.,
	 * on FreeBSD.
	 */
	if (fseeko(f, 0, SEEK_SET) != 0)
		panic_syserror("can't fseek to the beginning of pid file");

	if (fgets(buf, sizeof(buf), f) != NULL && strlen(buf) > 0) {
		pid = strtol(buf, NULL, 10);
		if (pid > 0 && kill(pid, 0) == 0)
			panic("the daemon is already running");
		else
			say_info("updating a stale pid file");
		if (fseeko(f, 0, SEEK_SET) != 0)
			panic_syserror("can't fseek to the beginning of pid file");
		if (ftruncate(fileno(f), 0) == -1)
			panic_syserror("ftruncate(`%s')", cfg.pid_file);
	}

	master_pid = getpid();
	fprintf(f, "%i\n", master_pid);
	fclose(f);
}

/** Run in the background. */
static void
background()
{
	switch (fork()) {
	case -1:
		goto error;
	case 0:                                     /* child */
		break;
	default:                                    /* parent */
		exit(EXIT_SUCCESS);
	}

	if (setsid() == -1)
		goto error;

	/*
	 * Prints to stdout on failure, so got to be done before
	 * we close it.
	 */
	create_pid();

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	return;
error:
	exit(EXIT_FAILURE);
}

void
tarantool_lua_free()
{
	/*
	 * Got to be done prior to anything else, since GC
	 * handlers can refer to other subsystems (e.g. fibers).
	 */
	if (tarantool_L)
		tarantool_lua_close(tarantool_L);
	tarantool_L = NULL;
}

void
tarantool_free(void)
{
	/* Do nothing in a fork. */
	if (getpid() != master_pid)
		return;
	/* Close the file descriptors. */
	iproto_exit();
	replication_exit();
	admin_exit();
	signal_free();
	memcached_free();
	tarantool_lua_free();
	box_free();
	recovery_free();
	stat_free();

	if (cfg_filename_fullpath)
		free(cfg_filename_fullpath);
	if (main_opt)
		gopt_free(main_opt);
	free_proc_title(main_argc, main_argv);

	/* unlink pidfile. */
	if (cfg.pid_file != NULL)
		unlink(cfg.pid_file);
	destroy_tarantool_cfg(&cfg);

	session_free();
	fiber_free();
	palloc_free();
	ev_default_destroy();
#ifdef ENABLE_GCOV
	__gcov_flush();
#endif
#ifdef HAVE_BFD
	symbols_free();
#endif
	/* A hack for cc/ld, see ffisyms.c */
	if (time(NULL) == 0) {
		/* never executed */
		extern void *ffi_symbols[];
		write(0, ffi_symbols, 0);
	}
}

static void
initialize_minimal()
{
	if (!salloc_init(64 * 1000 * 1000, 4, 2))
		panic("can't initialize slab allocator");

	fiber_init();
	coeio_init();
}

int
main(int argc, char **argv)
{
	const char *cfg_paramname = NULL;

#ifndef HAVE_LIBC_STACK_END
/*
 * GNU libc provides a way to get at the top of the stack. This
 * is, of course, not-standard and doesn't work on non-GNU
 * systems, such as FreeBSD. But as far as we're concerned, argv
 * is at the top of the main thread's stack, so save the address
 * of it.
 */
	__libc_stack_end = (void*) &argv;
#endif

	crc32_init();
	stat_init();
	palloc_init();

#ifdef HAVE_BFD
	symbols_load(argv[0]);
#endif

	argv = init_set_proc_title(argc, argv);
	main_argc = argc;
	main_argv = argv;

	void *opt = gopt_sort(&argc, (const char **)argv, opt_def);
	main_opt = opt;
	binary_filename = argv[0];

	if (gopt(opt, 'V')) {
		printf("Tarantool %s\n", tarantool_version());
		printf("Target: %s\n", BUILD_INFO);
		printf("Build options: %s\n", BUILD_OPTIONS);
		printf("Compiler: %s\n", COMPILER_INFO);
		printf("C_FLAGS:%s\n", TARANTOOL_C_FLAGS);
		printf("CXX_FLAGS:%s\n", TARANTOOL_CXX_FLAGS);
		return 0;
	}

	if (gopt(opt, 'h')) {
		puts("Tarantool -- an efficient in-memory data store.");
		printf("Usage: %s [OPTIONS]\n", basename(argv[0]));
		puts("");
		gopt_help(opt_def);
		puts("");
		puts("Please visit project home page at http://tarantool.org");
		puts("to see online documentation, submit bugs or contribute a patch.");
		return 0;
	}

	gopt_arg(opt, 'c', &cfg_filename);
	/*
	 * if config is not specified trying ./tarantool.cfg then
	 * /etc/tarantool.cfg
	 */
	if (cfg_filename == NULL) {
		if (access(DEFAULT_CFG_FILENAME, F_OK) == 0)
			cfg_filename = DEFAULT_CFG_FILENAME;
		else if (access(DEFAULT_CFG, F_OK) == 0)
			cfg_filename = DEFAULT_CFG;
		else
			panic("can't load config " "%s or %s", DEFAULT_CFG_FILENAME, DEFAULT_CFG);
	}

	cfg.log_level += gopt(opt, 'v');

	if (argc != 1) {
		fprintf(stderr, "Can't parse command line: try --help or -h for help.\n");
		exit(EX_USAGE);
	}

	if (cfg_filename[0] != '/') {
		cfg_filename_fullpath = (char *) malloc(PATH_MAX);
		if (getcwd(cfg_filename_fullpath, PATH_MAX - strlen(cfg_filename) - 1) == NULL) {
			say_syserror("getcwd");
			exit(EX_OSERR);
		}

		strcat(cfg_filename_fullpath, "/");
		strcat(cfg_filename_fullpath, cfg_filename);
	}

	cfg_out = tbuf_new(eter_pool);
	assert(cfg_out);

	if (gopt(opt, 'k')) {
		if (fill_default_tarantool_cfg(&cfg) != 0 || load_cfg(&cfg, 0) != 0) {
			say_error("check_config FAILED"
				  "%.*s", (uint32_t)cfg_out->size, (char *)cfg_out->data);

			return 1;
		}

		return 0;
	}

	if (fill_default_tarantool_cfg(&cfg) != 0 || load_cfg(&cfg, 0) != 0)
		panic("can't load config:"
		      "%.*s", (uint32_t)cfg_out->size, (char *)cfg_out->data);

	if (gopt_arg(opt, 'g', &cfg_paramname)) {
		tarantool_cfg_iterator_t *i;
		char *key, *value;

		i = tarantool_cfg_iterator_init();
		while ((key = tarantool_cfg_iterator_next(i, &cfg, &value)) != NULL) {
			if (strcmp(key, cfg_paramname) == 0 && value != NULL) {
				printf("%s\n", value);
				free(value);

				return 0;
			}

			free(value);
		}

		return 1;
	}

	if (cfg.work_dir != NULL && chdir(cfg.work_dir) == -1)
		say_syserror("can't chdir to `%s'", cfg.work_dir);

	if (cfg.username != NULL) {
		if (getuid() == 0 || geteuid() == 0) {
			struct passwd *pw;
			errno = 0;
			if ((pw = getpwnam(cfg.username)) == 0) {
				if (errno) {
					say_syserror("getpwnam: %s",
						     cfg.username);
				} else {
					say_error("User not found: %s",
						  cfg.username);
				}
				exit(EX_NOUSER);
			}
			if (setgid(pw->pw_gid) < 0 || setuid(pw->pw_uid) < 0 || seteuid(pw->pw_uid)) {
				say_syserror("setgit/setuid");
				exit(EX_OSERR);
			}
		} else {
			say_error("can't switch to %s: i'm not root", cfg.username);
		}
	}

	if (cfg.coredump) {
		struct rlimit c = { 0, 0 };
		if (getrlimit(RLIMIT_CORE, &c) < 0) {
			say_syserror("getrlimit");
			exit(EX_OSERR);
		}
		c.rlim_cur = c.rlim_max;
		if (setrlimit(RLIMIT_CORE, &c) < 0) {
			say_syserror("setrlimit");
			exit(EX_OSERR);
		}
#if defined(TARGET_OS_LINUX) && defined(HAVE_PRCTL_H)
		if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) < 0) {
			say_syserror("prctl");
			exit(EX_OSERR);
		}
#endif
	}

	if (gopt(opt, 'I')) {
		initialize_minimal();
		box_init(true);
		set_lsn(recovery_state, 1);
		snapshot_save(recovery_state, NULL);
		exit(EXIT_SUCCESS);
	}

	if (gopt(opt, 'B')) {
		if (cfg.logger == NULL) {
			say_crit("--background requires 'logger' configuration option to be set");
			exit(EXIT_FAILURE);
		}
		background();
	}
	else {
		create_pid();
	}

	/* init process title - used for logging */
	if (cfg.custom_proc_title == NULL) {
		custom_proc_title = (char *) palloc(eter_pool, 1);
		custom_proc_title[0] = '\0';
	} else {
		custom_proc_title = (char *) palloc(eter_pool, strlen(cfg.custom_proc_title) + 2);
		strcpy(custom_proc_title, "@");
		strcat(custom_proc_title, cfg.custom_proc_title);
	}

	say_logger_init(cfg.logger_nonblock);

	/* main core cleanup routine */
	atexit(tarantool_free);

	ev_default_loop(EVFLAG_AUTO);
	fiber_init();
	replication_prefork();
	coeio_init();
	if (!salloc_init(cfg.slab_alloc_arena * (1 << 30) /* GB */,
		    cfg.slab_alloc_minimal, cfg.slab_alloc_factor))
		panic("can't initialize slab allocator");

	signal_init();

	try {
		say_crit("version %s", tarantool_version());
		tarantool_L = tarantool_lua_init();
		box_init(false);
		memcached_init(cfg.bind_ipaddr, cfg.memcached_port);
		tarantool_lua_load_cfg(tarantool_L, &cfg);
		/*
		 * init iproto before admin and after memcached:
		 * recovery is finished on bind to the primary port,
		 * and it has to happen before requests on the
		 * administrative port start to arrive.
		 * And when recovery is finalized, memcached
		 * expire loop is started, so binding can happen
		 * only after memcached is initialized.
		 */
		iproto_init(cfg.bind_ipaddr, cfg.primary_port,
			    cfg.secondary_port);
		admin_init(cfg.bind_ipaddr, cfg.admin_port);
		replication_init(cfg.bind_ipaddr, cfg.replication_port);
		session_init();
		/*
		 * Load user init script.  The script should have access
		 * to Tarantool Lua API (box.cfg, box.fiber, etc...) that
		 * is why script must run only after the server was fully
		 * initialized.
		 */
		tarantool_lua_load_init_script(tarantool_L);
		prelease(fiber_ptr->gc_pool);
		say_crit("log level %i", cfg.log_level);
		say_crit("entering event loop");
		if (cfg.io_collect_interval > 0)
			ev_set_io_collect_interval(cfg.io_collect_interval);
		ev_now_update();
		start_time = ev_now();
		signal_start();
		ev_loop(0);
	} catch (const Exception& e) {
		e.log();
		panic("%s", "Fatal error, exiting loop");
	}

	say_crit("exiting loop");
	/* freeing resources */
	return 0;
}
