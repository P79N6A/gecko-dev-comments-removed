





































#ifndef __CF2STACK_H__
#define __CF2STACK_H__


FT_BEGIN_HEADER


  
  typedef struct  CF2_StackNumber_
  {
    union
    {
      CF2_Fixed  r;      
      CF2_Frac   f;      
      CF2_Int    i;
    } u;

    CF2_NumberType  type;

  } CF2_StackNumber;


  typedef struct  CF2_StackRec_
  {
    FT_Memory         memory;
    FT_Error*         error;
    CF2_StackNumber   buffer[CF2_OPERAND_STACK_SIZE];
    CF2_StackNumber*  top;

  } CF2_StackRec, *CF2_Stack;


  FT_LOCAL( CF2_Stack )
  cf2_stack_init( FT_Memory  memory,
                  FT_Error*  error );
  FT_LOCAL( void )
  cf2_stack_free( CF2_Stack  stack );

  FT_LOCAL( CF2_UInt )
  cf2_stack_count( CF2_Stack  stack );

  FT_LOCAL( void )
  cf2_stack_pushInt( CF2_Stack  stack,
                     CF2_Int    val );
  FT_LOCAL( void )
  cf2_stack_pushFixed( CF2_Stack  stack,
                       CF2_Fixed  val );

  FT_LOCAL( CF2_Int )
  cf2_stack_popInt( CF2_Stack  stack );
  FT_LOCAL( CF2_Fixed )
  cf2_stack_popFixed( CF2_Stack  stack );

  FT_LOCAL( CF2_Fixed )
  cf2_stack_getReal( CF2_Stack  stack,
                     CF2_UInt   idx );

  FT_LOCAL( void )
  cf2_stack_clear( CF2_Stack  stack );


FT_END_HEADER


#endif 



