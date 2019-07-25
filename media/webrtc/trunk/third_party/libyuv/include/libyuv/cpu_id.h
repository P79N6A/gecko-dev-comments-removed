









#ifndef INCLUDE_LIBYUV_CPU_ID_H_
#define INCLUDE_LIBYUV_CPU_ID_H_

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


static const int kCpuHasSSE2 = 1;
static const int kCpuHasSSSE3 = 2;


static const int kCpuHasNEON = 4;


static const int kCpuInitialized = 8;




static __inline int TestCpuFlag(int test_flag) {
  extern int cpu_info_;
  extern int InitCpuFlags();
  return (cpu_info_ ? cpu_info_ : InitCpuFlags()) & test_flag;
}





void MaskCpuFlags(int enable_flags);

#ifdef __cplusplus
}  
}  
#endif

#endif  
