






































#ifndef jsdtoa_h___
#define jsdtoa_h___





#include "jscompat.h"

JS_BEGIN_EXTERN_C














#define JS_DTOA_ERANGE 1
#define JS_DTOA_ENOMEM 2
JS_FRIEND_API(double)
JS_strtod(const char *s00, char **se, int *err);













typedef enum JSDToStrMode {
    DTOSTR_STANDARD,              
    DTOSTR_STANDARD_EXPONENTIAL,  
    DTOSTR_FIXED,                 
    DTOSTR_EXPONENTIAL,           
    DTOSTR_PRECISION              
} JSDToStrMode;




#define DTOSTR_STANDARD_BUFFER_SIZE 26



#define DTOSTR_VARIABLE_BUFFER_SIZE(precision) ((precision)+24 > DTOSTR_STANDARD_BUFFER_SIZE ? (precision)+24 : DTOSTR_STANDARD_BUFFER_SIZE)








JS_FRIEND_API(char *)
JS_dtostr(char *buffer, size_t bufferSize, JSDToStrMode mode, int precision, double dval);












JS_FRIEND_API(char *)
JS_dtobasestr(int base, double d);





extern void js_FinishDtoa(void);

JS_END_EXTERN_C

#endif 
