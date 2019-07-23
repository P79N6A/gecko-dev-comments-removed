






































#ifndef __nanojit_Native__
#define __nanojit_Native__


#ifdef NANOJIT_IA32
#include "Nativei386.h"
#elif defined(NANOJIT_ARM)
#ifdef THUMB
#include "NativeThumb.h"
#else
#include "NativeARM.h"
#endif
#elif defined(NANOJIT_PPC)
#include "NativePpc.h"
#elif defined(NANOJIT_AMD64)
#include "NativeAMD64.h"
#else
#error "unknown nanojit architecture"
#endif

namespace nanojit {
	const uint32_t NJ_PAGE_SIZE = 1 << NJ_LOG2_PAGE_SIZE;
	
    class Fragment;
    struct SideExit;
    
    struct GuardRecord 
    {
        void* jmpToStub;
        void* stubEntry;
        void* jmpToTarget;
        GuardRecord* next;
        SideExit* exit;
    };
    
    struct SideExit
    {
        GuardRecord* guards;
        Fragment* from;
        Fragment* target;
        
        void addGuard(GuardRecord* lr) 
        {
            lr->next = guards;
            guards = lr;
        }
    };
}

	#ifdef NJ_STACK_GROWTH_UP
		#define stack_direction(n)   n
	#else
		#define stack_direction(n)  -n
	#endif
	
	#define isSPorFP(r)		( (r)==SP || (r)==FP )

	#ifdef NJ_VERBOSE
		#define asm_output(FMT, ...) do {\
			counter_increment(native);\
			if (verbose_enabled()) {\
				outline[0]='\0';\
				if (outputAddr) sprintf(outline, "  %10p  ",_nIns);\
				else sprintf(outline, "              ");\
				sprintf(&outline[14], FMT, ##__VA_ARGS__);\
				Assembler::outputAlign(outline, 45);\
				RegAlloc::formatRegisters(_allocator, outline, _thisfrag);\
				Assembler::output_asm(outline);\
			}\
		} while (0) /* no semi */ 
		#define gpn(r)					regNames[(r)] 
		#define fpn(r)					regNames[(r)] 
	#else
		#define asm_output(f, ...)
		#define gpn(r)		
		#define fpn(r)		
	#endif /* NJ_VERBOSE */

#endif 
