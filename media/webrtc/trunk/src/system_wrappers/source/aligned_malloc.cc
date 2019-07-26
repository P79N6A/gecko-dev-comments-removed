









#include "aligned_malloc.h"

#include <assert.h>
#include <memory.h>

#ifdef WEBRTC_ANDROID
#include <stdlib.h>
#endif

#if WEBRTC_MAC
  #include <malloc/malloc.h>
#else
  #include <malloc.h>
#endif

#if _WIN32
    #include <windows.h>
#else
    #include <stdint.h>
#endif

#ifdef WEBRTC_GONK
#include <string.h>
#endif

#include "typedefs.h"




namespace webrtc
{



struct AlignedMemory
{
  void* alignedBuffer;
  void* memoryPointer;
};

void* AlignedMalloc(size_t size, size_t alignment)
{
    if(alignment == 0)
    {
        
        return NULL;
    }
    
    if(alignment & (alignment - 1))
    {
        return NULL;
    }

    AlignedMemory* returnValue = new AlignedMemory();
    if(returnValue == NULL)
    {
        return NULL;
    }

    
    
    
    
    returnValue->memoryPointer = malloc(size + sizeof(uintptr_t) +
                                        alignment - 1);
    if(returnValue->memoryPointer == NULL)
    {
        delete returnValue;
        return NULL;
    }

    
    
    uintptr_t alignStartPos = (uintptr_t)returnValue->memoryPointer;
    alignStartPos += sizeof(uintptr_t);

    
    
    uintptr_t alignedPos = (alignStartPos + alignment - 1) & ~(alignment - 1);

    
    returnValue->alignedBuffer = (void*)alignedPos;

    
    
    uintptr_t headerPos = alignedPos;
    headerPos -= sizeof(uintptr_t);
    void* headerPtr = (void*) headerPos;
    uintptr_t headerValue = (uintptr_t)returnValue;
    memcpy(headerPtr,&headerValue,sizeof(uintptr_t));

    return returnValue->alignedBuffer;
}

void AlignedFree(void* memBlock)
{
    if(memBlock == NULL)
    {
        return;
    }
    uintptr_t alignedPos = (uintptr_t)memBlock;
    uintptr_t headerPos = alignedPos - sizeof(uintptr_t);

    
    uintptr_t* headerPtr = (uintptr_t*)headerPos;
    AlignedMemory* deleteMemory = (AlignedMemory*) *headerPtr;

    if(deleteMemory->memoryPointer != NULL)
    {
        free(deleteMemory->memoryPointer);
    }
    delete deleteMemory;
}
} 
