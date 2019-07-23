






































#include "nanojit.h"


#include "../vprof/vprof.h"

#ifdef FEATURE_NANOJIT

namespace nanojit
{
    static const bool verbose = false;
    static const int pagesPerAlloc = 1;
    static const int bytesPerAlloc = pagesPerAlloc * GCHeap::kBlockSize;

    CodeAlloc::CodeAlloc(GCHeap* heap)
        : heap(heap), heapblocks(0)
    {}

    CodeAlloc::~CodeAlloc() {
        
        
        for (CodeList* b = heapblocks; b != 0; ) {
            _nvprof("free page",1);
            CodeList* next = b->next;
            void *mem = firstBlock(b);
            VMPI_setPageProtection(mem, bytesPerAlloc, false , true );
            heap->Free(mem);
            b = next;
        }
    }

    CodeList* CodeAlloc::firstBlock(CodeList* term) {
        
        return (CodeList*) alignTo(term, bytesPerAlloc);
    }

    int round(size_t x) {
        return (int)((x + 512) >> 10);
    }
    void CodeAlloc::logStats() {
        size_t total = 0;
        size_t frag_size = 0;
        size_t free_size = 0;
        int free_count = 0;
        for (CodeList* hb = heapblocks; hb != 0; hb = hb->next) {
            total += bytesPerAlloc;
            for (CodeList* b = hb->lower; b != 0; b = b->lower) {
                if (b->isFree) {
                    free_count++;
                    free_size += b->blockSize();
                    if (b->size() < minAllocSize)
                        frag_size += b->blockSize();
                }
            }
        }
        avmplus::AvmLog("code-heap: %dk free %dk fragmented %d\n",
            round(total), round(free_size), frag_size);
    }

    void CodeAlloc::alloc(NIns* &start, NIns* &end) {
        
        for (CodeList* hb = heapblocks; hb != 0; hb = hb->next) {
            
            for (CodeList* b = hb->lower; b != 0; b = b->lower) {
                if (b->isFree && b->size() >= minAllocSize) {
                    
                    b->isFree = false;
                    start = b->start();
                    end = b->end;
                    if (verbose)
                        avmplus::AvmLog("alloc %p-%p %d\n", start, end, int(end-start));
                    return;
                }
            }
        }
        
        void *mem = heap->Alloc(pagesPerAlloc);  
        _nvprof("alloc page", uintptr_t(mem)>>12);
        VMPI_setPageProtection(mem, bytesPerAlloc, true, true);
        CodeList* b = addMem(mem, bytesPerAlloc);
        b->isFree = false;
        start = b->start();
        end = b->end;
        if (verbose)
            avmplus::AvmLog("alloc %p-%p %d\n", start, end, int(end-start));
    }

    void CodeAlloc::free(NIns* start, NIns *end) {
        CodeList *blk = getBlock(start, end);
        if (verbose)
            avmplus::AvmLog("free %p-%p %d\n", start, end, (int)blk->size());

        AvmAssert(!blk->isFree);

        
        if (blk->lower && blk->lower->isFree) {
            
            CodeList* lower = blk->lower;
            CodeList* higher = blk->higher;
            lower->higher = higher;
            higher->lower = lower;
            debug_only( sanity_check();)
            blk = lower;
        }
        
        if (blk->higher->isFree) {
            
            CodeList *higher = blk->higher->higher;
            blk->higher = higher;
            higher->lower = blk;
            debug_only(sanity_check();)
        }
        blk->isFree = true;
        NanoAssert(!blk->lower || !blk->lower->isFree);
        NanoAssert(blk->higher && !blk->higher->isFree);
        
    }

    void CodeAlloc::sweep() {
        debug_only(sanity_check();)
        CodeList** prev = &heapblocks;
        for (CodeList* hb = heapblocks; hb != 0; hb = *prev) {
            NanoAssert(hb->lower != 0);
            if (!hb->lower->lower && hb->lower->isFree) {
                
                void* mem = hb->lower;
                *prev = hb->next;
                _nvprof("free page",1);
                VMPI_setPageProtection(mem, bytesPerAlloc, false , true );
                heap->Free(mem);
            } else {
                prev = &hb->next;
            }
        }
    }

    void CodeAlloc::freeAll(CodeList* &code) {
        while (code) {
            CodeList *b = removeBlock(code);
            free(b->start(), b->end);
        }
    }

#if defined(AVMPLUS_UNIX) && defined(NANOJIT_ARM)
#include <asm/unistd.h>
extern "C" void __clear_cache(char *BEG, char *END);
#endif

#ifdef AVMPLUS_SPARC
extern  "C"    void sync_instruction_memory(caddr_t v, u_int len);
#endif

#if defined NANOJIT_IA32 || defined NANOJIT_X64
    
    void CodeAlloc::flushICache(CodeList* &)
    {}

#elif defined NANOJIT_ARM && defined UNDER_CE
    
    
    void CodeAlloc::flushICache(CodeList* &) {
        
        FlushInstructionCache(GetCurrentProcess(), NULL, NULL);
    }

#elif defined AVMPLUS_MAC && defined NANOJIT_PPC

#  ifdef NANOJIT_64BIT
    extern "C" void sys_icache_invalidate(const void*, size_t len);
    extern "C" void sys_dcache_flush(const void*, size_t len);

    
    void CodeAlloc::flushICache(CodeList* &blocks) {
        for (CodeList *b = blocks; b != 0; b = b->next) {
            void *start = b->start();
            size_t bytes = b->size();
            sys_dcache_flush(start, bytes);
            sys_icache_invalidate(start, bytes);
        }
    }
#  else
    
    
    
    void CodeAlloc::flushICache(CodeList* &blocks) {
        for (CodeList *b = blocks; b != 0; b = b->next)
            MakeDataExecutable(b->start(), b->size());
    }
#  endif

#elif defined AVMPLUS_SPARC
    
    void CodeAlloc::flushICache(CodeList* &blocks) {
        for (CodeList *b = blocks; b != 0; b = b->next)
            sync_instruction_memory((char*)b->start(), b->size());
    }

#elif defined AVMPLUS_UNIX
    
    void CodeAlloc::flushICache(CodeList* &blocks) {
        for (CodeList *b = blocks; b != 0; b = b->next) {
            __clear_cache((char*)b->start(), (char*)b->start()+b->size());
        }
    }
#endif 

    void CodeAlloc::addBlock(CodeList* &blocks, CodeList* b) {
        b->next = blocks;
        blocks = b;
    }

    CodeList* CodeAlloc::addMem(void *mem, size_t bytes) {
        CodeList* b = (CodeList*)mem;
        b->lower = 0;
        b->end = (NIns*) (uintptr_t(mem) + bytes - sizeofMinBlock);
        b->next = 0;
        b->isFree = true;

        
        
        CodeList* terminator = b->higher;
        terminator->lower = b;
        terminator->end = 0; 
        terminator->isFree = false;
        debug_only(sanity_check();)

        
        addBlock(heapblocks, terminator);
        return b;
    }

    CodeList* CodeAlloc::getBlock(NIns* start, NIns* end) {
        CodeList* b = (CodeList*) (uintptr_t(start) - offsetof(CodeList, code));
        NanoAssert(b->end == end && b->next == 0); (void) end;
        return b;
    }

    CodeList* CodeAlloc::removeBlock(CodeList* &blocks) {
        CodeList* b = blocks;
        blocks = b->next;
        b->next = 0;
        return b;
    }

    void CodeAlloc::add(CodeList* &blocks, NIns* start, NIns* end) {
        addBlock(blocks, getBlock(start, end));
    }

    



    void CodeAlloc::addRemainder(CodeList* &blocks, NIns* start, NIns* end, NIns* holeStart, NIns* holeEnd) {
        NanoAssert(start < end && start <= holeStart && holeStart <= holeEnd && holeEnd <= end);
        
        holeStart = (NIns*) ((uintptr_t(holeStart) + sizeof(NIns*)-1) & ~(sizeof(NIns*)-1));
        holeEnd = (NIns*) (uintptr_t(holeEnd) & ~(sizeof(NIns*)-1));
        size_t minHole = minAllocSize;
        if (minHole < 2*sizeofMinBlock)
            minHole = 2*sizeofMinBlock;
        if (uintptr_t(holeEnd) - uintptr_t(holeStart) < minHole) {
            
            
            add(blocks, start, end);
        } else if (holeStart == start && holeEnd == end) {
            
            this->free(start, end);
        } else if (holeStart == start) {
            
            
            CodeList* b1 = getBlock(start, end);
            CodeList* b2 = (CodeList*) (uintptr_t(holeEnd) - offsetof(CodeList, code));
            b2->isFree = false;
            b2->next = 0;
            b2->higher = b1->higher;
            b2->lower = b1;
            b2->higher->lower = b2;
            b1->higher = b2;
            debug_only(sanity_check();)
            this->free(b1->start(), b1->end);
            addBlock(blocks, b2);
        } else if (holeEnd == end) {
            
            
            NanoAssert(false);
        } else {
            
            CodeList* b1 = getBlock(start, end);
            CodeList* b2 = (CodeList*) holeStart;
            CodeList* b3 = (CodeList*) (uintptr_t(holeEnd) - offsetof(CodeList, code));
            b1->higher = b2;
            b2->lower = b1;
            b2->higher = b3;
            b2->isFree = false; 
            b3->lower = b2;
            b3->end = end;
            b3->isFree = false;
            b3->higher->lower = b3;
            b2->next = 0;
            b3->next = 0;
            debug_only(sanity_check();)
            this->free(b2->start(), b2->end);
            addBlock(blocks, b3);
            addBlock(blocks, b1);
        }
    }

    size_t CodeAlloc::size(const CodeList* blocks) {
        size_t size = 0;
        for (const CodeList* b = blocks; b != 0; b = b->next)
            size += int((uintptr_t)b->end - (uintptr_t)b);
        return size;
    }

    bool CodeAlloc::contains(const CodeList* blocks, NIns* p) {
        for (const CodeList *b = blocks; b != 0; b = b->next) {
            _nvprof("block contains",1);
            if (b->contains(p))
                return true;
        }
        return false;
    }

    void CodeAlloc::moveAll(CodeList* &blocks, CodeList* &other) {
        if (other) {
            CodeList* last = other;
            while (last->next)
                last = last->next;
            last->next = blocks;
            blocks = other;
            other = 0;
        }
    }

    
    
    CodeAlloc::CodePointerKind CodeAlloc::classifyPtr(NIns *p) {
        for (CodeList* hb = heapblocks; hb != 0; hb = hb->next) {
            CodeList* b = firstBlock(hb);
            if (!containsPtr((NIns*)b, (NIns*)((uintptr_t)b + bytesPerAlloc), p))
                continue;
            do {
                if (b->contains(p))
                    return b->isFree ? kFree : kUsed;
            } while ((b = b->higher) != 0);
        }
        return kUnknown;
    }

    
    #ifdef _DEBUG
    void CodeAlloc::sanity_check() {
        for (CodeList* hb = heapblocks; hb != 0; hb = hb->next) {
            NanoAssert(hb->higher == 0);
            for (CodeList* b = hb->lower; b != 0; b = b->lower) {
                NanoAssert(b->higher->lower == b);
            }
        }
    }
    #endif
}
#endif 
