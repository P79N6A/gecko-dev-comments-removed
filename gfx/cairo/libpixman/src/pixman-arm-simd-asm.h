

























































#undef DEBUG_PARAMS





.set FLAG_DST_WRITEONLY,         0
.set FLAG_DST_READWRITE,         1
.set FLAG_COND_EXEC,             0
.set FLAG_BRANCH_OVER,           2
.set FLAG_PROCESS_PRESERVES_PSR, 0
.set FLAG_PROCESS_CORRUPTS_PSR,  4
.set FLAG_PROCESS_DOESNT_STORE,  0
.set FLAG_PROCESS_DOES_STORE,    8 
.set FLAG_NO_SPILL_LINE_VARS,        0
.set FLAG_SPILL_LINE_VARS_WIDE,      16
.set FLAG_SPILL_LINE_VARS_NON_WIDE,  32
.set FLAG_SPILL_LINE_VARS,           48
.set FLAG_PROCESS_CORRUPTS_SCRATCH,  0
.set FLAG_PROCESS_PRESERVES_SCRATCH, 64




#ifdef DEBUG_PARAMS
.set ARGS_STACK_OFFSET,        (9*4+9*4)
#else
.set ARGS_STACK_OFFSET,        (9*4)
#endif




.set PREFETCH_TYPE_NONE,       0
.set PREFETCH_TYPE_STANDARD,   1





.macro pixldst op, cond=al, numbytes, reg0, reg1, reg2, reg3, base, unaligned=0
 .if numbytes == 16
  .if unaligned == 1
        op&r&cond    WK&reg0, [base], #4
        op&r&cond    WK&reg1, [base], #4
        op&r&cond    WK&reg2, [base], #4
        op&r&cond    WK&reg3, [base], #4
  .else
        op&m&cond&ia base!, {WK&reg0,WK&reg1,WK&reg2,WK&reg3}
  .endif
 .elseif numbytes == 8
  .if unaligned == 1
        op&r&cond    WK&reg0, [base], #4
        op&r&cond    WK&reg1, [base], #4
  .else
        op&m&cond&ia base!, {WK&reg0,WK&reg1}
  .endif
 .elseif numbytes == 4
        op&r&cond    WK&reg0, [base], #4
 .elseif numbytes == 2
        op&r&cond&h  WK&reg0, [base], #2
 .elseif numbytes == 1
        op&r&cond&b  WK&reg0, [base], #1
 .else
  .error "unsupported size: numbytes"
 .endif
.endm

.macro pixst_baseupdated cond, numbytes, reg0, reg1, reg2, reg3, base
 .if numbytes == 16
        stm&cond&db base, {WK&reg0,WK&reg1,WK&reg2,WK&reg3}
 .elseif numbytes == 8
        stm&cond&db base, {WK&reg0,WK&reg1}
 .elseif numbytes == 4
        str&cond    WK&reg0, [base, #-4]
 .elseif numbytes == 2
        str&cond&h  WK&reg0, [base, #-2]
 .elseif numbytes == 1
        str&cond&b  WK&reg0, [base, #-1]
 .else
  .error "unsupported size: numbytes"
 .endif
.endm

.macro pixld cond, numbytes, firstreg, base, unaligned
        pixldst ld, cond, numbytes, %(firstreg+0), %(firstreg+1), %(firstreg+2), %(firstreg+3), base, unaligned
.endm

.macro pixst cond, numbytes, firstreg, base
 .if (flags) & FLAG_DST_READWRITE
        pixst_baseupdated cond, numbytes, %(firstreg+0), %(firstreg+1), %(firstreg+2), %(firstreg+3), base
 .else
        pixldst st, cond, numbytes, %(firstreg+0), %(firstreg+1), %(firstreg+2), %(firstreg+3), base
 .endif
.endm

.macro PF a, x:vararg
 .if (PREFETCH_TYPE_CURRENT == PREFETCH_TYPE_STANDARD)
        a x
 .endif
.endm


.macro preload_leading_step1  bpp, ptr, base




 .if bpp > 0
        PF  bic,    ptr, base, #31
  .set OFFSET, 0
  .rept prefetch_distance+1
        PF  pld,    [ptr, #OFFSET]
   .set OFFSET, OFFSET+32
  .endr
 .endif
.endm

.macro preload_leading_step2  bpp, bpp_shift, ptr, base











 .if bpp > 0
  .ifc base,DST
        
        PF  tst,    base, #16
        PF  beq,    61f
  .else
   .if bpp/dst_w_bpp == 4
        PF  add,    SCRATCH, base, WK0, lsl #bpp_shift-dst_bpp_shift
        PF  and,    SCRATCH, SCRATCH, #31
        PF  rsb,    SCRATCH, SCRATCH, WK0, lsl #bpp_shift-dst_bpp_shift
        PF  sub,    SCRATCH, SCRATCH, #1    
        PF  movs,   SCRATCH, SCRATCH, #32-6 
        PF  bcs,    61f
        PF  bpl,    60f
        PF  pld,    [ptr, #32*(prefetch_distance+2)]
   .else
        PF  mov,    SCRATCH, base, lsl #32-5
        PF  add,    SCRATCH, SCRATCH, WK0, lsl #32-5+bpp_shift-dst_bpp_shift
        PF  rsbs,   SCRATCH, SCRATCH, WK0, lsl #32-5+bpp_shift-dst_bpp_shift
        PF  bls,    61f
   .endif
  .endif
60:     PF  pld,    [ptr, #32*(prefetch_distance+1)]
61:
 .endif
.endm

#define IS_END_OF_GROUP(INDEX,SIZE) ((SIZE) < 2 || ((INDEX) & ~((INDEX)+1)) & ((SIZE)/2))
.macro preload_middle   bpp, base, scratch_holds_offset
 .if bpp > 0
        
  .if IS_END_OF_GROUP(SUBBLOCK,256/128*dst_w_bpp/bpp)
   .if scratch_holds_offset
        PF  pld,    [base, SCRATCH]
   .else
        PF  bic,    SCRATCH, base, #31
        PF  pld,    [SCRATCH, #32*prefetch_distance]
   .endif
  .endif
 .endif
.endm

.macro preload_trailing  bpp, bpp_shift, base
 .if bpp > 0
  .if bpp*pix_per_block > 256
        
        PF  and,    WK1, base, #31
        PF  add,    WK1, WK1, WK0, lsl #bpp_shift
        PF  add,    WK1, WK1, #32*(bpp*pix_per_block/256-1)*(prefetch_distance+1)
        PF  bic,    SCRATCH, base, #31
80:     PF  pld,    [SCRATCH, #32*(prefetch_distance+1)]
        PF  add,    SCRATCH, SCRATCH, #32
        PF  subs,   WK1, WK1, #32
        PF  bhi,    80b
  .else
        
        PF  mov,    SCRATCH, base, lsl #32-5
        PF  adds,   SCRATCH, SCRATCH, X, lsl #32-5+bpp_shift
        PF  adceqs, SCRATCH, SCRATCH, #0
        



        PF  beq,    82f
        PF  bic,    SCRATCH, base, #31
        PF  bcc,    81f
        PF  pld,    [SCRATCH, #32*(prefetch_distance+2)]
81:     PF  pld,    [SCRATCH, #32*(prefetch_distance+1)]
82:
  .endif
 .endif
.endm


.macro preload_line    narrow_case, bpp, bpp_shift, base












 .if bpp > 0
  .if narrow_case && (bpp <= dst_w_bpp)
        
        PF  bic,    WK0, base, #31
        PF  pld,    [WK0]
        PF  add,    WK1, base, X, LSL #bpp_shift
        PF  sub,    WK1, WK1, #1
        PF  bic,    WK1, WK1, #31
        PF  cmp,    WK1, WK0
        PF  beq,    90f
        PF  pld,    [WK1]
90:
  .else
        PF  bic,    WK0, base, #31
        PF  pld,    [WK0]
        PF  add,    WK1, base, X, lsl #bpp_shift
        PF  sub,    WK1, WK1, #1
        PF  bic,    WK1, WK1, #31
        PF  cmp,    WK1, WK0
        PF  beq,    92f
91:     PF  add,    WK0, WK0, #32
        PF  cmp,    WK0, WK1
        PF  pld,    [WK0]
        PF  bne,    91b
92:
  .endif
 .endif
.endm


.macro conditional_process1_helper  cond, process_head, process_tail, numbytes, firstreg, unaligned_src, unaligned_mask, decrementx
        process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, 0
 .if decrementx
        sub&cond X, X, #8*numbytes/dst_w_bpp
 .endif
        process_tail  cond, numbytes, firstreg
 .if !((flags) & FLAG_PROCESS_DOES_STORE)
        pixst   cond, numbytes, firstreg, DST
 .endif
.endm

.macro conditional_process1  cond, process_head, process_tail, numbytes, firstreg, unaligned_src, unaligned_mask, decrementx
 .if (flags) & FLAG_BRANCH_OVER
  .ifc cond,mi
        bpl     100f
  .endif
  .ifc cond,cs
        bcc     100f
  .endif
  .ifc cond,ne
        beq     100f
  .endif
        conditional_process1_helper  , process_head, process_tail, numbytes, firstreg, unaligned_src, unaligned_mask, decrementx
100:
 .else
        conditional_process1_helper  cond, process_head, process_tail, numbytes, firstreg, unaligned_src, unaligned_mask, decrementx
 .endif
.endm

.macro conditional_process2  test, cond1, cond2, process_head, process_tail, numbytes1, numbytes2, firstreg1, firstreg2, unaligned_src, unaligned_mask, decrementx
 .if (flags) & (FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE)
        
        test
        conditional_process1  cond1, process_head, process_tail, numbytes1, firstreg1, unaligned_src, unaligned_mask, decrementx
  .if (flags) & FLAG_PROCESS_CORRUPTS_PSR
        test
  .endif
        conditional_process1  cond2, process_head, process_tail, numbytes2, firstreg2, unaligned_src, unaligned_mask, decrementx
 .else
        
        test
        process_head  cond1, numbytes1, firstreg1, unaligned_src, unaligned_mask, 0
        process_head  cond2, numbytes2, firstreg2, unaligned_src, unaligned_mask, 0
  .if decrementx
        sub&cond1 X, X, #8*numbytes1/dst_w_bpp
        sub&cond2 X, X, #8*numbytes2/dst_w_bpp
  .endif
        process_tail  cond1, numbytes1, firstreg1
        process_tail  cond2, numbytes2, firstreg2
        pixst   cond1, numbytes1, firstreg1, DST
        pixst   cond2, numbytes2, firstreg2, DST
 .endif
.endm


.macro test_bits_1_0_ptr
        movs    SCRATCH, WK0, lsl #32-1  
.endm

.macro test_bits_3_2_ptr
        movs    SCRATCH, WK0, lsl #32-3  
.endm

.macro leading_15bytes  process_head, process_tail
        
        
 .if dst_w_bpp == 8
        conditional_process2  test_bits_1_0_ptr, mi, cs, process_head, process_tail, 1, 2, 1, 2, 1, 1, 1
 .elseif dst_w_bpp == 16
        test_bits_1_0_ptr
        conditional_process1  cs, process_head, process_tail, 2, 2, 1, 1, 1
 .endif
        conditional_process2  test_bits_3_2_ptr, mi, cs, process_head, process_tail, 4, 8, 1, 2, 1, 1, 1
.endm

.macro test_bits_3_2_pix
        movs    SCRATCH, X, lsl #dst_bpp_shift+32-3
.endm

.macro test_bits_1_0_pix
 .if dst_w_bpp == 8
        movs    SCRATCH, X, lsl #dst_bpp_shift+32-1
 .else
        movs    SCRATCH, X, lsr #1
 .endif
.endm

.macro trailing_15bytes  process_head, process_tail, unaligned_src, unaligned_mask
        conditional_process2  test_bits_3_2_pix, cs, mi, process_head, process_tail, 8, 4, 0, 2, unaligned_src, unaligned_mask, 0
 .if dst_w_bpp == 16
        test_bits_1_0_pix
        conditional_process1  cs, process_head, process_tail, 2, 0, unaligned_src, unaligned_mask, 0
 .elseif dst_w_bpp == 8
        conditional_process2  test_bits_1_0_pix, cs, mi, process_head, process_tail, 2, 1, 0, 1, unaligned_src, unaligned_mask, 0
 .endif
.endm


.macro wide_case_inner_loop  process_head, process_tail, unaligned_src, unaligned_mask, dst_alignment
110:
 .set SUBBLOCK, 0 
 .rept pix_per_block*dst_w_bpp/128
        process_head  , 16, 0, unaligned_src, unaligned_mask, 1
  .if (src_bpp > 0) && (mask_bpp == 0) && ((flags) & FLAG_PROCESS_PRESERVES_SCRATCH)
        preload_middle  src_bpp, SRC, 1
  .elseif (src_bpp == 0) && (mask_bpp > 0) && ((flags) & FLAG_PROCESS_PRESERVES_SCRATCH)
        preload_middle  mask_bpp, MASK, 1
  .else
        preload_middle  src_bpp, SRC, 0
        preload_middle  mask_bpp, MASK, 0
  .endif
  .if (dst_r_bpp > 0) && ((SUBBLOCK % 2) == 0)
        




        PF  pld,    [DST, #32*prefetch_distance - dst_alignment]
  .endif
        process_tail  , 16, 0
  .if !((flags) & FLAG_PROCESS_DOES_STORE)
        pixst   , 16, 0, DST
  .endif
  .set SUBBLOCK, SUBBLOCK+1
 .endr
        subs    X, X, #pix_per_block
        bhs     110b
.endm

.macro wide_case_inner_loop_and_trailing_pixels  process_head, process_tail, process_inner_loop, exit_label, unaligned_src, unaligned_mask
        
 .if dst_r_bpp > 0
        tst     DST, #16
        bne     111f
        process_inner_loop  process_head, process_tail, unaligned_src, unaligned_mask, 16
        b       112f
111:
 .endif
        process_inner_loop  process_head, process_tail, unaligned_src, unaligned_mask, 0
112:
        
 .if (src_bpp*pix_per_block > 256) || (mask_bpp*pix_per_block > 256) || (dst_r_bpp*pix_per_block > 256)
        PF  and,    WK0, X, #pix_per_block-1
 .endif
        preload_trailing  src_bpp, src_bpp_shift, SRC
        preload_trailing  mask_bpp, mask_bpp_shift, MASK
        preload_trailing  dst_r_bpp, dst_bpp_shift, DST
        add     X, X, #(prefetch_distance+2)*pix_per_block - 128/dst_w_bpp
        
        medium_case_inner_loop_and_trailing_pixels  process_head, process_tail,, exit_label, unaligned_src, unaligned_mask
.endm

.macro medium_case_inner_loop_and_trailing_pixels  process_head, process_tail, unused, exit_label, unaligned_src, unaligned_mask
120:
        process_head  , 16, 0, unaligned_src, unaligned_mask, 0
        process_tail  , 16, 0
 .if !((flags) & FLAG_PROCESS_DOES_STORE)
        pixst   , 16, 0, DST
 .endif
        subs    X, X, #128/dst_w_bpp
        bhs     120b
        
        tst     X, #128/dst_w_bpp - 1
        beq     exit_label
        trailing_15bytes  process_head, process_tail, unaligned_src, unaligned_mask
.endm

.macro narrow_case_inner_loop_and_trailing_pixels  process_head, process_tail, unused, exit_label, unaligned_src, unaligned_mask
        tst     X, #16*8/dst_w_bpp
        conditional_process1  ne, process_head, process_tail, 16, 0, unaligned_src, unaligned_mask, 0
        
        
        trailing_15bytes  process_head, process_tail, unaligned_src, unaligned_mask
.endm

.macro switch_on_alignment  action, process_head, process_tail, process_inner_loop, exit_label
 
 .if mask_bpp == 8 || mask_bpp == 16
        tst     MASK, #3
        bne     141f
 .endif
  .if src_bpp == 8 || src_bpp == 16
        tst     SRC, #3
        bne     140f
  .endif
        action  process_head, process_tail, process_inner_loop, exit_label, 0, 0
  .if src_bpp == 8 || src_bpp == 16
        b       exit_label
140:
        action  process_head, process_tail, process_inner_loop, exit_label, 1, 0
  .endif
 .if mask_bpp == 8 || mask_bpp == 16
        b       exit_label
141:
  .if src_bpp == 8 || src_bpp == 16
        tst     SRC, #3
        bne     142f
  .endif
        action  process_head, process_tail, process_inner_loop, exit_label, 0, 1
  .if src_bpp == 8 || src_bpp == 16
        b       exit_label
142:
        action  process_head, process_tail, process_inner_loop, exit_label, 1, 1
  .endif
 .endif
.endm


.macro end_of_line      restore_x, vars_spilled, loop_label, last_one
 .if vars_spilled
        
        
        .word   0xE89D0000 | LINE_SAVED_REGS
 .endif
        subs    Y, Y, #1
 .if vars_spilled
  .if (LINE_SAVED_REGS) & (1<<1)
        str     Y, [sp]
  .endif
 .endif
        add     DST, DST, STRIDE_D
 .if src_bpp > 0
        add     SRC, SRC, STRIDE_S
 .endif
 .if mask_bpp > 0
        add     MASK, MASK, STRIDE_M
 .endif
 .if restore_x
        mov     X, ORIG_W
 .endif
        bhs     loop_label
 .ifc "last_one",""
  .if vars_spilled
        b       197f
  .else
        b       198f
  .endif
 .else
  .if (!vars_spilled) && ((flags) & FLAG_SPILL_LINE_VARS)
        b       198f
  .endif
 .endif
.endm


.macro generate_composite_function fname, \
                                   src_bpp_, \
                                   mask_bpp_, \
                                   dst_w_bpp_, \
                                   flags_, \
                                   prefetch_distance_, \
                                   init, \
                                   newline, \
                                   cleanup, \
                                   process_head, \
                                   process_tail, \
                                   process_inner_loop

 .func fname
 .global fname
 
#ifdef __ELF__
 .hidden fname
 .type fname, %function
#endif





 .set src_bpp, src_bpp_
 .set mask_bpp, mask_bpp_
 .set dst_w_bpp, dst_w_bpp_
 .set flags, flags_
 .set prefetch_distance, prefetch_distance_




 .if prefetch_distance == 0
  .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_NONE
 .else
  .set PREFETCH_TYPE_CURRENT, PREFETCH_TYPE_STANDARD
 .endif

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

 .set pix_per_block, 16*8/dst_w_bpp
 .if src_bpp != 0
  .if 32*8/src_bpp > pix_per_block
   .set pix_per_block, 32*8/src_bpp
  .endif
 .endif
 .if mask_bpp != 0
  .if 32*8/mask_bpp > pix_per_block
   .set pix_per_block, 32*8/mask_bpp
  .endif
 .endif
 .if dst_r_bpp != 0
  .if 32*8/dst_r_bpp > pix_per_block
   .set pix_per_block, 32*8/dst_r_bpp
  .endif
 .endif
















    X           .req    r0  
    Y           .req    r1  
    DST         .req    r2  
    STRIDE_D    .req    r3  
    SRC         .req    r4  
    STRIDE_S    .req    r5  
    MASK        .req    r6  
    STRIDE_M    .req    r7  
    WK0         .req    r8  
    WK1         .req    r9
    WK2         .req    r10
    WK3         .req    r11
    SCRATCH     .req    r12
    ORIG_W      .req    r14 

fname:
        push    {r4-r11, lr}        

        subs    Y, Y, #1
        blo     199f

#ifdef DEBUG_PARAMS
        sub     sp, sp, #9*4
#endif

 .if src_bpp > 0
        ldr     SRC, [sp, #ARGS_STACK_OFFSET]
        ldr     STRIDE_S, [sp, #ARGS_STACK_OFFSET+4]
 .endif
 .if mask_bpp > 0
        ldr     MASK, [sp, #ARGS_STACK_OFFSET+8]
        ldr     STRIDE_M, [sp, #ARGS_STACK_OFFSET+12]
 .endif
        
#ifdef DEBUG_PARAMS
        add     Y, Y, #1
        stmia   sp, {r0-r7,pc}
        sub     Y, Y, #1
#endif

        init
        
        lsl     STRIDE_D, #dst_bpp_shift
        sub     STRIDE_D, STRIDE_D, X, lsl #dst_bpp_shift
 .if src_bpp > 0
        lsl     STRIDE_S, #src_bpp_shift
        sub     STRIDE_S, STRIDE_S, X, lsl #src_bpp_shift
 .endif
 .if mask_bpp > 0
        lsl     STRIDE_M, #mask_bpp_shift
        sub     STRIDE_M, STRIDE_M, X, lsl #mask_bpp_shift
 .endif
 
        
        cmp     X, #2*16*8/dst_w_bpp - 1
        blo     170f
 .if src_bpp || mask_bpp || dst_r_bpp 
        
        cmp     X, #(prefetch_distance+3)*pix_per_block - 1
        blo     160f

        
        


        sub     X, X, #(prefetch_distance+2)*pix_per_block
        mov     ORIG_W, X
  .if (flags) & FLAG_SPILL_LINE_VARS_WIDE
        
        .word   0xE92D0000 | LINE_SAVED_REGS
  .endif
151:    
        newline
        preload_leading_step1  src_bpp, WK1, SRC
        preload_leading_step1  mask_bpp, WK2, MASK
        preload_leading_step1  dst_r_bpp, WK3, DST
        
        tst     DST, #15
        beq     154f
        rsb     WK0, DST, #0 
  .if (src_bpp != 0 && src_bpp != 2*dst_w_bpp) || (mask_bpp != 0 && mask_bpp != 2*dst_w_bpp)
        PF  and,    WK0, WK0, #15
  .endif

        preload_leading_step2  src_bpp, src_bpp_shift, WK1, SRC
        preload_leading_step2  mask_bpp, mask_bpp_shift, WK2, MASK
        preload_leading_step2  dst_r_bpp, dst_bpp_shift, WK3, DST

        leading_15bytes  process_head, process_tail
        
154:    
 .if (src_bpp > 0) && (mask_bpp == 0) && ((flags) & FLAG_PROCESS_PRESERVES_SCRATCH)
        and     SCRATCH, SRC, #31
        rsb     SCRATCH, SCRATCH, #32*prefetch_distance
 .elseif (src_bpp == 0) && (mask_bpp > 0) && ((flags) & FLAG_PROCESS_PRESERVES_SCRATCH)
        and     SCRATCH, MASK, #31
        rsb     SCRATCH, SCRATCH, #32*prefetch_distance
 .endif
 .ifc "process_inner_loop",""
        switch_on_alignment  wide_case_inner_loop_and_trailing_pixels, process_head, process_tail, wide_case_inner_loop, 157f
 .else
        switch_on_alignment  wide_case_inner_loop_and_trailing_pixels, process_head, process_tail, process_inner_loop, 157f
 .endif

157:    
        end_of_line 1, %((flags) & FLAG_SPILL_LINE_VARS_WIDE), 151b
 .endif

 .ltorg

160:    
        mov     ORIG_W, X
 .if (flags) & FLAG_SPILL_LINE_VARS_NON_WIDE
        
        .word   0xE92D0000 | LINE_SAVED_REGS
 .endif
161:    
        newline
        preload_line 0, src_bpp, src_bpp_shift, SRC  
        preload_line 0, mask_bpp, mask_bpp_shift, MASK
        preload_line 0, dst_r_bpp, dst_bpp_shift, DST
        
        sub     X, X, #128/dst_w_bpp     
        tst     DST, #15
        beq     164f
        rsb     WK0, DST, #0 
        
        leading_15bytes  process_head, process_tail
        
164:    
        switch_on_alignment  medium_case_inner_loop_and_trailing_pixels, process_head, process_tail,, 167f
        
167:    
        end_of_line 1, %((flags) & FLAG_SPILL_LINE_VARS_NON_WIDE), 161b

 .ltorg

170:    
 .if dst_w_bpp < 32
        mov     ORIG_W, X
 .endif
 .if (flags) & FLAG_SPILL_LINE_VARS_NON_WIDE
        
        .word   0xE92D0000 | LINE_SAVED_REGS
 .endif
171:    
        newline
        preload_line 1, src_bpp, src_bpp_shift, SRC  
        preload_line 1, mask_bpp, mask_bpp_shift, MASK
        preload_line 1, dst_r_bpp, dst_bpp_shift, DST
        
 .if dst_w_bpp == 8
        tst     DST, #3
        beq     174f
172:    subs    X, X, #1
        blo     177f
        process_head  , 1, 0, 1, 1, 0
        process_tail  , 1, 0
  .if !((flags) & FLAG_PROCESS_DOES_STORE)
        pixst   , 1, 0, DST
  .endif
        tst     DST, #3
        bne     172b
 .elseif dst_w_bpp == 16
        tst     DST, #2
        beq     174f
        subs    X, X, #1
        blo     177f
        process_head  , 2, 0, 1, 1, 0
        process_tail  , 2, 0
  .if !((flags) & FLAG_PROCESS_DOES_STORE)
        pixst   , 2, 0, DST
  .endif
 .endif

174:    
        switch_on_alignment  narrow_case_inner_loop_and_trailing_pixels, process_head, process_tail,, 177f

177:    
        end_of_line %(dst_w_bpp < 32), %((flags) & FLAG_SPILL_LINE_VARS_NON_WIDE), 171b, last_one

197:
 .if (flags) & FLAG_SPILL_LINE_VARS
        add     sp, sp, #LINE_SAVED_REG_COUNT*4
 .endif
198:
        cleanup

#ifdef DEBUG_PARAMS
        add     sp, sp, #9*4 
#endif
199:
        pop     {r4-r11, pc}  

 .ltorg

    .unreq  X
    .unreq  Y
    .unreq  DST
    .unreq  STRIDE_D
    .unreq  SRC
    .unreq  STRIDE_S
    .unreq  MASK
    .unreq  STRIDE_M
    .unreq  WK0
    .unreq  WK1
    .unreq  WK2
    .unreq  WK3
    .unreq  SCRATCH
    .unreq  ORIG_W
    .endfunc
.endm

.macro line_saved_regs  x:vararg
 .set LINE_SAVED_REGS, 0
 .set LINE_SAVED_REG_COUNT, 0
 .irp SAVED_REG,x
  .ifc "SAVED_REG","Y"
   .set LINE_SAVED_REGS, LINE_SAVED_REGS | (1<<1)
   .set LINE_SAVED_REG_COUNT, LINE_SAVED_REG_COUNT + 1
  .endif
  .ifc "SAVED_REG","STRIDE_D"
   .set LINE_SAVED_REGS, LINE_SAVED_REGS | (1<<3)
   .set LINE_SAVED_REG_COUNT, LINE_SAVED_REG_COUNT + 1
  .endif
  .ifc "SAVED_REG","STRIDE_S"
   .set LINE_SAVED_REGS, LINE_SAVED_REGS | (1<<5)
   .set LINE_SAVED_REG_COUNT, LINE_SAVED_REG_COUNT + 1
  .endif
  .ifc "SAVED_REG","STRIDE_M"
   .set LINE_SAVED_REGS, LINE_SAVED_REGS | (1<<7)
   .set LINE_SAVED_REG_COUNT, LINE_SAVED_REG_COUNT + 1
  .endif
  .ifc "SAVED_REG","ORIG_W"
   .set LINE_SAVED_REGS, LINE_SAVED_REGS | (1<<14)
   .set LINE_SAVED_REG_COUNT, LINE_SAVED_REG_COUNT + 1
  .endif
 .endr
.endm

.macro nop_macro x:vararg
.endm
