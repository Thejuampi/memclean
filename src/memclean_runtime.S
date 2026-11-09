.intel_syntax noprefix

.text

.globl memset
.p2align 4
memset:
    cmp byte ptr [rip + g_cpu_level], 1
    jb .Lmemset_init
    je memset_sse2
    jmp memset_avx2
.Lmemset_init:
    push rcx
    push rdx
    push r8
    call detect_cpu
    pop r8
    pop rdx
    pop rcx
    cmp byte ptr [rip + g_cpu_level], 1
    je memset_sse2
    jmp memset_avx2

.globl memcpy
.p2align 4
memcpy:
    cmp byte ptr [rip + g_cpu_level], 1
    jb .Lmemcpy_init
    je copy_forward_sse2
    jmp copy_forward_avx2
.Lmemcpy_init:
    push rcx
    push rdx
    push r8
    call detect_cpu
    pop r8
    pop rdx
    pop rcx
    cmp byte ptr [rip + g_cpu_level], 1
    je copy_forward_sse2
    jmp copy_forward_avx2

.globl memmove
.p2align 4
memmove:
    cmp byte ptr [rip + g_cpu_level], 1
    jb .Lmemmove_init
    je memmove_sse2
    jmp memmove_avx2
.Lmemmove_init:
    push rcx
    push rdx
    push r8
    call detect_cpu
    pop r8
    pop rdx
    pop rcx
    cmp byte ptr [rip + g_cpu_level], 1
    je memmove_sse2
    jmp memmove_avx2

.p2align 4
detect_cpu:
    push rbx
    xor eax, eax
    cpuid
    cmp eax, 7
    jb .Ldetect_sse
    mov eax, 1
    cpuid
    mov eax, ecx
    and eax, 0x18000000
    cmp eax, 0x18000000
    jne .Ldetect_sse
    xor ecx, ecx
    xgetbv
    and eax, 6
    cmp eax, 6
    jne .Ldetect_sse
    mov eax, 7
    xor ecx, ecx
    cpuid
    test ebx, 32
    jz .Ldetect_sse
    mov byte ptr [rip + g_cpu_level], 2
    pop rbx
    ret
.Ldetect_sse:
    mov byte ptr [rip + g_cpu_level], 1
    pop rbx
    ret

.p2align 4
memset_sse2:
    mov rax, rcx
    cmp r8, 16
    jb .Lms_small
    movd xmm0, edx
    punpcklbw xmm0, xmm0
    punpcklwd xmm0, xmm0
    pshufd xmm0, xmm0, 0
.Lms_loop64:
    cmp r8, 64
    jb .Lms_loop16
    movdqu xmmword ptr [rcx], xmm0
    movdqu xmmword ptr [rcx + 16], xmm0
    movdqu xmmword ptr [rcx + 32], xmm0
    movdqu xmmword ptr [rcx + 48], xmm0
    add rcx, 64
    sub r8, 64
    jmp .Lms_loop64
.Lms_loop16:
    cmp r8, 16
    jb .Lms_small
    movdqu xmmword ptr [rcx], xmm0
    add rcx, 16
    sub r8, 16
    jmp .Lms_loop16
.Lms_small:
    test r8, r8
    jz .Lms_done
    mov r9d, edx
.Lms_byte:
    mov byte ptr [rcx], r9b
    inc rcx
    dec r8
    jnz .Lms_byte
.Lms_done:
    ret

.p2align 4
memset_avx2:
    cmp r8, 32
    jb memset_sse2
    mov rax, rcx
    vmovd xmm0, edx
    vpbroadcastb ymm0, xmm0
.Lma_loop128:
    cmp r8, 128
    jb .Lma_loop32
    vmovdqu ymmword ptr [rcx], ymm0
    vmovdqu ymmword ptr [rcx + 32], ymm0
    vmovdqu ymmword ptr [rcx + 64], ymm0
    vmovdqu ymmword ptr [rcx + 96], ymm0
    add rcx, 128
    sub r8, 128
    jmp .Lma_loop128
.Lma_loop32:
    cmp r8, 32
    jb .Lma_tail
    vmovdqu ymmword ptr [rcx], ymm0
    add rcx, 32
    sub r8, 32
    jmp .Lma_loop32
.Lma_tail:
    vzeroupper
    cmp r8, 16
    jb .Lma_bytes
    movdqu xmmword ptr [rcx], xmm0
    add rcx, 16
    sub r8, 16
.Lma_bytes:
    test r8, r8
    jz .Lma_done
    mov r9d, edx
.Lma_byte:
    mov byte ptr [rcx], r9b
    inc rcx
    dec r8
    jnz .Lma_byte
.Lma_done:
    ret

.p2align 4
copy_forward_sse2:
    mov rax, rcx
    cmp r8, 16
    jb memcpy_small
.Lcf_loop64:
    cmp r8, 64
    jb .Lcf_loop16
    movdqu xmm0, xmmword ptr [rdx]
    movdqu xmm1, xmmword ptr [rdx + 16]
    movdqu xmm2, xmmword ptr [rdx + 32]
    movdqu xmm3, xmmword ptr [rdx + 48]
    movdqu xmmword ptr [rcx], xmm0
    movdqu xmmword ptr [rcx + 16], xmm1
    movdqu xmmword ptr [rcx + 32], xmm2
    movdqu xmmword ptr [rcx + 48], xmm3
    add rcx, 64
    add rdx, 64
    sub r8, 64
    jmp .Lcf_loop64
.Lcf_loop16:
    cmp r8, 16
    jb memcpy_small
    movdqu xmm0, xmmword ptr [rdx]
    movdqu xmmword ptr [rcx], xmm0
    add rcx, 16
    add rdx, 16
    sub r8, 16
    jmp .Lcf_loop16

.p2align 4
memcpy_small:
    test r8, r8
    jz .Lcs_done
    cmp r8, 8
    jae .Lcs_8
    cmp r8, 4
    jae .Lcs_4
    cmp r8, 2
    jae .Lcs_2
    mov r9b, byte ptr [rdx]
    mov byte ptr [rcx], r9b
    ret
.Lcs_2:
    movzx r9d, word ptr [rdx]
    movzx r10d, word ptr [rdx + r8 - 2]
    mov word ptr [rcx], r9w
    cmp r8, 2
    je .Lcs_done
    mov word ptr [rcx + r8 - 2], r10w
    ret
.Lcs_4:
    mov r9d, dword ptr [rdx]
    mov r10d, dword ptr [rdx + r8 - 4]
    mov dword ptr [rcx], r9d
    cmp r8, 4
    je .Lcs_done
    mov dword ptr [rcx + r8 - 4], r10d
    ret
.Lcs_8:
    mov r9, qword ptr [rdx]
    mov r10, qword ptr [rdx + r8 - 8]
    mov qword ptr [rcx], r9
    cmp r8, 8
    je .Lcs_done
    mov qword ptr [rcx + r8 - 8], r10
    ret
.Lcs_done:
    ret

.p2align 4
copy_forward_avx2:
    cmp r8, 32
    jb copy_forward_sse2
    mov rax, rcx
.Lcfa_loop128:
    cmp r8, 128
    jb .Lcfa_loop32
    vmovdqu ymm0, ymmword ptr [rdx]
    vmovdqu ymm1, ymmword ptr [rdx + 32]
    vmovdqu ymm2, ymmword ptr [rdx + 64]
    vmovdqu ymm3, ymmword ptr [rdx + 96]
    vmovdqu ymmword ptr [rcx], ymm0
    vmovdqu ymmword ptr [rcx + 32], ymm1
    vmovdqu ymmword ptr [rcx + 64], ymm2
    vmovdqu ymmword ptr [rcx + 96], ymm3
    add rcx, 128
    add rdx, 128
    sub r8, 128
    jmp .Lcfa_loop128
.Lcfa_loop32:
    cmp r8, 32
    jb .Lcfa_tail
    vmovdqu ymm0, ymmword ptr [rdx]
    vmovdqu ymmword ptr [rcx], ymm0
    add rcx, 32
    add rdx, 32
    sub r8, 32
    jmp .Lcfa_loop32
.Lcfa_tail:
    vzeroupper
    cmp r8, 16
    jb memcpy_small
    movdqu xmm0, xmmword ptr [rdx]
    movdqu xmmword ptr [rcx], xmm0
    add rcx, 16
    add rdx, 16
    sub r8, 16
    jmp memcpy_small

.p2align 4
memmove_sse2:
    mov rax, rcx
    test r8, r8
    jz .Lmm_done
    cmp rcx, rdx
    je .Lmm_done
    jb copy_forward_sse2
    mov r9, rcx
    sub r9, rdx
    cmp r9, r8
    jae copy_forward_sse2
    add rcx, r8
    add rdx, r8
.Lmm_back16:
    cmp r8, 16
    jb .Lmm_back_small
    sub rcx, 16
    sub rdx, 16
    movdqu xmm0, xmmword ptr [rdx]
    movdqu xmmword ptr [rcx], xmm0
    sub r8, 16
    jmp .Lmm_back16
.Lmm_back_small:
    test r8, r8
    jz .Lmm_done
.Lmm_back_byte:
    dec rcx
    dec rdx
    mov r9b, byte ptr [rdx]
    mov byte ptr [rcx], r9b
    dec r8
    jnz .Lmm_back_byte
.Lmm_done:
    ret

.p2align 4
memmove_avx2:
    mov rax, rcx
    test r8, r8
    jz .Lmmma_done
    cmp rcx, rdx
    je .Lmmma_done
    jb copy_forward_avx2
    mov r9, rcx
    sub r9, rdx
    cmp r9, r8
    jae copy_forward_avx2
    add rcx, r8
    add rdx, r8
    cmp r8, 32
    jb .Lmmma_tail
.Lmmma_loop32:
    sub rcx, 32
    sub rdx, 32
    vmovdqu ymm0, ymmword ptr [rdx]
    vmovdqu ymmword ptr [rcx], ymm0
    sub r8, 32
    cmp r8, 32
    jae .Lmmma_loop32
    vzeroupper
.Lmmma_tail:
    cmp r8, 16
    jb .Lmmma_bytes
    sub rcx, 16
    sub rdx, 16
    movdqu xmm0, xmmword ptr [rdx]
    movdqu xmmword ptr [rcx], xmm0
    sub r8, 16
.Lmmma_bytes:
    test r8, r8
    jz .Lmmma_done
.Lmmma_byte:
    dec rcx
    dec rdx
    mov r9b, byte ptr [rdx]
    mov byte ptr [rcx], r9b
    dec r8
    jnz .Lmmma_byte
.Lmmma_done:
    ret

.data
.p2align 2
g_cpu_level:
    .byte 0
