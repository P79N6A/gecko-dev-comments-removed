






































#ifndef __nanojit_NativeCommon__
#define __nanojit_NativeCommon__

namespace nanojit
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
#if defined(NJ_USE_UINT32_REGISTER)
    #define REGNUM(r) (r)

#elif defined(DEBUG)
    struct Register {
        uint32_t n;     
    };

    static inline uint32_t REGNUM(Register r) {
        return r.n;
    }

    static inline Register REGINC(Register r) {
        r.n++;
        return r;
    }

    static inline bool operator==(Register r1, Register r2)
    {
        return r1.n == r2.n;
    }

    static inline bool operator!=(Register r1, Register r2)
    {
        return r1.n != r2.n;
    }
#else
    typedef uint32_t Register;

    static inline uint32_t REGNUM(Register r) {
        return r;
    }

    static inline Register REGINC(Register r) {
        return r+1;
    }
#endif
} 

#endif 
