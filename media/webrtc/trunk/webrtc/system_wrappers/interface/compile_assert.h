











#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_COMPILE_ASSERT_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_COMPILE_ASSERT_H_


















#if !defined(COMPILE_ASSERT)
#if __cplusplus >= 201103L

#define COMPILE_ASSERT(expr, msg) static_assert(expr, #msg)

#else
template <bool>
struct CompileAssert {
};

#define COMPILE_ASSERT(expr, msg) \
  typedef CompileAssert<(bool(expr))> msg[bool(expr) ? 1 : -1]

#endif  
#endif  










































#endif  
