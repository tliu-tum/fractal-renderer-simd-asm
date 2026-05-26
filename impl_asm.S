// impl_asm.S - SIMD Version
.intel_syntax noprefix
.global burning_ship

.data
    .align 16  // data alignment to avoid seg fault
    // vector constants
    vec_four:    .float 4.0, 4.0, 4.0, 4.0
    vec_two:     .float 2.0, 2.0, 2.0, 2.0
    vec_one:     .float 1.0, 1.0, 1.0, 1.0
    vec_abs_mask:.long 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF
    vec_x_offsets: .float 0.0, 1.0, 2.0, 3.0

    // coloring constants
    float_one:   .float 1.0
    float_255:   .float 255.0
    c_9:         .float 9.0
    c_15:        .float 15.0
    c_8_5:       .float 8.5

.text
burning_ship:
    push rbp
    mov rbp, rsp
    
    // backup called-saved registers
    push rbx
    push r12
    push r13
    push r14
    push r15

    // SSE requires address multiple of 16
    // 8 (rip) + 48 (push) + 24 (sub) = 80 (16*5)
    sub rsp, 24 // padding

    // Initialized registers
    // float arguments: xmm0 = start [Real, Imag, 0, 0], xmm1 = res
    // integer arguments: rdi=width, rsi=height, rdx=n, rcx=img

    // xmm2: [res, res, res, res]
    // 0x00 = 00 00 00 00
    movaps xmm2, xmm1            
    shufps xmm2, xmm2, 0x00      // fulfill register with Index 0 (res)
    
    // xmm1: [imag, imag, imag, imag]
    // 0x55 = 01 01 01 01
    movaps xmm1, xmm0            
    shufps xmm1, xmm1, 0x55      // fulfill register with Index 1 (imag)
    
    // xmm0: [real, real, real, real]
    shufps xmm0, xmm0, 0x00      // fulfill register with Index 0 (real)

    mov r10, rcx                 // r10 = img pointer
    xor r8, r8                   // y = 0

loop_y:
    cmp r8, rsi
    jge end_func                 // jmp to end if y >= height

    xor r9, r9                   // x = 0

loop_x:
    cmp r9, rdi
    jge next_line                // jmp to next line if x >= width

    // if pixels of the remaining line is less than 4
    mov r14, rdi
    sub r14, r9                 // r14 = width - x
    cmp r14, 4
    jge .lanes_4                // if >= 4, lanes = 4
    jmp .lanes_done             // < 4, keep remains
.lanes_4:
    mov r14, 4
.lanes_done:
    
    // Calculate imag offset
    // Ci = start_imag + y * res
    // Result: [Ci, Ci, Ci, Ci]
    cvtsi2ss xmm3, r8            // convert y to float
    shufps xmm3, xmm3, 0         // [y, y, y, y]
    mulps xmm3, xmm2             // y * res
    addps xmm3, xmm1             // + start_imag

    // Calculate real offset
    // Cr = start_real + (x + offset) * res
    // Result: [Cr0, Cr1, Cr2, Cr3]
    cvtsi2ss xmm4, r9            // convert x to float
    shufps xmm4, xmm4, 0         // [x, x, x, x]
    addps xmm4, [rip + vec_x_offsets] // + [0, 1, 2, 3]
    mulps xmm4, xmm2             // * res
    addps xmm4, xmm0             // + start_real
    
    // prepare to calculate Z = (Zr + Zi)^2 + C
    xorps xmm5, xmm5             // Zr = 0
    xorps xmm6, xmm6             // Zi = 0
    xorps xmm7, xmm7             // Counters = 0
    
    xor r11, r11                 // k = 0

iteration_loop:
    // compare k >= n
    cmp r11, rdx                
    jge end_simd_loop

    // calculate |Zr|, |Zi|
    movaps xmm11, xmm5
    andps xmm11, [rip + vec_abs_mask] // xmm11 = |Zr|
    movaps xmm12, xmm6
    andps xmm12, [rip + vec_abs_mask] // xmm12 = |Zi|

    // New Zr = Zr^2 - Zi^2 + Cr
    movaps xmm13, xmm11
    mulps xmm13, xmm13           // xmm13 = |Zr|^2
    movaps xmm14, xmm12
    mulps xmm14, xmm14           // xmm14 = |Zi|^2
    subps xmm13, xmm14           // |Zr|^2 - |Zi|^2
    addps xmm13, xmm4            // + Cr

    // New Zi = 2 * |Zr| * |Zi| + Ci
    movaps xmm14, xmm11
    mulps xmm14, xmm12           // |Zr||Zi|
    mulps xmm14, [rip + vec_two] // * 2
    addps xmm14, xmm3            // + Ci

    // Update Zr, Zi
    movaps xmm5, xmm13
    movaps xmm6, xmm14

    // calculate Zr^2 + Zi^2
    movaps xmm13, xmm5
    mulps xmm13, xmm13
    movaps xmm14, xmm6
    mulps xmm14, xmm14
    addps xmm13, xmm14

    // escape: mag < 4.0 ? xmm15 = all 1s : xmm15 = all 0s
    movaps xmm15, xmm13
    cmpleps xmm15, [rip + vec_four] // mag < 4.0 ?

    // check all escaped
    movmskps eax, xmm15 // fetch highest bits of 4 floats
    test eax, eax // check all 0s
    jz end_simd_loop // end loop if all 0s

    // update counters
    movaps xmm13, [rip + vec_one]
    andps xmm13, xmm15 // if xmm15 = all 1s, xmm13 = 1, otherwise = 0
    addps xmm7, xmm13  // add counter only if xmm13 = 1

    inc r11 // max recursion
    jmp iteration_loop

end_simd_loop:
    movups [rsp], xmm7           // save k to stack
    xor r12, r12 

extract_color_loop:
    cmp r12, r14
    jge advance_x                // jump to next

    // load k into xmm5
    movss xmm5, dword ptr [rsp + r12 * 4]

    // check max iter
    cvttss2si rax, xmm5
    cmp rax, rdx
    jge pixel_black

    // t = k / n
    cvtsi2ss xmm6, rdx
    divss xmm5, xmm6             // xmm5 = t

    // 1.0 - (k/n)
    movss xmm6, [rip + float_one]
    subss xmm6, xmm5
    movaps xmm5, xmm6

    // 1-t
    movss xmm6, [rip + float_one]
    subss xmm6, xmm5             // xmm6 = 1-t

    // powers
    movss xmm7, xmm5
    mulss xmm7, xmm5             // t^2
    movss xmm8, xmm7
    mulss xmm8, xmm5             // t^3

    movss xmm9, xmm6
    mulss xmm9, xmm6             // (1-t)^2
    movss xmm10, xmm9
    mulss xmm10, xmm6            // (1-t)^3

    // Red
    movss xmm11, [rip + c_9]
    mulss xmm11, xmm6
    mulss xmm11, xmm8
    mulss xmm11, [rip + float_255]

    // Green
    movss xmm12, [rip + c_15]
    mulss xmm12, xmm9
    mulss xmm12, xmm7
    mulss xmm12, [rip + float_255]

    // Blue
    movss xmm13, [rip + c_8_5]
    mulss xmm13, xmm10
    mulss xmm13, xmm5
    mulss xmm13, [rip + float_255]

    // convert & clamp
    cvttss2si rax, xmm11
    cvttss2si rbx, xmm12
    cvttss2si rcx, xmm13

    // clamp R
    cmp rax, 255
    jle r_chk
    mov rax, 255
r_chk:
    cmp rax, 0
    jge r_ok
    mov rax, 0
r_ok:

    // clamp G
    cmp rbx, 255
    jle g_chk
    mov rbx, 255
g_chk:
    cmp rbx, 0
    jge g_ok
    mov rbx, 0
g_ok:

    // clamp B
    cmp rcx, 255
    jle b_chk
    mov rcx, 255
b_chk:
    cmp rcx, 0
    jge b_ok
    mov rcx, 0
b_ok:

    // write BGR
    mov byte ptr [r10], cl
    mov byte ptr [r10+1], bl
    mov byte ptr [r10+2], al
    jmp pixel_done

pixel_black:
    mov byte ptr [r10], 0
    mov byte ptr [r10+1], 0
    mov byte ptr [r10+2], 0

pixel_done:
    add r10, 3
    inc r12
    jmp extract_color_loop

advance_x:
    add r9, r14
    jmp loop_x

next_line:
    inc r8
    jmp loop_y

end_func:
    add rsp, 24
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
