

















#ifndef __FTVALID_H__
#define __FTVALID_H__

#include <ft2build.h>
#include FT_CONFIG_STANDARD_LIBRARY_H   


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  
  
  

  
  typedef struct FT_ValidatorRec_ volatile*  FT_Validator;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef enum  FT_ValidationLevel_
  {
    FT_VALIDATE_DEFAULT = 0,
    FT_VALIDATE_TIGHT,
    FT_VALIDATE_PARANOID

  } FT_ValidationLevel;


#if defined( _MSC_VER )      
  
  
  
#pragma warning( push )
#pragma warning( disable : 4324 )
#endif 

  
  typedef struct  FT_ValidatorRec_
  {
    ft_jmp_buf          jump_buffer; 

    const FT_Byte*      base;        
    const FT_Byte*      limit;       
    FT_ValidationLevel  level;       
    FT_Error            error;       

  } FT_ValidatorRec;

#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#define FT_VALIDATOR( x )  ( (FT_Validator)( x ) )


  FT_BASE( void )
  ft_validator_init( FT_Validator        valid,
                     const FT_Byte*      base,
                     const FT_Byte*      limit,
                     FT_ValidationLevel  level );

  
  
  FT_BASE( FT_Int )
  ft_validator_run( FT_Validator  valid );

  
  
  
  
  FT_BASE( void )
  ft_validator_error( FT_Validator  valid,
                      FT_Error      error );


  
  
  
#define FT_INVALID( _error )  FT_INVALID_( _error )
#define FT_INVALID_( _error ) \
          ft_validator_error( valid, FT_THROW( _error ) )

  
#define FT_INVALID_TOO_SHORT \
          FT_INVALID( Invalid_Table )

  
#define FT_INVALID_OFFSET \
          FT_INVALID( Invalid_Offset )

  
#define FT_INVALID_FORMAT \
          FT_INVALID( Invalid_Table )

  
#define FT_INVALID_GLYPH_ID \
          FT_INVALID( Invalid_Glyph_Index )

  
#define FT_INVALID_DATA \
          FT_INVALID( Invalid_Table )


FT_END_HEADER

#endif 



