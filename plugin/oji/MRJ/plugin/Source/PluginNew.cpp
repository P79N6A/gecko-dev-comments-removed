











































#include <new.h>

#include "jni.h"
#include "nsIMemory.h"





extern nsIMemory* theMemoryAllocator;

void* operator new(size_t size)
{
	if (theMemoryAllocator != NULL)
		return theMemoryAllocator->Alloc(size);
	return NULL;
}

void operator delete(void* ptr)
{
	if (ptr != NULL && theMemoryAllocator != NULL)
		theMemoryAllocator->Free(ptr);
}
