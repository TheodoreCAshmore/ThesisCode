#include "textflag.h"

// func rdPmcTsc() uint64
TEXT ·rdPmcTsc(SB), NOSPLIT, $0
    MOVL $0x10000, CX
    RDPMC
    SHLQ $32, DX
    ORQ DX, AX
    MOVQ AX, ret+0(FP)
    RET
