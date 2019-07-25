








#include "mach_override.h"

#include <mach-o/dyld.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <sys/mman.h>

#include <CoreServices/CoreServices.h>






#pragma mark	-
#pragma mark	(Constants)

#if defined(__ppc__) || defined(__POWERPC__)

long kIslandTemplate[] = {
	0x9001FFFC,	
	0x3C00DEAD,	
	0x6000BEEF,	
	0x7C0903A6,	
	0x8001FFFC,	
	0x60000000,	
	0x4E800420 	
};

#define kAddressHi			3
#define kAddressLo			5
#define kInstructionHi		10
#define kInstructionLo		11

#elif defined(__i386__) 

#define kOriginalInstructionsSize 16

char kIslandTemplate[] = {
	
	
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
	
	0xE9, 0xEF, 0xBE, 0xAD, 0xDE
};

#define kInstructions	0
#define kJumpAddress    kInstructions + kOriginalInstructionsSize + 1
#elif defined(__x86_64__)

#define kOriginalInstructionsSize 32

#define kJumpAddress    kOriginalInstructionsSize + 6

char kIslandTemplate[] = {
	
	
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
	
	0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
};

#endif

#define	kAllocateHigh		1
#define	kAllocateNormal		0






#pragma mark	-
#pragma mark	(Data Types)

typedef	struct	{
	char	instructions[sizeof(kIslandTemplate)];
	int		allocatedHigh;
}	BranchIsland;






#pragma mark	-
#pragma mark	(Funky Protos)

	mach_error_t
allocateBranchIsland(
		BranchIsland	**island,
		int				allocateHigh,
		void *originalFunctionAddress);

	mach_error_t
freeBranchIsland(
		BranchIsland	*island );

#if defined(__ppc__) || defined(__POWERPC__)
	mach_error_t
setBranchIslandTarget(
		BranchIsland	*island,
		const void		*branchTo,
		long			instruction );
#endif 

#if defined(__i386__) || defined(__x86_64__)
mach_error_t
setBranchIslandTarget_i386(
						   BranchIsland	*island,
						   const void		*branchTo,
						   char*			instructions );
void 
atomic_mov64(
		uint64_t *targetAddress,
		uint64_t value );

	static Boolean 
eatKnownInstructions( 
	unsigned char	*code, 
	uint64_t		*newInstruction,
	int				*howManyEaten, 
	char			*originalInstructions,
	int				*originalInstructionCount, 
	uint8_t			*originalInstructionSizes );

	static void
fixupInstructions(
    void		*originalFunction,
    void		*escapeIsland,
    void		*instructionsToFix,
	int			instructionCount,
	uint8_t		*instructionSizes );
#endif






#pragma mark	-
#pragma mark	(Interface)

#if defined(__i386__) || defined(__x86_64__)
mach_error_t makeIslandExecutable(void *address) {
	mach_error_t err = err_none;
    vm_size_t pageSize;
    host_page_size( mach_host_self(), &pageSize );
    uintptr_t page = (uintptr_t)address & ~(uintptr_t)(pageSize-1);
    int e = err_none;
    e |= mprotect((void *)page, pageSize, PROT_EXEC | PROT_READ | PROT_WRITE);
    e |= msync((void *)page, pageSize, MS_INVALIDATE );
    if (e) {
        err = err_cannot_override;
    }
    return err;
}
#endif

    mach_error_t
mach_override_ptr(
	void *originalFunctionAddress,
    const void *overrideFunctionAddress,
    void **originalFunctionReentryIsland )
{
	assert( originalFunctionAddress );
	assert( overrideFunctionAddress );
	
	
	
#if defined(__x86_64__)
    for(;;){
        if(*(uint16_t*)originalFunctionAddress==0x25FF)    
            originalFunctionAddress=*(void**)((char*)originalFunctionAddress+6+*(int32_t *)((uint16_t*)originalFunctionAddress+1));
        else break;
    }
#elif defined(__i386__)
    for(;;){
        if(*(uint16_t*)originalFunctionAddress==0x25FF)    
            originalFunctionAddress=**(void***)((uint16_t*)originalFunctionAddress+1);
        else break;
    }
#endif

	long	*originalFunctionPtr = (long*) originalFunctionAddress;
	mach_error_t	err = err_none;
	
#if defined(__ppc__) || defined(__POWERPC__)
	
	#define	kMFCTRMask			0xfc1fffff
	#define	kMFCTRInstruction	0x7c0903a6
	
	long	originalInstruction = *originalFunctionPtr;
	if( !err && ((originalInstruction & kMFCTRMask) == kMFCTRInstruction) )
		err = err_cannot_override;
#elif defined(__i386__) || defined(__x86_64__)
	int eatenCount = 0;
	int originalInstructionCount = 0;
	char originalInstructions[kOriginalInstructionsSize];
	uint8_t originalInstructionSizes[kOriginalInstructionsSize];
	uint64_t jumpRelativeInstruction = 0; 

	Boolean overridePossible = eatKnownInstructions ((unsigned char *)originalFunctionPtr, 
										&jumpRelativeInstruction, &eatenCount, 
										originalInstructions, &originalInstructionCount, 
										originalInstructionSizes );
	if (eatenCount > kOriginalInstructionsSize) {
		
		overridePossible = false;
	}
	if (!overridePossible) err = err_cannot_override;
	if (err) fprintf(stderr, "err = %x %s:%d\n", err, __FILE__, __LINE__);
#endif
	
	
	if( !err ) {
		err = vm_protect( mach_task_self(),
				(vm_address_t) originalFunctionPtr, 8, false,
				(VM_PROT_ALL | VM_PROT_COPY) );
		if( err )
			err = vm_protect( mach_task_self(),
					(vm_address_t) originalFunctionPtr, 8, false,
					(VM_PROT_DEFAULT | VM_PROT_COPY) );
	}
	if (err) fprintf(stderr, "err = %x %s:%d\n", err, __FILE__, __LINE__);
	
	
	BranchIsland	*escapeIsland = NULL;
	if( !err )	
		err = allocateBranchIsland( &escapeIsland, kAllocateHigh, originalFunctionAddress );
		if (err) fprintf(stderr, "err = %x %s:%d\n", err, __FILE__, __LINE__);

	
#if defined(__ppc__) || defined(__POWERPC__)
	if( !err )
		err = setBranchIslandTarget( escapeIsland, overrideFunctionAddress, 0 );
	
	
	long	branchAbsoluteInstruction = 0; 
	if( !err ) {
		long escapeIslandAddress = ((long) escapeIsland) & 0x3FFFFFF;
		branchAbsoluteInstruction = 0x48000002 | escapeIslandAddress;
	}
#elif defined(__i386__) || defined(__x86_64__)
        if (err) fprintf(stderr, "err = %x %s:%d\n", err, __FILE__, __LINE__);

	if( !err )
		err = setBranchIslandTarget_i386( escapeIsland, overrideFunctionAddress, 0 );
 
	if (err) fprintf(stderr, "err = %x %s:%d\n", err, __FILE__, __LINE__);
	
#endif


#if defined(__i386__) || defined(__x86_64__)
	if (!err) {
		uint32_t addressOffset = ((char*)escapeIsland - (char*)originalFunctionPtr - 5);
		addressOffset = OSSwapInt32(addressOffset);
		
		jumpRelativeInstruction |= 0xE900000000000000LL; 
		jumpRelativeInstruction |= ((uint64_t)addressOffset & 0xffffffff) << 24;
		jumpRelativeInstruction = OSSwapInt64(jumpRelativeInstruction);		
	}
#endif
	
	
	
	
	
	BranchIsland	*reentryIsland = NULL;
	if( !err && originalFunctionReentryIsland ) {
		err = allocateBranchIsland( &reentryIsland, kAllocateHigh, escapeIsland);
		if( !err )
			*originalFunctionReentryIsland = reentryIsland;
	}
	
#if defined(__ppc__) || defined(__POWERPC__)	
	
	
	
	
	
	
	if( !err ) {
		int escapeIslandEngaged = false;
		do {
			if( reentryIsland )
				err = setBranchIslandTarget( reentryIsland,
						(void*) (originalFunctionPtr+1), originalInstruction );
			if( !err ) {
				escapeIslandEngaged = CompareAndSwap( originalInstruction,
										branchAbsoluteInstruction,
										(UInt32*)originalFunctionPtr );
				if( !escapeIslandEngaged ) {
					
					
					
					originalInstruction = *originalFunctionPtr;
					if( (originalInstruction & kMFCTRMask) == kMFCTRInstruction)
						err = err_cannot_override;
				}
			}
		} while( !err && !escapeIslandEngaged );
	}
#elif defined(__i386__) || defined(__x86_64__)
	
	
	
	
	
	
	
	
	if ( !err ) {
		fixupInstructions(originalFunctionPtr, reentryIsland, originalInstructions,
					originalInstructionCount, originalInstructionSizes );
	
		if( reentryIsland )
			err = setBranchIslandTarget_i386( reentryIsland,
										 (void*) ((char *)originalFunctionPtr+eatenCount), originalInstructions );
		
#if defined(__x86_64__) || defined(__i386__)
        if( !err )
            err = makeIslandExecutable(escapeIsland);
        if( !err && reentryIsland )
            err = makeIslandExecutable(reentryIsland);
#endif
		if ( !err )
			atomic_mov64((uint64_t *)originalFunctionPtr, jumpRelativeInstruction);
	}
#endif
	
	
	if( err ) {
		if( reentryIsland )
			freeBranchIsland( reentryIsland );
		if( escapeIsland )
			freeBranchIsland( escapeIsland );
	}

	return err;
}






#pragma mark	-
#pragma mark	(Implementation)












	mach_error_t
allocateBranchIsland(
		BranchIsland	**island,
		int				allocateHigh,
		void *originalFunctionAddress)
{
	assert( island );
	
	mach_error_t	err = err_none;
	
	if( allocateHigh ) {
		vm_size_t pageSize;
		err = host_page_size( mach_host_self(), &pageSize );
		if( !err ) {
			assert( sizeof( BranchIsland ) <= pageSize );
#if defined(__ppc__) || defined(__POWERPC__)
			vm_address_t first = 0xfeffffff;
			vm_address_t last = 0xfe000000 + pageSize;
#elif defined(__x86_64__)
			vm_address_t first = (uint64_t)originalFunctionAddress & ~(uint64_t)(((uint64_t)1 << 31) - 1) | ((uint64_t)1 << 31); 
			vm_address_t last = 0x0;
#else
			vm_address_t first = 0xffc00000;
			vm_address_t last = 0xfffe0000;
#endif

			vm_address_t page = first;
			int allocated = 0;
			vm_map_t task_self = mach_task_self();
			
			while( !err && !allocated && page != last ) {

				err = vm_allocate( task_self, &page, pageSize, 0 );
				if( err == err_none )
					allocated = 1;
				else if( err == KERN_NO_SPACE ) {
#if defined(__x86_64__)
					page -= pageSize;
#else
					page += pageSize;
#endif
					err = err_none;
				}
			}
			if( allocated )
				*island = (BranchIsland*) page;
			else if( !allocated && !err )
				err = KERN_NO_SPACE;
		}
	} else {
		void *block = malloc( sizeof( BranchIsland ) );
		if( block )
			*island = block;
		else
			err = KERN_NO_SPACE;
	}
	if( !err )
		(**island).allocatedHigh = allocateHigh;
	
	return err;
}









	mach_error_t
freeBranchIsland(
		BranchIsland	*island )
{
	assert( island );
	assert( (*(long*)&island->instructions[0]) == kIslandTemplate[0] );
	assert( island->allocatedHigh );
	
	mach_error_t	err = err_none;
	
	if( island->allocatedHigh ) {
		vm_size_t pageSize;
		err = host_page_size( mach_host_self(), &pageSize );
		if( !err ) {
			assert( sizeof( BranchIsland ) <= pageSize );
			err = vm_deallocate(
					mach_task_self(),
					(vm_address_t) island, pageSize );
		}
	} else {
		free( island );
	}
	
	return err;
}












#if defined(__ppc__) || defined(__POWERPC__)
	mach_error_t
setBranchIslandTarget(
		BranchIsland	*island,
		const void		*branchTo,
		long			instruction )
{
	
    bcopy( kIslandTemplate, island->instructions, sizeof( kIslandTemplate ) );
    
    
    ((short*)island->instructions)[kAddressLo] = ((long) branchTo) & 0x0000FFFF;
    ((short*)island->instructions)[kAddressHi]
    	= (((long) branchTo) >> 16) & 0x0000FFFF;
    
    
    if( instruction != 0 ) {
        ((short*)island->instructions)[kInstructionLo]
        	= instruction & 0x0000FFFF;
        ((short*)island->instructions)[kInstructionHi]
        	= (instruction >> 16) & 0x0000FFFF;
    }
    
    
	msync( island->instructions, sizeof( kIslandTemplate ), MS_INVALIDATE );
    
    return err_none;
}
#endif 

#if defined(__i386__)
	mach_error_t
setBranchIslandTarget_i386(
	BranchIsland	*island,
	const void		*branchTo,
	char*			instructions )
{

	
    bcopy( kIslandTemplate, island->instructions, sizeof( kIslandTemplate ) );

	
	if (instructions) {
		bcopy (instructions, island->instructions + kInstructions, kOriginalInstructionsSize);
	}
	
    
    int32_t addressOffset = (char *)branchTo - (island->instructions + kJumpAddress + 4);
    *((int32_t *)(island->instructions + kJumpAddress)) = addressOffset; 

    msync( island->instructions, sizeof( kIslandTemplate ), MS_INVALIDATE );
    return err_none;
}

#elif defined(__x86_64__)
mach_error_t
setBranchIslandTarget_i386(
        BranchIsland	*island,
        const void		*branchTo,
        char*			instructions )
{
    
    bcopy( kIslandTemplate, island->instructions, sizeof( kIslandTemplate ) );

    
    if (instructions) {
        bcopy (instructions, island->instructions, kOriginalInstructionsSize);
    }

    
    *((uint64_t *)(island->instructions + kJumpAddress)) = (uint64_t)branchTo; 
    msync( island->instructions, sizeof( kIslandTemplate ), MS_INVALIDATE );

    return err_none;
}
#endif


#if defined(__i386__) || defined(__x86_64__)

typedef struct {
	unsigned int length; 
	unsigned char mask[15]; 
	unsigned char constraint[15]; 
}	AsmInstructionMatch;

#if defined(__i386__)
static AsmInstructionMatch possibleInstructions[] = {
	{ 0x5, {0xFF, 0x00, 0x00, 0x00, 0x00}, {0xE9, 0x00, 0x00, 0x00, 0x00} },	
	{ 0x5, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0x55, 0x89, 0xe5, 0xc9, 0xc3} },	
	{ 0x1, {0xFF}, {0x90} },							
	{ 0x1, {0xFF}, {0x55} },							
	{ 0x2, {0xFF, 0xFF}, {0x89, 0xE5} },				                
	{ 0x1, {0xFF}, {0x53} },							
	{ 0x3, {0xFF, 0xFF, 0x00}, {0x83, 0xEC, 0x00} },	                        
	{ 0x6, {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00}, {0x81, 0xEC, 0x00, 0x00, 0x00, 0x00} },	
	{ 0x1, {0xFF}, {0x57} },							
	{ 0x1, {0xFF}, {0x56} },							
	{ 0x2, {0xFF, 0xFF}, {0x31, 0xC0} },						
	{ 0x3, {0xFF, 0x4F, 0x00}, {0x8B, 0x45, 0x00} },  
	{ 0x3, {0xFF, 0x4C, 0x00}, {0x8B, 0x40, 0x00} },  
	{ 0x4, {0xFF, 0xFF, 0xFF, 0x00}, {0x8B, 0x4C, 0x24, 0x00} },  
	{ 0x5, {0xFF, 0x00, 0x00, 0x00, 0x00}, {0xB8, 0x00, 0x00, 0x00, 0x00} },	
	{ 0x0 }
};
#elif defined(__x86_64__)
static AsmInstructionMatch possibleInstructions[] = {
	{ 0x5, {0xFF, 0x00, 0x00, 0x00, 0x00}, {0xE9, 0x00, 0x00, 0x00, 0x00} },	
	{ 0x1, {0xFF}, {0x90} },							
	{ 0x1, {0xF8}, {0x50} },							
	{ 0x3, {0xFF, 0xFF, 0xFF}, {0x48, 0x89, 0xE5} },				
	{ 0x4, {0xFF, 0xFF, 0xFF, 0x00}, {0x48, 0x83, 0xEC, 0x00} },	                
	{ 0x4, {0xFB, 0xFF, 0x00, 0x00}, {0x48, 0x89, 0x00, 0x00} },	                
	{ 0x2, {0xFF, 0x00}, {0x41, 0x00} },						
	{ 0x2, {0xFF, 0x00}, {0x85, 0x00} },						
	{ 0x5, {0xF8, 0x00, 0x00, 0x00, 0x00}, {0xB8, 0x00, 0x00, 0x00, 0x00} },   
	{ 0x3, {0xFF, 0xFF, 0x00}, {0xFF, 0x77, 0x00} },  
	{ 0x2, {0xFF, 0xFF}, {0x31, 0xC0} },						
	{ 0x0 }
};
#endif

static Boolean codeMatchesInstruction(unsigned char *code, AsmInstructionMatch* instruction) 
{
	Boolean match = true;
	
	size_t i;
	for (i=0; i<instruction->length; i++) {
		unsigned char mask = instruction->mask[i];
		unsigned char constraint = instruction->constraint[i];
		unsigned char codeValue = code[i];
				
		match = ((codeValue & mask) == constraint);
		if (!match) break;
	}
	
	return match;
}

#if defined(__i386__) || defined(__x86_64__)
	static Boolean 
eatKnownInstructions( 
	unsigned char	*code, 
	uint64_t		*newInstruction,
	int				*howManyEaten, 
	char			*originalInstructions,
	int				*originalInstructionCount, 
	uint8_t			*originalInstructionSizes )
{
	Boolean allInstructionsKnown = true;
	int totalEaten = 0;
	unsigned char* ptr = code;
	int remainsToEat = 5; 
	int instructionIndex = 0;
	
	if (howManyEaten) *howManyEaten = 0;
	if (originalInstructionCount) *originalInstructionCount = 0;
	while (remainsToEat > 0) {
		Boolean curInstructionKnown = false;
		
		
		AsmInstructionMatch* curInstr = possibleInstructions;
		do { 
			if ((curInstructionKnown = codeMatchesInstruction(ptr, curInstr))) break;
			curInstr++;
		} while (curInstr->length > 0);
		
		
		if (!curInstructionKnown) { 
			allInstructionsKnown = false;
			fprintf(stderr, "mach_override: some instructions unknown! Need to update mach_override.c\n");
			break;
		}
		
		
		int eaten = curInstr->length;
		ptr += eaten;
		remainsToEat -= eaten;
		totalEaten += eaten;
		
		if (originalInstructionSizes) originalInstructionSizes[instructionIndex] = eaten;
		instructionIndex += 1;
		if (originalInstructionCount) *originalInstructionCount = instructionIndex;
	}


	if (howManyEaten) *howManyEaten = totalEaten;

	if (originalInstructions) {
		Boolean enoughSpaceForOriginalInstructions = (totalEaten < kOriginalInstructionsSize);
		
		if (enoughSpaceForOriginalInstructions) {
			memset(originalInstructions, 0x90 , kOriginalInstructionsSize); 
			bcopy(code, originalInstructions, totalEaten);
		} else {
			
			return false;
		}
	}
	
	if (allInstructionsKnown) {
		
		uint64_t currentFirst64BitsOfCode = *((uint64_t *)code);
		currentFirst64BitsOfCode = OSSwapInt64(currentFirst64BitsOfCode); 
		currentFirst64BitsOfCode &= 0x0000000000FFFFFFLL; 
		
		
		*newInstruction &= 0xFFFFFFFFFF000000LL; 
		*newInstruction |= (currentFirst64BitsOfCode & 0x0000000000FFFFFFLL); 
	}

	return allInstructionsKnown;
}

	static void
fixupInstructions(
    void		*originalFunction,
    void		*escapeIsland,
    void		*instructionsToFix,
	int			instructionCount,
	uint8_t		*instructionSizes )
{
	int	index;
	for (index = 0;index < instructionCount;index += 1)
	{
		if (*(uint8_t*)instructionsToFix == 0xE9) 
		{
			uint32_t offset = (uintptr_t)originalFunction - (uintptr_t)escapeIsland;
			uint32_t *jumpOffsetPtr = (uint32_t*)((uintptr_t)instructionsToFix + 1);
			*jumpOffsetPtr += offset;
		}
		
		originalFunction = (void*)((uintptr_t)originalFunction + instructionSizes[index]);
		escapeIsland = (void*)((uintptr_t)escapeIsland + instructionSizes[index]);
		instructionsToFix = (void*)((uintptr_t)instructionsToFix + instructionSizes[index]);
    }
}
#endif

#if defined(__i386__)
__asm(
			".text;"
			".align 2, 0x90;"
			"_atomic_mov64:;"
			"	pushl %ebp;"
			"	movl %esp, %ebp;"
			"	pushl %esi;"
			"	pushl %ebx;"
			"	pushl %ecx;"
			"	pushl %eax;"
			"	pushl %edx;"
	
			
			
			
			
			
			
			"	mov		8(%ebp), %esi;"  
			"	mov		12(%ebp), %ebx;"
			"	mov		16(%ebp), %ecx;" 
			"	mov		(%esi), %eax;"
			"	mov		4(%esi), %edx;"  
			"	lock; cmpxchg8b	(%esi);" 
			
			
			"	popl %edx;"
			"	popl %eax;"
			"	popl %ecx;"
			"	popl %ebx;"
			"	popl %esi;"
			"	popl %ebp;"
			"	ret"
);
#elif defined(__x86_64__)
void atomic_mov64(
		uint64_t *targetAddress,
		uint64_t value )
{
    *targetAddress = value;
}
#endif
#endif
