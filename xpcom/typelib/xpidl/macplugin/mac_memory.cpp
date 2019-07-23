







































#include <new.h>
#include <stdlib.h>

#include <Files.h>
#include <Memory.h>

#include "DropInCompilerLinker.h"
#include "CompilerMapping.h"
#include "CWPluginErrors.h"

extern CWPluginContext gPluginContext;








const Boolean kTemporaryAllocation = false;

void* operator new(size_t size)
{
	void* ptr = NULL;
	if (CWAllocateMemory(gPluginContext, size, kTemporaryAllocation, &ptr) == cwNoErr)
		return ptr;
	return NULL;
}

void operator delete(void* ptr)
{
	if (ptr != NULL)
		CWFreeMemory(gPluginContext, ptr, kTemporaryAllocation);
}

void* operator new[] (size_t size)
{
	void* ptr = NULL;
	if (CWAllocateMemory(gPluginContext, size, kTemporaryAllocation, &ptr) == cwNoErr)
		return ptr;
	return NULL;
}

void operator delete[](void* ptr)
{
	if (ptr != NULL)
		CWFreeMemory(gPluginContext, ptr, kTemporaryAllocation);
}

namespace std {

#define TRACK_ALLOCATION
#define kTrackedCookie 'TRKD'

void* malloc(size_t size)
{
#if defined(TRACK_ALLOCATION)
	OSType* ptr = (OSType*) new char[sizeof(OSType) + size];
	if (ptr != NULL)
		*ptr++ = kTrackedCookie;
	return ptr;
#else
	return new char[size];
#endif
}

void free(void *ptr)
{
#if defined(TRACK_ALLOCATION)
	OSType* type = (OSType*)ptr;
	if (*--type == kTrackedCookie)
		delete[] (char*) type;
	else
		DebugStr("\pillegal block passed to free.");
#else
	delete[] (char*) ptr;
#endif
}

void* calloc(size_t nmemb, size_t size)
{
	size *= nmemb;
	void* ptr = malloc(size);
	if (ptr != NULL) {
		BlockZero(ptr, size);
	}
	return ptr;
}

void* realloc(void * ptr, size_t size)
{
	void* newptr = NULL;

	if (size > 0)
		newptr = malloc(size);

	if (ptr != NULL && newptr != NULL)
		BlockMoveData(ptr, newptr, size);

	if (ptr != NULL)
		free(ptr);

	return newptr;
}

}
