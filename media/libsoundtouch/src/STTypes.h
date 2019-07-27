





































#ifndef STTypes_H
#define STTypes_H

typedef unsigned int    uint;
typedef unsigned long   ulong;


#ifdef _WIN64
    typedef unsigned long long ulongptr;
#else
    typedef ulong ulongptr;
#endif



#define SOUNDTOUCH_ALIGN_POINTER_16(x)      ( ( (ulongptr)(x) + 15 ) & ~(ulongptr)15 )


#include "soundtouch_config.h"

#if defined(WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

namespace soundtouch
{
    
    
    
    

    
    
    
    
    
    

    #if (defined(__SOFTFP__) && defined(ANDROID))
        
        
        #undef  SOUNDTOUCH_FLOAT_SAMPLES
        #define SOUNDTOUCH_INTEGER_SAMPLES      1
    #endif

    #if !(SOUNDTOUCH_INTEGER_SAMPLES || SOUNDTOUCH_FLOAT_SAMPLES)
       
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        #define SOUNDTOUCH_FLOAT_SAMPLES       1    //< 32bit float samples
     
    #endif

    #if (_M_IX86 || __i386__ || __x86_64__ || _M_X64)
        
        
        
        
        

        #define SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS     1

        
        
        
        
        #ifdef SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS
            #undef SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS
        #endif
    #else
        
        #undef SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS

    #endif

    
    
    
    #define SOUNDTOUCH_ALLOW_NONEXACT_SIMD_OPTIMIZATION    1


    #ifdef SOUNDTOUCH_INTEGER_SAMPLES
        
        typedef short SAMPLETYPE;
        
        typedef long  LONG_SAMPLETYPE;

        #ifdef SOUNDTOUCH_FLOAT_SAMPLES
            
            #error "conflicting sample types defined"
        #endif 

        #ifdef SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS
            
            #define SOUNDTOUCH_ALLOW_MMX   1
        #endif

    #else

        
        typedef float  SAMPLETYPE;
        
        typedef double LONG_SAMPLETYPE;

        #ifdef SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS
            
            #define SOUNDTOUCH_ALLOW_SSE       1
        #endif

    #endif  

};



#ifdef ST_NO_EXCEPTION_HANDLING
    
    #include <assert.h>
    #define ST_THROW_RT_ERROR(x)    {assert((const char *)x);}
#else
    
    #include <stdexcept>
    #include <string>
    #define ST_THROW_RT_ERROR(x)    {throw std::runtime_error(x);}
#endif







#endif
