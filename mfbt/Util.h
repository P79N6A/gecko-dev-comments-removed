






































#ifndef mozilla_Util_h_
#define mozilla_Util_h_

#ifdef __cplusplus

namespace mozilla {


















template <typename T>
struct DebugOnly
{
#ifdef DEBUG
    T value;

    DebugOnly() {}
    DebugOnly(const T& other) : value(other) {}
    DebugOnly& operator=(const T& rhs) {
        value = rhs;
        return *this;
    }

    operator T&() { return value; }
    operator const T&() const { return value; }

#else
    DebugOnly() {}
    DebugOnly(const T&) {}
    DebugOnly& operator=(const T&) {}   
#endif

    




    ~DebugOnly() {}
};

} 

#endif 

#endif  
