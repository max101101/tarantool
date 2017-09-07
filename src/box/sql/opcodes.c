/* Automatically generated.  Do not edit */
/* See the tool/mkopcodec.tcl script for details. */
#include "sqliteInt.h"
#if !defined(SQLITE_OMIT_EXPLAIN) \
 || defined(VDBE_PROFILE) \
 || defined(SQLITE_DEBUG)
#if defined(SQLITE_ENABLE_EXPLAIN_COMMENTS) || defined(SQLITE_DEBUG)
# define OpHelp(X) "\0" X
#else
# define OpHelp(X)
#endif
const char *sqlite3OpcodeName(int i){
 static const char *const azName[] = {
    /*   0 */ "Savepoint"        OpHelp(""),
    /*   1 */ "AutoCommit"       OpHelp(""),
    /*   2 */ "Transaction"      OpHelp(""),
    /*   3 */ "SorterNext"       OpHelp(""),
    /*   4 */ "PrevIfOpen"       OpHelp(""),
    /*   5 */ "Or"               OpHelp("r[P3]=(r[P1] || r[P2])"),
    /*   6 */ "And"              OpHelp("r[P3]=(r[P1] && r[P2])"),
    /*   7 */ "Not"              OpHelp("r[P2]= !r[P1]"),
    /*   8 */ "NextIfOpen"       OpHelp(""),
    /*   9 */ "Prev"             OpHelp(""),
    /*  10 */ "Next"             OpHelp(""),
    /*  11 */ "Checkpoint"       OpHelp(""),
    /*  12 */ "JournalMode"      OpHelp(""),
    /*  13 */ "IsNull"           OpHelp("if r[P1]==NULL goto P2"),
    /*  14 */ "NotNull"          OpHelp("if r[P1]!=NULL goto P2"),
    /*  15 */ "Ne"               OpHelp("IF r[P3]!=r[P1]"),
    /*  16 */ "Eq"               OpHelp("IF r[P3]==r[P1]"),
    /*  17 */ "Gt"               OpHelp("IF r[P3]>r[P1]"),
    /*  18 */ "Le"               OpHelp("IF r[P3]<=r[P1]"),
    /*  19 */ "Lt"               OpHelp("IF r[P3]<r[P1]"),
    /*  20 */ "Ge"               OpHelp("IF r[P3]>=r[P1]"),
    /*  21 */ "ElseNotEq"        OpHelp(""),
    /*  22 */ "BitAnd"           OpHelp("r[P3]=r[P1]&r[P2]"),
    /*  23 */ "BitOr"            OpHelp("r[P3]=r[P1]|r[P2]"),
    /*  24 */ "ShiftLeft"        OpHelp("r[P3]=r[P2]<<r[P1]"),
    /*  25 */ "ShiftRight"       OpHelp("r[P3]=r[P2]>>r[P1]"),
    /*  26 */ "Add"              OpHelp("r[P3]=r[P1]+r[P2]"),
    /*  27 */ "Subtract"         OpHelp("r[P3]=r[P2]-r[P1]"),
    /*  28 */ "Multiply"         OpHelp("r[P3]=r[P1]*r[P2]"),
    /*  29 */ "Divide"           OpHelp("r[P3]=r[P2]/r[P1]"),
    /*  30 */ "Remainder"        OpHelp("r[P3]=r[P2]%r[P1]"),
    /*  31 */ "Concat"           OpHelp("r[P3]=r[P2]+r[P1]"),
    /*  32 */ "VFilter"          OpHelp("iplan=r[P3] zplan='P4'"),
    /*  33 */ "BitNot"           OpHelp("r[P1]= ~r[P1]"),
    /*  34 */ "VUpdate"          OpHelp("data=r[P3@P2]"),
    /*  35 */ "Goto"             OpHelp(""),
    /*  36 */ "Gosub"            OpHelp(""),
    /*  37 */ "InitCoroutine"    OpHelp(""),
    /*  38 */ "Yield"            OpHelp(""),
    /*  39 */ "MustBeInt"        OpHelp(""),
    /*  40 */ "Jump"             OpHelp(""),
    /*  41 */ "Once"             OpHelp(""),
    /*  42 */ "If"               OpHelp(""),
    /*  43 */ "IfNot"            OpHelp(""),
    /*  44 */ "SeekLT"           OpHelp("key=r[P3@P4]"),
    /*  45 */ "SeekLE"           OpHelp("key=r[P3@P4]"),
    /*  46 */ "SeekGE"           OpHelp("key=r[P3@P4]"),
    /*  47 */ "SeekGT"           OpHelp("key=r[P3@P4]"),
    /*  48 */ "NoConflict"       OpHelp("key=r[P3@P4]"),
    /*  49 */ "NotFound"         OpHelp("key=r[P3@P4]"),
    /*  50 */ "Found"            OpHelp("key=r[P3@P4]"),
    /*  51 */ "SeekRowid"        OpHelp("intkey=r[P3]"),
    /*  52 */ "NotExists"        OpHelp("intkey=r[P3]"),
    /*  53 */ "Last"             OpHelp(""),
    /*  54 */ "SorterSort"       OpHelp(""),
    /*  55 */ "Sort"             OpHelp(""),
    /*  56 */ "Rewind"           OpHelp(""),
    /*  57 */ "IdxLE"            OpHelp("key=r[P3@P4]"),
    /*  58 */ "IdxGT"            OpHelp("key=r[P3@P4]"),
    /*  59 */ "IdxLT"            OpHelp("key=r[P3@P4]"),
    /*  60 */ "IdxGE"            OpHelp("key=r[P3@P4]"),
    /*  61 */ "RowSetRead"       OpHelp("r[P3]=rowset(P1)"),
    /*  62 */ "RowSetTest"       OpHelp("if r[P3] in rowset(P1) goto P2"),
    /*  63 */ "Program"          OpHelp(""),
    /*  64 */ "FkIfZero"         OpHelp("if fkctr[P1]==0 goto P2"),
    /*  65 */ "IfPos"            OpHelp("if r[P1]>0 then r[P1]-=P3, goto P2"),
    /*  66 */ "IfNotZero"        OpHelp("if r[P1]!=0 then r[P1]--, goto P2"),
    /*  67 */ "DecrJumpZero"     OpHelp("if (--r[P1])==0 goto P2"),
    /*  68 */ "VNext"            OpHelp(""),
    /*  69 */ "Init"             OpHelp("Start at P2"),
    /*  70 */ "Return"           OpHelp(""),
    /*  71 */ "EndCoroutine"     OpHelp(""),
    /*  72 */ "HaltIfNull"       OpHelp("if r[P3]=null halt"),
    /*  73 */ "Halt"             OpHelp(""),
    /*  74 */ "Integer"          OpHelp("r[P2]=P1"),
    /*  75 */ "Int64"            OpHelp("r[P2]=P4"),
    /*  76 */ "String"           OpHelp("r[P2]='P4' (len=P1)"),
    /*  77 */ "Null"             OpHelp("r[P2..P3]=NULL"),
    /*  78 */ "SoftNull"         OpHelp("r[P1]=NULL"),
    /*  79 */ "Blob"             OpHelp("r[P2]=P4 (len=P1, subtype=P3)"),
    /*  80 */ "Variable"         OpHelp("r[P2]=parameter(P1,P4)"),
    /*  81 */ "Move"             OpHelp("r[P2@P3]=r[P1@P3]"),
    /*  82 */ "Copy"             OpHelp("r[P2@P3+1]=r[P1@P3+1]"),
    /*  83 */ "SCopy"            OpHelp("r[P2]=r[P1]"),
    /*  84 */ "IntCopy"          OpHelp("r[P2]=r[P1]"),
    /*  85 */ "ResultRow"        OpHelp("output=r[P1@P2]"),
    /*  86 */ "CollSeq"          OpHelp(""),
    /*  87 */ "Function0"        OpHelp("r[P3]=func(r[P2@P5])"),
    /*  88 */ "Function"         OpHelp("r[P3]=func(r[P2@P5])"),
    /*  89 */ "AddImm"           OpHelp("r[P1]=r[P1]+P2"),
    /*  90 */ "RealAffinity"     OpHelp(""),
    /*  91 */ "Cast"             OpHelp("affinity(r[P1])"),
    /*  92 */ "Permutation"      OpHelp(""),
    /*  93 */ "Compare"          OpHelp("r[P1@P3] <-> r[P2@P3]"),
    /*  94 */ "Column"           OpHelp("r[P3]=PX"),
    /*  95 */ "String8"          OpHelp("r[P2]='P4'"),
    /*  96 */ "Affinity"         OpHelp("affinity(r[P1@P2])"),
    /*  97 */ "MakeRecord"       OpHelp("r[P3]=mkrec(r[P1@P2])"),
    /*  98 */ "Count"            OpHelp("r[P2]=count()"),
    /*  99 */ "TTransaction"     OpHelp(""),
    /* 100 */ "ReadCookie"       OpHelp(""),
    /* 101 */ "SetCookie"        OpHelp(""),
    /* 102 */ "ReopenIdx"        OpHelp("root=P2 iDb=P3"),
    /* 103 */ "OpenRead"         OpHelp("root=P2 iDb=P3"),
    /* 104 */ "OpenWrite"        OpHelp("root=P2 iDb=P3"),
    /* 105 */ "OpenAutoindex"    OpHelp("nColumn=P2"),
    /* 106 */ "OpenEphemeral"    OpHelp("nColumn=P2"),
    /* 107 */ "SorterOpen"       OpHelp(""),
    /* 108 */ "SequenceTest"     OpHelp("if( cursor[P1].ctr++ ) pc = P2"),
    /* 109 */ "OpenPseudo"       OpHelp("P3 columns in r[P2]"),
    /* 110 */ "Close"            OpHelp(""),
    /* 111 */ "ColumnsUsed"      OpHelp(""),
    /* 112 */ "Sequence"         OpHelp("r[P2]=cursor[P1].ctr++"),
    /* 113 */ "MaxId"            OpHelp("r[P3]=get_max(space_index[P1]{Column[P2]})"),
    /* 114 */ "FCopy"            OpHelp("reg[P2@cur_frame]= reg[P1@root_frame(OPFLAG_SAME_FRAME)]"),
    /* 115 */ "NewRowid"         OpHelp("r[P2]=rowid"),
    /* 116 */ "Insert"           OpHelp("intkey=r[P3] data=r[P2]"),
    /* 117 */ "InsertInt"        OpHelp("intkey=P3 data=r[P2]"),
    /* 118 */ "Delete"           OpHelp(""),
    /* 119 */ "ResetCount"       OpHelp(""),
    /* 120 */ "SorterCompare"    OpHelp("if key(P1)!=trim(r[P3],P4) goto P2"),
    /* 121 */ "SorterData"       OpHelp("r[P2]=data"),
    /* 122 */ "RowData"          OpHelp("r[P2]=data"),
    /* 123 */ "Rowid"            OpHelp("r[P2]=rowid"),
    /* 124 */ "NullRow"          OpHelp(""),
    /* 125 */ "SorterInsert"     OpHelp("key=r[P2]"),
    /* 126 */ "IdxInsert"        OpHelp("key=r[P2]"),
    /* 127 */ "IdxDelete"        OpHelp("key=r[P2@P3]"),
    /* 128 */ "Seek"             OpHelp("Move P3 to P1.rowid"),
    /* 129 */ "IdxRowid"         OpHelp("r[P2]=rowid"),
    /* 130 */ "Real"             OpHelp("r[P2]=P4"),
    /* 131 */ "Destroy"          OpHelp(""),
    /* 132 */ "Clear"            OpHelp(""),
    /* 133 */ "ResetSorter"      OpHelp(""),
    /* 134 */ "CreateIndex"      OpHelp("r[P2]=root iDb=P1"),
    /* 135 */ "CreateTable"      OpHelp("r[P2]=root iDb=P1"),
    /* 136 */ "ParseSchema"      OpHelp(""),
    /* 137 */ "ParseSchema2"     OpHelp("rows=r[P1@P2] iDb=P3"),
    /* 138 */ "ParseSchema3"     OpHelp("name=r[P1] sql=r[P1+1] iDb=P2"),
    /* 139 */ "LoadAnalysis"     OpHelp(""),
    /* 140 */ "DropTable"        OpHelp(""),
    /* 141 */ "DropIndex"        OpHelp(""),
    /* 142 */ "DropTrigger"      OpHelp(""),
    /* 143 */ "IntegrityCk"      OpHelp(""),
    /* 144 */ "RowSetAdd"        OpHelp("rowset(P1)=r[P2]"),
    /* 145 */ "Param"            OpHelp(""),
    /* 146 */ "FkCounter"        OpHelp("fkctr[P1]+=P2"),
    /* 147 */ "MemMax"           OpHelp("r[P1]=max(r[P1],r[P2])"),
    /* 148 */ "OffsetLimit"      OpHelp("if r[P1]>0 then r[P2]=r[P1]+max(0,r[P3]) else r[P2]=(-1)"),
    /* 149 */ "AggStep0"         OpHelp("accum=r[P3] step(r[P2@P5])"),
    /* 150 */ "AggStep"          OpHelp("accum=r[P3] step(r[P2@P5])"),
    /* 151 */ "AggFinal"         OpHelp("accum=r[P1] N=P2"),
    /* 152 */ "Expire"           OpHelp(""),
    /* 153 */ "TableLock"        OpHelp("iDb=P1 root=P2 write=P3"),
    /* 154 */ "VBegin"           OpHelp(""),
    /* 155 */ "VCreate"          OpHelp(""),
    /* 156 */ "VDestroy"         OpHelp(""),
    /* 157 */ "VOpen"            OpHelp(""),
    /* 158 */ "VColumn"          OpHelp("r[P3]=vcolumn(P2)"),
    /* 159 */ "VRename"          OpHelp(""),
    /* 160 */ "Pagecount"        OpHelp(""),
    /* 161 */ "MaxPgcnt"         OpHelp(""),
    /* 162 */ "CursorHint"       OpHelp(""),
    /* 163 */ "IncMaxid"         OpHelp(""),
    /* 164 */ "Noop"             OpHelp(""),
    /* 165 */ "Explain"          OpHelp(""),
  };
  return azName[i];
}
#endif
