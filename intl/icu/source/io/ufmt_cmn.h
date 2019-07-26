


















#ifndef UFMT_CMN_H
#define UFMT_CMN_H

#include "unicode/utypes.h"
#include "unicode/utf16.h"

#define UFMT_DEFAULT_BUFFER_SIZE 128
#define MAX_UCHAR_BUFFER_SIZE(buffer) (sizeof(buffer)/(U16_MAX_LENGTH*sizeof(UChar)))
#define MAX_UCHAR_BUFFER_NEEDED(strLen) ((strLen+1)*U16_MAX_LENGTH*sizeof(UChar))




typedef enum ufmt_type_info {
    ufmt_empty = 0,
    ufmt_simple_percent, 
    ufmt_count,      
    ufmt_int,        
    ufmt_char,       
    ufmt_string,     
    ufmt_pointer,    
    ufmt_float,      
    ufmt_double,     
    ufmt_uchar,      
    ufmt_ustring     
          
        
           
    
} ufmt_type_info;




typedef union ufmt_args {
    int64_t int64Value;    
    float   floatValue;    
    double  doubleValue;   
    void    *ptrValue;     
            
         
} ufmt_args;







#define ufmt_min(a,b) ((a) < (b) ? (a) : (b))






int
ufmt_digitvalue(UChar c);







UBool
ufmt_isdigit(UChar     c,
         int32_t     radix);












void 
ufmt_64tou(UChar     *buffer, 
      int32_t     *len,
      uint64_t     value, 
      uint8_t     radix,
      UBool    uselower,
      int32_t    minDigits);






void 
ufmt_ptou(UChar    *buffer, 
          int32_t   *len,
          void      *value, 
          UBool     uselower);









int64_t
ufmt_uto64(const UChar     *buffer, 
      int32_t     *len,
      int8_t     radix);









void *
ufmt_utop(const UChar     *buffer,
      int32_t     *len);










UChar*
ufmt_defaultCPToUnicode(const char *s, int32_t sSize,
                        UChar *target, int32_t tSize);



#endif

