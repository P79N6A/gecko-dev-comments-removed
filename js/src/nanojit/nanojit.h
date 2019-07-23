




































#ifndef __nanojit_h_
#define __nanojit_h__

#include <stddef.h>
#include "avmplus.h"

#ifdef AVMPLUS_IA32
#define NANOJIT_IA32
#elif AVMPLUS_ARM
#define NANOJIT_ARM
#elif AVMPLUS_PPC
#define NANOJIT_PPC
#else
#error "unknown nanojit architecture"
#endif

namespace nanojit
{
	




	class Fragment;
	class LIns;
	struct SideExit;
	class RegAlloc;
	typedef avmplus::AvmCore AvmCore;
	typedef avmplus::OSDep OSDep;
	typedef const uint16_t* FragID;
	typedef avmplus::SortedMap<FragID,Fragment*,avmplus::LIST_GCObjects> FragmentMap;
	typedef avmplus::SortedMap<SideExit*,RegAlloc*,avmplus::LIST_GCObjects> RegAllocMap;
	typedef avmplus::List<LIns*,avmplus::LIST_NonGCObjects>	InsList;
	typedef avmplus::List<intptr_t, avmplus::LIST_GCObjects> NInsList;
	typedef avmplus::List<char*, avmplus::LIST_GCObjects> StringList;

	#if defined(_DEBUG)
		
		#ifndef WIN32
			inline void DebugBreak() { AvmAssert(0); }
		#endif

		#define _NanoAssertMsg(a,m)		do { if ((a)==0) { AvmDebugLog(("%s", m)); DebugBreak(); } } while (0)
		#define NanoAssertMsg(x,y)				do { _NanoAssertMsg((x), (y)); } while (0) /* no semi */
		#define _NanoAssertMsgf(a,m)		do { if ((a)==0) { AvmDebugLog(m); DebugBreak(); } } while (0)
		#define NanoAssertMsgf(x,y)				do { _NanoAssertMsgf((x), y); } while (0) /* no semi */
		#define NanoAssert(x)					_NanoAssert((x), __LINE__,__FILE__)
		#define _NanoAssert(x, line_, file_)	__NanoAssert((x), line_, file_)
		#define __NanoAssert(x, line_, file_)	do { NanoAssertMsg((x), "Assertion failed: \"" #x "\" (" #file_ ":" #line_ ")"); } while (0) /* no semi */
	#else
		#define NanoAssertMsgf(x,y)	do { } while (0) /* no semi */
		#define NanoAssertMsg(x,y)	do { } while (0) /* no semi */
		#define NanoAssert(x)		do { } while (0) /* no semi */
	#endif

	




}

#ifdef AVMPLUS_VERBOSE
#define NJ_VERBOSE 1
#define NJ_PROFILE 1
#endif

#ifdef NJ_VERBOSE
	#include <stdio.h>
	#define verbose_output						if (verbose_enabled()) Assembler::output
	#define verbose_outputf						if (verbose_enabled()) Assembler::outputf
	#define verbose_enabled()					(_verbose)
	#define verbose_only(x)						x
#else
	#define verbose_output
	#define verbose_outputf
	#define verbose_enabled()
	#define verbose_only(x)
#endif 

#ifdef _DEBUG
	#define debug_only(x)			x
#else
	#define debug_only(x)
#endif 

#ifdef NJ_PROFILE
	#define counter_struct_begin()  struct {
	#define counter_struct_end()	} _stats;
	#define counter_define(x) 		int32_t x
	#define counter_value(x)		_stats.x
	#define counter_set(x,v)		(counter_value(x)=(v))
	#define counter_adjust(x,i)		(counter_value(x)+=(int32_t)(i))
	#define counter_reset(x)		counter_set(x,0)
	#define counter_increment(x)	counter_adjust(x,1)
	#define counter_decrement(x)	counter_adjust(x,-1)
	#define profile_only(x)			x
#else
	#define counter_struct_begin()
	#define counter_struct_end()
	#define counter_define(x) 		
	#define counter_value(x)
	#define counter_set(x,v)
	#define counter_adjust(x,i)
	#define counter_reset(x)
	#define counter_increment(x)	
	#define counter_decrement(x)	
	#define profile_only(x)	
#endif 

#define isS8(i)  ( int32_t(i) == int8_t(i) )
#define isU8(i)  ( int32_t(i) == uint8_t(i) )
#define isS16(i) ( int32_t(i) == int16_t(i) )
#define isU16(i) ( int32_t(i) == uint16_t(i) )

#define alignTo(x,s)		((((uint32_t)(x)))&~((s)-1))
#define alignUp(x,s)		((((uint32_t)(x))+((s)-1))&~((s)-1))

#define pageTop(x)			( (int*)alignTo(x,NJ_PAGE_SIZE) )
#define pageBottom(x)		( (int*)(alignTo(x,NJ_PAGE_SIZE)+NJ_PAGE_SIZE)-1 )
#define samepage(x,y)		(pageTop(x) == pageTop(y))

#include "Native.h"
#include "LIR.h"




#endif 
