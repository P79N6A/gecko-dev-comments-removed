





































#ifndef __nanojit_h__
#define __nanojit_h__

#include <stddef.h>
#include "avmplus.h"

#ifdef FEATURE_NANOJIT

#ifdef AVMPLUS_IA32
#define NANOJIT_IA32
#elif AVMPLUS_ARM
#define NANOJIT_ARM
#elif AVMPLUS_PPC
#define NANOJIT_PPC
#elif AVMPLUS_SPARC
#define NANOJIT_SPARC
#elif AVMPLUS_AMD64
#define NANOJIT_AMD64
#define NANOJIT_64BIT
#else
#error "unknown nanojit architecture"
#endif











#ifdef MMGC_API
	
	
	
	inline void mmgc_delete(GCObject* o)
	{
		GC* g = GC::GetGC(o); 
		if (g->Collecting()) 
			g->Free(o); 
		else 
			delete o; 
	}

	inline void mmgc_delete(GCFinalizedObject* o)
	{
		GC* g = GC::GetGC(o); 
		if (g->Collecting()) 
			g->Free(o); 
		else 
			delete o; 
	}

	#define NJ_NEW(gc, cls)			new (gc) cls
	#define NJ_DELETE(obj)			do { mmgc_delete(obj); } while (0)
#else
	#define NJ_NEW(gc, cls)			new (gc) cls
	#define NJ_DELETE(obj)			do { delete obj; } while (0)
#endif


#ifdef MOZ_VALGRIND
#  define JS_VALGRIND
#endif
#ifdef JS_VALGRIND
#  include <valgrind/valgrind.h>
#else
#  define VALGRIND_DISCARD_TRANSLATIONS(addr, szB)
#endif
 
namespace nanojit
{
	




	class Fragment;
	class LIns;
	struct SideExit;
	class RegAlloc;
	struct Page;
	typedef avmplus::AvmCore AvmCore;
	typedef avmplus::OSDep OSDep;
	typedef avmplus::GCSortedMap<const void*,Fragment*,avmplus::LIST_GCObjects> FragmentMap;
	typedef avmplus::SortedMap<SideExit*,RegAlloc*,avmplus::LIST_GCObjects> RegAllocMap;
	typedef avmplus::List<LIns*,avmplus::LIST_NonGCObjects>	InsList;
	typedef avmplus::List<char*, avmplus::LIST_GCObjects> StringList;
	typedef avmplus::List<Page*,avmplus::LIST_NonGCObjects>	PageList;

    const uint32_t MAXARGS = 8;

	#if defined(_MSC_VER) && _MSC_VER < 1400
		static void NanoAssertMsgf(bool a,const char *f,...) {}
		static void NanoAssertMsg(bool a,const char *m) {}
		static void NanoAssert(bool a) {}
	#elif defined(_DEBUG)
		
		#define __NanoAssertMsgf(a, file_, line_, f, ...)  \
			if (!(a)) { \
				fprintf(stderr, "Assertion failed: " f "%s (%s:%d)\n", __VA_ARGS__, #a, file_, line_); \
				NanoAssertFail(); \
			}
			
		#define _NanoAssertMsgf(a, file_, line_, f, ...)   __NanoAssertMsgf(a, file_, line_, f, __VA_ARGS__)

		#define NanoAssertMsgf(a,f,...)   do { __NanoAssertMsgf(a, __FILE__, __LINE__, f ": ", __VA_ARGS__); } while (0)
		#define NanoAssertMsg(a,m)        do { __NanoAssertMsgf(a, __FILE__, __LINE__, "\"%s\": ", m); } while (0)
		#define NanoAssert(a)             do { __NanoAssertMsgf(a, __FILE__, __LINE__, "%s", ""); } while (0)
	#else
		#define NanoAssertMsgf(a,f,...)   do { } while (0) /* no semi */
		#define NanoAssertMsg(a,m)        do { } while (0) /* no semi */
		#define NanoAssert(a)             do { } while (0) /* no semi */
	#endif

	





	#ifdef __SUNPRO_CC
		#define NanoStaticAssert(condition)
	#else
		#define NanoStaticAssert(condition) \
			extern void nano_static_assert(int arg[(condition) ? 1 : -1])
	#endif


	




}

#ifdef AVMPLUS_VERBOSE
#define NJ_VERBOSE 1
#define NJ_PROFILE 1
#endif

#if defined(_MSC_VER) && _MSC_VER < 1400
	#include <stdio.h>
	#define verbose_output						if (verbose_enabled()) Assembler::output
	#define verbose_outputf						if (verbose_enabled()) Assembler::outputf
	#define verbose_enabled()					(_verbose)
	#define verbose_only(x)						x
#elif defined(NJ_VERBOSE)
	#include <stdio.h>
	#define verbose_output						if (verbose_enabled()) Assembler::output
	#define verbose_outputf						if (verbose_enabled()) Assembler::outputf
	#define verbose_enabled()					(_verbose)
	#define verbose_only(...)					__VA_ARGS__
#else
	#define verbose_output
	#define verbose_outputf
	#define verbose_enabled()
	#define verbose_only(...)
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
#define isS24(i) ( ((int32_t(i)<<8)>>8) == (i) )

#define alignTo(x,s)		((((uintptr_t)(x)))&~(((uintptr_t)s)-1))
#define alignUp(x,s)		((((uintptr_t)(x))+(((uintptr_t)s)-1))&~(((uintptr_t)s)-1))

#define pageTop(x)			( (int*)alignTo(x,NJ_PAGE_SIZE) )
#define pageDataStart(x)    ( (int*)(alignTo(x,NJ_PAGE_SIZE) + sizeof(PageHeader)) )
#define pageBottom(x)		( (int*)(alignTo(x,NJ_PAGE_SIZE)+NJ_PAGE_SIZE)-1 )
#define samepage(x,y)		(pageTop(x) == pageTop(y))

#include "Native.h"
#include "LIR.h"
#include "RegAlloc.h"
#include "Fragmento.h"
#include "Assembler.h"
#include "TraceTreeDrawer.h"

#endif 
#endif 
