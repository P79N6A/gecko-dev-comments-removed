











#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_COMPILE_ASSERT_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_COMPILE_ASSERT_H_


















#if !defined(COMPILE_ASSERT)
template <bool>
struct CompileAssert {
};

#define COMPILE_ASSERT(expr, msg) \
  typedef CompileAssert<(bool(expr))> msg[bool(expr) ? 1 : -1]
#endif  










































#endif  
