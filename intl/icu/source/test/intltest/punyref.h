





























#ifndef _PUNYREF_H
#define _PUNYREF_H




#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

enum punycode_status {
  punycode_success,
  punycode_bad_input,   
  punycode_big_output,  
  punycode_overflow     
};


typedef uint32_t punycode_uint;

U_CDECL_BEGIN

enum punycode_status  punycode_encode(
  punycode_uint input_length,
  const punycode_uint input[],
  const unsigned char case_flags[],
  punycode_uint *output_length,
  char output[] );

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

enum punycode_status punycode_decode(
  punycode_uint input_length,
  const char input[],
  punycode_uint *output_length,
  punycode_uint output[],
  unsigned char case_flags[] );

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
U_CDECL_END

#endif 

#endif
