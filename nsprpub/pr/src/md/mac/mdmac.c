




































#include <Types.h>
#include <Timer.h>
#include <Files.h>
#include <Errors.h>
#include <Folders.h>
#include <Gestalt.h>
#include <Events.h>
#include <Processes.h>
#include <TextUtils.h>
#include <MixedMode.h>
#include <LowMem.h>

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stat.h>
#include <stdarg.h>
#include <unix.h>

#include "MacErrorHandling.h"

#include "primpl.h"
#include "prgc.h"

#include "mactime.h"

#include "mdmac.h"


#undef getenv




unsigned char GarbageCollectorCacheFlusher(PRUint32 size);

extern PRThread *gPrimaryThread;
extern ProcessSerialNumber gApplicationProcess;     




#pragma mark -
#pragma mark CREATING MACINTOSH THREAD STACKS


enum {
	uppExitToShellProcInfo 				= kPascalStackBased,
	uppStackSpaceProcInfo				= kRegisterBased 
										  | RESULT_SIZE(SIZE_CODE(sizeof(long)))
										  | REGISTER_RESULT_LOCATION(kRegisterD0)
		 								  | REGISTER_ROUTINE_PARAMETER(1, kRegisterD1, SIZE_CODE(sizeof(UInt16)))
};

typedef CALLBACK_API( long , StackSpacePatchPtr )(UInt16 trapNo);
typedef REGISTER_UPP_TYPE(StackSpacePatchPtr)	StackSpacePatchUPP;

StackSpacePatchUPP	  gStackSpacePatchUPP = NULL;
UniversalProcPtr	  gStackSpacePatchCallThru = NULL;
long				(*gCallOSTrapUniversalProc)(UniversalProcPtr,ProcInfoType,...) = NULL;


pascal long StackSpacePatch(UInt16 trapNo)
{
	char		tos;
	PRThread	*thisThread;
	
	thisThread = PR_GetCurrentThread();
	
	
	
	
	if ((thisThread == gPrimaryThread) || 	
		(&tos < thisThread->stack->stackBottom) || 
		(&tos > thisThread->stack->stackTop)) {
		return gCallOSTrapUniversalProc(gStackSpacePatchCallThru, uppStackSpaceProcInfo, trapNo);
	}
	else {
		return &tos - thisThread->stack->stackBottom;
	}
}


static void InstallStackSpacePatch(void)
{
	long				systemVersion;
	OSErr				err;
	CFragConnectionID	connID;
	Str255				errMessage;
	Ptr					interfaceLibAddr;
	CFragSymbolClass	symClass;
	UniversalProcPtr	(*getOSTrapAddressProc)(UInt16);
	void				(*setOSTrapAddressProc)(StackSpacePatchUPP, UInt16);
	UniversalProcPtr	(*newRoutineDescriptorProc)(ProcPtr,ProcInfoType,ISAType);
	

	err = Gestalt(gestaltSystemVersion,&systemVersion);
	if (systemVersion >= 0x00000A00)	
		return;

	
	err = GetSharedLibrary("\pInterfaceLib", kPowerPCCFragArch, kFindCFrag,
											&connID, &interfaceLibAddr, errMessage);
	PR_ASSERT(err == noErr);
	if (err != noErr)
		return;

	
	err = FindSymbol(connID, "\pGetOSTrapAddress", &(Ptr)getOSTrapAddressProc, &symClass);
	if (err != noErr)
		return;

	
	err = FindSymbol(connID, "\pSetOSTrapAddress", &(Ptr)setOSTrapAddressProc, &symClass);
	if (err != noErr)
		return;
	
	
	err = FindSymbol(connID, "\pNewRoutineDescriptor", &(Ptr)newRoutineDescriptorProc, &symClass);
	if (err != noErr)
		return;
	
	
	err = FindSymbol(connID, "\pCallOSTrapUniversalProc", &(Ptr)gCallOSTrapUniversalProc, &symClass);
	if (err != noErr)
		return;

	
	gStackSpacePatchCallThru = getOSTrapAddressProc(0x0065);
	if (gStackSpacePatchCallThru)
	{
		gStackSpacePatchUPP =
			(StackSpacePatchUPP)newRoutineDescriptorProc((ProcPtr)(StackSpacePatch), uppStackSpaceProcInfo, GetCurrentArchitecture());
		setOSTrapAddressProc(gStackSpacePatchUPP, 0x0065);
	}

#if DEBUG
	StackSpace();
#endif
}




#pragma mark -
#pragma mark ENVIRONMENT VARIABLES


typedef struct EnvVariable EnvVariable;

struct EnvVariable {
	char 			*variable;
	char			*value;
	EnvVariable		*next;
};

EnvVariable		*gEnvironmentVariables = NULL;

char *_MD_GetEnv(const char *name)
{
	EnvVariable 	*currentVariable = gEnvironmentVariables;

	while (currentVariable) {
		if (!strcmp(currentVariable->variable, name))
			return currentVariable->value;
			
		currentVariable = currentVariable->next;
	}

	return getenv(name);
}

PR_IMPLEMENT(int) 
_MD_PutEnv(const char *string)
{
	EnvVariable 	*currentVariable = gEnvironmentVariables;
	char			*variableCopy,
				    *value,
					*current;
					
	variableCopy = strdup(string);
	PR_ASSERT(variableCopy != NULL);

	current = variableCopy;
	while (*current != '=')
		current++;

	*current = 0;
	current++;

	value = current;

	while (currentVariable) {
		if (!strcmp(currentVariable->variable, variableCopy))
			break;
		
		currentVariable = currentVariable->next;
	}

	if (currentVariable == NULL) {
		currentVariable = PR_NEW(EnvVariable);
		
		if (currentVariable == NULL) {
			PR_DELETE(variableCopy);
			return -1;
		}
		
		currentVariable->variable = strdup(variableCopy);
		currentVariable->value = strdup(value);
		currentVariable->next = gEnvironmentVariables;
		gEnvironmentVariables = currentVariable;
	}
	
	else {
		PR_DELETE(currentVariable->value);
		currentVariable->value = strdup(current);

		
		
		
		
		
		
		if (strcmp(currentVariable->variable, "LD_LIBRARY_PATH") == 0)
			PR_SetLibraryPath(currentVariable->value);
	}
	
	PR_DELETE(variableCopy);
	return 0;
}





#pragma mark -
#pragma mark MISCELLANEOUS

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) setjmp(t->md.jb);
    }
    *np = sizeof(t->md.jb) / sizeof(PRUint32);
    return (PRWord*) (t->md.jb);
}

void _MD_GetRegisters(PRUint32 *to)
{
  (void) setjmp((void*) to);
}

void _MD_EarlyInit()
{
	Handle				environmentVariables;

	GetCurrentProcess(&gApplicationProcess);

	INIT_CRITICAL_REGION();
	InitIdleSemaphore();

#if !defined(MAC_NSPR_STANDALONE)
	
#else
	MacintoshInitializeMemory();
#endif
	MacintoshInitializeTime();
	
	
	
	environmentVariables = GetResource('Envi', 128);
	if (environmentVariables != NULL) {
	
		Size 	resourceSize;
		char	*currentPutEnvString = (char *)*environmentVariables,
				*currentScanChar = currentPutEnvString;
				
		resourceSize = GetHandleSize(environmentVariables);			
		DetachResource(environmentVariables);
		HLock(environmentVariables);
		
		while (resourceSize--) {
		
			if ((*currentScanChar == '\n') || (*currentScanChar == '\r')) {
				*currentScanChar = 0;
				_MD_PutEnv (currentPutEnvString);
				currentPutEnvString = currentScanChar + 1;
			}
		
			currentScanChar++;
		
		}
		
		DisposeHandle(environmentVariables);

	}

#ifdef PR_INTERNAL_LOGGING
	_MD_PutEnv ("NSPR_LOG_MODULES=clock:6,cmon:6,io:6,mon:6,linker:6,cvar:6,sched:6,thread:6");
#endif

	InstallStackSpacePatch();
}

void _MD_FinalInit()
{
	_MD_InitNetAccess();
}

void PR_InitMemory(void) {
#ifndef NSPR_AS_SHARED_LIB
	
	
	
	
	MacintoshInitializeMemory();
#endif
}



#pragma mark -
#pragma mark TERMINATION








#if TARGET_CARBON
extern OTClientContextPtr	clientContext;
#define CLOSE_OPEN_TRANSPORT()	CloseOpenTransportInContext(clientContext)

#else

#define CLOSE_OPEN_TRANSPORT()	CloseOpenTransport()
#endif 

extern pascal void __NSTerminate(void);

void CleanupTermProc(void)
{
	_MD_StopInterrupts();	

	CLOSE_OPEN_TRANSPORT();
	TermIdleSemaphore();
	TERM_CRITICAL_REGION();
	
	__NSTerminate();
}





#pragma mark -
#pragma mark STRING OPERATIONS

#if !defined(MAC_NSPR_STANDALONE)






void 
PStrFromCStr(const char* src, Str255 dst)
{
	short 	length  = 0;
	
	
	if ( (void*)src == (void*)dst )
	{
		unsigned char*		curdst = &dst[1];
		unsigned char		thisChar;
				
		thisChar = *(const unsigned char*)src++;
		while ( thisChar != '\0' ) 
		{
			unsigned char	nextChar;
			
			
			nextChar = *(const unsigned char*)src++;
			*curdst++ = thisChar;
			thisChar = nextChar;
			
			if ( ++length >= 255 )
				break;
		}
	}
	else if ( src != NULL )
	{
		unsigned char*		curdst = &dst[1];
		short 				overflow = 255;		
		register char		temp;
	
		
		
		while ( (temp = *src++) != 0 ) 
		{
			*(char*)curdst++ = temp;
				
			if ( --overflow <= 0 )
				break;
		}
		length = 255 - overflow;
	}
	dst[0] = length;
}


void CStrFromPStr(ConstStr255Param pString, char **cString)
{
	
	unsigned int	len;
	
	len = pString[0];
	*cString = malloc(len+1);
	
	if (*cString != NULL) {
		strncpy(*cString, (char *)&pString[1], len);
		(*cString)[len] = NULL;
	}
}


void dprintf(const char *format, ...)
{
#if DEBUG
    va_list ap;
	Str255 buffer;
	
	va_start(ap, format);
	buffer[0] = PR_vsnprintf((char *)buffer + 1, sizeof(buffer) - 1, format, ap);
	va_end(ap);
	
	DebugStr(buffer);
#endif 
}

#else

void debugstr(const char *debuggerMsg)
{
	Str255		pStr;
	
	PStrFromCStr(debuggerMsg, pStr);
	DebugStr(pStr);
}


char *strdup(const char *source)
{
	char 	*newAllocation;
	size_t	stringLength;

	PR_ASSERT(source);
	
	stringLength = strlen(source) + 1;
	
	newAllocation = (char *)PR_MALLOC(stringLength);
	if (newAllocation == NULL)
		return NULL;
	BlockMoveData(source, newAllocation, stringLength);
	return newAllocation;
}






void PStrFromCStr(const char* src, Str255 dst)
{
	short 	length  = 0;
	
	
	if ( (void*)src == (void*)dst )
	{
		unsigned char*		curdst = &dst[1];
		unsigned char		thisChar;
				
		thisChar = *(const unsigned char*)src++;
		while ( thisChar != '\0' ) 
		{
			unsigned char	nextChar;
			
			
			nextChar = *(const unsigned char*)src++;
			*curdst++ = thisChar;
			thisChar = nextChar;
			
			if ( ++length >= 255 )
				break;
		}
	}
	else if ( src != NULL )
	{
		unsigned char*		curdst = &dst[1];
		short 				overflow = 255;		
		register char		temp;
	
		
		
		while ( (temp = *src++) != 0 ) 
		{
			*(char*)curdst++ = temp;
				
			if ( --overflow <= 0 )
				break;
		}
		length = 255 - overflow;
	}
	dst[0] = length;
}


void CStrFromPStr(ConstStr255Param pString, char **cString)
{
	
	unsigned int	len;
	
	len = pString[0];
	*cString = PR_MALLOC(len+1);
	
	if (*cString != NULL) {
		strncpy(*cString, (char *)&pString[1], len);
		(*cString)[len] = NULL;
	}
}


size_t strlen(const char *source)
{
	size_t currentLength = 0;
	
	if (source == NULL)
		return currentLength;
			
	while (*source++ != '\0')
		currentLength++;
		
	return currentLength;
}

int strcmpcore(const char *str1, const char *str2, int caseSensitive)
{
	char 	currentChar1, currentChar2;

	while (1) {
	
		currentChar1 = *str1;
		currentChar2 = *str2;
		
		if (!caseSensitive) {
			
			if ((currentChar1 >= 'a') && (currentChar1 <= 'z'))
				currentChar1 += ('A' - 'a');
		
			if ((currentChar2 >= 'a') && (currentChar2 <= 'z'))
				currentChar2 += ('A' - 'a');
		
		}
	
		if (currentChar1 == '\0')
			break;
	
		if (currentChar1 != currentChar2)
			return currentChar1 - currentChar2;
			
		str1++;
		str2++;
	
	}
	
	return currentChar1 - currentChar2;
}

int strcmp(const char *str1, const char *str2)
{
	return strcmpcore(str1, str2, true);
}

int strcasecmp(const char *str1, const char *str2)
{
	return strcmpcore(str1, str2, false);
}


void *memcpy(void *to, const void *from, size_t size)
{
	if (size != 0) {
#if DEBUG
		if ((UInt32)to < 0x1000)
			DebugStr("\pmemcpy has illegal to argument");
		if ((UInt32)from < 0x1000)
			DebugStr("\pmemcpy has illegal from argument");
#endif
		BlockMoveData(from, to, size);
	}
	return to;
}

void dprintf(const char *format, ...)
{
    va_list ap;
	char	*buffer;
	
	va_start(ap, format);
	buffer = (char *)PR_vsmprintf(format, ap);
	va_end(ap);
	
	debugstr(buffer);
	PR_DELETE(buffer);
}

void
exit(int result)
{
#pragma unused (result)

		ExitToShell();
}

void abort(void)
{
	exit(-1);
}

#endif



#pragma mark -
#pragma mark FLUSHING THE GARBAGE COLLECTOR

#if !defined(MAC_NSPR_STANDALONE)

unsigned char GarbageCollectorCacheFlusher(PRUint32)
{

    PRIntn is;

	UInt32		oldPriority;

	
	
	
	if (PR_GetGCInfo()->lock == NULL)
		return false;

#if DEBUG
	if (_MD_GET_INTSOFF() == 1)
		DebugStr("\pGarbageCollectorCacheFlusher at interrupt time!");
#endif

	
	
	
	
	
	
	

	oldPriority = PR_GetThreadPriority(PR_GetCurrentThread());
	_PR_INTSOFF(is);
	_PR_SetThreadPriority(PR_GetCurrentThread(), (PRThreadPriority)30);
	_PR_INTSON(is);

	
	
	
	
	
	
	PR_GC();
	
	
	
	
	
	
	PR_Yield();

	PR_GC();
	
	
	
	_PR_INTSOFF(is);
	_PR_SetThreadPriority(PR_GetCurrentThread(), (PRThreadPriority)oldPriority);
	_PR_INTSON(is);

	return false;
}

#endif



#pragma mark -
#pragma mark MISCELLANEOUS-HACKS





extern long _MD_GetOSName(char *buf, long count)
{
	long	len;
	
	len = PR_snprintf(buf, count, "Mac OS");
	
	return 0;
}

extern long _MD_GetOSVersion(char *buf, long count)
{
	long	len;
	
	len = PR_snprintf(buf, count, "7.5");

	return 0;
}

extern long _MD_GetArchitecture(char *buf, long count)
{
	long	len;
	
#if defined(TARGET_CPU_PPC) && TARGET_CPU_PPC	
	len = PR_snprintf(buf, count, "PowerPC");
#else
	len = PR_snprintf(buf, count, "Motorola68k");
#endif

	return 0;
}
