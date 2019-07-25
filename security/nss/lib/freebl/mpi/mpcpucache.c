




































#include "mpi.h"

















#if defined(i386) || defined(__i386) || defined(__X86__) || defined (_M_IX86) || defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64)

#include "string.h"

#if defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64)
#define AMD_64 1
#endif


#if defined(AMD_64)

#if defined(__GNUC__)

void freebl_cpuid(unsigned long op, unsigned long *eax, 
	                 unsigned long *ebx, unsigned long *ecx, 
                         unsigned long *edx)
{
	__asm__("cpuid\n\t"
		: "=a" (*eax),
		  "=b" (*ebx),
		  "=c" (*ecx),
		  "=d" (*edx)
		: "0" (op));
}

#elif defined(_MSC_VER)

#include <intrin.h>

void freebl_cpuid(unsigned long op, unsigned long *eax, 
           unsigned long *ebx, unsigned long *ecx, 
           unsigned long *edx)
{
    int intrinsic_out[4];

    __cpuid(intrinsic_out, op);
    *eax = intrinsic_out[0];
    *ebx = intrinsic_out[1];
    *ecx = intrinsic_out[2];
    *edx = intrinsic_out[3];
}

#endif

#else 



#if defined(__GNUC__)
void freebl_cpuid(unsigned long op, unsigned long *eax, 
	                 unsigned long *ebx, unsigned long *ecx, 
                         unsigned long *edx)
{


	__asm__("pushl %%ebx\n\t"
		  "cpuid\n\t"
		  "mov %%ebx,%1\n\t"
		  "popl %%ebx\n\t"
		: "=a" (*eax),
		  "=r" (*ebx),
		  "=c" (*ecx),
		  "=d" (*edx)
		: "0" (op));
}




static unsigned long changeFlag(unsigned long flag)
{
	unsigned long changedFlags, originalFlags;
	__asm__("pushfl\n\t"            
	        "popl %0\n\t"
	        "movl %0,%1\n\t"	
	        "xorl %2,%0\n\t" 	
		"pushl %0\n\t"  	
	        "popfl\n\t"
		"pushfl\n\t"		
		"popl %0\n\t"
		"pushl %1\n\t"		
		 "popfl\n\t"
		: "=r" (changedFlags),
		  "=r" (originalFlags),
		  "=r" (flag)
		: "2" (flag));
	return changedFlags ^ originalFlags;
}

#elif defined(_MSC_VER)




#define wcpuid __asm __emit 0fh __asm __emit 0a2h
void freebl_cpuid(unsigned long op,    unsigned long *Reax, 
    unsigned long *Rebx, unsigned long *Recx, unsigned long *Redx)
{
        unsigned long  Leax, Lebx, Lecx, Ledx;
        __asm {
        pushad
        mov     eax,op
        wcpuid
        mov     Leax,eax
        mov     Lebx,ebx
        mov     Lecx,ecx
        mov     Ledx,edx
        popad
        }
        *Reax = Leax;
        *Rebx = Lebx;
        *Recx = Lecx;
        *Redx = Ledx;
}

static unsigned long changeFlag(unsigned long flag)
{
	unsigned long changedFlags, originalFlags;
	__asm {
		push eax
		push ebx
		pushfd 	                
	        pop  eax
		push eax		
	        mov  originalFlags,eax  
		mov  ebx,flag
	        xor  eax,ebx            
		push eax                
	        popfd
		pushfd                  
		pop  eax	
		popfd                   
		mov changedFlags,eax
		pop ebx
		pop eax
	}
	return changedFlags ^ originalFlags;
}
#endif

#endif

#if !defined(AMD_64)
#define AC_FLAG 0x40000
#define ID_FLAG 0x200000


static int is386()
{
    return changeFlag(AC_FLAG) == 0;
}


static int is486()
{
    return changeFlag(ID_FLAG) == 0;
}
#endif







typedef unsigned char CacheTypeEntry;

typedef enum {
    Cache_NONE    = 0,
    Cache_UNKNOWN = 1,
    Cache_TLB     = 2,
    Cache_TLBi    = 3,
    Cache_TLBd    = 4,
    Cache_Trace   = 5,
    Cache_L1      = 6,
    Cache_L1i     = 7,
    Cache_L1d     = 8,
    Cache_L2      = 9 ,
    Cache_L2i     = 10 ,
    Cache_L2d     = 11 ,
    Cache_L3      = 12 ,
    Cache_L3i     = 13,
    Cache_L3d     = 14
} CacheType;

struct _cache {
    CacheTypeEntry type;
    unsigned char lineSize;
};
static const struct _cache CacheMap[256] = {
 {Cache_NONE,    0   },
 {Cache_TLBi,    0   },
 {Cache_TLBi,    0   },
 {Cache_TLBd,    0   },
 {Cache_TLBd,        },
 {Cache_UNKNOWN, 0   },
 {Cache_L1i,     32  },
 {Cache_UNKNOWN, 0   },
 {Cache_L1i,     32  },
 {Cache_UNKNOWN, 0   },
 {Cache_L1d,     32  },
 {Cache_UNKNOWN, 0   },
 {Cache_L1d,     32  },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_L3,      64  },
 {Cache_L3,      64  },
 {Cache_UNKNOWN, 0   },
 {Cache_L3,      64  },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_L3,      64  },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_L1d,     64  },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_L1i,     64  },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_L2,      64  },
 {Cache_UNKNOWN, 0   },
 {Cache_L2,      64  },
 {Cache_L2,      64  },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_L2,      0   },
 {Cache_L2,      32  },
 {Cache_L2,      32  },
 {Cache_L2,      32  },
 {Cache_L2,      32  },
 {Cache_L2,      32  },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_TLBi,    0   },
 {Cache_TLBi,    0   },
 {Cache_TLBi,    0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_TLBd,    0   },
 {Cache_TLBd,    0   },
 {Cache_TLBd,    0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_L1d,     64  },
 {Cache_L1d,     64  },
 {Cache_L1d,     64  },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_Trace,   1   },
 {Cache_Trace,   1   },
 {Cache_Trace,   1   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_L2,      64  },
 {Cache_L2,      64  },
 {Cache_L2,      64  },
 {Cache_L2,      64  },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_L2,      32  },
 {Cache_L2,      32  },
 {Cache_L2,      32  },
 {Cache_L2,      32  },
 {Cache_L2,      64  },
 {Cache_L2,      64  },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_TLBi,    0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_TLBd,    0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   },
 {Cache_UNKNOWN, 0   }
};





static void
getIntelCacheEntryLineSize(unsigned long val, int *level, 
						unsigned long *lineSize)
{
    CacheType type;

    type = CacheMap[val].type;
    
    


    if (CacheMap[val].lineSize == 0) {
	return;
    }
    


    if ((type == Cache_L1)|| (type == Cache_L1d)) {
	*level = 1;
	*lineSize = CacheMap[val].lineSize;
    } else if ((*level >= 2) && ((type == Cache_L2) || (type == Cache_L2d))) {
	*level = 2;
	*lineSize = CacheMap[val].lineSize;
    } else if ((*level >= 3) && ((type == Cache_L3) || (type == Cache_L3d))) {
	*level = 3;
	*lineSize = CacheMap[val].lineSize;
    }
    return;
}


static void
getIntelRegisterCacheLineSize(unsigned long val, 
			int *level, unsigned long *lineSize)
{
    getIntelCacheEntryLineSize(val >> 24 & 0xff, level, lineSize);
    getIntelCacheEntryLineSize(val >> 16 & 0xff, level, lineSize);
    getIntelCacheEntryLineSize(val >> 8 & 0xff, level, lineSize);
    getIntelCacheEntryLineSize(val & 0xff, level, lineSize);
}





static unsigned long
getIntelCacheLineSize(int cpuidLevel)
{
    int level = 4;
    unsigned long lineSize = 0;
    unsigned long eax, ebx, ecx, edx;
    int repeat, count;

    if (cpuidLevel < 2) {
	return 0;
    }

    









    freebl_cpuid(2, &eax, &ebx, &ecx, &edx);
    repeat = eax & 0xf;
    for (count = 0; count < repeat; count++) {
	if ((eax & 0x80000000) == 0) {
	    getIntelRegisterCacheLineSize(eax & 0xffffff00, &level, &lineSize);
	}
	if ((ebx & 0x80000000) == 0) {
	    getIntelRegisterCacheLineSize(ebx, &level, &lineSize);
	}
	if ((ecx & 0x80000000) == 0) {
	    getIntelRegisterCacheLineSize(ecx, &level, &lineSize);
	}
	if ((edx & 0x80000000) == 0) {
	    getIntelRegisterCacheLineSize(edx, &level, &lineSize);
	}
	if (count+1 != repeat) {
	    freebl_cpuid(2, &eax, &ebx, &ecx, &edx);
	}
    }
    return lineSize;
}








static unsigned long
getOtherCacheLineSize(unsigned long cpuidLevel)
{
    unsigned long lineSize = 0;
    unsigned long eax, ebx, ecx, edx;

    
    freebl_cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    cpuidLevel = eax;

    if (cpuidLevel >= 0x80000005) {
	freebl_cpuid(0x80000005, &eax, &ebx, &ecx, &edx);
	lineSize = ecx & 0xff; 
    }
    return lineSize;
}

static const char * const manMap[] = {
#define INTEL     0
    "GenuineIntel",
#define AMD       1
    "AuthenticAMD",
#define CYRIX     2
    "CyrixInstead",
#define CENTAUR   2
    "CentaurHauls",
#define NEXGEN    3
    "NexGenDriven",
#define TRANSMETA 4
    "GenuineTMx86",
#define RISE      5
    "RiseRiseRise",
#define UMC       6
    "UMC UMC UMC ",
#define SIS       7
    "Sis Sis Sis ",
#define NATIONAL  8
    "Geode by NSC",
};

static const int n_manufacturers = sizeof(manMap)/sizeof(manMap[0]);


#define MAN_UNKNOWN 9

#if !defined(AMD_64)
#define SSE2_FLAG (1<<26)
unsigned long
s_mpi_is_sse2()
{
    unsigned long eax, ebx, ecx, edx;
    int manufacturer = MAN_UNKNOWN;
    int i;
    char string[13];

    if (is386() || is486()) {
	return 0;
    }
    freebl_cpuid(0, &eax, &ebx, &ecx, &edx);
    



    *(int *)string = ebx;
    *(int *)&string[4] = (int)edx;
    *(int *)&string[8] = (int)ecx;
    string[12] = 0;

    
    if (eax == 0) {
	return 0;
    }

    for (i=0; i < n_manufacturers; i++) {
	if ( strcmp(manMap[i],string) == 0) {
	    manufacturer = i;
	    break;
	}
    }

    freebl_cpuid(1,&eax,&ebx,&ecx,&edx);
    return (edx & SSE2_FLAG) == SSE2_FLAG;
}
#endif

unsigned long
s_mpi_getProcessorLineSize()
{
    unsigned long eax, ebx, ecx, edx;
    unsigned long cpuidLevel;
    unsigned long cacheLineSize = 0;
    int manufacturer = MAN_UNKNOWN;
    int i;
    char string[65];

#if !defined(AMD_64)
    if (is386()) {
	return 0; 
    } if (is486()) {
	return 32; 
    }
#endif

    
    freebl_cpuid(0, &eax, &ebx, &ecx, &edx);
    cpuidLevel = eax;
    



    *(int *)string = ebx;
    *(int *)&string[4] = (int)edx;
    *(int *)&string[8] = (int)ecx;
    string[12] = 0;

    manufacturer = MAN_UNKNOWN;
    for (i=0; i < n_manufacturers; i++) {
	if ( strcmp(manMap[i],string) == 0) {
	    manufacturer = i;
	}
    }

    if (manufacturer == INTEL) {
	cacheLineSize = getIntelCacheLineSize(cpuidLevel);
    } else {
	cacheLineSize = getOtherCacheLineSize(cpuidLevel);
    }
    



    if (cacheLineSize == 0) {
	cacheLineSize = 32;
    }
    return cacheLineSize;
}
#define MPI_GET_PROCESSOR_LINE_SIZE_DEFINED 1
#endif

#if defined(__ppc64__) 















#include "memory.h"


static inline void dcbzl(char *array)
{
	register char *a asm("r2") = array;
	__asm__ __volatile__( "dcbzl %0,r0" : "=r" (a): "0"(a) );
}


#define PPC_DO_ALIGN(x,y) ((char *)\
			((((long long) (x))+((y)-1))&~((y)-1)))

#define PPC_MAX_LINE_SIZE 256
unsigned long
s_mpi_getProcessorLineSize()
{
    char testArray[2*PPC_MAX_LINE_SIZE+1];
    char *test;
    int i;

    

    test = PPC_DO_ALIGN(testArray, PPC_MAX_LINE_SIZE); 
    
    memset(test, 0xff, PPC_MAX_LINE_SIZE);
    
    dcbzl(test);

    
    for (i=PPC_MAX_LINE_SIZE; i != 0; i = i/2) {
	if (test[i-1] == 0) {
	    return i;
	}
    }
    return 0;
}

#define MPI_GET_PROCESSOR_LINE_SIZE_DEFINED 1
#endif


















#if defined(MPI_CACHE_LINE_SIZE) && !defined(MPI_GET_PROCESSOR_LINE_SIZE_DEFINED)

unsigned long
s_mpi_getProcessorLineSize()
{
   return MPI_CACHE_LINE_SIZE;
}
#define MPI_GET_PROCESSOR_LINE_SIZE_DEFINED 1
#endif




 
#ifndef MPI_GET_PROCESSOR_LINE_SIZE_DEFINED
unsigned long
s_mpi_getProcessorLineSize()
{
   return 32;
}
#endif

#ifdef TEST_IT
#include <stdio.h>

main()
{
    printf("line size = %d\n", s_mpi_getProcessorLineSize());
} 
#endif
