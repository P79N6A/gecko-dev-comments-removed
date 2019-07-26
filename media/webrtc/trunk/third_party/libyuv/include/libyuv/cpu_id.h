









#ifndef INCLUDE_LIBYUV_CPU_ID_H_  
#define INCLUDE_LIBYUV_CPU_ID_H_

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif



#define kCpuInit 0x1


static const int kCpuHasARM = 0x2;
static const int kCpuHasNEON = 0x4;



static const int kCpuHasX86 = 0x10;
static const int kCpuHasSSE2 = 0x20;
static const int kCpuHasSSSE3 = 0x40;
static const int kCpuHasSSE41 = 0x80;
static const int kCpuHasSSE42 = 0x100;
static const int kCpuHasAVX = 0x200;
static const int kCpuHasAVX2 = 0x400;
static const int kCpuHasERMS = 0x800;
static const int kCpuHasFMA3 = 0x1000;



static const int kCpuHasMIPS = 0x10000;
static const int kCpuHasMIPS_DSP = 0x20000;
static const int kCpuHasMIPS_DSPR2 = 0x40000;


LIBYUV_API
int InitCpuFlags(void);


LIBYUV_API
int ArmCpuCaps(const char* cpuinfo_name);




static __inline int TestCpuFlag(int test_flag) {
  LIBYUV_API extern int cpu_info_;
  return (cpu_info_ == kCpuInit ? InitCpuFlags() : cpu_info_) & test_flag;
}





LIBYUV_API
void MaskCpuFlags(int enable_flags);




LIBYUV_API
void CpuId(uint32 eax, uint32 ecx, uint32* cpu_info);

#ifdef __cplusplus
}  
}  
#endif

#endif  
