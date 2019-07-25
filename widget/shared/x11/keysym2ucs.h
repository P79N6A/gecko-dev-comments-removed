














































#ifdef MOZ_X11
#include <X11/X.h>
#else
#define KeySym unsigned int
#endif 

#ifdef __cplusplus
extern "C" { 
#endif

long keysym2ucs(KeySym keysym); 

#ifdef __cplusplus
} 
#endif



