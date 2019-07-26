























#ifndef DenormalDisabler_h
#define DenormalDisabler_h

#define _USE_MATH_DEFINES
#include <cmath>
#include <float.h>

namespace WebCore {




#if defined(XP_WIN) && defined(_MSC_VER)
#define HAVE_DENORMAL
#endif

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define HAVE_DENORMAL
#endif

#ifdef HAVE_DENORMAL
class DenormalDisabler {
public:
    DenormalDisabler()
            : m_savedCSR(0)
    {
#if defined(XP_WIN) && defined(_MSC_VER)
        
        
        
        _controlfp_s(&m_savedCSR, 0, 0);
        unsigned int unused;
        _controlfp_s(&unused, _DN_FLUSH, _MCW_DN);
#else
        m_savedCSR = getCSR();
        setCSR(m_savedCSR | 0x8040);
#endif
    }

    ~DenormalDisabler()
    {
#if defined(XP_WIN) && defined(_MSC_VER)
        unsigned int unused;
        _controlfp_s(&unused, m_savedCSR, _MCW_DN);
#else
        setCSR(m_savedCSR);
#endif
    }

    
    static inline float flushDenormalFloatToZero(float f)
    {
#if defined(XP_WIN) && defined(_MSC_VER) && _M_IX86_FP
        
        
        
        return (fabs(f) < FLT_MIN) ? 0.0f : f;
#else
        return f;
#endif
    }
private:
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    inline int getCSR()
    {
        int result;
        asm volatile("stmxcsr %0" : "=m" (result));
        return result;
    }

    inline void setCSR(int a)
    {
        int temp = a;
        asm volatile("ldmxcsr %0" : : "m" (temp));
    }

#endif

    unsigned int m_savedCSR;
};

#else

class DenormalDisabler {
public:
    DenormalDisabler() { }

    
    
    static inline float flushDenormalFloatToZero(float f)
    {
        return (fabs(f) < FLT_MIN) ? 0.0f : f;
    }
};

#endif

} 

#undef HAVE_DENORMAL
#endif 
