





































#include "cf2ft.h"
#include FT_INTERNAL_DEBUG_H

#include "cf2glue.h"
#include "cf2font.h"
#include "cf2stack.h"

#include "cf2error.h"


  
  
  
  FT_LOCAL_DEF( CF2_Stack )
  cf2_stack_init( FT_Memory  memory,
                  FT_Error*  e )
  {
    FT_Error  error = FT_Err_Ok;     

    CF2_Stack  stack = NULL;


    if ( !FT_QNEW( stack ) )
    {
      
      stack->memory = memory;
      stack->error  = e;
      stack->top    = &stack->buffer[0]; 
    }

    return stack;
  }


  FT_LOCAL_DEF( void )
  cf2_stack_free( CF2_Stack  stack )
  {
    if ( stack )
    {
      FT_Memory  memory = stack->memory;


      
      FT_FREE( stack );
    }
  }


  FT_LOCAL_DEF( CF2_UInt )
  cf2_stack_count( CF2_Stack  stack )
  {
    return (CF2_UInt)( stack->top - &stack->buffer[0] );
  }


  FT_LOCAL_DEF( void )
  cf2_stack_pushInt( CF2_Stack  stack,
                     CF2_Int    val )
  {
    if ( stack->top == &stack->buffer[CF2_OPERAND_STACK_SIZE] )
    {
      CF2_SET_ERROR( stack->error, Stack_Overflow );
      return;     
    }

    stack->top->u.i  = val;
    stack->top->type = CF2_NumberInt;
    ++stack->top;
  }


  FT_LOCAL_DEF( void )
  cf2_stack_pushFixed( CF2_Stack  stack,
                       CF2_Fixed  val )
  {
    if ( stack->top == &stack->buffer[CF2_OPERAND_STACK_SIZE] )
    {
      CF2_SET_ERROR( stack->error, Stack_Overflow );
      return;     
    }

    stack->top->u.r  = val;
    stack->top->type = CF2_NumberFixed;
    ++stack->top;
  }


  
  FT_LOCAL_DEF( CF2_Int )
  cf2_stack_popInt( CF2_Stack  stack )
  {
    if ( stack->top == &stack->buffer[0] )
    {
      CF2_SET_ERROR( stack->error, Stack_Underflow );
      return 0;   
    }
    if ( stack->top[-1].type != CF2_NumberInt )
    {
      CF2_SET_ERROR( stack->error, Syntax_Error );
      return 0;   
    }

    --stack->top;

    return stack->top->u.i;
  }


  
  
  FT_LOCAL_DEF( CF2_Fixed )
  cf2_stack_popFixed( CF2_Stack  stack )
  {
    if ( stack->top == &stack->buffer[0] )
    {
      CF2_SET_ERROR( stack->error, Stack_Underflow );
      return cf2_intToFixed( 0 );    
    }

    --stack->top;

    switch ( stack->top->type )
    {
    case CF2_NumberInt:
      return cf2_intToFixed( stack->top->u.i );
    case CF2_NumberFrac:
      return cf2_fracToFixed( stack->top->u.f );
    default:
      return stack->top->u.r;
    }
  }


  
  
  FT_LOCAL_DEF( CF2_Fixed )
  cf2_stack_getReal( CF2_Stack  stack,
                     CF2_UInt   idx )
  {
    FT_ASSERT( cf2_stack_count( stack ) <= CF2_OPERAND_STACK_SIZE );

    if ( idx >= cf2_stack_count( stack ) )
    {
      CF2_SET_ERROR( stack->error, Stack_Overflow );
      return cf2_intToFixed( 0 );    
    }

    switch ( stack->buffer[idx].type )
    {
    case CF2_NumberInt:
      return cf2_intToFixed( stack->buffer[idx].u.i );
    case CF2_NumberFrac:
      return cf2_fracToFixed( stack->buffer[idx].u.f );
    default:
      return stack->buffer[idx].u.r;
    }
  }


  FT_LOCAL_DEF( void )
  cf2_stack_clear( CF2_Stack  stack )
  {
    stack->top = &stack->buffer[0];
  }



