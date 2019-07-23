






































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
		#define PRFX					counter_increment(native);\
			if (verbose_enabled()) {\
				outline[0]='\0';\
				if (outputAddr) { sprintf(outline, "         %8p  ",_nIns); } \
				else { sprintf(outline, "                   "); } \
				sprintf(&outline[19]
		#define PSFX					Assembler::outputAlign(outline, 45);\
			RegAlloc::formatRegisters(_allocator, outline, _thisfrag);\
			strcat(outline,outlineEOL); \
			outlineEOL[0]='\0'; \
			outputAddr = false; \
			Assembler::output_asm(outline); }
		
		
		#define asm_output(s)			PRFX,s); PSFX
		#define asm_output1(s,x)		PRFX,s,x); PSFX
		#define asm_output2(s,x,y)		PRFX,s,x,y); PSFX
		#define asm_output3(s,x,y,z)	PRFX,s,x,y,z); PSFX
		#define asm_output5(s,x,y,z,a,b) PRFX,s,x,y,z,a,b); PSFX
		#define gpn(r)					regNames[(r)] 
		#define fpn(r)					regNames[(r)] 
	#else
		#define PRFX			
		#define asm_output(s)
		#define asm_output1(s,x)	
		#define asm_output2(s,x,y)	
		#define asm_output3(s,x,y,z)	
		#define asm_output5(s,x,y,z,a,b)	
		#define gpn(r)		
	#endif /* NJ_VERBOSE */


#endif 
