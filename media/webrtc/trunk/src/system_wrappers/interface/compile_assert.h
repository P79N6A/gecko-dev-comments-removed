









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_COMPILE_ASSERT_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_COMPILE_ASSERT_H_

template <bool>
struct CompileAssert {};






#undef COMPILE_ASSERT
#define COMPILE_ASSERT(expr, msg) \
    typedef CompileAssert<static_cast<bool>(expr)> \
        msg[static_cast<bool>(expr) ? 1 : -1]

#endif  
