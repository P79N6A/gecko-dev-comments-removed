





#ifndef jsdtoa_h
#define jsdtoa_h






#include <stddef.h>

struct DtoaState;

namespace js {

extern DtoaState *
NewDtoaState();

extern void
DestroyDtoaState(DtoaState *state);

} 














#define JS_DTOA_ENOMEM 2
double
js_strtod_harder(DtoaState *state, const char *s00, char **se, int *err);













typedef enum JSDToStrMode {
    DTOSTR_STANDARD,              
    DTOSTR_STANDARD_EXPONENTIAL,  
    DTOSTR_FIXED,                 
    DTOSTR_EXPONENTIAL,           
    DTOSTR_PRECISION              
} JSDToStrMode;




#define DTOSTR_STANDARD_BUFFER_SIZE 26



#define DTOSTR_VARIABLE_BUFFER_SIZE(precision) ((precision)+24 > DTOSTR_STANDARD_BUFFER_SIZE ? (precision)+24 : DTOSTR_STANDARD_BUFFER_SIZE)

















char *
js_dtostr(DtoaState *state, char *buffer, size_t bufferSize, JSDToStrMode mode, int precision,
          double dval);



















char *
js_dtobasestr(DtoaState *state, int base, double d);

#endif 
