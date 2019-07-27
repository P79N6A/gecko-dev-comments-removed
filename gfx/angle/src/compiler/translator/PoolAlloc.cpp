





#include "compiler/translator/PoolAlloc.h"

#include "compiler/translator/InitializeGlobals.h"

#include "common/platform.h"
#include "common/angleutils.h"
#include "common/tls.h"

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

TLSIndex PoolIndex = TLS_INVALID_INDEX;

bool InitializePoolIndex()
{
    assert(PoolIndex == TLS_INVALID_INDEX);

    PoolIndex = CreateTLSIndex();
    return PoolIndex != TLS_INVALID_INDEX;
}

void FreePoolIndex()
{
    assert(PoolIndex != TLS_INVALID_INDEX);

    DestroyTLSIndex(PoolIndex);
    PoolIndex = TLS_INVALID_INDEX;
}

TPoolAllocator* GetGlobalPoolAllocator()
{
    assert(PoolIndex != TLS_INVALID_INDEX);
    return static_cast<TPoolAllocator*>(GetTLSValue(PoolIndex));
}

void SetGlobalPoolAllocator(TPoolAllocator* poolAllocator)
{
    assert(PoolIndex != TLS_INVALID_INDEX);
    SetTLSValue(PoolIndex, poolAllocator);
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

            
#if defined(_MSC_VER)
            snprintf(assertMsg, sizeof(assertMsg), "PoolAlloc: Damage %s %Iu byte allocation at 0x%p\n",
                    locText, size, data());
#else
            snprintf(assertMsg, sizeof(assertMsg), "PoolAlloc: Damage %s %zu byte allocation at 0x%p\n",
                    locText, size, data());
#endif
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
    
    
    
    ++numCalls;
    totalBytes += numBytes;

    
    
    
    
    
    size_t allocationSize = TAllocation::allocationSize(numBytes);
    
    if (allocationSize < numBytes)
        return 0;

    
    
    
    
    if (allocationSize <= pageSize - currentPageOffset) {
        
        
        
        unsigned char* memory = reinterpret_cast<unsigned char *>(inUseList) + currentPageOffset;
        currentPageOffset += allocationSize;
        currentPageOffset = (currentPageOffset + alignmentMask) & ~alignmentMask;

        return initializeAllocation(inUseList, memory, numBytes);
    }

    if (allocationSize > pageSize - headerSkip) {
        
        
        
        
        size_t numBytesToAlloc = allocationSize + headerSkip;
        
        if (numBytesToAlloc < allocationSize)
            return 0;

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
