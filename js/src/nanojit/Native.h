





































#ifndef __nanojit_Native__
#define __nanojit_Native__


#ifdef NANOJIT_IA32
#include "Nativei386.h"
#elif defined(NANOJIT_ARM)
#ifdef THUMB
#include "NativeThumb.h"
#else
#include "NativeArm.h"
#endif
#elif defined(NANOJIT_PPC)
#include "NativePpc.h"
#else
#error "unknown nanojit architecture"
#endif

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
				sprintf(outline, "                   ");\
				sprintf(&outline[19]
		#define PSFX					Assembler::outputAlign(outline, 45);\
			RegAlloc::formatRegisters(_allocator, outline, _thisfrag);\
			Assembler::output_asm(outline); }
		
		
		#define asm_output(s)			PRFX,s); PSFX
		#define asm_output1(s,x)		PRFX,s,x); PSFX
		#define asm_output2(s,x,y)		PRFX,s,x,y); PSFX
		#define asm_output3(s,x,y,z)	PRFX,s,x,y,z); PSFX
		#define gpn(r)					regNames[(r)] 
		#define fpn(r)					regNames[(r)] 
	#else
		#define PRFX			
		#define asm_output(s)
		#define asm_output1(s,x)	
		#define asm_output2(s,x,y)	
		#define asm_output3(s,x,y,z)	
		#define gpn(r)		
	#endif 


#endif 
