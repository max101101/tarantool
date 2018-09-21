#ifndef TARANTOOL_REPLICATION_H_INCLUDED
#define TARANTOOL_REPLICATION_H_INCLUDED
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
#include <tarantool.h>
#include "tarantool/util.h"

/**
 * Check replication configuration.
 *
 * @param config  config file to check.
 *
 * @return 0 on success, -1 on error
 */
int
replication_check_config(struct tarantool_cfg *config);

/**
 * Pre-fork replication spawner process.
 *
 * @return None. Panics and exits on error.
 */
void
replication_prefork();

/**
 * Initialize replication module.
 *
 * @return None. Panics and exits on error.
 */
void
replication_init(const char *bind_ipaddr, int replication_port);

/**
 * Closes replication listen socket
 *
 * @return None.
 */
void
replication_exit();

#endif // TARANTOOL_REPLICATION_H_INCLUDED

