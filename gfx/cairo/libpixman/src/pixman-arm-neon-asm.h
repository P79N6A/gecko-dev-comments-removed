




















































.set FLAG_DST_WRITEONLY,       0
.set FLAG_DST_READWRITE,       1
.set FLAG_DEINTERLEAVE_32BPP,  2





.set ARGS_STACK_OFFSET,        40




.set PREFETCH_TYPE_NONE,       0 
.set PREFETCH_TYPE_SIMPLE,     1 
.set PREFETCH_TYPE_ADVANCED,   2 






.macro pixldst1 op, elem_size, reg1, mem_operand, abits
.if abits > 0
    op&.&elem_size {d&reg1}, [&mem_operand&, :&abits&]!
.else
    op&.&elem_size {d&reg1}, [&mem_operand&]!
.endif
.endm

.macro pixldst2 op, elem_size, reg1, reg2, mem_operand, abits
.if abits > 0
    op&.&elem_size {d&reg1, d&reg2}, [&mem_operand&, :&abits&]!
.else
    op&.&elem_size {d&reg1, d&reg2}, [&mem_operand&]!
.endif
.endm

.macro pixldst4 op, elem_size, reg1, reg2, reg3, reg4, mem_operand, abits
.if abits > 0
    op&.&elem_size {d&reg1, d&reg2, d&reg3, d&reg4}, [&mem_operand&, :&abits&]!
.else
    op&.&elem_size {d&reg1, d&reg2, d&reg3, d&reg4}, [&mem_operand&]!
.endif
.endm

.macro pixldst0 op, elem_size, reg1, idx, mem_operand, abits
    op&.&elem_size {d&reg1[idx]}, [&mem_operand&]!
.endm

.macro pixldst3 op, elem_size, reg1, reg2, reg3, mem_operand
    op&.&elem_size {d&reg1, d&reg2, d&reg3}, [&mem_operand&]!
.endm

.macro pixldst30 op, elem_size, reg1, reg2, reg3, idx, mem_operand
    op&.&elem_size {d&reg1[idx], d&reg2[idx], d&reg3[idx]}, [&mem_operand&]!
.endm

.macro pixldst numbytes, op, elem_size, basereg, mem_operand, abits
.if numbytes == 32
    pixldst4 op, elem_size, %(basereg+4), %(basereg+5), \
                              %(basereg+6), %(basereg+7), mem_operand, abits
.elseif numbytes == 16
    pixldst2 op, elem_size, %(basereg+2), %(basereg+3), mem_operand, abits
.elseif numbytes == 8
    pixldst1 op, elem_size, %(basereg+1), mem_operand, abits
.elseif numbytes == 4
    .if !RESPECT_STRICT_ALIGNMENT || (elem_size == 32)
        pixldst0 op, 32, %(basereg+0), 1, mem_operand, abits
    .elseif elem_size == 16
        pixldst0 op, 16, %(basereg+0), 2, mem_operand, abits
        pixldst0 op, 16, %(basereg+0), 3, mem_operand, abits
    .else
        pixldst0 op, 8, %(basereg+0), 4, mem_operand, abits
        pixldst0 op, 8, %(basereg+0), 5, mem_operand, abits
        pixldst0 op, 8, %(basereg+0), 6, mem_operand, abits
        pixldst0 op, 8, %(basereg+0), 7, mem_operand, abits
    .endif
.elseif numbytes == 2
    .if !RESPECT_STRICT_ALIGNMENT || (elem_size == 16)
        pixldst0 op, 16, %(basereg+0), 1, mem_operand, abits
    .else
        pixldst0 op, 8, %(basereg+0), 2, mem_operand, abits
        pixldst0 op, 8, %(basereg+0), 3, mem_operand, abits
    .endif
.elseif numbytes == 1
    pixldst0 op, 8, %(basereg+0), 1, mem_operand, abits
.else
    .error "unsupported size: numbytes"
.endif
.endm

.macro pixld numpix, bpp, basereg, mem_operand, abits=0
.if bpp > 0
.if (bpp == 32) && (numpix == 8) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    pixldst4 vld4, 8, %(basereg+4), %(basereg+5), \
                      %(basereg+6), %(basereg+7), mem_operand, abits
.elseif (bpp == 24) && (numpix == 8)
    pixldst3 vld3, 8, %(basereg+3), %(basereg+4), %(basereg+5), mem_operand
.elseif (bpp == 24) && (numpix == 4)
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 4, mem_operand
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 5, mem_operand
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 6, mem_operand
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 7, mem_operand
.elseif (bpp == 24) && (numpix == 2)
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 2, mem_operand
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 3, mem_operand
.elseif (bpp == 24) && (numpix == 1)
    pixldst30 vld3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 1, mem_operand
.else
    pixldst %(numpix * bpp / 8), vld1, %(bpp), basereg, mem_operand, abits
.endif
.endif
.endm

.macro pixst numpix, bpp, basereg, mem_operand, abits=0
.if bpp > 0
.if (bpp == 32) && (numpix == 8) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    pixldst4 vst4, 8, %(basereg+4), %(basereg+5), \
                      %(basereg+6), %(basereg+7), mem_operand, abits
.elseif (bpp == 24) && (numpix == 8)
    pixldst3 vst3, 8, %(basereg+3), %(basereg+4), %(basereg+5), mem_operand
.elseif (bpp == 24) && (numpix == 4)
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 4, mem_operand
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 5, mem_operand
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 6, mem_operand
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 7, mem_operand
.elseif (bpp == 24) && (numpix == 2)
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 2, mem_operand
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 3, mem_operand
.elseif (bpp == 24) && (numpix == 1)
    pixldst30 vst3, 8, %(basereg+0), %(basereg+1), %(basereg+2), 1, mem_operand
.else
    pixldst %(numpix * bpp / 8), vst1, %(bpp), basereg, mem_operand, abits
.endif
.endif
.endm

.macro pixld_a numpix, bpp, basereg, mem_operand
.if (bpp * numpix) <= 128
    pixld numpix, bpp, basereg, mem_operand, %(bpp * numpix)
.else
    pixld numpix, bpp, basereg, mem_operand, 128
.endif
.endm

.macro pixst_a numpix, bpp, basereg, mem_operand
.if (bpp * numpix) <= 128
    pixst numpix, bpp, basereg, mem_operand, %(bpp * numpix)
.else
    pixst numpix, bpp, basereg, mem_operand, 128
.endif
.endm

.macro vuzp8 reg1, reg2
    vuzp.8 d&reg1, d&reg2
.endm

.macro vzip8 reg1, reg2
    vzip.8 d&reg1, d&reg2
.endm


.macro pixdeinterleave bpp, basereg
.if (bpp == 32) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    vuzp8 %(basereg+0), %(basereg+1)
    vuzp8 %(basereg+2), %(basereg+3)
    vuzp8 %(basereg+1), %(basereg+3)
    vuzp8 %(basereg+0), %(basereg+2)
.endif
.endm


.macro pixinterleave bpp, basereg
.if (bpp == 32) && (DEINTERLEAVE_32BPP_ENABLED != 0)
    vzip8 %(basereg+0), %(basereg+2)
    vzip8 %(basereg+1), %(basereg+3)
    vzip8 %(basereg+2), %(basereg+3)
    vzip8 %(basereg+0), %(basereg+1)
.endif
.endm





























.macro PF a, x:vararg
.if (PREFETCH_TYPE_CURRENT == PREFETCH_TYPE_ADVANCED)
    a x
.endif
.endm

.macro cache_preload std_increment, boost_increment
.if (src_bpp_shift >= 0) || (dst_r_bpp != 0) || (mask_bpp_shift >= 0)
.if regs_shortage
    PF ldr ORIG_W, [sp] 
.endif
.if std_increment != 0
    PF add PF_X, PF_X, #std_increment
.endif
    PF tst PF_CTL, #0xF
    PF addne PF_X, PF_X, #boost_increment
    PF subne PF_CTL, PF_CTL, #1
    PF cmp PF_X, ORIG_W
.if src_bpp_shift >= 0
    PF pld, [PF_SRC, PF_X, lsl #src_bpp_shift]
.endif
.if dst_r_bpp != 0
    PF pld, [PF_DST, PF_X, lsl #dst_bpp_shift]
.endif
.if mask_bpp_shift >= 0
    PF pld, [PF_MASK, PF_X, lsl #mask_bpp_shift]
.endif
    PF subge PF_X, PF_X, ORIG_W
    PF subges PF_CTL, PF_CTL, #0x10
.if src_bpp_shift >= 0
    PF ldrgeb DUMMY, [PF_SRC, SRC_STRIDE, lsl #src_bpp_shift]!
.endif
.if dst_r_bpp != 0
    PF ldrgeb DUMMY, [PF_DST, DST_STRIDE, lsl #dst_bpp_shift]!
.endif
.if mask_bpp_shift >= 0
    PF ldrgeb DUMMY, [PF_MASK, MASK_STRIDE, lsl #mask_bpp_shift]!
.endif
.endif
.endm

.macro cache_preload_simple
.if (PREFETCH_TYPE_CURRENT == PREFETCH_TYPE_SIMPLE)
.if src_bpp > 0
    pld [SRC, #(PREFETCH_DISTANCE_SIMPLE * src_bpp / 8)]
.endif
.if dst_r_bpp > 0
    pld [DST_R, #(PREFETCH_DISTANCE_SIMPLE * dst_r_bpp / 8)]
.endif
.if mask_bpp > 0
    pld [MASK, #(PREFETCH_DISTANCE_SIMPLE * mask_bpp / 8)]
.endif
.endif
.endm






.macro ensure_destination_ptr_alignment process_pixblock_head, \
                                        process_pixblock_tail, \
                                        process_pixblock_tail_head
.if dst_w_bpp != 24
    tst         DST_R, #0xF
    beq         2f

.irp lowbit, 1, 2, 4, 8, 16
local skip1
.if (dst_w_bpp <= (lowbit * 8)) && ((lowbit * 8) < (pixblock_size * dst_w_bpp))
.if lowbit < 16 
    tst         DST_R, #lowbit
    beq         1f
.endif
    pixld       (lowbit * 8 / dst_w_bpp), src_bpp, src_basereg, SRC
    pixld       (lowbit * 8 / dst_w_bpp), mask_bpp, mask_basereg, MASK
.if dst_r_bpp > 0
    pixld_a     (lowbit * 8 / dst_r_bpp), dst_r_bpp, dst_r_basereg, DST_R
.else
    add         DST_R, DST_R, #lowbit
.endif
    PF add      PF_X, PF_X, #(lowbit * 8 / dst_w_bpp)
    sub         W, W, #(lowbit * 8 / dst_w_bpp)
1:
.endif
.endr
    pixdeinterleave src_bpp, src_basereg
    pixdeinterleave mask_bpp, mask_basereg
    pixdeinterleave dst_r_bpp, dst_r_basereg

    process_pixblock_head
    cache_preload 0, pixblock_size
    cache_preload_simple
    process_pixblock_tail

    pixinterleave dst_w_bpp, dst_w_basereg
.irp lowbit, 1, 2, 4, 8, 16
.if (dst_w_bpp <= (lowbit * 8)) && ((lowbit * 8) < (pixblock_size * dst_w_bpp))
.if lowbit < 16 
    tst         DST_W, #lowbit
    beq         1f
.endif
    pixst_a     (lowbit * 8 / dst_w_bpp), dst_w_bpp, dst_w_basereg, DST_W
1:
.endif
.endr
.endif
2:
.endm
















.macro process_trailing_pixels cache_preload_flag, \
                               dst_aligned_flag, \
                               process_pixblock_head, \
                               process_pixblock_tail, \
                               process_pixblock_tail_head
    tst         W, #(pixblock_size - 1)
    beq         2f
.irp chunk_size, 16, 8, 4, 2, 1
.if pixblock_size > chunk_size
    tst         W, #chunk_size
    beq         1f
    pixld       chunk_size, src_bpp, src_basereg, SRC
    pixld       chunk_size, mask_bpp, mask_basereg, MASK
.if dst_aligned_flag != 0
    pixld_a     chunk_size, dst_r_bpp, dst_r_basereg, DST_R
.else
    pixld       chunk_size, dst_r_bpp, dst_r_basereg, DST_R
.endif
.if cache_preload_flag != 0
    PF add      PF_X, PF_X, #chunk_size
.endif
1:
.endif
.endr
    pixdeinterleave src_bpp, src_basereg
    pixdeinterleave mask_bpp, mask_basereg
    pixdeinterleave dst_r_bpp, dst_r_basereg

    process_pixblock_head
.if cache_preload_flag != 0
    cache_preload 0, pixblock_size
    cache_preload_simple
.endif
    process_pixblock_tail
    pixinterleave dst_w_bpp, dst_w_basereg
.irp chunk_size, 16, 8, 4, 2, 1
.if pixblock_size > chunk_size
    tst         W, #chunk_size
    beq         1f
.if dst_aligned_flag != 0
    pixst_a     chunk_size, dst_w_bpp, dst_w_basereg, DST_W
.else
    pixst       chunk_size, dst_w_bpp, dst_w_basereg, DST_W
.endif
1:
.endif
.endr
2:
.endm






.macro advance_to_next_scanline start_of_loop_label
.if regs_shortage
    ldrd        W, [sp] 
.else
    mov         W, ORIG_W
.endif
    add         DST_W, DST_W, DST_STRIDE, lsl #dst_bpp_shift
.if src_bpp != 0
    add         SRC, SRC, SRC_STRIDE, lsl #src_bpp_shift
.endif
.if mask_bpp != 0
    add         MASK, MASK, MASK_STRIDE, lsl #mask_bpp_shift
.endif
.if (dst_w_bpp != 24)
    sub         DST_W, DST_W, W, lsl #dst_bpp_shift
.endif
.if (src_bpp != 24) && (src_bpp != 0)
    sub         SRC, SRC, W, lsl #src_bpp_shift
.endif
.if (mask_bpp != 24) && (mask_bpp != 0)
    sub         MASK, MASK, W, lsl #mask_bpp_shift
.endif
    subs        H, H, #1
    mov         DST_R, DST_W
.if regs_shortage
    str         H, [sp, #4] 
.endif
    bge         start_of_loop_label
.endm








.macro generate_composite_function fname, \
                                   src_bpp_, \
                                   mask_bpp_, \
                                   dst_w_bpp_, \
                                   flags, \
                                   pixblock_size_, \
                                   prefetch_distance, \
                                   init, \
                                   cleanup, \
                                   process_pixblock_head, \
                                   process_pixblock_tail, \
                                   process_pixblock_tail_head, \
                                   dst_w_basereg_ = 28, \
                                   dst_r_basereg_ = 4, \
                                   src_basereg_   = 0, \
                                   mask_basereg_  = 24

    .func fname
    .global fname
    
#ifdef __ELF__
    .hidden fname
    .type fname, %function
#endif
fname:
    push        {r4-r12, lr}        






    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_DEFAULT
.if prefetch_distance == 0
    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_NONE
.elseif (PREFETCH_TYPE_CURRENT > PREFETCH_TYPE_SIMPLE) && \
        ((src_bpp_ == 24) || (mask_bpp_ == 24) || (dst_w_bpp_ == 24))
    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_SIMPLE
.endif





    .set src_bpp, src_bpp_
    .set mask_bpp, mask_bpp_
    .set dst_w_bpp, dst_w_bpp_
    .set pixblock_size, pixblock_size_
    .set dst_w_basereg, dst_w_basereg_
    .set dst_r_basereg, dst_r_basereg_
    .set src_basereg, src_basereg_
    .set mask_basereg, mask_basereg_




    W           .req        r0      
    H           .req        r1      
    DST_W       .req        r2      
    DST_STRIDE  .req        r3      
    SRC         .req        r4      
    SRC_STRIDE  .req        r5      
    DST_R       .req        r6      

    MASK        .req        r7      
    MASK_STRIDE .req        r8      

    PF_CTL      .req        r9      
                                    
    PF_X        .req        r10     
                                    
    PF_SRC      .req        r11     
                                    
    PF_DST      .req        r12     
                                    
    PF_MASK     .req        r14     
                                    








.if (PREFETCH_TYPE_CURRENT < PREFETCH_TYPE_ADVANCED)
    ORIG_W      .req        r10     
    DUMMY       .req        r12     
    .set        regs_shortage, 0
.elseif mask_bpp == 0
    ORIG_W      .req        r7      
    DUMMY       .req        r8      
    .set        regs_shortage, 0
.elseif src_bpp == 0
    ORIG_W      .req        r4      
    DUMMY       .req        r5      
    .set        regs_shortage, 0
.else
    ORIG_W      .req        r1      
    DUMMY       .req        r1      
    .set        regs_shortage, 1
.endif

    .set mask_bpp_shift, -1
.if src_bpp == 32
    .set src_bpp_shift, 2
.elseif src_bpp == 24
    .set src_bpp_shift, 0
.elseif src_bpp == 16
    .set src_bpp_shift, 1
.elseif src_bpp == 8
    .set src_bpp_shift, 0
.elseif src_bpp == 0
    .set src_bpp_shift, -1
.else
    .error "requested src bpp (src_bpp) is not supported"
.endif
.if mask_bpp == 32
    .set mask_bpp_shift, 2
.elseif mask_bpp == 24
    .set mask_bpp_shift, 0
.elseif mask_bpp == 8
    .set mask_bpp_shift, 0
.elseif mask_bpp == 0
    .set mask_bpp_shift, -1
.else
    .error "requested mask bpp (mask_bpp) is not supported"
.endif
.if dst_w_bpp == 32
    .set dst_bpp_shift, 2
.elseif dst_w_bpp == 24
    .set dst_bpp_shift, 0
.elseif dst_w_bpp == 16
    .set dst_bpp_shift, 1
.elseif dst_w_bpp == 8
    .set dst_bpp_shift, 0
.else
    .error "requested dst bpp (dst_w_bpp) is not supported"
.endif

.if (((flags) & FLAG_DST_READWRITE) != 0)
    .set dst_r_bpp, dst_w_bpp
.else
    .set dst_r_bpp, 0
.endif
.if (((flags) & FLAG_DEINTERLEAVE_32BPP) != 0)
    .set DEINTERLEAVE_32BPP_ENABLED, 1
.else
    .set DEINTERLEAVE_32BPP_ENABLED, 0
.endif

.if prefetch_distance < 0 || prefetch_distance > 15
    .error "invalid prefetch distance (prefetch_distance)"
.endif

.if src_bpp > 0
    ldr         SRC, [sp, #40]
.endif
.if mask_bpp > 0
    ldr         MASK, [sp, #48]
.endif
    PF mov      PF_X, #0
.if src_bpp > 0
    ldr         SRC_STRIDE, [sp, #44]
.endif
.if mask_bpp > 0
    ldr         MASK_STRIDE, [sp, #52]
.endif
    mov         DST_R, DST_W

.if src_bpp == 24
    sub         SRC_STRIDE, SRC_STRIDE, W
    sub         SRC_STRIDE, SRC_STRIDE, W, lsl #1
.endif
.if mask_bpp == 24
    sub         MASK_STRIDE, MASK_STRIDE, W
    sub         MASK_STRIDE, MASK_STRIDE, W, lsl #1
.endif
.if dst_w_bpp == 24
    sub         DST_STRIDE, DST_STRIDE, W
    sub         DST_STRIDE, DST_STRIDE, W, lsl #1
.endif




    PF mov      PF_SRC, SRC
    PF mov      PF_DST, DST_R
    PF mov      PF_MASK, MASK
    
    PF mov      PF_CTL, H, lsl #4
    PF add      PF_CTL, #(prefetch_distance - 0x10)

    init
.if regs_shortage
    push        {r0, r1}
.endif
    subs        H, H, #1
.if regs_shortage
    str         H, [sp, #4] 
.else
    mov         ORIG_W, W
.endif
    blt         9f
    cmp         W, #(pixblock_size * 2)
    blt         8f




0:
    ensure_destination_ptr_alignment process_pixblock_head, \
                                     process_pixblock_tail, \
                                     process_pixblock_tail_head

    
    pixld_a     pixblock_size, dst_r_bpp, \
                (dst_r_basereg - pixblock_size * dst_r_bpp / 64), DST_R
    pixld       pixblock_size, src_bpp, \
                (src_basereg - pixblock_size * src_bpp / 64), SRC
    pixld       pixblock_size, mask_bpp, \
                (mask_basereg - pixblock_size * mask_bpp / 64), MASK
    PF add      PF_X, PF_X, #pixblock_size
    process_pixblock_head
    cache_preload 0, pixblock_size
    cache_preload_simple
    subs        W, W, #(pixblock_size * 2)
    blt         2f
1:
    process_pixblock_tail_head
    cache_preload_simple
    subs        W, W, #pixblock_size
    bge         1b
2:
    process_pixblock_tail
    pixst_a     pixblock_size, dst_w_bpp, \
                (dst_w_basereg - pixblock_size * dst_w_bpp / 64), DST_W

    
    process_trailing_pixels 1, 1, \
                            process_pixblock_head, \
                            process_pixblock_tail, \
                            process_pixblock_tail_head
    advance_to_next_scanline 0b

.if regs_shortage
    pop         {r0, r1}
.endif
    cleanup
    pop         {r4-r12, pc}  





8:
    
    tst         W, #pixblock_size
    beq         1f
    pixld       pixblock_size, dst_r_bpp, \
                (dst_r_basereg - pixblock_size * dst_r_bpp / 64), DST_R
    pixld       pixblock_size, src_bpp, \
                (src_basereg - pixblock_size * src_bpp / 64), SRC
    pixld       pixblock_size, mask_bpp, \
                (mask_basereg - pixblock_size * mask_bpp / 64), MASK
    process_pixblock_head
    process_pixblock_tail
    pixst       pixblock_size, dst_w_bpp, \
                (dst_w_basereg - pixblock_size * dst_w_bpp / 64), DST_W
1:
    
    process_trailing_pixels 0, 0, \
                            process_pixblock_head, \
                            process_pixblock_tail, \
                            process_pixblock_tail_head
    advance_to_next_scanline 8b
9:
.if regs_shortage
    pop         {r0, r1}
.endif
    cleanup
    pop         {r4-r12, pc}  

    .unreq      SRC
    .unreq      MASK
    .unreq      DST_R
    .unreq      DST_W
    .unreq      ORIG_W
    .unreq      W
    .unreq      H
    .unreq      SRC_STRIDE
    .unreq      DST_STRIDE
    .unreq      MASK_STRIDE
    .unreq      PF_CTL
    .unreq      PF_X
    .unreq      PF_SRC
    .unreq      PF_DST
    .unreq      PF_MASK
    .unreq      DUMMY
    .endfunc
.endm





.macro generate_composite_function_single_scanline fname, \
                                                   src_bpp_, \
                                                   mask_bpp_, \
                                                   dst_w_bpp_, \
                                                   flags, \
                                                   pixblock_size_, \
                                                   init, \
                                                   cleanup, \
                                                   process_pixblock_head, \
                                                   process_pixblock_tail, \
                                                   process_pixblock_tail_head, \
                                                   dst_w_basereg_ = 28, \
                                                   dst_r_basereg_ = 4, \
                                                   src_basereg_   = 0, \
                                                   mask_basereg_  = 24

    .func fname
    .global fname
    
#ifdef __ELF__
    .hidden fname
    .type fname, %function
#endif
fname:
    .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_NONE




    .set src_bpp, src_bpp_
    .set mask_bpp, mask_bpp_
    .set dst_w_bpp, dst_w_bpp_
    .set pixblock_size, pixblock_size_
    .set dst_w_basereg, dst_w_basereg_
    .set dst_r_basereg, dst_r_basereg_
    .set src_basereg, src_basereg_
    .set mask_basereg, mask_basereg_



    W           .req        r0      
    DST_W       .req        r1      
    SRC         .req        r2      
    DST_R       .req        ip      
    MASK        .req        r3      

.if (((flags) & FLAG_DST_READWRITE) != 0)
    .set dst_r_bpp, dst_w_bpp
.else
    .set dst_r_bpp, 0
.endif
.if (((flags) & FLAG_DEINTERLEAVE_32BPP) != 0)
    .set DEINTERLEAVE_32BPP_ENABLED, 1
.else
    .set DEINTERLEAVE_32BPP_ENABLED, 0
.endif

    init
    mov         DST_R, DST_W

    cmp         W, #pixblock_size
    blt         8f

    ensure_destination_ptr_alignment process_pixblock_head, \
                                     process_pixblock_tail, \
                                     process_pixblock_tail_head

    subs        W, W, #pixblock_size
    blt         7f

    
    pixld_a     pixblock_size, dst_r_bpp, \
                (dst_r_basereg - pixblock_size * dst_r_bpp / 64), DST_R
    pixld       pixblock_size, src_bpp, \
                (src_basereg - pixblock_size * src_bpp / 64), SRC
    pixld       pixblock_size, mask_bpp, \
                (mask_basereg - pixblock_size * mask_bpp / 64), MASK
    process_pixblock_head
    subs        W, W, #pixblock_size
    blt         2f
1:
    process_pixblock_tail_head
    subs        W, W, #pixblock_size
    bge         1b
2:
    process_pixblock_tail
    pixst_a     pixblock_size, dst_w_bpp, \
                (dst_w_basereg - pixblock_size * dst_w_bpp / 64), DST_W
7:
    
    process_trailing_pixels 0, 1, \
                            process_pixblock_head, \
                            process_pixblock_tail, \
                            process_pixblock_tail_head

    cleanup
    bx         lr  
8:
    
    process_trailing_pixels 0, 0, \
                            process_pixblock_head, \
                            process_pixblock_tail, \
                            process_pixblock_tail_head

    cleanup
    bx          lr  

    .unreq      SRC
    .unreq      MASK
    .unreq      DST_R
    .unreq      DST_W
    .unreq      W
    .endfunc
.endm



.macro default_init
.endm

.macro default_cleanup
.endm







.macro default_init_need_all_regs
    vpush       {d8-d15}
.endm

.macro default_cleanup_need_all_regs
    vpop        {d8-d15}
.endm











.macro convert_0565_to_8888 in, out_a, out_r, out_g, out_b
    vshrn.u16   out_r, in,    #8
    vshrn.u16   out_g, in,    #3
    vsli.u16    in,    in,    #5
    vmov.u8     out_a, #255
    vsri.u8     out_r, out_r, #5
    vsri.u8     out_g, out_g, #6
    vshrn.u16   out_b, in,    #2
.endm

.macro convert_0565_to_x888 in, out_r, out_g, out_b
    vshrn.u16   out_r, in,    #8
    vshrn.u16   out_g, in,    #3
    vsli.u16    in,    in,    #5
    vsri.u8     out_r, out_r, #5
    vsri.u8     out_g, out_g, #6
    vshrn.u16   out_b, in,    #2
.endm







.macro convert_8888_to_0565 in_r, in_g, in_b, out, tmp1, tmp2
    vshll.u8    tmp1, in_g, #8
    vshll.u8    out, in_r, #8
    vshll.u8    tmp2, in_b, #8
    vsri.u16    out, tmp1, #5
    vsri.u16    out, tmp2, #11
.endm
