

























#include "jit/arm64/vixl/Cpu-vixl.h"

#include "jsutil.h"

#include "jit/arm64/vixl/Utils-vixl.h"

namespace vixl {


unsigned CPU::dcache_line_size_ = 1;
unsigned CPU::icache_line_size_ = 1;


void
CPU::SetUp()
{
    uint32_t cache_type_register = GetCacheType();

    
    
    static const int kDCacheLineSizeShift = 16;
    static const int kICacheLineSizeShift = 0;
    static const uint32_t kDCacheLineSizeMask = 0xf << kDCacheLineSizeShift;
    static const uint32_t kICacheLineSizeMask = 0xf << kICacheLineSizeShift;

    
    
    uint32_t dcache_line_size_power_of_two =
        (cache_type_register & kDCacheLineSizeMask) >> kDCacheLineSizeShift;
    uint32_t icache_line_size_power_of_two =
        (cache_type_register & kICacheLineSizeMask) >> kICacheLineSizeShift;

    dcache_line_size_ = 4 << dcache_line_size_power_of_two;
    icache_line_size_ = 4 << icache_line_size_power_of_two;
}

uint32_t
CPU::GetCacheType()
{
#ifdef JS_SIMULATOR_ARM64
    
    
    return 0;
#else
    uint32_t cache_type_register;
    
    __asm__ __volatile__ ("mrs %[ctr], ctr_el0"  
                          : [ctr] "=r" (cache_type_register));
    return cache_type_register;
#endif
}


void
CPU::EnsureIAndDCacheCoherency(void* address, size_t length)
{
#ifdef JS_SIMULATOR_ARM64
    USE(address);
    USE(length);
#else
    if (length == 0)
        return;

    

    
    
    uintptr_t start = reinterpret_cast<uintptr_t>(address);
    uintptr_t dsize = static_cast<uintptr_t>(dcache_line_size_);
    uintptr_t isize = static_cast<uintptr_t>(icache_line_size_);
    uintptr_t dline = start & ~(dsize - 1);
    uintptr_t iline = start & ~(isize - 1);

    
    MOZ_ASSERT(IsPowerOfTwo(dsize));
    MOZ_ASSERT(IsPowerOfTwo(isize));
    uintptr_t end = start + length;

    do {
        __asm__ __volatile__ (
                
                
                
                
                
                
                
                
                
                "   dc    cvau, %[dline]\n"
                :
                : [dline] "r" (dline)
                
                
                : "memory");
        dline += dsize;
    } while (dline < end);

    __asm__ __volatile__ (
        
        
        
        
        
        
        
        
        
        
        
        "   dsb   ish\n"
        : : : "memory");

    do {
        __asm__ __volatile__ (
            
            
            
            
            
            
            "   ic   ivau, %[iline]\n"
            :
            : [iline] "r" (iline)
            : "memory");
        iline += isize;
    } while (iline < end);

    __asm__ __volatile__ (
        
        
        "   dsb  ish\n"

        
        
        
        "   isb\n"
        : : : "memory");
#endif 
}

} 
