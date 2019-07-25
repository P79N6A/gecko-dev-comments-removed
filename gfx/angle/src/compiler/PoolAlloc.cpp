





#include "compiler/PoolAlloc.h"

#ifndef _MSC_VER
#include <stdint.h>
#endif
#include <stdio.h>

#include "compiler/InitializeGlobals.h"
#include "compiler/osinclude.h"

OS_TLSIndex PoolIndex = OS_INVALID_TLS_INDEX;

void InitializeGlobalPools()
{
    TThreadGlobalPools* globalPools= static_cast<TThreadGlobalPools*>(OS_GetTLSValue(PoolIndex));    
    if (globalPools)
        return;

    TThreadGlobalPools* threadData = new TThreadGlobalPools();
    threadData->globalPoolAllocator = 0;

    OS_SetTLSValue(PoolIndex, threadData);
}

void FreeGlobalPools()
{
    
    TThreadGlobalPools* globalPools= static_cast<TThreadGlobalPools*>(OS_GetTLSValue(PoolIndex));    
    if (!globalPools)
        return;
 
    delete globalPools;
}

bool InitializePoolIndex()
{
    
    if ((PoolIndex = OS_AllocTLSIndex()) == OS_INVALID_TLS_INDEX)
        return false;

    return true;
}

void FreePoolIndex()
{
    
    OS_FreeTLSIndex(PoolIndex);
}

TPoolAllocator& GetGlobalPoolAllocator()
{
    TThreadGlobalPools* threadData = static_cast<TThreadGlobalPools*>(OS_GetTLSValue(PoolIndex));

    return *threadData->globalPoolAllocator;
}

void SetGlobalPoolAllocator(TPoolAllocator* poolAllocator)
{
    TThreadGlobalPools* threadData = static_cast<TThreadGlobalPools*>(OS_GetTLSValue(PoolIndex));

    threadData->globalPoolAllocator = poolAllocator;
}





TPoolAllocator::TPoolAllocator(int growthIncrement, int allocationAlignment) : 
    pageSize(growthIncrement),
    alignment(allocationAlignment),
    freeList(0),
    inUseList(0),
    numCalls(0),
    totalBytes(0)
{
    
    
    
    
    if (pageSize < 4*1024)
        pageSize = 4*1024;

    
    
    
    
    currentPageOffset = pageSize;

    
    
    
    
    size_t minAlign = sizeof(void*);
    alignment &= ~(minAlign - 1);
    if (alignment < minAlign)
        alignment = minAlign;
    size_t a = 1;
    while (a < alignment)
        a <<= 1;
    alignment = a;
    alignmentMask = a - 1;

    
    
    
    headerSkip = minAlign;
    if (headerSkip < sizeof(tHeader)) {
        headerSkip = (sizeof(tHeader) + alignmentMask) & ~alignmentMask;
    }
}

TPoolAllocator::~TPoolAllocator()
{
    while (inUseList) {
        tHeader* next = inUseList->nextPage;
        inUseList->~tHeader();
        delete [] reinterpret_cast<char*>(inUseList);
        inUseList = next;
    }

    
    
    
    
    while (freeList) {
        tHeader* next = freeList->nextPage;
        delete [] reinterpret_cast<char*>(freeList);
        freeList = next;
    }
}


const unsigned char TAllocation::guardBlockBeginVal = 0xfb;
const unsigned char TAllocation::guardBlockEndVal   = 0xfe;
const unsigned char TAllocation::userDataFill       = 0xcd;

#ifdef GUARD_BLOCKS
    const size_t TAllocation::guardBlockSize = 16;
#else
    const size_t TAllocation::guardBlockSize = 0;
#endif




void TAllocation::checkGuardBlock(unsigned char* blockMem, unsigned char val, const char* locText) const
{
#ifdef GUARD_BLOCKS
    for (size_t x = 0; x < guardBlockSize; x++) {
        if (blockMem[x] != val) {
            char assertMsg[80];

            
            sprintf(assertMsg, "PoolAlloc: Damage %s %lu byte allocation at 0x%p\n",
                    locText, size, data());
            assert(0 && "PoolAlloc: Damage in guard block");
        }
    }
#endif
}


void TPoolAllocator::push()
{
    tAllocState state = { currentPageOffset, inUseList };

    stack.push_back(state);
        
    
    
    
    currentPageOffset = pageSize;
}








void TPoolAllocator::pop()
{
    if (stack.size() < 1)
        return;

    tHeader* page = stack.back().page;
    currentPageOffset = stack.back().offset;

    while (inUseList != page) {
        
        inUseList->~tHeader();
        
        tHeader* nextInUse = inUseList->nextPage;
        if (inUseList->pageCount > 1)
            delete [] reinterpret_cast<char*>(inUseList);
        else {
            inUseList->nextPage = freeList;
            freeList = inUseList;
        }
        inUseList = nextInUse;
    }

    stack.pop_back();
}





void TPoolAllocator::popAll()
{
    while (stack.size() > 0)
        pop();
}

void* TPoolAllocator::allocate(size_t numBytes)
{
    
    
    
    
    
    size_t allocationSize = TAllocation::allocationSize(numBytes);
    
    
    
    
    ++numCalls;
    totalBytes += numBytes;

    
    
    
    
    if (currentPageOffset + allocationSize <= pageSize) {
        
        
        
        unsigned char* memory = reinterpret_cast<unsigned char *>(inUseList) + currentPageOffset;
        currentPageOffset += allocationSize;
        currentPageOffset = (currentPageOffset + alignmentMask) & ~alignmentMask;

        return initializeAllocation(inUseList, memory, numBytes);
    }

    if (allocationSize + headerSkip > pageSize) {
        
        
        
        
        size_t numBytesToAlloc = allocationSize + headerSkip;
        tHeader* memory = reinterpret_cast<tHeader*>(::new char[numBytesToAlloc]);
        if (memory == 0)
            return 0;

        
        new(memory) tHeader(inUseList, (numBytesToAlloc + pageSize - 1) / pageSize);
        inUseList = memory;

        currentPageOffset = pageSize;  

        
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(memory) + headerSkip);
    }

    
    
    
    tHeader* memory;
    if (freeList) {
        memory = freeList;
        freeList = freeList->nextPage;
    } else {
        memory = reinterpret_cast<tHeader*>(::new char[pageSize]);
        if (memory == 0)
            return 0;
    }

    
    new(memory) tHeader(inUseList, 1);
    inUseList = memory;
    
    unsigned char* ret = reinterpret_cast<unsigned char *>(inUseList) + headerSkip;
    currentPageOffset = (headerSkip + allocationSize + alignmentMask) & ~alignmentMask;

    return initializeAllocation(inUseList, ret, numBytes);
}





void TAllocation::checkAllocList() const
{
    for (const TAllocation* alloc = this; alloc != 0; alloc = alloc->prevAlloc)
        alloc->check();
}
