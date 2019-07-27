




































#ifndef LulDwarfInt_h
#define LulDwarfInt_h

#include "LulCommonExt.h"
#include "LulDwarfExt.h"

namespace lul {







enum DwarfCFI
  {
    DW_CFA_advance_loc        = 0x40,
    DW_CFA_offset             = 0x80,
    DW_CFA_restore            = 0xc0,
    DW_CFA_nop                = 0x00,
    DW_CFA_set_loc            = 0x01,
    DW_CFA_advance_loc1       = 0x02,
    DW_CFA_advance_loc2       = 0x03,
    DW_CFA_advance_loc4       = 0x04,
    DW_CFA_offset_extended    = 0x05,
    DW_CFA_restore_extended   = 0x06,
    DW_CFA_undefined          = 0x07,
    DW_CFA_same_value         = 0x08,
    DW_CFA_register           = 0x09,
    DW_CFA_remember_state     = 0x0a,
    DW_CFA_restore_state      = 0x0b,
    DW_CFA_def_cfa            = 0x0c,
    DW_CFA_def_cfa_register   = 0x0d,
    DW_CFA_def_cfa_offset     = 0x0e,
    DW_CFA_def_cfa_expression = 0x0f,
    DW_CFA_expression         = 0x10,
    DW_CFA_offset_extended_sf = 0x11,
    DW_CFA_def_cfa_sf         = 0x12,
    DW_CFA_def_cfa_offset_sf  = 0x13,
    DW_CFA_val_offset         = 0x14,
    DW_CFA_val_offset_sf      = 0x15,
    DW_CFA_val_expression     = 0x16,

    
    DW_CFA_lo_user = 0x1c,
    DW_CFA_hi_user = 0x3f,

    
    DW_CFA_MIPS_advance_loc8 = 0x1d,

    
    DW_CFA_GNU_window_save = 0x2d,
    DW_CFA_GNU_args_size = 0x2e,
    DW_CFA_GNU_negative_offset_extended = 0x2f
  };


enum DwarfZAugmentationCodes {
  
  
  
  DW_Z_augmentation_start = 'z',

  
  
  
  
  DW_Z_has_LSDA = 'L',

  
  
  
  DW_Z_has_personality_routine = 'P',

  
  
  
  
  DW_Z_has_FDE_address_encoding = 'R',

  
  
  
  
  DW_Z_is_signal_trampoline = 'S'
};


enum DwarfExpressionOpcodes {
  DW_OP_addr    = 0x03,
  DW_OP_deref   = 0x06,
  DW_OP_const1s = 0x09,
  DW_OP_const2u = 0x0a,
  DW_OP_const2s = 0x0b,
  DW_OP_const4u = 0x0c,
  DW_OP_const4s = 0x0d,
  DW_OP_const8u = 0x0e,
  DW_OP_const8s = 0x0f,
  DW_OP_constu  = 0x10,
  DW_OP_consts  = 0x11,
  DW_OP_dup     = 0x12,
  DW_OP_drop    = 0x13,
  DW_OP_over    = 0x14,
  DW_OP_pick    = 0x15,
  DW_OP_swap    = 0x16,
  DW_OP_rot     = 0x17,
  DW_OP_xderef  = 0x18,
  DW_OP_abs     = 0x19,
  DW_OP_and     = 0x1a,
  DW_OP_div     = 0x1b,
  DW_OP_minus   = 0x1c,
  DW_OP_mod     = 0x1d,
  DW_OP_mul     = 0x1e,
  DW_OP_neg     = 0x1f,
  DW_OP_not     = 0x20,
  DW_OP_or      = 0x21,
  DW_OP_plus    = 0x22,
  DW_OP_plus_uconst = 0x23,
  DW_OP_shl     = 0x24,
  DW_OP_shr     = 0x25,
  DW_OP_shra    = 0x26,
  DW_OP_xor     = 0x27,
  DW_OP_skip    = 0x2f,
  DW_OP_bra     = 0x28,
  DW_OP_eq      = 0x29,
  DW_OP_ge      = 0x2a,
  DW_OP_gt      = 0x2b,
  DW_OP_le      = 0x2c,
  DW_OP_lt      = 0x2d,
  DW_OP_ne      = 0x2e,
  DW_OP_lit0    = 0x30,
  DW_OP_lit31   = 0x4f,
  DW_OP_reg0    = 0x50,
  DW_OP_reg31   = 0x6f,
  DW_OP_breg0   = 0x70,
  DW_OP_breg31  = 0x8f,
  DW_OP_regx    = 0x90,
  DW_OP_fbreg     = 0x91,
  DW_OP_bregx     = 0x92,
  DW_OP_piece     = 0x93,
  DW_OP_deref_size = 0x94,
  DW_OP_xderef_size = 0x95,
  DW_OP_nop       = 0x96,
  DW_OP_push_object_address = 0x97,
  DW_OP_call2     = 0x98,
  DW_OP_call4     = 0x99,
  DW_OP_call_ref  = 0x9a,
  DW_OP_form_tls_address = 0x9b,
  DW_OP_call_frame_cfa = 0x9c,
  DW_OP_bit_piece = 0x9d,
  DW_OP_lo_user   = 0xe0,
  DW_OP_hi_user   = 0xff
};

} 

#endif 
