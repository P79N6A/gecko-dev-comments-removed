




































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

} 

#endif 
