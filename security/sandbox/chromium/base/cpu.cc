



#include "base/cpu.h"

#include <string.h>

#include <algorithm>

#include "base/basictypes.h"
#include "build/build_config.h"

#if defined(ARCH_CPU_ARM_FAMILY) && (defined(OS_ANDROID) || defined(OS_LINUX))
#include "base/file_util.h"
#include "base/lazy_instance.h"
#endif

#if defined(ARCH_CPU_X86_FAMILY)
#if defined(_MSC_VER)
#include <intrin.h>
#include <immintrin.h>  
#endif
#endif

namespace base {

CPU::CPU()
  : signature_(0),
    type_(0),
    family_(0),
    model_(0),
    stepping_(0),
    ext_model_(0),
    ext_family_(0),
    has_mmx_(false),
    has_sse_(false),
    has_sse2_(false),
    has_sse3_(false),
    has_ssse3_(false),
    has_sse41_(false),
    has_sse42_(false),
    has_avx_(false),
    has_avx_hardware_(false),
    has_aesni_(false),
    has_non_stop_time_stamp_counter_(false),
    cpu_vendor_("unknown") {
  Initialize();
}

namespace {

#if defined(ARCH_CPU_X86_FAMILY)
#ifndef _MSC_VER

#if defined(__pic__) && defined(__i386__)

void __cpuid(int cpu_info[4], int info_type) {
  __asm__ volatile (
    "mov %%ebx, %%edi\n"
    "cpuid\n"
    "xchg %%edi, %%ebx\n"
    : "=a"(cpu_info[0]), "=D"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type)
  );
}

#else

void __cpuid(int cpu_info[4], int info_type) {
  __asm__ volatile (
    "cpuid \n\t"
    : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type)
  );
}

#endif



uint64 _xgetbv(uint32 xcr) {
  uint32 eax, edx;

  __asm__ volatile ("xgetbv" : "=a" (eax), "=d" (edx) : "c" (xcr));
  return (static_cast<uint64>(edx) << 32) | eax;
}

#endif  
#endif  

#if defined(ARCH_CPU_ARM_FAMILY) && (defined(OS_ANDROID) || defined(OS_LINUX))





std::string ParseCpuInfo() {
  const char kModelNamePrefix[] = "model name\t: ";
  const char kProcessorPrefix[] = "Processor\t: ";
  std::string contents;
  ReadFileToString(FilePath("/proc/cpuinfo"), &contents);
  DCHECK(!contents.empty());
  std::string cpu_brand;
  if (!contents.empty()) {
    std::istringstream iss(contents);
    std::string line;
    while (std::getline(iss, line)) {
      if (line.compare(0, strlen(kModelNamePrefix), kModelNamePrefix) == 0) {
        cpu_brand.assign(line.substr(strlen(kModelNamePrefix)));
        break;
      }
      if (line.compare(0, strlen(kProcessorPrefix), kProcessorPrefix) == 0) {
        cpu_brand.assign(line.substr(strlen(kProcessorPrefix)));
        break;
      }
    }
  }
  return cpu_brand;
}

class LazyCpuInfoValue {
 public:
  LazyCpuInfoValue() : value_(ParseCpuInfo()) {}
  const std::string& value() { return value_; }

 private:
  const std::string value_;
  DISALLOW_COPY_AND_ASSIGN(LazyCpuInfoValue);
};

base::LazyInstance<LazyCpuInfoValue> g_lazy_cpu_brand =
    LAZY_INSTANCE_INITIALIZER;

const std::string& CpuBrandInfo() {
  return g_lazy_cpu_brand.Get().value();
}

#endif  
        

}  

void CPU::Initialize() {
#if defined(ARCH_CPU_X86_FAMILY)
  int cpu_info[4] = {-1};
  char cpu_string[48];

  
  
  
  
  
  
  
  __cpuid(cpu_info, 0);
  int num_ids = cpu_info[0];
  std::swap(cpu_info[2], cpu_info[3]);
  memcpy(cpu_string, &cpu_info[1], 3 * sizeof(cpu_info[1]));
  cpu_vendor_.assign(cpu_string, 3 * sizeof(cpu_info[1]));

  
  if (num_ids > 0) {
    __cpuid(cpu_info, 1);
    signature_ = cpu_info[0];
    stepping_ = cpu_info[0] & 0xf;
    model_ = ((cpu_info[0] >> 4) & 0xf) + ((cpu_info[0] >> 12) & 0xf0);
    family_ = (cpu_info[0] >> 8) & 0xf;
    type_ = (cpu_info[0] >> 12) & 0x3;
    ext_model_ = (cpu_info[0] >> 16) & 0xf;
    ext_family_ = (cpu_info[0] >> 20) & 0xff;
    has_mmx_ =   (cpu_info[3] & 0x00800000) != 0;
    has_sse_ =   (cpu_info[3] & 0x02000000) != 0;
    has_sse2_ =  (cpu_info[3] & 0x04000000) != 0;
    has_sse3_ =  (cpu_info[2] & 0x00000001) != 0;
    has_ssse3_ = (cpu_info[2] & 0x00000200) != 0;
    has_sse41_ = (cpu_info[2] & 0x00080000) != 0;
    has_sse42_ = (cpu_info[2] & 0x00100000) != 0;
    has_avx_hardware_ =
                 (cpu_info[2] & 0x10000000) != 0;
    
    
    
    
    
    
    
    
    
    
    has_avx_ =
        has_avx_hardware_ &&
        (cpu_info[2] & 0x04000000) != 0  &&
        (cpu_info[2] & 0x08000000) != 0  &&
        (_xgetbv(0) & 6) == 6 ;
    has_aesni_ = (cpu_info[2] & 0x02000000) != 0;
  }

  
  __cpuid(cpu_info, 0x80000000);
  const int parameter_end = 0x80000004;
  int max_parameter = cpu_info[0];

  if (cpu_info[0] >= parameter_end) {
    char* cpu_string_ptr = cpu_string;

    for (int parameter = 0x80000002; parameter <= parameter_end &&
         cpu_string_ptr < &cpu_string[sizeof(cpu_string)]; parameter++) {
      __cpuid(cpu_info, parameter);
      memcpy(cpu_string_ptr, cpu_info, sizeof(cpu_info));
      cpu_string_ptr += sizeof(cpu_info);
    }
    cpu_brand_.assign(cpu_string, cpu_string_ptr - cpu_string);
  }

  const int parameter_containing_non_stop_time_stamp_counter = 0x80000007;
  if (max_parameter >= parameter_containing_non_stop_time_stamp_counter) {
    __cpuid(cpu_info, parameter_containing_non_stop_time_stamp_counter);
    has_non_stop_time_stamp_counter_ = (cpu_info[3] & (1 << 8)) != 0;
  }
#elif defined(ARCH_CPU_ARM_FAMILY) && (defined(OS_ANDROID) || defined(OS_LINUX))
  cpu_brand_.assign(CpuBrandInfo());
#endif
}

CPU::IntelMicroArchitecture CPU::GetIntelMicroArchitecture() const {
  if (has_avx()) return AVX;
  if (has_sse42()) return SSE42;
  if (has_sse41()) return SSE41;
  if (has_ssse3()) return SSSE3;
  if (has_sse3()) return SSE3;
  if (has_sse2()) return SSE2;
  if (has_sse()) return SSE;
  return PENTIUM;
}

}  
