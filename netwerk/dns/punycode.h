






















#ifdef __cplusplus
extern "C" {
#endif




#include <limits.h>

enum punycode_status {
  punycode_success,
  punycode_bad_input,   
  punycode_big_output,  
  punycode_overflow     
};

#if UINT_MAX >= (1 << 26) - 1
typedef unsigned int punycode_uint;
#else
typedef unsigned long punycode_uint;
#endif

enum punycode_status punycode_encode(
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

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

#ifdef __cplusplus
}
#endif 
