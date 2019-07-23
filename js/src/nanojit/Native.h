






































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
#elif defined(NANOJIT_SPARC)
#include "NativeSparc.h"
#elif defined(NANOJIT_AMD64)
#include "NativeAMD64.h"
#else
#error "unknown nanojit architecture"
#endif

namespace nanojit {
	const uint32_t NJ_PAGE_SIZE = 1 << NJ_LOG2_PAGE_SIZE;
	
    class Fragment;
    struct SideExit;
	struct SwitchInfo;
    
    struct GuardRecord 
    {
        void* jmp;
        GuardRecord* next;
        SideExit* exit;
    };
    
    struct SideExit
    {
        GuardRecord* guards;
        Fragment* from;
        Fragment* target;
		SwitchInfo* switchInfo;
        
        void addGuard(GuardRecord* gr)
        {
            NanoAssert(gr->next == NULL);
            NanoAssert(guards != gr);
            gr->next = guards;
            guards = gr;
        }
    };
}

	#ifdef NJ_STACK_GROWTH_UP
		#define stack_direction(n)   n
	#else
		#define stack_direction(n)  -n
	#endif
	
	#define isSPorFP(r)		( (r)==SP || (r)==FP )

	#if defined(_MSC_VER) && _MSC_VER < 1400
		static void asm_output(const char *f, ...) {}
		#define gpn(r)					regNames[(r)]
		#define fpn(r)					regNames[(r)]
	#elif defined(NJ_VERBOSE)
		#define asm_output(...) do {\
			counter_increment(native);\
			if (verbose_enabled()) {\
				outline[0]='\0';\
				if (outputAddr) sprintf(outline, "  %10p  ",_nIns);\
				else sprintf(outline, "              ");\
				sprintf(&outline[14], ##__VA_ARGS__);\
				Assembler::outputAlign(outline, 45);\
				RegAlloc::formatRegisters(_allocator, outline, _thisfrag);\
				Assembler::output_asm(outline);\
				outputAddr=false; /* set =true if you like to see addresses for each native instruction */ \
			}\
		} while (0) /* no semi */ 
		#define gpn(r)					regNames[(r)] 
		#define fpn(r)					regNames[(r)] 
	#else
		#define asm_output(...)
		#define gpn(r)		
		#define fpn(r)		
	#endif /* NJ_VERBOSE */

#endif 
