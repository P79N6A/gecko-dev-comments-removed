

















#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H
#include FT_TRIGONOMETRY_H
#include FT_SYSTEM_H

#include "ttinterp.h"

#include "tterrors.h"


#ifdef TT_USE_BYTECODE_INTERPRETER


#define TT_MULFIX           FT_MulFix
#define TT_MULDIV           FT_MulDiv
#define TT_MULDIV_NO_ROUND  FT_MulDiv_No_Round


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttinterp

  
  
  
  
  
  
#define MAX_RUNNABLE_OPCODES  1000000L


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#ifndef TT_CONFIG_OPTION_STATIC_INTERPRETER     

#define CUR  (*exc)                             /* see ttobjs.h */

  
  
  
  
  
#define FT_UNUSED_EXEC  FT_UNUSED( exc )

#else                                           

#define CUR  cur

#define FT_UNUSED_EXEC  int  __dummy = __dummy

  static
  TT_ExecContextRec  cur;   

  
  
  

#endif 


  
  
  
  
#define INS_ARG  EXEC_OP_ FT_Long*  args    /* see ttobjs.h for EXEC_OP_ */


  
  
  
  
  
#define FT_UNUSED_ARG  FT_UNUSED_EXEC; FT_UNUSED( args )


  
  
  
  
  
  


#define SKIP_Code() \
          SkipCode( EXEC_ARG )

#define GET_ShortIns() \
          GetShortIns( EXEC_ARG )

#define NORMalize( x, y, v ) \
          Normalize( EXEC_ARG_ x, y, v )

#define SET_SuperRound( scale, flags ) \
          SetSuperRound( EXEC_ARG_ scale, flags )

#define ROUND_None( d, c ) \
          Round_None( EXEC_ARG_ d, c )

#define INS_Goto_CodeRange( range, ip ) \
          Ins_Goto_CodeRange( EXEC_ARG_ range, ip )

#define CUR_Func_move( z, p, d ) \
          CUR.func_move( EXEC_ARG_ z, p, d )

#define CUR_Func_move_orig( z, p, d ) \
          CUR.func_move_orig( EXEC_ARG_ z, p, d )

#define CUR_Func_round( d, c ) \
          CUR.func_round( EXEC_ARG_ d, c )

#define CUR_Func_read_cvt( index ) \
          CUR.func_read_cvt( EXEC_ARG_ index )

#define CUR_Func_write_cvt( index, val ) \
          CUR.func_write_cvt( EXEC_ARG_ index, val )

#define CUR_Func_move_cvt( index, val ) \
          CUR.func_move_cvt( EXEC_ARG_ index, val )

#define CURRENT_Ratio() \
          Current_Ratio( EXEC_ARG )

#define CURRENT_Ppem() \
          Current_Ppem( EXEC_ARG )

#define CUR_Ppem() \
          Cur_PPEM( EXEC_ARG )

#define INS_SxVTL( a, b, c, d ) \
          Ins_SxVTL( EXEC_ARG_ a, b, c, d )

#define COMPUTE_Funcs() \
          Compute_Funcs( EXEC_ARG )

#define COMPUTE_Round( a ) \
          Compute_Round( EXEC_ARG_ a )

#define COMPUTE_Point_Displacement( a, b, c, d ) \
          Compute_Point_Displacement( EXEC_ARG_ a, b, c, d )

#define MOVE_Zp2_Point( a, b, c, t ) \
          Move_Zp2_Point( EXEC_ARG_ a, b, c, t )


#define CUR_Func_project( v1, v2 )  \
          CUR.func_project( EXEC_ARG_ (v1)->x - (v2)->x, (v1)->y - (v2)->y )

#define CUR_Func_dualproj( v1, v2 )  \
          CUR.func_dualproj( EXEC_ARG_ (v1)->x - (v2)->x, (v1)->y - (v2)->y )

#define CUR_fast_project( v ) \
          CUR.func_project( EXEC_ARG_ (v)->x, (v)->y )

#define CUR_fast_dualproj( v ) \
          CUR.func_dualproj( EXEC_ARG_ (v)->x, (v)->y )


  
  
  
  
  typedef void  (*TInstruction_Function)( INS_ARG );


  
  
  
  
#define BOUNDS( x, n )  ( (FT_UInt)(x) >= (FT_UInt)(n) )

#undef  SUCCESS
#define SUCCESS  0

#undef  FAILURE
#define FAILURE  1

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
#define GUESS_VECTOR( V )                                         \
  if ( CUR.face->unpatented_hinting )                             \
  {                                                               \
    CUR.GS.V.x = (FT_F2Dot14)( CUR.GS.both_x_axis ? 0x4000 : 0 ); \
    CUR.GS.V.y = (FT_F2Dot14)( CUR.GS.both_x_axis ? 0 : 0x4000 ); \
  }
#else
#define GUESS_VECTOR( V )
#endif

  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  TT_Goto_CodeRange( TT_ExecContext  exec,
                     FT_Int          range,
                     FT_Long         IP )
  {
    TT_CodeRange*  coderange;


    FT_ASSERT( range >= 1 && range <= 3 );

    coderange = &exec->codeRangeTable[range - 1];

    FT_ASSERT( coderange->base != NULL );

    
    
    
    
    FT_ASSERT( (FT_ULong)IP <= coderange->size );

    exec->code     = coderange->base;
    exec->codeSize = coderange->size;
    exec->IP       = IP;
    exec->curRange = range;

    return TT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  TT_Set_CodeRange( TT_ExecContext  exec,
                    FT_Int          range,
                    void*           base,
                    FT_Long         length )
  {
    FT_ASSERT( range >= 1 && range <= 3 );

    exec->codeRangeTable[range - 1].base = (FT_Byte*)base;
    exec->codeRangeTable[range - 1].size = length;

    return TT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  TT_Clear_CodeRange( TT_ExecContext  exec,
                      FT_Int          range )
  {
    FT_ASSERT( range >= 1 && range <= 3 );

    exec->codeRangeTable[range - 1].base = NULL;
    exec->codeRangeTable[range - 1].size = 0;

    return TT_Err_Ok;
  }


  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  TT_Done_Context( TT_ExecContext  exec )
  {
    FT_Memory  memory = exec->memory;


    
    exec->maxPoints   = 0;
    exec->maxContours = 0;

    
    FT_FREE( exec->stack );
    exec->stackSize = 0;

    
    FT_FREE( exec->callStack );
    exec->callSize = 0;
    exec->callTop  = 0;

    
    FT_FREE( exec->glyphIns );
    exec->glyphSize = 0;

    exec->size = NULL;
    exec->face = NULL;

    FT_FREE( exec );

    return TT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Error
  Init_Context( TT_ExecContext  exec,
                FT_Memory       memory )
  {
    FT_Error  error;


    FT_TRACE1(( "Init_Context: new object at 0x%08p\n", exec ));

    exec->memory   = memory;
    exec->callSize = 32;

    if ( FT_NEW_ARRAY( exec->callStack, exec->callSize ) )
      goto Fail_Memory;

    
    
    exec->maxPoints   = 0;
    exec->maxContours = 0;

    exec->stackSize = 0;
    exec->glyphSize = 0;

    exec->stack     = NULL;
    exec->glyphIns  = NULL;

    exec->face = NULL;
    exec->size = NULL;

    return TT_Err_Ok;

  Fail_Memory:
    FT_ERROR(( "Init_Context: not enough memory for 0x%08lx\n",
               (FT_Long)exec ));
    TT_Done_Context( exec );

    return error;
 }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Error
  Update_Max( FT_Memory  memory,
              FT_ULong*  size,
              FT_Long    multiplier,
              void*      _pbuff,
              FT_ULong   new_max )
  {
    FT_Error  error;
    void**    pbuff = (void**)_pbuff;


    if ( *size < new_max )
    {
      if ( FT_REALLOC( *pbuff, *size * multiplier, new_max * multiplier ) )
        return error;
      *size = new_max;
    }

    return TT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  TT_Load_Context( TT_ExecContext  exec,
                   TT_Face         face,
                   TT_Size         size )
  {
    FT_Int          i;
    FT_ULong        tmp;
    TT_MaxProfile*  maxp;
    FT_Error        error;


    exec->face = face;
    maxp       = &face->max_profile;
    exec->size = size;

    if ( size )
    {
      exec->numFDefs   = size->num_function_defs;
      exec->maxFDefs   = size->max_function_defs;
      exec->numIDefs   = size->num_instruction_defs;
      exec->maxIDefs   = size->max_instruction_defs;
      exec->FDefs      = size->function_defs;
      exec->IDefs      = size->instruction_defs;
      exec->tt_metrics = size->ttmetrics;
      exec->metrics    = size->metrics;

      exec->maxFunc    = size->max_func;
      exec->maxIns     = size->max_ins;

      for ( i = 0; i < TT_MAX_CODE_RANGES; i++ )
        exec->codeRangeTable[i] = size->codeRangeTable[i];

      
      exec->GS = size->GS;

      exec->cvtSize = size->cvt_size;
      exec->cvt     = size->cvt;

      exec->storeSize = size->storage_size;
      exec->storage   = size->storage;

      exec->twilight  = size->twilight;
    }

    
    
    tmp = exec->stackSize;
    error = Update_Max( exec->memory,
                        &tmp,
                        sizeof ( FT_F26Dot6 ),
                        (void*)&exec->stack,
                        maxp->maxStackElements + 32 );
    exec->stackSize = (FT_UInt)tmp;
    if ( error )
      return error;

    tmp = exec->glyphSize;
    error = Update_Max( exec->memory,
                        &tmp,
                        sizeof ( FT_Byte ),
                        (void*)&exec->glyphIns,
                        maxp->maxSizeOfInstructions );
    exec->glyphSize = (FT_UShort)tmp;
    if ( error )
      return error;

    exec->pts.n_points   = 0;
    exec->pts.n_contours = 0;

    exec->zp1 = exec->pts;
    exec->zp2 = exec->pts;
    exec->zp0 = exec->pts;

    exec->instruction_trap = FALSE;

    return TT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  TT_Save_Context( TT_ExecContext  exec,
                   TT_Size         size )
  {
    FT_Int  i;


    
    
    
    size->num_function_defs    = exec->numFDefs;
    size->num_instruction_defs = exec->numIDefs;

    size->max_func = exec->maxFunc;
    size->max_ins  = exec->maxIns;

    for ( i = 0; i < TT_MAX_CODE_RANGES; i++ )
      size->codeRangeTable[i] = exec->codeRangeTable[i];

    return TT_Err_Ok;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  TT_Run_Context( TT_ExecContext  exec,
                  FT_Bool         debug )
  {
    FT_Error  error;


    if ( ( error = TT_Goto_CodeRange( exec, tt_coderange_glyph, 0  ) )
           != TT_Err_Ok )
      return error;

    exec->zp0 = exec->pts;
    exec->zp1 = exec->pts;
    exec->zp2 = exec->pts;

    exec->GS.gep0 = 1;
    exec->GS.gep1 = 1;
    exec->GS.gep2 = 1;

    exec->GS.projVector.x = 0x4000;
    exec->GS.projVector.y = 0x0000;

    exec->GS.freeVector = exec->GS.projVector;
    exec->GS.dualVector = exec->GS.projVector;

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    exec->GS.both_x_axis = TRUE;
#endif

    exec->GS.round_state = 1;
    exec->GS.loop        = 1;

    
    
    exec->top     = 0;
    exec->callTop = 0;

#if 1
    FT_UNUSED( debug );

    return exec->face->interpreter( exec );
#else
    if ( !debug )
      return TT_RunIns( exec );
    else
      return TT_Err_Ok;
#endif
  }


  const TT_GraphicsState  tt_default_graphics_state =
  {
    0, 0, 0,
    { 0x4000, 0 },
    { 0x4000, 0 },
    { 0x4000, 0 },

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    TRUE,
#endif

    1, 64, 1,
    TRUE, 68, 0, 0, 9, 3,
    0, FALSE, 2, 1, 1, 1
  };


  

  FT_EXPORT_DEF( TT_ExecContext )
  TT_New_Context( TT_Driver  driver )
  {
    TT_ExecContext  exec;
    FT_Memory       memory;


    memory = driver->root.root.memory;
    exec   = driver->context;

    if ( !driver->context )
    {
      FT_Error  error;


      
      if ( FT_NEW( exec ) )
        goto Exit;

      
      error = Init_Context( exec, memory );
      if ( error )
        goto Fail;

      
      driver->context = exec;
    }

  Exit:
    return driver->context;

  Fail:
    FT_FREE( exec );

    return 0;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#undef  PACK
#define PACK( x, y )  ( ( x << 4 ) | y )


  static
  const FT_Byte  Pop_Push_Count[256] =
  {
    
    

      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 0, 2 ),
      PACK( 0, 2 ),
      PACK( 0, 0 ),
      PACK( 5, 0 ),

      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 1, 0 ),
      PACK( 0, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),

      PACK( 1, 2 ),
      PACK( 1, 0 ),
      PACK( 0, 0 ),
      PACK( 2, 2 ),
      PACK( 0, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 0 ),
      PACK( 2, 0 ),
      PACK( 0, 0 ),
      PACK( 1, 0 ),
      PACK( 2, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 0, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),

      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 0, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),

      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 2, 0 ),
      PACK( 1, 1 ),
      PACK( 2, 0 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 2, 0 ),
      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 0, 1 ),
      PACK( 0, 1 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 1, 0 ),

      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 0 ),
      PACK( 0, 0 ),
      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),

      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),
      PACK( 1, 1 ),

      PACK( 2, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
     PACK( 1, 0 ),
     PACK( 1, 0 ),
     PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),

      PACK( 0, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 0, 0 ),
      PACK( 0, 0 ),
      PACK( 1, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 1, 1 ),
      PACK( 1, 0 ),
      PACK( 3, 3 ),
      PACK( 2, 1 ),
      PACK( 2, 1 ),
      PACK( 1, 0 ),
      PACK( 2, 0 ),
      PACK( 0, 0 ),

       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),

       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),
       PACK( 0, 0 ),

      PACK( 0, 1 ),
      PACK( 0, 2 ),
      PACK( 0, 3 ),
      PACK( 0, 4 ),
      PACK( 0, 5 ),
      PACK( 0, 6 ),
      PACK( 0, 7 ),
      PACK( 0, 8 ),
      PACK( 0, 1 ),
      PACK( 0, 2 ),
      PACK( 0, 3 ),
      PACK( 0, 4 ),
      PACK( 0, 5 ),
      PACK( 0, 6 ),
      PACK( 0, 7 ),
      PACK( 0, 8 ),

      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),

      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),
      PACK( 1, 0 ),

      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),

      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 ),
      PACK( 2, 0 )
  };


  static
  const FT_Char  opcode_length[256] =
  {
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,

   -1,-2, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    2, 3, 4, 5,  6, 7, 8, 9,  3, 5, 7, 9, 11,13,15,17,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1
  };

#undef PACK

#if 1

  static FT_Int32
  TT_MulFix14( FT_Int32  a,
               FT_Int    b )
  {
    FT_Int32   sign;
    FT_UInt32  ah, al, mid, lo, hi;


    sign = a ^ b;

    if ( a < 0 )
      a = -a;
    if ( b < 0 )
      b = -b;

    ah = (FT_UInt32)( ( a >> 16 ) & 0xFFFFU );
    al = (FT_UInt32)( a & 0xFFFFU );

    lo    = al * b;
    mid   = ah * b;
    hi    = mid >> 16;
    mid   = ( mid << 16 ) + ( 1 << 13 ); 
    lo   += mid;
    if ( lo < mid )
      hi += 1;

    mid = ( lo >> 14 ) | ( hi << 18 );

    return sign >= 0 ? (FT_Int32)mid : -(FT_Int32)mid;
  }

#else

  
  static FT_Int32
  TT_MulFix14( FT_Int32  a,
               FT_Int    b )
  {
    FT_Int32   m, s, hi;
    FT_UInt32  l, lo;


    
    l  = (FT_UInt32)( ( a & 0xFFFFU ) * b );
    m  = ( a >> 16 ) * b;

    lo = l + (FT_UInt32)( m << 16 );
    hi = ( m >> 16 ) + ( (FT_Int32)l >> 31 ) + ( lo < l );

    
    s   = hi >> 31;
    l   = lo + (FT_UInt32)s;
    hi += s + ( l < lo );
    lo  = l;

    l   = lo + 0x2000U;
    hi += l < lo;

    return ( hi << 18 ) | ( l >> 14 );
  }
#endif


  
  static FT_Int32
  TT_DotFix14( FT_Int32  ax,
               FT_Int32  ay,
               FT_Int    bx,
               FT_Int    by )
  {
    FT_Int32   m, s, hi1, hi2, hi;
    FT_UInt32  l, lo1, lo2, lo;


    
    l = (FT_UInt32)( ( ax & 0xFFFFU ) * bx );
    m = ( ax >> 16 ) * bx;

    lo1 = l + (FT_UInt32)( m << 16 );
    hi1 = ( m >> 16 ) + ( (FT_Int32)l >> 31 ) + ( lo1 < l );

    
    l = (FT_UInt32)( ( ay & 0xFFFFU ) * by );
    m = ( ay >> 16 ) * by;

    lo2 = l + (FT_UInt32)( m << 16 );
    hi2 = ( m >> 16 ) + ( (FT_Int32)l >> 31 ) + ( lo2 < l );

    
    lo = lo1 + lo2;
    hi = hi1 + hi2 + ( lo < lo1 );

    
    s   = hi >> 31;
    l   = lo + (FT_UInt32)s;
    hi += s + ( l < lo );
    lo  = l;

    l   = lo + 0x2000U;
    hi += ( l < lo );

    return ( hi << 18 ) | ( l >> 14 );
  }


  

#if 0

  static FT_Int32
  TT_VecLen( FT_Int32  x,
             FT_Int32  y )
  {
    FT_Int32   m, hi1, hi2, hi;
    FT_UInt32  l, lo1, lo2, lo;


    
    lo = (FT_UInt32)( x & 0xFFFFU );
    hi = x >> 16;

    l  = lo * lo;
    m  = hi * lo;
    hi = hi * hi;

    lo1 = l + (FT_UInt32)( m << 17 );
    hi1 = hi + ( m >> 15 ) + ( lo1 < l );

    
    lo = (FT_UInt32)( y & 0xFFFFU );
    hi = y >> 16;

    l  = lo * lo;
    m  = hi * lo;
    hi = hi * hi;

    lo2 = l + (FT_UInt32)( m << 17 );
    hi2 = hi + ( m >> 15 ) + ( lo2 < l );

    
    lo = lo1 + lo2;
    hi = hi1 + hi2 + ( lo < lo1 );

    
    {
      FT_UInt32  root, rem, test_div;
      FT_Int     count;


      root = 0;

      {
        rem   = 0;
        count = 32;
        do
        {
          rem      = ( rem << 2 ) | ( (FT_UInt32)hi >> 30 );
          hi       = (  hi << 2 ) | (            lo >> 30 );
          lo     <<= 2;
          root   <<= 1;
          test_div = ( root << 1 ) + 1;

          if ( rem >= test_div )
          {
            rem  -= test_div;
            root += 1;
          }
        } while ( --count );
      }

      return (FT_Int32)root;
    }
  }

#else

  
  
  
  static FT_F26Dot6
  TT_VecLen( FT_F26Dot6  X,
             FT_F26Dot6  Y )
  {
    FT_Vector  v;


    v.x = X;
    v.y = Y;

    return FT_Vector_Length( &v );
  }

#endif


  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Long
  Current_Ratio( EXEC_OP )
  {
    if ( !CUR.tt_metrics.ratio )
    {
#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
      if ( CUR.face->unpatented_hinting )
      {
        if ( CUR.GS.both_x_axis )
          CUR.tt_metrics.ratio = CUR.tt_metrics.x_ratio;
        else
          CUR.tt_metrics.ratio = CUR.tt_metrics.y_ratio;
      }
      else
#endif
      {
        if ( CUR.GS.projVector.y == 0 )
          CUR.tt_metrics.ratio = CUR.tt_metrics.x_ratio;

        else if ( CUR.GS.projVector.x == 0 )
          CUR.tt_metrics.ratio = CUR.tt_metrics.y_ratio;

        else
        {
          FT_Long  x, y;


          x = TT_MULDIV( CUR.GS.projVector.x,
                         CUR.tt_metrics.x_ratio, 0x4000 );
          y = TT_MULDIV( CUR.GS.projVector.y,
                         CUR.tt_metrics.y_ratio, 0x4000 );
          CUR.tt_metrics.ratio = TT_VecLen( x, y );
        }
      }
    }
    return CUR.tt_metrics.ratio;
  }


  static FT_Long
  Current_Ppem( EXEC_OP )
  {
    return TT_MULFIX( CUR.tt_metrics.ppem, CURRENT_Ratio() );
  }


  
  
  
  
  


  FT_CALLBACK_DEF( FT_F26Dot6 )
  Read_CVT( EXEC_OP_ FT_ULong  idx )
  {
    return CUR.cvt[idx];
  }


  FT_CALLBACK_DEF( FT_F26Dot6 )
  Read_CVT_Stretched( EXEC_OP_ FT_ULong  idx )
  {
    return TT_MULFIX( CUR.cvt[idx], CURRENT_Ratio() );
  }


  FT_CALLBACK_DEF( void )
  Write_CVT( EXEC_OP_ FT_ULong    idx,
                      FT_F26Dot6  value )
  {
    CUR.cvt[idx] = value;
  }


  FT_CALLBACK_DEF( void )
  Write_CVT_Stretched( EXEC_OP_ FT_ULong    idx,
                                FT_F26Dot6  value )
  {
    CUR.cvt[idx] = FT_DivFix( value, CURRENT_Ratio() );
  }


  FT_CALLBACK_DEF( void )
  Move_CVT( EXEC_OP_ FT_ULong    idx,
                     FT_F26Dot6  value )
  {
    CUR.cvt[idx] += value;
  }


  FT_CALLBACK_DEF( void )
  Move_CVT_Stretched( EXEC_OP_ FT_ULong    idx,
                               FT_F26Dot6  value )
  {
    CUR.cvt[idx] += FT_DivFix( value, CURRENT_Ratio() );
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Short
  GetShortIns( EXEC_OP )
  {
    
    CUR.IP += 2;
    return (FT_Short)( ( CUR.code[CUR.IP - 2] << 8 ) +
                         CUR.code[CUR.IP - 1]      );
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_Bool
  Ins_Goto_CodeRange( EXEC_OP_ FT_Int    aRange,
                               FT_ULong  aIP )
  {
    TT_CodeRange*  range;


    if ( aRange < 1 || aRange > 3 )
    {
      CUR.error = TT_Err_Bad_Argument;
      return FAILURE;
    }

    range = &CUR.codeRangeTable[aRange - 1];

    if ( range->base == NULL )     
    {
      CUR.error = TT_Err_Invalid_CodeRange;
      return FAILURE;
    }

    
    
    

    if ( aIP > range->size )
    {
      CUR.error = TT_Err_Code_Overflow;
      return FAILURE;
    }

    CUR.code     = range->base;
    CUR.codeSize = range->size;
    CUR.IP       = aIP;
    CUR.curRange = aRange;

    return SUCCESS;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void
  Direct_Move( EXEC_OP_ TT_GlyphZone  zone,
                        FT_UShort     point,
                        FT_F26Dot6    distance )
  {
    FT_F26Dot6  v;


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    FT_ASSERT( !CUR.face->unpatented_hinting );
#endif

    v = CUR.GS.freeVector.x;

    if ( v != 0 )
    {
      zone->cur[point].x += TT_MULDIV( distance,
                                       v * 0x10000L,
                                       CUR.F_dot_P );

      zone->tags[point] |= FT_CURVE_TAG_TOUCH_X;
    }

    v = CUR.GS.freeVector.y;

    if ( v != 0 )
    {
      zone->cur[point].y += TT_MULDIV( distance,
                                       v * 0x10000L,
                                       CUR.F_dot_P );

      zone->tags[point] |= FT_CURVE_TAG_TOUCH_Y;
    }
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void
  Direct_Move_Orig( EXEC_OP_ TT_GlyphZone  zone,
                             FT_UShort     point,
                             FT_F26Dot6    distance )
  {
    FT_F26Dot6  v;


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    FT_ASSERT( !CUR.face->unpatented_hinting );
#endif

    v = CUR.GS.freeVector.x;

    if ( v != 0 )
      zone->org[point].x += TT_MULDIV( distance,
                                       v * 0x10000L,
                                       CUR.F_dot_P );

    v = CUR.GS.freeVector.y;

    if ( v != 0 )
      zone->org[point].y += TT_MULDIV( distance,
                                       v * 0x10000L,
                                       CUR.F_dot_P );
  }


  
  
  
  
  
  
  
  


  static void
  Direct_Move_X( EXEC_OP_ TT_GlyphZone  zone,
                          FT_UShort     point,
                          FT_F26Dot6    distance )
  {
    FT_UNUSED_EXEC;

    zone->cur[point].x += distance;
    zone->tags[point]  |= FT_CURVE_TAG_TOUCH_X;
  }


  static void
  Direct_Move_Y( EXEC_OP_ TT_GlyphZone  zone,
                          FT_UShort     point,
                          FT_F26Dot6    distance )
  {
    FT_UNUSED_EXEC;

    zone->cur[point].y += distance;
    zone->tags[point]  |= FT_CURVE_TAG_TOUCH_Y;
  }


  
  
  
  
  
  
  
  


  static void
  Direct_Move_Orig_X( EXEC_OP_ TT_GlyphZone  zone,
                               FT_UShort     point,
                               FT_F26Dot6    distance )
  {
    FT_UNUSED_EXEC;

    zone->org[point].x += distance;
  }


  static void
  Direct_Move_Orig_Y( EXEC_OP_ TT_GlyphZone  zone,
                               FT_UShort     point,
                               FT_F26Dot6    distance )
  {
    FT_UNUSED_EXEC;

    zone->org[point].y += distance;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Round_None( EXEC_OP_ FT_F26Dot6  distance,
                       FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation;
      if ( distance && val < 0 )
        val = 0;
    }
    else {
      val = distance - compensation;
      if ( val > 0 )
        val = 0;
    }
    return val;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Round_To_Grid( EXEC_OP_ FT_F26Dot6  distance,
                          FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation + 32;
      if ( distance && val > 0 )
        val &= ~63;
      else
        val = 0;
    }
    else
    {
      val = -FT_PIX_ROUND( compensation - distance );
      if ( val > 0 )
        val = 0;
    }

    return  val;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Round_To_Half_Grid( EXEC_OP_ FT_F26Dot6  distance,
                               FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = FT_PIX_FLOOR( distance + compensation ) + 32;
      if ( distance && val < 0 )
        val = 0;
    }
    else
    {
      val = -( FT_PIX_FLOOR( compensation - distance ) + 32 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Round_Down_To_Grid( EXEC_OP_ FT_F26Dot6  distance,
                               FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation;
      if ( distance && val > 0 )
        val &= ~63;
      else
        val = 0;
    }
    else
    {
      val = -( ( compensation - distance ) & -64 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Round_Up_To_Grid( EXEC_OP_ FT_F26Dot6  distance,
                             FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation + 63;
      if ( distance && val > 0 )
        val &= ~63;
      else
        val = 0;
    }
    else
    {
      val = - FT_PIX_CEIL( compensation - distance );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Round_To_Double_Grid( EXEC_OP_ FT_F26Dot6  distance,
                                 FT_F26Dot6  compensation )
  {
    FT_F26Dot6 val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation + 16;
      if ( distance && val > 0 )
        val &= ~31;
      else
        val = 0;
    }
    else
    {
      val = -FT_PAD_ROUND( compensation - distance, 32 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Round_Super( EXEC_OP_ FT_F26Dot6  distance,
                        FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;


    if ( distance >= 0 )
    {
      val = ( distance - CUR.phase + CUR.threshold + compensation ) &
              -CUR.period;
      if ( distance && val < 0 )
        val = 0;
      val += CUR.phase;
    }
    else
    {
      val = -( ( CUR.threshold - CUR.phase - distance + compensation ) &
               -CUR.period );
      if ( val > 0 )
        val = 0;
      val -= CUR.phase;
    }

    return val;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Round_Super_45( EXEC_OP_ FT_F26Dot6  distance,
                           FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;


    if ( distance >= 0 )
    {
      val = ( ( distance - CUR.phase + CUR.threshold + compensation ) /
                CUR.period ) * CUR.period;
      if ( distance && val < 0 )
        val = 0;
      val += CUR.phase;
    }
    else
    {
      val = -( ( ( CUR.threshold - CUR.phase - distance + compensation ) /
                   CUR.period ) * CUR.period );
      if ( val > 0 )
        val = 0;
      val -= CUR.phase;
    }

    return val;
  }


  
  
  
  
  
  
  
  
  
  
  
  static void
  Compute_Round( EXEC_OP_ FT_Byte  round_mode )
  {
    switch ( round_mode )
    {
    case TT_Round_Off:
      CUR.func_round = (TT_Round_Func)Round_None;
      break;

    case TT_Round_To_Grid:
      CUR.func_round = (TT_Round_Func)Round_To_Grid;
      break;

    case TT_Round_Up_To_Grid:
      CUR.func_round = (TT_Round_Func)Round_Up_To_Grid;
      break;

    case TT_Round_Down_To_Grid:
      CUR.func_round = (TT_Round_Func)Round_Down_To_Grid;
      break;

    case TT_Round_To_Half_Grid:
      CUR.func_round = (TT_Round_Func)Round_To_Half_Grid;
      break;

    case TT_Round_To_Double_Grid:
      CUR.func_round = (TT_Round_Func)Round_To_Double_Grid;
      break;

    case TT_Round_Super:
      CUR.func_round = (TT_Round_Func)Round_Super;
      break;

    case TT_Round_Super_45:
      CUR.func_round = (TT_Round_Func)Round_Super_45;
      break;
    }
  }


  
  
  
  
  
  
  
  
  
  
  
  
  static void
  SetSuperRound( EXEC_OP_ FT_F26Dot6  GridPeriod,
                          FT_Long     selector )
  {
    switch ( (FT_Int)( selector & 0xC0 ) )
    {
      case 0:
        CUR.period = GridPeriod / 2;
        break;

      case 0x40:
        CUR.period = GridPeriod;
        break;

      case 0x80:
        CUR.period = GridPeriod * 2;
        break;

      

      case 0xC0:
        CUR.period = GridPeriod;
        break;
    }

    switch ( (FT_Int)( selector & 0x30 ) )
    {
    case 0:
      CUR.phase = 0;
      break;

    case 0x10:
      CUR.phase = CUR.period / 4;
      break;

    case 0x20:
      CUR.phase = CUR.period / 2;
      break;

    case 0x30:
      CUR.phase = CUR.period * 3 / 4;
      break;
    }

    if ( ( selector & 0x0F ) == 0 )
      CUR.threshold = CUR.period - 1;
    else
      CUR.threshold = ( (FT_Int)( selector & 0x0F ) - 4 ) * CUR.period / 8;

    CUR.period    /= 256;
    CUR.phase     /= 256;
    CUR.threshold /= 256;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Project( EXEC_OP_ FT_Pos  dx,
                    FT_Pos  dy )
  {
#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    FT_ASSERT( !CUR.face->unpatented_hinting );
#endif

    return TT_DotFix14( dx, dy,
                        CUR.GS.projVector.x,
                        CUR.GS.projVector.y );
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Dual_Project( EXEC_OP_ FT_Pos  dx,
                         FT_Pos  dy )
  {
    return TT_DotFix14( dx, dy,
                        CUR.GS.dualVector.x,
                        CUR.GS.dualVector.y );
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Project_x( EXEC_OP_ FT_Pos  dx,
                      FT_Pos  dy )
  {
    FT_UNUSED_EXEC;
    FT_UNUSED( dy );

    return dx;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static FT_F26Dot6
  Project_y( EXEC_OP_ FT_Pos  dx,
                      FT_Pos  dy )
  {
    FT_UNUSED_EXEC;
    FT_UNUSED( dx );

    return dy;
  }


  
  
  
  
  
  
  
  
  
  static void
  Compute_Funcs( EXEC_OP )
  {
#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    if ( CUR.face->unpatented_hinting )
    {
      
      
      
      
      CUR.GS.both_x_axis = (FT_Bool)( CUR.GS.projVector.x == 0x4000 &&
                                      CUR.GS.freeVector.x == 0x4000 );

      
      
      
      CUR.GS.projVector.x = 0;
      CUR.GS.projVector.y = 0;
      CUR.GS.freeVector.x = 0;
      CUR.GS.freeVector.y = 0;

      if ( CUR.GS.both_x_axis )
      {
        CUR.func_project   = Project_x;
        CUR.func_move      = Direct_Move_X;
        CUR.func_move_orig = Direct_Move_Orig_X;
      }
      else
      {
        CUR.func_project   = Project_y;
        CUR.func_move      = Direct_Move_Y;
        CUR.func_move_orig = Direct_Move_Orig_Y;
      }

      if ( CUR.GS.dualVector.x == 0x4000 )
        CUR.func_dualproj = Project_x;
      else
      {
        if ( CUR.GS.dualVector.y == 0x4000 )
          CUR.func_dualproj = Project_y;
        else
          CUR.func_dualproj = Dual_Project;
      }

      
      CUR.tt_metrics.ratio = 0;

      return;
    }
#endif 

    if ( CUR.GS.freeVector.x == 0x4000 )
      CUR.F_dot_P       = CUR.GS.projVector.x * 0x10000L;
    else
    {
      if ( CUR.GS.freeVector.y == 0x4000 )
        CUR.F_dot_P       = CUR.GS.projVector.y * 0x10000L;
      else
        CUR.F_dot_P = (FT_Long)CUR.GS.projVector.x * CUR.GS.freeVector.x * 4 +
                      (FT_Long)CUR.GS.projVector.y * CUR.GS.freeVector.y * 4;
    }

    if ( CUR.GS.projVector.x == 0x4000 )
      CUR.func_project = (TT_Project_Func)Project_x;
    else
    {
      if ( CUR.GS.projVector.y == 0x4000 )
        CUR.func_project = (TT_Project_Func)Project_y;
      else
        CUR.func_project = (TT_Project_Func)Project;
    }

    if ( CUR.GS.dualVector.x == 0x4000 )
      CUR.func_dualproj = (TT_Project_Func)Project_x;
    else
    {
      if ( CUR.GS.dualVector.y == 0x4000 )
        CUR.func_dualproj = (TT_Project_Func)Project_y;
      else
        CUR.func_dualproj = (TT_Project_Func)Dual_Project;
    }

    CUR.func_move      = (TT_Move_Func)Direct_Move;
    CUR.func_move_orig = (TT_Move_Func)Direct_Move_Orig;

    if ( CUR.F_dot_P == 0x40000000L )
    {
      if ( CUR.GS.freeVector.x == 0x4000 )
      {
        CUR.func_move      = (TT_Move_Func)Direct_Move_X;
        CUR.func_move_orig = (TT_Move_Func)Direct_Move_Orig_X;
      }
      else
      {
        if ( CUR.GS.freeVector.y == 0x4000 )
        {
          CUR.func_move      = (TT_Move_Func)Direct_Move_Y;
          CUR.func_move_orig = (TT_Move_Func)Direct_Move_Orig_Y;
        }
      }
    }

    
    

    if ( FT_ABS( CUR.F_dot_P ) < 0x4000000L )
      CUR.F_dot_P = 0x40000000L;

    
    CUR.tt_metrics.ratio = 0;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  static FT_Bool
  Normalize( EXEC_OP_ FT_F26Dot6      Vx,
                      FT_F26Dot6      Vy,
                      FT_UnitVector*  R )
  {
    FT_F26Dot6  W;
    FT_Bool     S1, S2;

    FT_UNUSED_EXEC;


    if ( FT_ABS( Vx ) < 0x10000L && FT_ABS( Vy ) < 0x10000L )
    {
      Vx *= 0x100;
      Vy *= 0x100;

      W = TT_VecLen( Vx, Vy );

      if ( W == 0 )
      {
        
        
        return SUCCESS;
      }

      R->x = (FT_F2Dot14)FT_MulDiv( Vx, 0x4000L, W );
      R->y = (FT_F2Dot14)FT_MulDiv( Vy, 0x4000L, W );

      return SUCCESS;
    }

    W = TT_VecLen( Vx, Vy );

    Vx = FT_MulDiv( Vx, 0x4000L, W );
    Vy = FT_MulDiv( Vy, 0x4000L, W );

    W = Vx * Vx + Vy * Vy;

    
    

    if ( Vx < 0 )
    {
      Vx = -Vx;
      S1 = TRUE;
    }
    else
      S1 = FALSE;

    if ( Vy < 0 )
    {
      Vy = -Vy;
      S2 = TRUE;
    }
    else
      S2 = FALSE;

    while ( W < 0x10000000L )
    {
      
      if ( Vx < Vy )
        Vx++;
      else
        Vy++;

      W = Vx * Vx + Vy * Vy;
    }

    while ( W >= 0x10004000L )
    {
      
      if ( Vx < Vy )
        Vx--;
      else
        Vy--;

      W = Vx * Vx + Vy * Vy;
    }

    
    

    if ( S1 )
      Vx = -Vx;

    if ( S2 )
      Vy = -Vy;

    R->x = (FT_F2Dot14)Vx;   
    R->y = (FT_F2Dot14)Vy;   

    return SUCCESS;
  }


  
  
  
  
  


  static FT_Bool
  Ins_SxVTL( EXEC_OP_ FT_UShort       aIdx1,
                      FT_UShort       aIdx2,
                      FT_Int          aOpc,
                      FT_UnitVector*  Vec )
  {
    FT_Long     A, B, C;
    FT_Vector*  p1;
    FT_Vector*  p2;


    if ( BOUNDS( aIdx1, CUR.zp2.n_points ) ||
         BOUNDS( aIdx2, CUR.zp1.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return FAILURE;
    }

    p1 = CUR.zp1.cur + aIdx2;
    p2 = CUR.zp2.cur + aIdx1;

    A = p1->x - p2->x;
    B = p1->y - p2->y;

    if ( ( aOpc & 1 ) != 0 )
    {
      C =  B;   
      B =  A;
      A = -C;
    }

    NORMalize( A, B, Vec );

    return SUCCESS;
  }


  
  
  
  
  

#define DO_SVTCA                            \
  {                                         \
    FT_Short  A, B;                         \
                                            \
                                            \
    A = (FT_Short)( CUR.opcode & 1 ) << 14; \
    B = A ^ (FT_Short)0x4000;               \
                                            \
    CUR.GS.freeVector.x = A;                \
    CUR.GS.projVector.x = A;                \
    CUR.GS.dualVector.x = A;                \
                                            \
    CUR.GS.freeVector.y = B;                \
    CUR.GS.projVector.y = B;                \
    CUR.GS.dualVector.y = B;                \
                                            \
    COMPUTE_Funcs();                        \
  }


#define DO_SPVTCA                           \
  {                                         \
    FT_Short  A, B;                         \
                                            \
                                            \
    A = (FT_Short)( CUR.opcode & 1 ) << 14; \
    B = A ^ (FT_Short)0x4000;               \
                                            \
    CUR.GS.projVector.x = A;                \
    CUR.GS.dualVector.x = A;                \
                                            \
    CUR.GS.projVector.y = B;                \
    CUR.GS.dualVector.y = B;                \
                                            \
    GUESS_VECTOR( freeVector );             \
                                            \
    COMPUTE_Funcs();                        \
  }


#define DO_SFVTCA                           \
  {                                         \
    FT_Short  A, B;                         \
                                            \
                                            \
    A = (FT_Short)( CUR.opcode & 1 ) << 14; \
    B = A ^ (FT_Short)0x4000;               \
                                            \
    CUR.GS.freeVector.x = A;                \
    CUR.GS.freeVector.y = B;                \
                                            \
    GUESS_VECTOR( projVector );             \
                                            \
    COMPUTE_Funcs();                        \
  }


#define DO_SPVTL                                      \
    if ( INS_SxVTL( (FT_UShort)args[1],               \
                    (FT_UShort)args[0],               \
                    CUR.opcode,                       \
                    &CUR.GS.projVector ) == SUCCESS ) \
    {                                                 \
      CUR.GS.dualVector = CUR.GS.projVector;          \
      GUESS_VECTOR( freeVector );                     \
      COMPUTE_Funcs();                                \
    }


#define DO_SFVTL                                      \
    if ( INS_SxVTL( (FT_UShort)args[1],               \
                    (FT_UShort)args[0],               \
                    CUR.opcode,                       \
                    &CUR.GS.freeVector ) == SUCCESS ) \
    {                                                 \
      GUESS_VECTOR( projVector );                     \
      COMPUTE_Funcs();                                \
    }


#define DO_SFVTPV                          \
    GUESS_VECTOR( projVector );            \
    CUR.GS.freeVector = CUR.GS.projVector; \
    COMPUTE_Funcs();


#define DO_SPVFS                                \
  {                                             \
    FT_Short  S;                                \
    FT_Long   X, Y;                             \
                                                \
                                                \
    /* Only use low 16bits, then sign extend */ \
    S = (FT_Short)args[1];                      \
    Y = (FT_Long)S;                             \
    S = (FT_Short)args[0];                      \
    X = (FT_Long)S;                             \
                                                \
    NORMalize( X, Y, &CUR.GS.projVector );      \
                                                \
    CUR.GS.dualVector = CUR.GS.projVector;      \
    GUESS_VECTOR( freeVector );                 \
    COMPUTE_Funcs();                            \
  }


#define DO_SFVFS                                \
  {                                             \
    FT_Short  S;                                \
    FT_Long   X, Y;                             \
                                                \
                                                \
    /* Only use low 16bits, then sign extend */ \
    S = (FT_Short)args[1];                      \
    Y = (FT_Long)S;                             \
    S = (FT_Short)args[0];                      \
    X = S;                                      \
                                                \
    NORMalize( X, Y, &CUR.GS.freeVector );      \
    GUESS_VECTOR( projVector );                 \
    COMPUTE_Funcs();                            \
  }


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
#define DO_GPV                                   \
    if ( CUR.face->unpatented_hinting )          \
    {                                            \
      args[0] = CUR.GS.both_x_axis ? 0x4000 : 0; \
      args[1] = CUR.GS.both_x_axis ? 0 : 0x4000; \
    }                                            \
    else                                         \
    {                                            \
      args[0] = CUR.GS.projVector.x;             \
      args[1] = CUR.GS.projVector.y;             \
    }
#else
#define DO_GPV                                   \
    args[0] = CUR.GS.projVector.x;               \
    args[1] = CUR.GS.projVector.y;
#endif


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
#define DO_GFV                                   \
    if ( CUR.face->unpatented_hinting )          \
    {                                            \
      args[0] = CUR.GS.both_x_axis ? 0x4000 : 0; \
      args[1] = CUR.GS.both_x_axis ? 0 : 0x4000; \
    }                                            \
    else                                         \
    {                                            \
      args[0] = CUR.GS.freeVector.x;             \
      args[1] = CUR.GS.freeVector.y;             \
    }
#else
#define DO_GFV                                   \
    args[0] = CUR.GS.freeVector.x;               \
    args[1] = CUR.GS.freeVector.y;
#endif


#define DO_SRP0                      \
    CUR.GS.rp0 = (FT_UShort)args[0];


#define DO_SRP1                      \
    CUR.GS.rp1 = (FT_UShort)args[0];


#define DO_SRP2                      \
    CUR.GS.rp2 = (FT_UShort)args[0];


#define DO_RTHG                                         \
    CUR.GS.round_state = TT_Round_To_Half_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_To_Half_Grid;


#define DO_RTG                                     \
    CUR.GS.round_state = TT_Round_To_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_To_Grid;


#define DO_RTDG                                           \
    CUR.GS.round_state = TT_Round_To_Double_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_To_Double_Grid;


#define DO_RUTG                                       \
    CUR.GS.round_state = TT_Round_Up_To_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_Up_To_Grid;


#define DO_RDTG                                         \
    CUR.GS.round_state = TT_Round_Down_To_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_Down_To_Grid;


#define DO_ROFF                                 \
    CUR.GS.round_state = TT_Round_Off;          \
    CUR.func_round = (TT_Round_Func)Round_None;


#define DO_SROUND                                \
    SET_SuperRound( 0x4000, args[0] );           \
    CUR.GS.round_state = TT_Round_Super;         \
    CUR.func_round = (TT_Round_Func)Round_Super;


#define DO_S45ROUND                                 \
    SET_SuperRound( 0x2D41, args[0] );              \
    CUR.GS.round_state = TT_Round_Super_45;         \
    CUR.func_round = (TT_Round_Func)Round_Super_45;


#define DO_SLOOP                       \
    if ( args[0] < 0 )                 \
      CUR.error = TT_Err_Bad_Argument; \
    else                               \
      CUR.GS.loop = args[0];


#define DO_SMD                         \
    CUR.GS.minimum_distance = args[0];


#define DO_SCVTCI                                     \
    CUR.GS.control_value_cutin = (FT_F26Dot6)args[0];


#define DO_SSWCI                                     \
    CUR.GS.single_width_cutin = (FT_F26Dot6)args[0];


    
    
    
    
    
    
#define DO_SSW                                                 \
    CUR.GS.single_width_value = (FT_F26Dot6)( args[0] >> 10 );


#define DO_FLIPON            \
    CUR.GS.auto_flip = TRUE;


#define DO_FLIPOFF            \
    CUR.GS.auto_flip = FALSE;


#define DO_SDB                             \
    CUR.GS.delta_base = (FT_Short)args[0];


#define DO_SDS                              \
    CUR.GS.delta_shift = (FT_Short)args[0];


#define DO_MD


#define DO_MPPEM              \
    args[0] = CURRENT_Ppem();


  
  
#if 0

#define DO_MPS                       \
    args[0] = CUR.metrics.pointSize;

#else

#define DO_MPS                \
    args[0] = CURRENT_Ppem();

#endif 


#define DO_DUP         \
    args[1] = args[0];


#define DO_CLEAR     \
    CUR.new_top = 0;


#define DO_SWAP        \
  {                    \
    FT_Long  L;        \
                       \
                       \
    L       = args[0]; \
    args[0] = args[1]; \
    args[1] = L;       \
  }


#define DO_DEPTH       \
    args[0] = CUR.top;


#define DO_CINDEX                           \
  {                                         \
    FT_Long  L;                             \
                                            \
                                            \
    L = args[0];                            \
                                            \
    if ( L <= 0 || L > CUR.args )           \
      CUR.error = TT_Err_Invalid_Reference; \
    else                                    \
      args[0] = CUR.stack[CUR.args - L];    \
  }


#define DO_JROT               \
    if ( args[1] != 0 )       \
    {                         \
      CUR.IP      += args[0]; \
      CUR.step_ins = FALSE;   \
    }


#define DO_JMPR             \
    CUR.IP      += args[0]; \
    CUR.step_ins = FALSE;


#define DO_JROF               \
    if ( args[1] == 0 )       \
    {                         \
      CUR.IP      += args[0]; \
      CUR.step_ins = FALSE;   \
    }


#define DO_LT                        \
    args[0] = ( args[0] < args[1] );


#define DO_LTEQ                       \
    args[0] = ( args[0] <= args[1] );


#define DO_GT                        \
    args[0] = ( args[0] > args[1] );


#define DO_GTEQ                       \
    args[0] = ( args[0] >= args[1] );


#define DO_EQ                         \
    args[0] = ( args[0] == args[1] );


#define DO_NEQ                        \
    args[0] = ( args[0] != args[1] );


#define DO_ODD                                                  \
    args[0] = ( ( CUR_Func_round( args[0], 0 ) & 127 ) == 64 );


#define DO_EVEN                                                \
    args[0] = ( ( CUR_Func_round( args[0], 0 ) & 127 ) == 0 );


#define DO_AND                        \
    args[0] = ( args[0] && args[1] );


#define DO_OR                         \
    args[0] = ( args[0] || args[1] );


#define DO_NOT          \
    args[0] = !args[0];


#define DO_ADD          \
    args[0] += args[1];


#define DO_SUB          \
    args[0] -= args[1];


#define DO_DIV                                               \
    if ( args[1] == 0 )                                      \
      CUR.error = TT_Err_Divide_By_Zero;                     \
    else                                                     \
      args[0] = TT_MULDIV_NO_ROUND( args[0], 64L, args[1] );


#define DO_MUL                                    \
    args[0] = TT_MULDIV( args[0], args[1], 64L );


#define DO_ABS                   \
    args[0] = FT_ABS( args[0] );


#define DO_NEG          \
    args[0] = -args[0];


#define DO_FLOOR    \
    args[0] = FT_PIX_FLOOR( args[0] );


#define DO_CEILING                    \
    args[0] = FT_PIX_CEIL( args[0] );


#define DO_RS                          \
   {                                   \
     FT_ULong  I = (FT_ULong)args[0];  \
                                       \
                                       \
     if ( BOUNDS( I, CUR.storeSize ) ) \
     {                                 \
       if ( CUR.pedantic_hinting )     \
       {                               \
         ARRAY_BOUND_ERROR;            \
       }                               \
       else                            \
         args[0] = 0;                  \
     }                                 \
     else                              \
       args[0] = CUR.storage[I];       \
   }


#define DO_WS                          \
   {                                   \
     FT_ULong  I = (FT_ULong)args[0];  \
                                       \
                                       \
     if ( BOUNDS( I, CUR.storeSize ) ) \
     {                                 \
       if ( CUR.pedantic_hinting )     \
       {                               \
         ARRAY_BOUND_ERROR;            \
       }                               \
     }                                 \
     else                              \
       CUR.storage[I] = args[1];       \
   }


#define DO_RCVT                          \
   {                                     \
     FT_ULong  I = (FT_ULong)args[0];    \
                                         \
                                         \
     if ( BOUNDS( I, CUR.cvtSize ) )     \
     {                                   \
       if ( CUR.pedantic_hinting )       \
       {                                 \
         ARRAY_BOUND_ERROR;              \
       }                                 \
       else                              \
         args[0] = 0;                    \
     }                                   \
     else                                \
       args[0] = CUR_Func_read_cvt( I ); \
   }


#define DO_WCVTP                         \
   {                                     \
     FT_ULong  I = (FT_ULong)args[0];    \
                                         \
                                         \
     if ( BOUNDS( I, CUR.cvtSize ) )     \
     {                                   \
       if ( CUR.pedantic_hinting )       \
       {                                 \
         ARRAY_BOUND_ERROR;              \
       }                                 \
     }                                   \
     else                                \
       CUR_Func_write_cvt( I, args[1] ); \
   }


#define DO_WCVTF                                                \
   {                                                            \
     FT_ULong  I = (FT_ULong)args[0];                           \
                                                                \
                                                                \
     if ( BOUNDS( I, CUR.cvtSize ) )                            \
     {                                                          \
       if ( CUR.pedantic_hinting )                              \
       {                                                        \
         ARRAY_BOUND_ERROR;                                     \
       }                                                        \
     }                                                          \
     else                                                       \
       CUR.cvt[I] = TT_MULFIX( args[1], CUR.tt_metrics.scale ); \
   }


#define DO_DEBUG                     \
    CUR.error = TT_Err_Debug_OpCode;


#define DO_ROUND                                                   \
    args[0] = CUR_Func_round(                                      \
                args[0],                                           \
                CUR.tt_metrics.compensations[CUR.opcode - 0x68] );


#define DO_NROUND                                                            \
    args[0] = ROUND_None( args[0],                                           \
                          CUR.tt_metrics.compensations[CUR.opcode - 0x6C] );


#define DO_MAX               \
    if ( args[1] > args[0] ) \
      args[0] = args[1];


#define DO_MIN               \
    if ( args[1] < args[0] ) \
      args[0] = args[1];


#ifndef TT_CONFIG_OPTION_INTERPRETER_SWITCH


#undef  ARRAY_BOUND_ERROR
#define ARRAY_BOUND_ERROR                   \
    {                                       \
      CUR.error = TT_Err_Invalid_Reference; \
      return;                               \
    }


  
  
  
  
  
  
  static void
  Ins_SVTCA( INS_ARG )
  {
    DO_SVTCA
  }


  
  
  
  
  
  
  static void
  Ins_SPVTCA( INS_ARG )
  {
    DO_SPVTCA
  }


  
  
  
  
  
  
  static void
  Ins_SFVTCA( INS_ARG )
  {
    DO_SFVTCA
  }


  
  
  
  
  
  
  static void
  Ins_SPVTL( INS_ARG )
  {
    DO_SPVTL
  }


  
  
  
  
  
  
  static void
  Ins_SFVTL( INS_ARG )
  {
    DO_SFVTL
  }


  
  
  
  
  
  
  static void
  Ins_SFVTPV( INS_ARG )
  {
    DO_SFVTPV
  }


  
  
  
  
  
  
  static void
  Ins_SPVFS( INS_ARG )
  {
    DO_SPVFS
  }


  
  
  
  
  
  
  static void
  Ins_SFVFS( INS_ARG )
  {
    DO_SFVFS
  }


  
  
  
  
  
  
  static void
  Ins_GPV( INS_ARG )
  {
    DO_GPV
  }


  
  
  
  
  
  static void
  Ins_GFV( INS_ARG )
  {
    DO_GFV
  }


  
  
  
  
  
  
  static void
  Ins_SRP0( INS_ARG )
  {
    DO_SRP0
  }


  
  
  
  
  
  
  static void
  Ins_SRP1( INS_ARG )
  {
    DO_SRP1
  }


  
  
  
  
  
  
  static void
  Ins_SRP2( INS_ARG )
  {
    DO_SRP2
  }


  
  
  
  
  
  
  static void
  Ins_RTHG( INS_ARG )
  {
    DO_RTHG
  }


  
  
  
  
  
  
  static void
  Ins_RTG( INS_ARG )
  {
    DO_RTG
  }


  
  
  
  
  
  static void
  Ins_RTDG( INS_ARG )
  {
    DO_RTDG
  }


  
  
  
  
  
  static void
  Ins_RUTG( INS_ARG )
  {
    DO_RUTG
  }


  
  
  
  
  
  
  static void
  Ins_RDTG( INS_ARG )
  {
    DO_RDTG
  }


  
  
  
  
  
  
  static void
  Ins_ROFF( INS_ARG )
  {
    DO_ROFF
  }


  
  
  
  
  
  
  static void
  Ins_SROUND( INS_ARG )
  {
    DO_SROUND
  }


  
  
  
  
  
  
  static void
  Ins_S45ROUND( INS_ARG )
  {
    DO_S45ROUND
  }


  
  
  
  
  
  
  static void
  Ins_SLOOP( INS_ARG )
  {
    DO_SLOOP
  }


  
  
  
  
  
  
  static void
  Ins_SMD( INS_ARG )
  {
    DO_SMD
  }


  
  
  
  
  
  
  static void
  Ins_SCVTCI( INS_ARG )
  {
    DO_SCVTCI
  }


  
  
  
  
  
  
  static void
  Ins_SSWCI( INS_ARG )
  {
    DO_SSWCI
  }


  
  
  
  
  
  
  static void
  Ins_SSW( INS_ARG )
  {
    DO_SSW
  }


  
  
  
  
  
  
  static void
  Ins_FLIPON( INS_ARG )
  {
    DO_FLIPON
  }


  
  
  
  
  
  
  static void
  Ins_FLIPOFF( INS_ARG )
  {
    DO_FLIPOFF
  }


  
  
  
  
  
  
  static void
  Ins_SANGW( INS_ARG )
  {
    
  }


  
  
  
  
  
  
  static void
  Ins_SDB( INS_ARG )
  {
    DO_SDB
  }


  
  
  
  
  
  
  static void
  Ins_SDS( INS_ARG )
  {
    DO_SDS
  }


  
  
  
  
  
  
  static void
  Ins_MPPEM( INS_ARG )
  {
    DO_MPPEM
  }


  
  
  
  
  
  
  static void
  Ins_MPS( INS_ARG )
  {
    DO_MPS
  }


  
  
  
  
  
  
  static void
  Ins_DUP( INS_ARG )
  {
    DO_DUP
  }


  
  
  
  
  
  
  static void
  Ins_POP( INS_ARG )
  {
    
  }


  
  
  
  
  
  
  static void
  Ins_CLEAR( INS_ARG )
  {
    DO_CLEAR
  }


  
  
  
  
  
  
  static void
  Ins_SWAP( INS_ARG )
  {
    DO_SWAP
  }


  
  
  
  
  
  
  static void
  Ins_DEPTH( INS_ARG )
  {
    DO_DEPTH
  }


  
  
  
  
  
  
  static void
  Ins_CINDEX( INS_ARG )
  {
    DO_CINDEX
  }


  
  
  
  
  
  
  static void
  Ins_EIF( INS_ARG )
  {
    
  }


  
  
  
  
  
  
  static void
  Ins_JROT( INS_ARG )
  {
    DO_JROT
  }


  
  
  
  
  
  
  static void
  Ins_JMPR( INS_ARG )
  {
    DO_JMPR
  }


  
  
  
  
  
  
  static void
  Ins_JROF( INS_ARG )
  {
    DO_JROF
  }


  
  
  
  
  
  
  static void
  Ins_LT( INS_ARG )
  {
    DO_LT
  }


  
  
  
  
  
  
  static void
  Ins_LTEQ( INS_ARG )
  {
    DO_LTEQ
  }


  
  
  
  
  
  
  static void
  Ins_GT( INS_ARG )
  {
    DO_GT
  }


  
  
  
  
  
  
  static void
  Ins_GTEQ( INS_ARG )
  {
    DO_GTEQ
  }


  
  
  
  
  
  
  static void
  Ins_EQ( INS_ARG )
  {
    DO_EQ
  }


  
  
  
  
  
  
  static void
  Ins_NEQ( INS_ARG )
  {
    DO_NEQ
  }


  
  
  
  
  
  
  static void
  Ins_ODD( INS_ARG )
  {
    DO_ODD
  }


  
  
  
  
  
  
  static void
  Ins_EVEN( INS_ARG )
  {
    DO_EVEN
  }


  
  
  
  
  
  
  static void
  Ins_AND( INS_ARG )
  {
    DO_AND
  }


  
  
  
  
  
  
  static void
  Ins_OR( INS_ARG )
  {
    DO_OR
  }


  
  
  
  
  
  
  static void
  Ins_NOT( INS_ARG )
  {
    DO_NOT
  }


  
  
  
  
  
  
  static void
  Ins_ADD( INS_ARG )
  {
    DO_ADD
  }


  
  
  
  
  
  
  static void
  Ins_SUB( INS_ARG )
  {
    DO_SUB
  }


  
  
  
  
  
  
  static void
  Ins_DIV( INS_ARG )
  {
    DO_DIV
  }


  
  
  
  
  
  
  static void
  Ins_MUL( INS_ARG )
  {
    DO_MUL
  }


  
  
  
  
  
  
  static void
  Ins_ABS( INS_ARG )
  {
    DO_ABS
  }


  
  
  
  
  
  
  static void
  Ins_NEG( INS_ARG )
  {
    DO_NEG
  }


  
  
  
  
  
  
  static void
  Ins_FLOOR( INS_ARG )
  {
    DO_FLOOR
  }


  
  
  
  
  
  
  static void
  Ins_CEILING( INS_ARG )
  {
    DO_CEILING
  }


  
  
  
  
  
  
  static void
  Ins_RS( INS_ARG )
  {
    DO_RS
  }


  
  
  
  
  
  
  static void
  Ins_WS( INS_ARG )
  {
    DO_WS
  }


  
  
  
  
  
  
  static void
  Ins_WCVTP( INS_ARG )
  {
    DO_WCVTP
  }


  
  
  
  
  
  
  static void
  Ins_WCVTF( INS_ARG )
  {
    DO_WCVTF
  }


  
  
  
  
  
  
  static void
  Ins_RCVT( INS_ARG )
  {
    DO_RCVT
  }


  
  
  
  
  
  
  static void
  Ins_AA( INS_ARG )
  {
    
  }


  
  
  
  
  
  
  
  
  static void
  Ins_DEBUG( INS_ARG )
  {
    DO_DEBUG
  }


  
  
  
  
  
  
  static void
  Ins_ROUND( INS_ARG )
  {
    DO_ROUND
  }


  
  
  
  
  
  
  static void
  Ins_NROUND( INS_ARG )
  {
    DO_NROUND
  }


  
  
  
  
  
  
  static void
  Ins_MAX( INS_ARG )
  {
    DO_MAX
  }


  
  
  
  
  
  
  static void
  Ins_MIN( INS_ARG )
  {
    DO_MIN
  }


#endif  


  
  
  
  
  


  
  
  
  
  
  
  static void
  Ins_MINDEX( INS_ARG )
  {
    FT_Long  L, K;


    L = args[0];

    if ( L <= 0 || L > CUR.args )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    K = CUR.stack[CUR.args - L];

    FT_ARRAY_MOVE( &CUR.stack[CUR.args - L    ],
                   &CUR.stack[CUR.args - L + 1],
                   ( L - 1 ) );

    CUR.stack[CUR.args - 1] = K;
  }


  
  
  
  
  
  
  static void
  Ins_ROLL( INS_ARG )
  {
    FT_Long  A, B, C;

    FT_UNUSED_EXEC;


    A = args[2];
    B = args[1];
    C = args[0];

    args[2] = C;
    args[1] = A;
    args[0] = B;
  }


  
  
  
  
  
  
  


  static FT_Bool
  SkipCode( EXEC_OP )
  {
    CUR.IP += CUR.length;

    if ( CUR.IP < CUR.codeSize )
    {
      CUR.opcode = CUR.code[CUR.IP];

      CUR.length = opcode_length[CUR.opcode];
      if ( CUR.length < 0 )
      {
        if ( CUR.IP + 1 > CUR.codeSize )
          goto Fail_Overflow;
        CUR.length = 2 - CUR.length * CUR.code[CUR.IP + 1];
      }

      if ( CUR.IP + CUR.length <= CUR.codeSize )
        return SUCCESS;
    }

  Fail_Overflow:
    CUR.error = TT_Err_Code_Overflow;
    return FAILURE;
  }


  
  
  
  
  
  
  static void
  Ins_IF( INS_ARG )
  {
    FT_Int   nIfs;
    FT_Bool  Out;


    if ( args[0] != 0 )
      return;

    nIfs = 1;
    Out = 0;

    do
    {
      if ( SKIP_Code() == FAILURE )
        return;

      switch ( CUR.opcode )
      {
      case 0x58:      
        nIfs++;
        break;

      case 0x1B:      
        Out = FT_BOOL( nIfs == 1 );
        break;

      case 0x59:      
        nIfs--;
        Out = FT_BOOL( nIfs == 0 );
        break;
      }
    } while ( Out == 0 );
  }


  
  
  
  
  
  
  static void
  Ins_ELSE( INS_ARG )
  {
    FT_Int  nIfs;

    FT_UNUSED_ARG;


    nIfs = 1;

    do
    {
      if ( SKIP_Code() == FAILURE )
        return;

      switch ( CUR.opcode )
      {
      case 0x58:    
        nIfs++;
        break;

      case 0x59:    
        nIfs--;
        break;
      }
    } while ( nIfs != 0 );
  }


  
  
  
  
  
  
  


  
  
  
  
  
  
  static void
  Ins_FDEF( INS_ARG )
  {
    FT_ULong       n;
    TT_DefRecord*  rec;
    TT_DefRecord*  limit;


    
    

    rec   = CUR.FDefs;
    limit = rec + CUR.numFDefs;
    n     = args[0];

    for ( ; rec < limit; rec++ )
    {
      if ( rec->opc == n )
        break;
    }

    if ( rec == limit )
    {
      
      if ( CUR.numFDefs >= CUR.maxFDefs )
      {
        CUR.error = TT_Err_Too_Many_Function_Defs;
        return;
      }
      CUR.numFDefs++;
    }

    rec->range  = CUR.curRange;
    rec->opc    = n;
    rec->start  = CUR.IP + 1;
    rec->active = TRUE;

    if ( n > CUR.maxFunc )
      CUR.maxFunc = n;

    
    

    while ( SKIP_Code() == SUCCESS )
    {
      switch ( CUR.opcode )
      {
      case 0x89:    
      case 0x2C:    
        CUR.error = TT_Err_Nested_DEFS;
        return;

      case 0x2D:   
        return;
      }
    }
  }


  
  
  
  
  
  
  static void
  Ins_ENDF( INS_ARG )
  {
    TT_CallRec*  pRec;

    FT_UNUSED_ARG;


    if ( CUR.callTop <= 0 )     
    {
      CUR.error = TT_Err_ENDF_In_Exec_Stream;
      return;
    }

    CUR.callTop--;

    pRec = &CUR.callStack[CUR.callTop];

    pRec->Cur_Count--;

    CUR.step_ins = FALSE;

    if ( pRec->Cur_Count > 0 )
    {
      CUR.callTop++;
      CUR.IP = pRec->Cur_Restart;
    }
    else
      
      INS_Goto_CodeRange( pRec->Caller_Range,
                          pRec->Caller_IP );

    

    
    
    
    
    
  }


  
  
  
  
  
  
  static void
  Ins_CALL( INS_ARG )
  {
    FT_ULong       F;
    TT_CallRec*    pCrec;
    TT_DefRecord*  def;


    

    F = args[0];
    if ( BOUNDS( F, CUR.maxFunc + 1 ) )
      goto Fail;

    
    
    
    
    
    
    
    

    def = CUR.FDefs + F;
    if ( CUR.maxFunc + 1 != CUR.numFDefs || def->opc != F )
    {
      
      TT_DefRecord*  limit;


      def   = CUR.FDefs;
      limit = def + CUR.numFDefs;

      while ( def < limit && def->opc != F )
        def++;

      if ( def == limit )
        goto Fail;
    }

    
    if ( !def->active )
      goto Fail;

    
    if ( CUR.callTop >= CUR.callSize )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    pCrec = CUR.callStack + CUR.callTop;

    pCrec->Caller_Range = CUR.curRange;
    pCrec->Caller_IP    = CUR.IP + 1;
    pCrec->Cur_Count    = 1;
    pCrec->Cur_Restart  = def->start;

    CUR.callTop++;

    INS_Goto_CodeRange( def->range,
                        def->start );

    CUR.step_ins = FALSE;
    return;

  Fail:
    CUR.error = TT_Err_Invalid_Reference;
  }


  
  
  
  
  
  
  static void
  Ins_LOOPCALL( INS_ARG )
  {
    FT_ULong       F;
    TT_CallRec*    pCrec;
    TT_DefRecord*  def;


    
    F = args[1];
    if ( BOUNDS( F, CUR.maxFunc + 1 ) )
      goto Fail;

    
    
    
    
    
    
    
    

    def = CUR.FDefs + F;
    if ( CUR.maxFunc + 1 != CUR.numFDefs || def->opc != F )
    {
      
      TT_DefRecord*  limit;


      def   = CUR.FDefs;
      limit = def + CUR.numFDefs;

      while ( def < limit && def->opc != F )
        def++;

      if ( def == limit )
        goto Fail;
    }

    
    if ( !def->active )
      goto Fail;

    
    if ( CUR.callTop >= CUR.callSize )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    if ( args[0] > 0 )
    {
      pCrec = CUR.callStack + CUR.callTop;

      pCrec->Caller_Range = CUR.curRange;
      pCrec->Caller_IP    = CUR.IP + 1;
      pCrec->Cur_Count    = (FT_Int)args[0];
      pCrec->Cur_Restart  = def->start;

      CUR.callTop++;

      INS_Goto_CodeRange( def->range, def->start );

      CUR.step_ins = FALSE;
    }
    return;

  Fail:
    CUR.error = TT_Err_Invalid_Reference;
  }


  
  
  
  
  
  
  static void
  Ins_IDEF( INS_ARG )
  {
    TT_DefRecord*  def;
    TT_DefRecord*  limit;


    

    def   = CUR.IDefs;
    limit = def + CUR.numIDefs;

    for ( ; def < limit; def++ )
      if ( def->opc == (FT_ULong)args[0] )
        break;

    if ( def == limit )
    {
      
      if ( CUR.numIDefs >= CUR.maxIDefs )
      {
        CUR.error = TT_Err_Too_Many_Instruction_Defs;
        return;
      }
      CUR.numIDefs++;
    }

    def->opc    = args[0];
    def->start  = CUR.IP+1;
    def->range  = CUR.curRange;
    def->active = TRUE;

    if ( (FT_ULong)args[0] > CUR.maxIns )
      CUR.maxIns = args[0];

    
    

    while ( SKIP_Code() == SUCCESS )
    {
      switch ( CUR.opcode )
      {
      case 0x89:   
      case 0x2C:   
        CUR.error = TT_Err_Nested_DEFS;
        return;
      case 0x2D:   
        return;
      }
    }
  }


  
  
  
  
  
  
  


  
  
  
  
  
  
  static void
  Ins_NPUSHB( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)CUR.code[CUR.IP + 1];

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    for ( K = 1; K <= L; K++ )
      args[K - 1] = CUR.code[CUR.IP + K + 1];

    CUR.new_top += L;
  }


  
  
  
  
  
  
  static void
  Ins_NPUSHW( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)CUR.code[CUR.IP + 1];

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    CUR.IP += 2;

    for ( K = 0; K < L; K++ )
      args[K] = GET_ShortIns();

    CUR.step_ins = FALSE;
    CUR.new_top += L;
  }


  
  
  
  
  
  
  static void
  Ins_PUSHB( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)( CUR.opcode - 0xB0 + 1 );

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    for ( K = 1; K <= L; K++ )
      args[K - 1] = CUR.code[CUR.IP + K];
  }


  
  
  
  
  
  
  static void
  Ins_PUSHW( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)( CUR.opcode - 0xB8 + 1 );

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    CUR.IP++;

    for ( K = 0; K < L; K++ )
      args[K] = GET_ShortIns();

    CUR.step_ins = FALSE;
  }


  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  static void
  Ins_GC( INS_ARG )
  {
    FT_ULong    L;
    FT_F26Dot6  R;


    L = (FT_ULong)args[0];

    if ( BOUNDS( L, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }
      else
        R = 0;
    }
    else
    {
      if ( CUR.opcode & 1 )
        R = CUR_fast_dualproj( &CUR.zp2.org[L] );
      else
        R = CUR_fast_project( &CUR.zp2.cur[L] );
    }

    args[0] = R;
  }


  
  
  
  
  
  
  
  
  
  
  static void
  Ins_SCFS( INS_ARG )
  {
    FT_Long    K;
    FT_UShort  L;


    L = (FT_UShort)args[0];

    if ( BOUNDS( L, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    K = CUR_fast_project( &CUR.zp2.cur[L] );

    CUR_Func_move( &CUR.zp2, L, args[1] - K );

    

    if ( CUR.GS.gep2 == 0 )
      CUR.zp2.org[L] = CUR.zp2.cur[L];
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void
  Ins_MD( INS_ARG )
  {
    FT_UShort   K, L;
    FT_F26Dot6  D;


    K = (FT_UShort)args[1];
    L = (FT_UShort)args[0];

    if( BOUNDS( L, CUR.zp0.n_points ) ||
        BOUNDS( K, CUR.zp1.n_points ) )
    {
      if ( CUR.pedantic_hinting )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }
      D = 0;
    }
    else
    {
      if ( CUR.opcode & 1 )
        D = CUR_Func_project( CUR.zp0.cur + L, CUR.zp1.cur + K );
      else
      {
        FT_Vector*  vec1 = CUR.zp0.orus + L;
        FT_Vector*  vec2 = CUR.zp1.orus + K;


        if ( CUR.metrics.x_scale == CUR.metrics.y_scale )
        {
          
          D = CUR_Func_dualproj( vec1, vec2 );
          D = TT_MULFIX( D, CUR.metrics.x_scale );
        }
        else
        {
          FT_Vector  vec;


          vec.x = TT_MULFIX( vec1->x - vec2->x, CUR.metrics.x_scale );
          vec.y = TT_MULFIX( vec1->y - vec2->y, CUR.metrics.y_scale );

          D = CUR_fast_dualproj( &vec );
        }
      }
    }

    args[0] = D;
  }


  
  
  
  
  
  
  static void
  Ins_SDPVTL( INS_ARG )
  {
    FT_Long    A, B, C;
    FT_UShort  p1, p2;   


    p1 = (FT_UShort)args[1];
    p2 = (FT_UShort)args[0];

    if ( BOUNDS( p2, CUR.zp1.n_points ) ||
         BOUNDS( p1, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    {
      FT_Vector* v1 = CUR.zp1.org + p2;
      FT_Vector* v2 = CUR.zp2.org + p1;


      A = v1->x - v2->x;
      B = v1->y - v2->y;
    }

    if ( ( CUR.opcode & 1 ) != 0 )
    {
      C =  B;   
      B =  A;
      A = -C;
    }

    NORMalize( A, B, &CUR.GS.dualVector );

    {
      FT_Vector*  v1 = CUR.zp1.cur + p2;
      FT_Vector*  v2 = CUR.zp2.cur + p1;


      A = v1->x - v2->x;
      B = v1->y - v2->y;
    }

    if ( ( CUR.opcode & 1 ) != 0 )
    {
      C =  B;   
      B =  A;
      A = -C;
    }

    NORMalize( A, B, &CUR.GS.projVector );

    GUESS_VECTOR( freeVector );

    COMPUTE_Funcs();
  }


  
  
  
  
  
  
  static void
  Ins_SZP0( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp0 = CUR.twilight;
      break;

    case 1:
      CUR.zp0 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep0 = (FT_UShort)args[0];
  }


  
  
  
  
  
  
  static void
  Ins_SZP1( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp1 = CUR.twilight;
      break;

    case 1:
      CUR.zp1 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep1 = (FT_UShort)args[0];
  }


  
  
  
  
  
  
  static void
  Ins_SZP2( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp2 = CUR.twilight;
      break;

    case 1:
      CUR.zp2 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep2 = (FT_UShort)args[0];
  }


  
  
  
  
  
  
  static void
  Ins_SZPS( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp0 = CUR.twilight;
      break;

    case 1:
      CUR.zp0 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.zp1 = CUR.zp0;
    CUR.zp2 = CUR.zp0;

    CUR.GS.gep0 = (FT_UShort)args[0];
    CUR.GS.gep1 = (FT_UShort)args[0];
    CUR.GS.gep2 = (FT_UShort)args[0];
  }


  
  
  
  
  
  
  static void
  Ins_INSTCTRL( INS_ARG )
  {
    FT_Long  K, L;


    K = args[1];
    L = args[0];

    if ( K < 1 || K > 2 )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( L != 0 )
        L = K;

    CUR.GS.instruct_control = FT_BOOL(
      ( (FT_Byte)CUR.GS.instruct_control & ~(FT_Byte)K ) | (FT_Byte)L );
  }


  
  
  
  
  
  
  static void
  Ins_SCANCTRL( INS_ARG )
  {
    FT_Int  A;


    
    A = (FT_Int)( args[0] & 0xFF );

    if ( A == 0xFF )
    {
      CUR.GS.scan_control = TRUE;
      return;
    }
    else if ( A == 0 )
    {
      CUR.GS.scan_control = FALSE;
      return;
    }

    if ( ( args[0] & 0x100 ) != 0 && CUR.tt_metrics.ppem < A )
      CUR.GS.scan_control = TRUE;

    if ( ( args[0] & 0x200 ) != 0 && CUR.tt_metrics.rotated )
      CUR.GS.scan_control = TRUE;

    if ( ( args[0] & 0x400 ) != 0 && CUR.tt_metrics.stretched )
      CUR.GS.scan_control = TRUE;

    if ( ( args[0] & 0x800 ) != 0 && CUR.tt_metrics.ppem >= A )
      CUR.GS.scan_control = FALSE;

    if ( ( args[0] & 0x1000 ) != 0 && CUR.tt_metrics.rotated )
      CUR.GS.scan_control = FALSE;

    if ( ( args[0] & 0x2000 ) != 0 && CUR.tt_metrics.stretched )
      CUR.GS.scan_control = FALSE;
  }


  
  
  
  
  
  
  static void
  Ins_SCANTYPE( INS_ARG )
  {
    if ( args[0] >= 0 )
      CUR.GS.scan_type = (FT_Int)args[0];
  }


  
  
  
  
  
  
  


  
  
  
  
  
  
  static void
  Ins_FLIPPT( INS_ARG )
  {
    FT_UShort  point;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Too_Few_Arguments;
      return;
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.pts.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
        CUR.pts.tags[point] ^= FT_CURVE_TAG_ON;

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  
  
  
  
  
  
  static void
  Ins_FLIPRGON( INS_ARG )
  {
    FT_UShort  I, K, L;


    K = (FT_UShort)args[1];
    L = (FT_UShort)args[0];

    if ( BOUNDS( K, CUR.pts.n_points ) ||
         BOUNDS( L, CUR.pts.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    for ( I = L; I <= K; I++ )
      CUR.pts.tags[I] |= FT_CURVE_TAG_ON;
  }


  
  
  
  
  
  
  static void
  Ins_FLIPRGOFF( INS_ARG )
  {
    FT_UShort  I, K, L;


    K = (FT_UShort)args[1];
    L = (FT_UShort)args[0];

    if ( BOUNDS( K, CUR.pts.n_points ) ||
         BOUNDS( L, CUR.pts.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    for ( I = L; I <= K; I++ )
      CUR.pts.tags[I] &= ~FT_CURVE_TAG_ON;
  }


  static FT_Bool
  Compute_Point_Displacement( EXEC_OP_ FT_F26Dot6*   x,
                                       FT_F26Dot6*   y,
                                       TT_GlyphZone  zone,
                                       FT_UShort*    refp )
  {
    TT_GlyphZoneRec  zp;
    FT_UShort        p;
    FT_F26Dot6       d;


    if ( CUR.opcode & 1 )
    {
      zp = CUR.zp0;
      p  = CUR.GS.rp1;
    }
    else
    {
      zp = CUR.zp1;
      p  = CUR.GS.rp2;
    }

    if ( BOUNDS( p, zp.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      *refp = 0;
      return FAILURE;
    }

    *zone = zp;
    *refp = p;

    d = CUR_Func_project( zp.cur + p, zp.org + p );

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    if ( CUR.face->unpatented_hinting )
    {
      if ( CUR.GS.both_x_axis )
      {
        *x = d;
        *y = 0;
      }
      else
      {
        *x = 0;
        *y = d;
      }
    }
    else
#endif
    {
      *x = TT_MULDIV( d,
                      (FT_Long)CUR.GS.freeVector.x * 0x10000L,
                      CUR.F_dot_P );
      *y = TT_MULDIV( d,
                      (FT_Long)CUR.GS.freeVector.y * 0x10000L,
                      CUR.F_dot_P );
    }

    return SUCCESS;
  }


  static void
  Move_Zp2_Point( EXEC_OP_ FT_UShort   point,
                           FT_F26Dot6  dx,
                           FT_F26Dot6  dy,
                           FT_Bool     touch )
  {
#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    if ( CUR.face->unpatented_hinting )
    {
      if ( CUR.GS.both_x_axis )
      {
        CUR.zp2.cur[point].x += dx;
        if ( touch )
          CUR.zp2.tags[point] |= FT_CURVE_TAG_TOUCH_X;
      }
      else
      {
        CUR.zp2.cur[point].y += dy;
        if ( touch )
          CUR.zp2.tags[point] |= FT_CURVE_TAG_TOUCH_Y;
      }
      return;
    }
#endif

    if ( CUR.GS.freeVector.x != 0 )
    {
      CUR.zp2.cur[point].x += dx;
      if ( touch )
        CUR.zp2.tags[point] |= FT_CURVE_TAG_TOUCH_X;
    }

    if ( CUR.GS.freeVector.y != 0 )
    {
      CUR.zp2.cur[point].y += dy;
      if ( touch )
        CUR.zp2.tags[point] |= FT_CURVE_TAG_TOUCH_Y;
    }
  }


  
  
  
  
  
  
  static void
  Ins_SHP( INS_ARG )
  {
    TT_GlyphZoneRec  zp;
    FT_UShort        refp;

    FT_F26Dot6       dx,
                     dy;
    FT_UShort        point;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;
      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
        
        MOVE_Zp2_Point( point, dx, dy, TRUE );

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  
  
  
  
  
  
  static void
  Ins_SHC( INS_ARG )
  {
    TT_GlyphZoneRec zp;
    FT_UShort       refp;
    FT_F26Dot6      dx,
                    dy;

    FT_Short        contour;
    FT_UShort       first_point, last_point, i;


    contour = (FT_UShort)args[0];

    if ( BOUNDS( contour, CUR.pts.n_contours ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    if ( contour == 0 )
      first_point = 0;
    else
      first_point = (FT_UShort)( CUR.pts.contours[contour - 1] + 1 -
                                 CUR.pts.first_point );

    last_point = (FT_UShort)( CUR.pts.contours[contour] -
                              CUR.pts.first_point );

    
    
    if ( BOUNDS( last_point, CUR.zp2.n_points ) )
    {
      if ( CUR.zp2.n_points > 0 )
        last_point = (FT_UShort)(CUR.zp2.n_points - 1);
      else
        last_point = 0;
    }

    
    for ( i = first_point; i <= last_point; i++ )
    {
      if ( zp.cur != CUR.zp2.cur || refp != i )
        MOVE_Zp2_Point( i, dx, dy, TRUE );
    }
  }


  
  
  
  
  
  
  static void
  Ins_SHZ( INS_ARG )
  {
    TT_GlyphZoneRec zp;
    FT_UShort       refp;
    FT_F26Dot6      dx,
                    dy;

    FT_UShort       last_point, i;


    if ( BOUNDS( args[0], 2 ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    
    
    
    
    if ( CUR.GS.gep2 == 0 && CUR.zp2.n_points > 0 )
      last_point = (FT_UShort)( CUR.zp2.n_points - 1 );
    else if ( CUR.GS.gep2 == 1 && CUR.zp2.n_contours > 0 )
      last_point = (FT_UShort)( CUR.zp2.contours[CUR.zp2.n_contours - 1] );
    else
      last_point = 0;

    
    for ( i = 0; i <= last_point; i++ )
    {
      if ( zp.cur != CUR.zp2.cur || refp != i )
        MOVE_Zp2_Point( i, dx, dy, FALSE );
    }
  }


  
  
  
  
  
  
  static void
  Ins_SHPIX( INS_ARG )
  {
    FT_F26Dot6  dx, dy;
    FT_UShort   point;


    if ( CUR.top < CUR.GS.loop + 1 )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    if ( CUR.face->unpatented_hinting )
    {
      if ( CUR.GS.both_x_axis )
      {
        dx = TT_MulFix14( args[0], 0x4000 );
        dy = 0;
      }
      else
      {
        dx = 0;
        dy = TT_MulFix14( args[0], 0x4000 );
      }
    }
    else
#endif
    {
      dx = TT_MulFix14( args[0], CUR.GS.freeVector.x );
      dy = TT_MulFix14( args[0], CUR.GS.freeVector.y );
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
        MOVE_Zp2_Point( point, dx, dy, TRUE );

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  
  
  
  
  
  
  static void
  Ins_MSIRP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  distance;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point,      CUR.zp1.n_points ) ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    
    if ( CUR.GS.gep1 == 0 )   
                              
    {
      CUR.zp1.org[point] = CUR.zp0.org[CUR.GS.rp0];
      CUR_Func_move_orig( &CUR.zp1, point, args[1] );
      CUR.zp1.cur[point] = CUR.zp1.org[point];
    }

    distance = CUR_Func_project( CUR.zp1.cur + point,
                                 CUR.zp0.cur + CUR.GS.rp0 );

    CUR_Func_move( &CUR.zp1, point, args[1] - distance );

    CUR.GS.rp1 = CUR.GS.rp0;
    CUR.GS.rp2 = point;

    if ( ( CUR.opcode & 1 ) != 0 )
      CUR.GS.rp0 = point;
  }


  
  
  
  
  
  
  static void
  Ins_MDAP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  cur_dist,
                distance;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    
    
    if ( ( CUR.opcode & 1 ) != 0 )
    {
      cur_dist = CUR_fast_project( &CUR.zp0.cur[point] );
      distance = CUR_Func_round( cur_dist,
                                 CUR.tt_metrics.compensations[0] ) - cur_dist;
    }
    else
      distance = 0;

    CUR_Func_move( &CUR.zp0, point, distance );

    CUR.GS.rp0 = point;
    CUR.GS.rp1 = point;
  }


  
  
  
  
  
  
  static void
  Ins_MIAP( INS_ARG )
  {
    FT_ULong    cvtEntry;
    FT_UShort   point;
    FT_F26Dot6  distance,
                org_dist;


    cvtEntry = (FT_ULong)args[1];
    point    = (FT_UShort)args[0];

    if ( BOUNDS( point,    CUR.zp0.n_points ) ||
         BOUNDS( cvtEntry, CUR.cvtSize )      )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    distance = CUR_Func_read_cvt( cvtEntry );

    if ( CUR.GS.gep0 == 0 )   
    {
      CUR.zp0.org[point].x = TT_MulFix14( distance, CUR.GS.freeVector.x );
      CUR.zp0.org[point].y = TT_MulFix14( distance, CUR.GS.freeVector.y ),
      CUR.zp0.cur[point]   = CUR.zp0.org[point];
    }

    org_dist = CUR_fast_project( &CUR.zp0.cur[point] );

    if ( ( CUR.opcode & 1 ) != 0 )   
    {
      if ( FT_ABS( distance - org_dist ) > CUR.GS.control_value_cutin )
        distance = org_dist;

      distance = CUR_Func_round( distance, CUR.tt_metrics.compensations[0] );
    }

    CUR_Func_move( &CUR.zp0, point, distance - org_dist );

    CUR.GS.rp0 = point;
    CUR.GS.rp1 = point;
  }


  
  
  
  
  
  
  static void
  Ins_MDRP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  org_dist, distance;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point,      CUR.zp1.n_points ) ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    
    

    

    if ( CUR.GS.gep0 == 0 || CUR.GS.gep1 == 0 )
    {
      FT_Vector*  vec1 = &CUR.zp1.org[point];
      FT_Vector*  vec2 = &CUR.zp0.org[CUR.GS.rp0];


      org_dist = CUR_Func_dualproj( vec1, vec2 );
    }
    else
    {
      FT_Vector*  vec1 = &CUR.zp1.orus[point];
      FT_Vector*  vec2 = &CUR.zp0.orus[CUR.GS.rp0];


      if ( CUR.metrics.x_scale == CUR.metrics.y_scale )
      {
        
        org_dist = CUR_Func_dualproj( vec1, vec2 );
        org_dist = TT_MULFIX( org_dist, CUR.metrics.x_scale );
      }
      else
      {
        FT_Vector  vec;


        vec.x = TT_MULFIX( vec1->x - vec2->x, CUR.metrics.x_scale );
        vec.y = TT_MULFIX( vec1->y - vec2->y, CUR.metrics.y_scale );

        org_dist = CUR_fast_dualproj( &vec );
      }
    }

    

    if ( FT_ABS( org_dist - CUR.GS.single_width_value ) <
         CUR.GS.single_width_cutin )
    {
      if ( org_dist >= 0 )
        org_dist = CUR.GS.single_width_value;
      else
        org_dist = -CUR.GS.single_width_value;
    }

    

    if ( ( CUR.opcode & 4 ) != 0 )
      distance = CUR_Func_round(
                   org_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );
    else
      distance = ROUND_None(
                   org_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );

    

    if ( ( CUR.opcode & 8 ) != 0 )
    {
      if ( org_dist >= 0 )
      {
        if ( distance < CUR.GS.minimum_distance )
          distance = CUR.GS.minimum_distance;
      }
      else
      {
        if ( distance > -CUR.GS.minimum_distance )
          distance = -CUR.GS.minimum_distance;
      }
    }

    

    org_dist = CUR_Func_project( CUR.zp1.cur + point,
                                 CUR.zp0.cur + CUR.GS.rp0 );

    CUR_Func_move( &CUR.zp1, point, distance - org_dist );

    CUR.GS.rp1 = CUR.GS.rp0;
    CUR.GS.rp2 = point;

    if ( ( CUR.opcode & 16 ) != 0 )
      CUR.GS.rp0 = point;
  }


  
  
  
  
  
  
  static void
  Ins_MIRP( INS_ARG )
  {
    FT_UShort   point;
    FT_ULong    cvtEntry;

    FT_F26Dot6  cvt_dist,
                distance,
                cur_dist,
                org_dist;


    point    = (FT_UShort)args[0];
    cvtEntry = (FT_ULong)( args[1] + 1 );

    

    if ( BOUNDS( point,      CUR.zp1.n_points ) ||
         BOUNDS( cvtEntry,   CUR.cvtSize + 1 )  ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( !cvtEntry )
      cvt_dist = 0;
    else
      cvt_dist = CUR_Func_read_cvt( cvtEntry - 1 );

    

    if ( FT_ABS( cvt_dist - CUR.GS.single_width_value ) <
         CUR.GS.single_width_cutin )
    {
      if ( cvt_dist >= 0 )
        cvt_dist =  CUR.GS.single_width_value;
      else
        cvt_dist = -CUR.GS.single_width_value;
    }

    

    if ( CUR.GS.gep1 == 0 )
    {
      CUR.zp1.org[point].x = CUR.zp0.org[CUR.GS.rp0].x +
                             TT_MulFix14( cvt_dist, CUR.GS.freeVector.x );

      CUR.zp1.org[point].y = CUR.zp0.org[CUR.GS.rp0].y +
                             TT_MulFix14( cvt_dist, CUR.GS.freeVector.y );

      CUR.zp1.cur[point] = CUR.zp0.cur[point];
    }

    org_dist = CUR_Func_dualproj( &CUR.zp1.org[point],
                                  &CUR.zp0.org[CUR.GS.rp0] );
    cur_dist = CUR_Func_project ( &CUR.zp1.cur[point],
                                  &CUR.zp0.cur[CUR.GS.rp0] );

    

    if ( CUR.GS.auto_flip )
    {
      if ( ( org_dist ^ cvt_dist ) < 0 )
        cvt_dist = -cvt_dist;
    }

    

    if ( ( CUR.opcode & 4 ) != 0 )
    {
      
      

      if ( CUR.GS.gep0 == CUR.GS.gep1 )
        if ( FT_ABS( cvt_dist - org_dist ) >= CUR.GS.control_value_cutin )
          cvt_dist = org_dist;

      distance = CUR_Func_round(
                   cvt_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );
    }
    else
      distance = ROUND_None(
                   cvt_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );

    

    if ( ( CUR.opcode & 8 ) != 0 )
    {
      if ( org_dist >= 0 )
      {
        if ( distance < CUR.GS.minimum_distance )
          distance = CUR.GS.minimum_distance;
      }
      else
      {
        if ( distance > -CUR.GS.minimum_distance )
          distance = -CUR.GS.minimum_distance;
      }
    }

    CUR_Func_move( &CUR.zp1, point, distance - cur_dist );

    CUR.GS.rp1 = CUR.GS.rp0;

    if ( ( CUR.opcode & 16 ) != 0 )
      CUR.GS.rp0 = point;

    
    CUR.GS.rp2 = point;
  }


  
  
  
  
  
  
  static void
  Ins_ALIGNRP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  distance;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp1.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
      {
        distance = CUR_Func_project( CUR.zp1.cur + point,
                                     CUR.zp0.cur + CUR.GS.rp0 );

        CUR_Func_move( &CUR.zp1, point, -distance );
      }

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  
  
  
  
  
  
  static void
  Ins_ISECT( INS_ARG )
  {
    FT_UShort   point,
                a0, a1,
                b0, b1;

    FT_F26Dot6  discriminant;

    FT_F26Dot6  dx,  dy,
                dax, day,
                dbx, dby;

    FT_F26Dot6  val;

    FT_Vector   R;


    point = (FT_UShort)args[0];

    a0 = (FT_UShort)args[1];
    a1 = (FT_UShort)args[2];
    b0 = (FT_UShort)args[3];
    b1 = (FT_UShort)args[4];

    if ( BOUNDS( b0, CUR.zp0.n_points )  ||
         BOUNDS( b1, CUR.zp0.n_points )  ||
         BOUNDS( a0, CUR.zp1.n_points )  ||
         BOUNDS( a1, CUR.zp1.n_points )  ||
         BOUNDS( point, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    dbx = CUR.zp0.cur[b1].x - CUR.zp0.cur[b0].x;
    dby = CUR.zp0.cur[b1].y - CUR.zp0.cur[b0].y;

    dax = CUR.zp1.cur[a1].x - CUR.zp1.cur[a0].x;
    day = CUR.zp1.cur[a1].y - CUR.zp1.cur[a0].y;

    dx = CUR.zp0.cur[b0].x - CUR.zp1.cur[a0].x;
    dy = CUR.zp0.cur[b0].y - CUR.zp1.cur[a0].y;

    CUR.zp2.tags[point] |= FT_CURVE_TAG_TOUCH_BOTH;

    discriminant = TT_MULDIV( dax, -dby, 0x40 ) +
                   TT_MULDIV( day, dbx, 0x40 );

    if ( FT_ABS( discriminant ) >= 0x40 )
    {
      val = TT_MULDIV( dx, -dby, 0x40 ) + TT_MULDIV( dy, dbx, 0x40 );

      R.x = TT_MULDIV( val, dax, discriminant );
      R.y = TT_MULDIV( val, day, discriminant );

      CUR.zp2.cur[point].x = CUR.zp1.cur[a0].x + R.x;
      CUR.zp2.cur[point].y = CUR.zp1.cur[a0].y + R.y;
    }
    else
    {
      

      CUR.zp2.cur[point].x = ( CUR.zp1.cur[a0].x +
                               CUR.zp1.cur[a1].x +
                               CUR.zp0.cur[b0].x +
                               CUR.zp0.cur[b1].x ) / 4;
      CUR.zp2.cur[point].y = ( CUR.zp1.cur[a0].y +
                               CUR.zp1.cur[a1].y +
                               CUR.zp0.cur[b0].y +
                               CUR.zp0.cur[b1].y ) / 4;
    }
  }


  
  
  
  
  
  
  static void
  Ins_ALIGNPTS( INS_ARG )
  {
    FT_UShort   p1, p2;
    FT_F26Dot6  distance;


    p1 = (FT_UShort)args[0];
    p2 = (FT_UShort)args[1];

    if ( BOUNDS( args[0], CUR.zp1.n_points ) ||
         BOUNDS( args[1], CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    distance = CUR_Func_project( CUR.zp0.cur + p2,
                                 CUR.zp1.cur + p1 ) / 2;

    CUR_Func_move( &CUR.zp1, p1, distance );
    CUR_Func_move( &CUR.zp0, p2, -distance );
  }


  
  
  
  
  
  

  

  static void
  Ins_IP( INS_ARG )
  {
    FT_F26Dot6  old_range, cur_range;
    FT_Vector*  orus_base;
    FT_Vector*  cur_base;
    FT_Int      twilight;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    




    twilight = CUR.GS.gep0 == 0 || CUR.GS.gep1 == 0 || CUR.GS.gep2 == 0;

    if ( BOUNDS( CUR.GS.rp1, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( twilight )
      orus_base = &CUR.zp0.org[CUR.GS.rp1];
    else
      orus_base = &CUR.zp0.orus[CUR.GS.rp1];

    cur_base = &CUR.zp0.cur[CUR.GS.rp1];

    
    
    
    
    if ( BOUNDS( CUR.GS.rp1, CUR.zp0.n_points ) ||
         BOUNDS( CUR.GS.rp2, CUR.zp1.n_points ) )
    {
      old_range = 0;
      cur_range = 0;
    }
    else
    {
      if ( twilight )
        old_range = CUR_Func_dualproj( &CUR.zp1.org[CUR.GS.rp2],
                                       orus_base );
      else
        old_range = CUR_Func_dualproj( &CUR.zp1.orus[CUR.GS.rp2],
                                       orus_base );

      cur_range = CUR_Func_project ( &CUR.zp1.cur[CUR.GS.rp2], cur_base );
    }

    for ( ; CUR.GS.loop > 0; --CUR.GS.loop )
    {
      FT_UInt     point = (FT_UInt)CUR.stack[--CUR.args];
      FT_F26Dot6  org_dist, cur_dist, new_dist;


      
      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
        continue;
      }

      if ( twilight )
        org_dist = CUR_Func_dualproj( &CUR.zp2.org[point], orus_base );
      else
        org_dist = CUR_Func_dualproj( &CUR.zp2.orus[point], orus_base );

      cur_dist = CUR_Func_project ( &CUR.zp2.cur[point], cur_base );

      if ( org_dist )
        new_dist = ( old_range != 0 )
                     ? TT_MULDIV( org_dist, cur_range, old_range )
                     : cur_dist;
      else
        new_dist = 0;

      CUR_Func_move( &CUR.zp2, (FT_UShort)point, new_dist - cur_dist );
    }
    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  
  
  
  
  
  
  static void
  Ins_UTP( INS_ARG )
  {
    FT_UShort  point;
    FT_Byte    mask;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    mask = 0xFF;

    if ( CUR.GS.freeVector.x != 0 )
      mask &= ~FT_CURVE_TAG_TOUCH_X;

    if ( CUR.GS.freeVector.y != 0 )
      mask &= ~FT_CURVE_TAG_TOUCH_Y;

    CUR.zp0.tags[point] &= mask;
  }


  
  typedef struct  IUP_WorkerRec_
  {
    FT_Vector*  orgs;   
    FT_Vector*  curs;   
    FT_Vector*  orus;
    FT_UInt     max_points;

  } IUP_WorkerRec, *IUP_Worker;


  static void
  _iup_worker_shift( IUP_Worker  worker,
                     FT_UInt     p1,
                     FT_UInt     p2,
                     FT_UInt     p )
  {
    FT_UInt     i;
    FT_F26Dot6  dx;


    dx = worker->curs[p].x - worker->orgs[p].x;
    if ( dx != 0 )
    {
      for ( i = p1; i < p; i++ )
        worker->curs[i].x += dx;

      for ( i = p + 1; i <= p2; i++ )
        worker->curs[i].x += dx;
    }
  }


  static void
  _iup_worker_interpolate( IUP_Worker  worker,
                           FT_UInt     p1,
                           FT_UInt     p2,
                           FT_UInt     ref1,
                           FT_UInt     ref2 )
  {
    FT_UInt     i;
    FT_F26Dot6  orus1, orus2, org1, org2, delta1, delta2;


    if ( p1 > p2 )
      return;

    if ( BOUNDS( ref1, worker->max_points ) ||
         BOUNDS( ref2, worker->max_points ) )
      return;

    orus1 = worker->orus[ref1].x;
    orus2 = worker->orus[ref2].x;

    if ( orus1 > orus2 )
    {
      FT_F26Dot6  tmp_o;
      FT_UInt     tmp_r;


      tmp_o = orus1;
      orus1 = orus2;
      orus2 = tmp_o;

      tmp_r = ref1;
      ref1  = ref2;
      ref2  = tmp_r;
    }

    org1   = worker->orgs[ref1].x;
    org2   = worker->orgs[ref2].x;
    delta1 = worker->curs[ref1].x - org1;
    delta2 = worker->curs[ref2].x - org2;

    if ( orus1 == orus2 )
    {
      
      for ( i = p1; i <= p2; i++ )
      {
        FT_F26Dot6  x = worker->orgs[i].x;


        if ( x <= org1 )
          x += delta1;
        else
          x += delta2;

        worker->curs[i].x = x;
      }
    }
    else
    {
      FT_Fixed  scale       = 0;
      FT_Bool   scale_valid = 0;


      
      for ( i = p1; i <= p2; i++ )
      {
        FT_F26Dot6  x = worker->orgs[i].x;


        if ( x <= org1 )
          x += delta1;

        else if ( x >= org2 )
          x += delta2;

        else
        {
          if ( !scale_valid )
          {
            scale_valid = 1;
            scale       = TT_MULDIV( org2 + delta2 - ( org1 + delta1 ),
                                     0x10000, orus2 - orus1 );
          }

          x = ( org1 + delta1 ) +
              TT_MULFIX( worker->orus[i].x - orus1, scale );
        }
        worker->curs[i].x = x;
      }
    }
  }


  
  
  
  
  
  
  static void
  Ins_IUP( INS_ARG )
  {
    IUP_WorkerRec  V;
    FT_Byte        mask;

    FT_UInt   first_point;   
    FT_UInt   end_point;     

    FT_UInt   first_touched; 
    FT_UInt   cur_touched;   

    FT_UInt   point;         
    FT_Short  contour;       

    FT_UNUSED_ARG;


    
    if ( CUR.pts.n_contours == 0 )
      return;

    if ( CUR.opcode & 1 )
    {
      mask   = FT_CURVE_TAG_TOUCH_X;
      V.orgs = CUR.pts.org;
      V.curs = CUR.pts.cur;
      V.orus = CUR.pts.orus;
    }
    else
    {
      mask   = FT_CURVE_TAG_TOUCH_Y;
      V.orgs = (FT_Vector*)( (FT_Pos*)CUR.pts.org + 1 );
      V.curs = (FT_Vector*)( (FT_Pos*)CUR.pts.cur + 1 );
      V.orus = (FT_Vector*)( (FT_Pos*)CUR.pts.orus + 1 );
    }
    V.max_points = CUR.pts.n_points;

    contour = 0;
    point   = 0;

    do
    {
      end_point   = CUR.pts.contours[contour] - CUR.pts.first_point;
      first_point = point;

      if ( CUR.pts.n_points <= end_point )
        end_point = CUR.pts.n_points;

      while ( point <= end_point && ( CUR.pts.tags[point] & mask ) == 0 )
        point++;

      if ( point <= end_point )
      {
        first_touched = point;
        cur_touched   = point;

        point++;

        while ( point <= end_point )
        {
          if ( ( CUR.pts.tags[point] & mask ) != 0 )
          {
            if ( point > 0 )
              _iup_worker_interpolate( &V,
                                       cur_touched + 1,
                                       point - 1,
                                       cur_touched,
                                       point );
            cur_touched = point;
          }

          point++;
        }

        if ( cur_touched == first_touched )
          _iup_worker_shift( &V, first_point, end_point, cur_touched );
        else
        {
          _iup_worker_interpolate( &V,
                                   (FT_UShort)( cur_touched + 1 ),
                                   end_point,
                                   cur_touched,
                                   first_touched );

          if ( first_touched > 0 )
            _iup_worker_interpolate( &V,
                                     first_point,
                                     first_touched - 1,
                                     cur_touched,
                                     first_touched );
        }
      }
      contour++;
    } while ( contour < CUR.pts.n_contours );
  }


  
  
  
  
  
  
  static void
  Ins_DELTAP( INS_ARG )
  {
    FT_ULong   k, nump;
    FT_UShort  A;
    FT_ULong   C;
    FT_Long    B;


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    
    if ( CUR.face->unpatented_hinting )
    {
      FT_Long  n = args[0] * 2;


      if ( CUR.args < n )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= n;
      CUR.new_top = CUR.args;
      return;
    }
#endif

    nump = (FT_ULong)args[0];   


    for ( k = 1; k <= nump; k++ )
    {
      if ( CUR.args < 2 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= 2;

      A = (FT_UShort)CUR.stack[CUR.args + 1];
      B = CUR.stack[CUR.args];

      
      
      
      
      

      if ( !BOUNDS( A, CUR.zp0.n_points ) )
      {
        C = ( (FT_ULong)B & 0xF0 ) >> 4;

        switch ( CUR.opcode )
        {
        case 0x5D:
          break;

        case 0x71:
          C += 16;
          break;

        case 0x72:
          C += 32;
          break;
        }

        C += CUR.GS.delta_base;

        if ( CURRENT_Ppem() == (FT_Long)C )
        {
          B = ( (FT_ULong)B & 0xF ) - 8;
          if ( B >= 0 )
            B++;
          B = B * 64 / ( 1L << CUR.GS.delta_shift );

          CUR_Func_move( &CUR.zp0, A, B );
        }
      }
      else
        if ( CUR.pedantic_hinting )
          CUR.error = TT_Err_Invalid_Reference;
    }

    CUR.new_top = CUR.args;
  }


  
  
  
  
  
  
  static void
  Ins_DELTAC( INS_ARG )
  {
    FT_ULong  nump, k;
    FT_ULong  A, C;
    FT_Long   B;


#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    
    if ( CUR.face->unpatented_hinting )
    {
      FT_Long  n = args[0] * 2;


      if ( CUR.args < n )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= n;
      CUR.new_top = CUR.args;
      return;
    }
#endif

    nump = (FT_ULong)args[0];

    for ( k = 1; k <= nump; k++ )
    {
      if ( CUR.args < 2 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= 2;

      A = (FT_ULong)CUR.stack[CUR.args + 1];
      B = CUR.stack[CUR.args];

      if ( BOUNDS( A, CUR.cvtSize ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
      {
        C = ( (FT_ULong)B & 0xF0 ) >> 4;

        switch ( CUR.opcode )
        {
        case 0x73:
          break;

        case 0x74:
          C += 16;
          break;

        case 0x75:
          C += 32;
          break;
        }

        C += CUR.GS.delta_base;

        if ( CURRENT_Ppem() == (FT_Long)C )
        {
          B = ( (FT_ULong)B & 0xF ) - 8;
          if ( B >= 0 )
            B++;
          B = B * 64 / ( 1L << CUR.GS.delta_shift );

          CUR_Func_move_cvt( A, B );
        }
      }
    }

    CUR.new_top = CUR.args;
  }


  
  
  
  
  


  
  
  
  
  
  
  static void
  Ins_GETINFO( INS_ARG )
  {
    FT_Long  K;


    K = 0;

    
    if ( ( args[0] & 1 ) != 0 )
      K = 35;

    
    if ( ( args[0] & 2 ) != 0 && CUR.tt_metrics.rotated )
      K |= 0x80;

    
    if ( ( args[0] & 4 ) != 0 && CUR.tt_metrics.stretched )
      K |= 1 << 8;

    
    if ( ( args[0] & 32 ) != 0 && CUR.grayscale )
      K |= 1 << 12;

    args[0] = K;
  }


  static void
  Ins_UNKNOWN( INS_ARG )
  {
    TT_DefRecord*  def   = CUR.IDefs;
    TT_DefRecord*  limit = def + CUR.numIDefs;

    FT_UNUSED_ARG;


    for ( ; def < limit; def++ )
    {
      if ( (FT_Byte)def->opc == CUR.opcode && def->active )
      {
        TT_CallRec*  call;


        if ( CUR.callTop >= CUR.callSize )
        {
          CUR.error = TT_Err_Stack_Overflow;
          return;
        }

        call = CUR.callStack + CUR.callTop++;

        call->Caller_Range = CUR.curRange;
        call->Caller_IP    = CUR.IP+1;
        call->Cur_Count    = 1;
        call->Cur_Restart  = def->start;

        INS_Goto_CodeRange( def->range, def->start );

        CUR.step_ins = FALSE;
        return;
      }
    }

    CUR.error = TT_Err_Invalid_Opcode;
  }


#ifndef TT_CONFIG_OPTION_INTERPRETER_SWITCH


  static
  TInstruction_Function  Instruct_Dispatch[256] =
  {
    
    

      Ins_SVTCA,
      Ins_SVTCA,
      Ins_SPVTCA,
      Ins_SPVTCA,
      Ins_SFVTCA,
      Ins_SFVTCA,
      Ins_SPVTL,
      Ins_SPVTL,
      Ins_SFVTL,
      Ins_SFVTL,
      Ins_SPVFS,
      Ins_SFVFS,
      Ins_GPV,
      Ins_GFV,
      Ins_SFVTPV,
      Ins_ISECT,

      Ins_SRP0,
      Ins_SRP1,
      Ins_SRP2,
      Ins_SZP0,
      Ins_SZP1,
      Ins_SZP2,
      Ins_SZPS,
      Ins_SLOOP,
      Ins_RTG,
      Ins_RTHG,
      Ins_SMD,
      Ins_ELSE,
      Ins_JMPR,
      Ins_SCVTCI,
      Ins_SSWCI,
      Ins_SSW,

      Ins_DUP,
      Ins_POP,
      Ins_CLEAR,
      Ins_SWAP,
      Ins_DEPTH,
      Ins_CINDEX,
      Ins_MINDEX,
      Ins_ALIGNPTS,
      Ins_UNKNOWN,
      Ins_UTP,
      Ins_LOOPCALL,
      Ins_CALL,
      Ins_FDEF,
      Ins_ENDF,
      Ins_MDAP,
      Ins_MDAP,

      Ins_IUP,
      Ins_IUP,
      Ins_SHP,
      Ins_SHP,
      Ins_SHC,
      Ins_SHC,
      Ins_SHZ,
      Ins_SHZ,
      Ins_SHPIX,
      Ins_IP,
      Ins_MSIRP,
      Ins_MSIRP,
      Ins_ALIGNRP,
      Ins_RTDG,
      Ins_MIAP,
      Ins_MIAP,

      Ins_NPUSHB,
      Ins_NPUSHW,
      Ins_WS,
      Ins_RS,
      Ins_WCVTP,
      Ins_RCVT,
      Ins_GC,
      Ins_GC,
      Ins_SCFS,
      Ins_MD,
      Ins_MD,
      Ins_MPPEM,
      Ins_MPS,
      Ins_FLIPON,
      Ins_FLIPOFF,
      Ins_DEBUG,

      Ins_LT,
      Ins_LTEQ,
      Ins_GT,
      Ins_GTEQ,
      Ins_EQ,
      Ins_NEQ,
      Ins_ODD,
      Ins_EVEN,
      Ins_IF,
      Ins_EIF,
      Ins_AND,
      Ins_OR,
      Ins_NOT,
      Ins_DELTAP,
      Ins_SDB,
      Ins_SDS,

      Ins_ADD,
      Ins_SUB,
      Ins_DIV,
      Ins_MUL,
      Ins_ABS,
      Ins_NEG,
      Ins_FLOOR,
      Ins_CEILING,
      Ins_ROUND,
      Ins_ROUND,
      Ins_ROUND,
      Ins_ROUND,
      Ins_NROUND,
      Ins_NROUND,
      Ins_NROUND,
      Ins_NROUND,

      Ins_WCVTF,
      Ins_DELTAP,
      Ins_DELTAP,
     Ins_DELTAC,
     Ins_DELTAC,
     Ins_DELTAC,
      Ins_SROUND,
      Ins_S45ROUND,
      Ins_JROT,
      Ins_JROF,
      Ins_ROFF,
      Ins_UNKNOWN,
      Ins_RUTG,
      Ins_RDTG,
      Ins_SANGW,
      Ins_AA,

      Ins_FLIPPT,
      Ins_FLIPRGON,
      Ins_FLIPRGOFF,
      Ins_UNKNOWN,
      Ins_UNKNOWN,
      Ins_SCANCTRL,
      Ins_SDPVTL,
      Ins_SDPVTL,
      Ins_GETINFO,
      Ins_IDEF,
      Ins_ROLL,
      Ins_MAX,
      Ins_MIN,
      Ins_SCANTYPE,
      Ins_INSTCTRL,
      Ins_UNKNOWN,

       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,

       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,
       Ins_UNKNOWN,

      Ins_PUSHB,
      Ins_PUSHB,
      Ins_PUSHB,
      Ins_PUSHB,
      Ins_PUSHB,
      Ins_PUSHB,
      Ins_PUSHB,
      Ins_PUSHB,
      Ins_PUSHW,
      Ins_PUSHW,
      Ins_PUSHW,
      Ins_PUSHW,
      Ins_PUSHW,
      Ins_PUSHW,
      Ins_PUSHW,
      Ins_PUSHW,

      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,

      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,
      Ins_MDRP,

      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,

      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP,
      Ins_MIRP
  };


#endif 


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  

  FT_EXPORT_DEF( FT_Error )
  TT_RunIns( TT_ExecContext  exc )
  {
    FT_Long  ins_counter = 0;  


#ifdef TT_CONFIG_OPTION_STATIC_RASTER
    cur = *exc;
#endif

    
    CUR.tt_metrics.ratio = 0;
    if ( CUR.metrics.x_ppem != CUR.metrics.y_ppem )
    {
      
      CUR.func_read_cvt  = Read_CVT_Stretched;
      CUR.func_write_cvt = Write_CVT_Stretched;
      CUR.func_move_cvt  = Move_CVT_Stretched;
    }
    else
    {
      
      CUR.func_read_cvt  = Read_CVT;
      CUR.func_write_cvt = Write_CVT;
      CUR.func_move_cvt  = Move_CVT;
    }

    COMPUTE_Funcs();
    COMPUTE_Round( (FT_Byte)exc->GS.round_state );

    do
    {
      CUR.opcode = CUR.code[CUR.IP];

      if ( ( CUR.length = opcode_length[CUR.opcode] ) < 0 )
      {
        if ( CUR.IP + 1 > CUR.codeSize )
          goto LErrorCodeOverflow_;

        CUR.length = 2 - CUR.length * CUR.code[CUR.IP + 1];
      }

      if ( CUR.IP + CUR.length > CUR.codeSize )
        goto LErrorCodeOverflow_;

      
      CUR.args = CUR.top - ( Pop_Push_Count[CUR.opcode] >> 4 );

      
      
      if ( CUR.args < 0 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        goto LErrorLabel_;
      }

      CUR.new_top = CUR.args + ( Pop_Push_Count[CUR.opcode] & 15 );

      
      
      
      if ( CUR.new_top > CUR.stackSize )
      {
        CUR.error = TT_Err_Stack_Overflow;
        goto LErrorLabel_;
      }

      CUR.step_ins = TRUE;
      CUR.error    = TT_Err_Ok;

#ifdef TT_CONFIG_OPTION_INTERPRETER_SWITCH

      {
        FT_Long*  args   = CUR.stack + CUR.args;
        FT_Byte   opcode = CUR.opcode;


#undef  ARRAY_BOUND_ERROR
#define ARRAY_BOUND_ERROR  goto Set_Invalid_Ref


        switch ( opcode )
        {
        case 0x00:  
        case 0x01:  
        case 0x02:  
        case 0x03:  
        case 0x04:  
        case 0x05:  
          {
            FT_Short AA, BB;


            AA = (FT_Short)( ( opcode & 1 ) << 14 );
            BB = (FT_Short)( AA ^ 0x4000 );

            if ( opcode < 4 )
            {
              CUR.GS.projVector.x = AA;
              CUR.GS.projVector.y = BB;

              CUR.GS.dualVector.x = AA;
              CUR.GS.dualVector.y = BB;
            }
            else
            {
              GUESS_VECTOR( projVector );
            }

            if ( ( opcode & 2 ) == 0 )
            {
              CUR.GS.freeVector.x = AA;
              CUR.GS.freeVector.y = BB;
            }
            else
            {
              GUESS_VECTOR( freeVector );
            }

            COMPUTE_Funcs();
          }
          break;

        case 0x06:  
        case 0x07:  
          DO_SPVTL
          break;

        case 0x08:  
        case 0x09:  
          DO_SFVTL
          break;

        case 0x0A:  
          DO_SPVFS
          break;

        case 0x0B:  
          DO_SFVFS
          break;

        case 0x0C:  
          DO_GPV
          break;

        case 0x0D:  
          DO_GFV
          break;

        case 0x0E:  
          DO_SFVTPV
          break;

        case 0x0F:  
          Ins_ISECT( EXEC_ARG_ args );
          break;

        case 0x10:  
          DO_SRP0
          break;

        case 0x11:  
          DO_SRP1
          break;

        case 0x12:  
          DO_SRP2
          break;

        case 0x13:  
          Ins_SZP0( EXEC_ARG_ args );
          break;

        case 0x14:  
          Ins_SZP1( EXEC_ARG_ args );
          break;

        case 0x15:  
          Ins_SZP2( EXEC_ARG_ args );
          break;

        case 0x16:  
          Ins_SZPS( EXEC_ARG_ args );
          break;

        case 0x17:  
          DO_SLOOP
          break;

        case 0x18:  
          DO_RTG
          break;

        case 0x19:  
          DO_RTHG
          break;

        case 0x1A:  
          DO_SMD
          break;

        case 0x1B:  
          Ins_ELSE( EXEC_ARG_ args );
          break;

        case 0x1C:  
          DO_JMPR
          break;

        case 0x1D:  
          DO_SCVTCI
          break;

        case 0x1E:  
          DO_SSWCI
          break;

        case 0x1F:  
          DO_SSW
          break;

        case 0x20:  
          DO_DUP
          break;

        case 0x21:  
          
          break;

        case 0x22:  
          DO_CLEAR
          break;

        case 0x23:  
          DO_SWAP
          break;

        case 0x24:  
          DO_DEPTH
          break;

        case 0x25:  
          DO_CINDEX
          break;

        case 0x26:  
          Ins_MINDEX( EXEC_ARG_ args );
          break;

        case 0x27:  
          Ins_ALIGNPTS( EXEC_ARG_ args );
          break;

        case 0x28:  
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        case 0x29:  
          Ins_UTP( EXEC_ARG_ args );
          break;

        case 0x2A:  
          Ins_LOOPCALL( EXEC_ARG_ args );
          break;

        case 0x2B:  
          Ins_CALL( EXEC_ARG_ args );
          break;

        case 0x2C:  
          Ins_FDEF( EXEC_ARG_ args );
          break;

        case 0x2D:  
          Ins_ENDF( EXEC_ARG_ args );
          break;

        case 0x2E:  
        case 0x2F:  
          Ins_MDAP( EXEC_ARG_ args );
          break;


        case 0x30:  
        case 0x31:  
          Ins_IUP( EXEC_ARG_ args );
          break;

        case 0x32:  
        case 0x33:  
          Ins_SHP( EXEC_ARG_ args );
          break;

        case 0x34:  
        case 0x35:  
          Ins_SHC( EXEC_ARG_ args );
          break;

        case 0x36:  
        case 0x37:  
          Ins_SHZ( EXEC_ARG_ args );
          break;

        case 0x38:  
          Ins_SHPIX( EXEC_ARG_ args );
          break;

        case 0x39:  
          Ins_IP( EXEC_ARG_ args );
          break;

        case 0x3A:  
        case 0x3B:  
          Ins_MSIRP( EXEC_ARG_ args );
          break;

        case 0x3C:  
          Ins_ALIGNRP( EXEC_ARG_ args );
          break;

        case 0x3D:  
          DO_RTDG
          break;

        case 0x3E:  
        case 0x3F:  
          Ins_MIAP( EXEC_ARG_ args );
          break;

        case 0x40:  
          Ins_NPUSHB( EXEC_ARG_ args );
          break;

        case 0x41:  
          Ins_NPUSHW( EXEC_ARG_ args );
          break;

        case 0x42:  
          DO_WS
          break;

      Set_Invalid_Ref:
            CUR.error = TT_Err_Invalid_Reference;
          break;

        case 0x43:  
          DO_RS
          break;

        case 0x44:  
          DO_WCVTP
          break;

        case 0x45:  
          DO_RCVT
          break;

        case 0x46:  
        case 0x47:  
          Ins_GC( EXEC_ARG_ args );
          break;

        case 0x48:  
          Ins_SCFS( EXEC_ARG_ args );
          break;

        case 0x49:  
        case 0x4A:  
          Ins_MD( EXEC_ARG_ args );
          break;

        case 0x4B:  
          DO_MPPEM
          break;

        case 0x4C:  
          DO_MPS
          break;

        case 0x4D:  
          DO_FLIPON
          break;

        case 0x4E:  
          DO_FLIPOFF
          break;

        case 0x4F:  
          DO_DEBUG
          break;

        case 0x50:  
          DO_LT
          break;

        case 0x51:  
          DO_LTEQ
          break;

        case 0x52:  
          DO_GT
          break;

        case 0x53:  
          DO_GTEQ
          break;

        case 0x54:  
          DO_EQ
          break;

        case 0x55:  
          DO_NEQ
          break;

        case 0x56:  
          DO_ODD
          break;

        case 0x57:  
          DO_EVEN
          break;

        case 0x58:  
          Ins_IF( EXEC_ARG_ args );
          break;

        case 0x59:  
          
          break;

        case 0x5A:  
          DO_AND
          break;

        case 0x5B:  
          DO_OR
          break;

        case 0x5C:  
          DO_NOT
          break;

        case 0x5D:  
          Ins_DELTAP( EXEC_ARG_ args );
          break;

        case 0x5E:  
          DO_SDB
          break;

        case 0x5F:  
          DO_SDS
          break;

        case 0x60:  
          DO_ADD
          break;

        case 0x61:  
          DO_SUB
          break;

        case 0x62:  
          DO_DIV
          break;

        case 0x63:  
          DO_MUL
          break;

        case 0x64:  
          DO_ABS
          break;

        case 0x65:  
          DO_NEG
          break;

        case 0x66:  
          DO_FLOOR
          break;

        case 0x67:  
          DO_CEILING
          break;

        case 0x68:  
        case 0x69:  
        case 0x6A:  
        case 0x6B:  
          DO_ROUND
          break;

        case 0x6C:  
        case 0x6D:  
        case 0x6E:  
        case 0x6F:  
          DO_NROUND
          break;

        case 0x70:  
          DO_WCVTF
          break;

        case 0x71:  
        case 0x72:  
          Ins_DELTAP( EXEC_ARG_ args );
          break;

        case 0x73:  
        case 0x74:  
        case 0x75:  
          Ins_DELTAC( EXEC_ARG_ args );
          break;

        case 0x76:  
          DO_SROUND
          break;

        case 0x77:  
          DO_S45ROUND
          break;

        case 0x78:  
          DO_JROT
          break;

        case 0x79:  
          DO_JROF
          break;

        case 0x7A:  
          DO_ROFF
          break;

        case 0x7B:  
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        case 0x7C:  
          DO_RUTG
          break;

        case 0x7D:  
          DO_RDTG
          break;

        case 0x7E:  
        case 0x7F:  
          
          break;

        case 0x80:  
          Ins_FLIPPT( EXEC_ARG_ args );
          break;

        case 0x81:  
          Ins_FLIPRGON( EXEC_ARG_ args );
          break;

        case 0x82:  
          Ins_FLIPRGOFF( EXEC_ARG_ args );
          break;

        case 0x83:  
        case 0x84:  
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        case 0x85:  
          Ins_SCANCTRL( EXEC_ARG_ args );
          break;

        case 0x86:  
        case 0x87:  
          Ins_SDPVTL( EXEC_ARG_ args );
          break;

        case 0x88:  
          Ins_GETINFO( EXEC_ARG_ args );
          break;

        case 0x89:  
          Ins_IDEF( EXEC_ARG_ args );
          break;

        case 0x8A:  
          Ins_ROLL( EXEC_ARG_ args );
          break;

        case 0x8B:  
          DO_MAX
          break;

        case 0x8C:  
          DO_MIN
          break;

        case 0x8D:  
          Ins_SCANTYPE( EXEC_ARG_ args );
          break;

        case 0x8E:  
          Ins_INSTCTRL( EXEC_ARG_ args );
          break;

        case 0x8F:
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        default:
          if ( opcode >= 0xE0 )
            Ins_MIRP( EXEC_ARG_ args );
          else if ( opcode >= 0xC0 )
            Ins_MDRP( EXEC_ARG_ args );
          else if ( opcode >= 0xB8 )
            Ins_PUSHW( EXEC_ARG_ args );
          else if ( opcode >= 0xB0 )
            Ins_PUSHB( EXEC_ARG_ args );
          else
            Ins_UNKNOWN( EXEC_ARG_ args );
        }

      }

#else

      Instruct_Dispatch[CUR.opcode]( EXEC_ARG_ &CUR.stack[CUR.args] );

#endif 

      if ( CUR.error != TT_Err_Ok )
      {
        switch ( CUR.error )
        {
        case TT_Err_Invalid_Opcode: 
          {
            TT_DefRecord*  def   = CUR.IDefs;
            TT_DefRecord*  limit = def + CUR.numIDefs;


            for ( ; def < limit; def++ )
            {
              if ( def->active && CUR.opcode == (FT_Byte)def->opc )
              {
                TT_CallRec*  callrec;


                if ( CUR.callTop >= CUR.callSize )
                {
                  CUR.error = TT_Err_Invalid_Reference;
                  goto LErrorLabel_;
                }

                callrec = &CUR.callStack[CUR.callTop];

                callrec->Caller_Range = CUR.curRange;
                callrec->Caller_IP    = CUR.IP + 1;
                callrec->Cur_Count    = 1;
                callrec->Cur_Restart  = def->start;

                if ( INS_Goto_CodeRange( def->range, def->start ) == FAILURE )
                  goto LErrorLabel_;

                goto LSuiteLabel_;
              }
            }
          }

          CUR.error = TT_Err_Invalid_Opcode;
          goto LErrorLabel_;

#if 0
          break;   
                   
                   
#endif

        default:
          goto LErrorLabel_;

#if 0
        break;
#endif
        }
      }

      CUR.top = CUR.new_top;

      if ( CUR.step_ins )
        CUR.IP += CUR.length;

      
      
      if ( ++ins_counter > MAX_RUNNABLE_OPCODES )
        return TT_Err_Execution_Too_Long;

    LSuiteLabel_:
      if ( CUR.IP >= CUR.codeSize )
      {
        if ( CUR.callTop > 0 )
        {
          CUR.error = TT_Err_Code_Overflow;
          goto LErrorLabel_;
        }
        else
          goto LNo_Error_;
      }
    } while ( !CUR.instruction_trap );

  LNo_Error_:

#ifdef TT_CONFIG_OPTION_STATIC_RASTER
    *exc = cur;
#endif

    return TT_Err_Ok;

  LErrorCodeOverflow_:
    CUR.error = TT_Err_Code_Overflow;

  LErrorLabel_:

#ifdef TT_CONFIG_OPTION_STATIC_RASTER
    *exc = cur;
#endif

    return CUR.error;
  }


#endif 



