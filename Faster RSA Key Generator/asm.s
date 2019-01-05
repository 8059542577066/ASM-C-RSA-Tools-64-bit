    .globl ASM_ZERO
ASM_ZERO:
    movq    $0, (%rcx)
    leaq    8(%rcx), %rcx
    decl    %edx
    jnz     ASM_ZERO
    ret

    .globl ASM_MOV_Y_X
ASM_MOV_Y_X:
    movq    (%rdx), %rax
    movq    %rax, (%rcx)
    leaq    8(%rcx), %rcx
    leaq    8(%rdx), %rdx
    decl    %r8d
    jnz     ASM_MOV_Y_X
    ret

    .globl ASM_ADD_XY_X
ASM_ADD_XY_X:
    clc
.ASM_ADD_XY_X_1:
    movq    (%rdx), %rax
    adcq    %rax, (%rcx)
    leaq    8(%rcx), %rcx
    leaq    8(%rdx), %rdx
    decl    %r8d
    jnz     .ASM_ADD_XY_X_1
    adcq    $0, (%rcx)
    ret

    .globl ASM_ADD_XYC_X
ASM_ADD_XYC_X:
    call    ASM_ADD_XY_X
    leaq    8(%rcx), %rcx
.ASM_ADD_XYC_X_1:
    adcq    $0, (%rcx)
    leaq    8(%rcx), %rcx
    decl    %r9d
    jnz     .ASM_ADD_XYC_X_1
    ret

    .globl ASM_SUB_XY_X
ASM_SUB_XY_X:
    clc
.ASM_SUB_XY_X_1:
    movq    (%rdx), %rax
    sbbq    %rax, (%rcx)
    leaq    8(%rcx), %rcx
    leaq    8(%rdx), %rdx
    decl    %r8d
    jnz     .ASM_SUB_XY_X_1
    ret

    .globl ASM_SUB_XYB_X
ASM_SUB_XYB_X:
    call    ASM_SUB_XY_X
.ASM_SUB_XYB_X_1:
    sbbq    $0, (%rcx)
    leaq    8(%rcx), %rcx
    decl    %r9d
    jnz     .ASM_SUB_XYB_X_1
    ret

    .globl ASM_MUL1_XY_Z
ASM_MUL1_XY_Z:
    pushq   %rbx
    movq    %rdx, %rbx
    xorq    %r10, %r10
.ASM_MUL1_XY_Z_1:
    movq    (%rcx), %rax
    mulq    %rbx
    addq    %r10, %rax
    adcq    $0, %rdx
    movq    %rdx, %r10
    movq    %rax, (%r8)
    leaq    8(%rcx), %rcx
    leaq    8(%r8), %r8
    decl    %r9d
    jnz     .ASM_MUL1_XY_Z_1
    movq    %r10, (%r8)
    popq    %rbx
    ret
