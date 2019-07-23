












































#include <new>

#include "nsIMemory.h"





extern nsIMemory* theMemoryAllocator;

void* operator new(size_t size)
{
	if (theMemoryAllocator)
		return theMemoryAllocator->Alloc(size);
	return NULL;
}

void operator delete(void* ptr)
{
	if (ptr && theMemoryAllocator)
		theMemoryAllocator->Free(ptr);
}

void* operator new[](size_t size)
{
	if (theMemoryAllocator)
		return theMemoryAllocator->Alloc(size);
	return NULL;
}

void operator delete[](void* ptr)
{
	if (ptr && theMemoryAllocator)
		theMemoryAllocator->Free(ptr);
}
