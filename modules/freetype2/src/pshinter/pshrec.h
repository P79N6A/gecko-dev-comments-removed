

















  
  
  
  
  
  
  
  
  
  


#ifndef __PSHREC_H__
#define __PSHREC_H__


#include <ft2build.h>
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include "pshglob.h"


FT_BEGIN_HEADER


  
  
  
  
  
  
  

  
  typedef struct PS_HintRec_*  PS_Hint;

  
  typedef enum  PS_Hint_Type_
  {
    PS_HINT_TYPE_1 = 1,
    PS_HINT_TYPE_2 = 2

  } PS_Hint_Type;


  
  typedef enum  PS_Hint_Flags_
  {
    PS_HINT_FLAG_GHOST  = 1,
    PS_HINT_FLAG_BOTTOM = 2

  } PS_Hint_Flags;


  
  typedef struct  PS_HintRec_
  {
    FT_Int   pos;
    FT_Int   len;
    FT_UInt  flags;

  } PS_HintRec;


#define ps_hint_is_active( x )  ( (x)->flags & PS_HINT_FLAG_ACTIVE )
#define ps_hint_is_ghost( x )   ( (x)->flags & PS_HINT_FLAG_GHOST  )
#define ps_hint_is_bottom( x )  ( (x)->flags & PS_HINT_FLAG_BOTTOM )


  
  typedef struct  PS_Hint_TableRec_
  {
    FT_UInt  num_hints;
    FT_UInt  max_hints;
    PS_Hint  hints;

  } PS_Hint_TableRec, *PS_Hint_Table;


  
  typedef struct  PS_MaskRec_
  {
    FT_UInt   num_bits;
    FT_UInt   max_bits;
    FT_Byte*  bytes;
    FT_UInt   end_point;

  } PS_MaskRec, *PS_Mask;


  
  typedef struct  PS_Mask_TableRec_
  {
    FT_UInt  num_masks;
    FT_UInt  max_masks;
    PS_Mask  masks;

  } PS_Mask_TableRec, *PS_Mask_Table;


 
  typedef struct  PS_DimensionRec_
  {
    PS_Hint_TableRec  hints;
    PS_Mask_TableRec  masks;
    PS_Mask_TableRec  counters;

  } PS_DimensionRec, *PS_Dimension;


  
  
  
  typedef struct  PS_HintsRec_
  {
    FT_Memory        memory;
    FT_Error         error;
    FT_UInt32        magic;
    PS_Hint_Type     hint_type;
    PS_DimensionRec  dimension[2];

  } PS_HintsRec, *PS_Hints;

  

  
  FT_LOCAL( FT_Error )
  ps_hints_init( PS_Hints   hints,
                 FT_Memory  memory );

  
  FT_LOCAL( void )
  ps_hints_done( PS_Hints  hints );

  
  FT_LOCAL( void )
  t1_hints_funcs_init( T1_Hints_FuncsRec*  funcs );

  
  FT_LOCAL( void )
  t2_hints_funcs_init( T2_Hints_FuncsRec*  funcs );


#ifdef DEBUG_HINTER
  extern PS_Hints  ps_debug_hints;
  extern  int      ps_debug_no_horz_hints;
  extern  int      ps_debug_no_vert_hints;
#endif

 


FT_END_HEADER


#endif 



