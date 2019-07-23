



















#ifndef __FT_ZOPEN_H__
#define __FT_ZOPEN_H__

#include <ft2build.h>
#include FT_FREETYPE_H


  






#define FT_LZW_IN_BUFF_SIZE        64
#define FT_LZW_DEFAULT_STACK_SIZE  64

#define LZW_INIT_BITS     9
#define LZW_MAX_BITS      16

#define LZW_CLEAR         256
#define LZW_FIRST         257

#define LZW_BIT_MASK      0x1f
#define LZW_BLOCK_MASK    0x80
#define LZW_MASK( n )     ( ( 1U << (n) ) - 1U )


  typedef enum  FT_LzwPhase_
  {
    FT_LZW_PHASE_START = 0,
    FT_LZW_PHASE_CODE,
    FT_LZW_PHASE_STACK,
    FT_LZW_PHASE_EOF

  } FT_LzwPhase;


  




















































  typedef struct  FT_LzwStateRec_
  {
    FT_LzwPhase  phase;
    FT_Int       in_eof;

    FT_Byte      buf_tab[16];
    FT_Int       buf_offset;
    FT_Int       buf_size;
    FT_Bool      buf_clear;
    FT_Int       buf_total;

    FT_UInt      max_bits;    
    FT_Int       block_mode;  
    FT_UInt      max_free;    

    FT_UInt      num_bits;    
    FT_UInt      free_ent;    
    FT_UInt      free_bits;   
    FT_UInt      old_code;
    FT_UInt      old_char;
    FT_UInt      in_code;

    FT_UShort*   prefix;      
    FT_Byte*     suffix;      
    FT_UInt      prefix_size; 

    FT_Byte*     stack;       
    FT_UInt      stack_top;
    FT_UInt      stack_size;
    FT_Byte      stack_0[FT_LZW_DEFAULT_STACK_SIZE]; 

    FT_Stream    source;      
    FT_Memory    memory;

  } FT_LzwStateRec, *FT_LzwState;


  FT_LOCAL( void )
  ft_lzwstate_init( FT_LzwState  state,
                    FT_Stream    source );

  FT_LOCAL( void )
  ft_lzwstate_done( FT_LzwState  state );


  FT_LOCAL( void )
  ft_lzwstate_reset( FT_LzwState  state );


  FT_LOCAL( FT_ULong )
  ft_lzwstate_io( FT_LzwState  state,
                  FT_Byte*     buffer,
                  FT_ULong     out_size );



#endif 



