






































#include "nanojit.h"


#include "../vprof/vprof.h"

#ifdef FEATURE_NANOJIT

namespace nanojit
{
    static const bool verbose = false;
#if defined(NANOJIT_ARM)
    
    
    static const int pagesPerAlloc = 1;
#else
    static const int pagesPerAlloc = 16;
#endif
    static const int bytesPerPage = 4096;
    static const int bytesPerAlloc = pagesPerAlloc * bytesPerPage;

    CodeAlloc::CodeAlloc()
        : heapblocks(0)
        , availblocks(0)
        , totalAllocated(0)
    {}

    CodeAlloc::~CodeAlloc() {
        reset();
    }

    void CodeAlloc::reset() {
        
        
        for (CodeList* b = heapblocks; b != 0; ) {
            _nvprof("free page",1);
            CodeList* next = b->next;
            void *mem = firstBlock(b);
            VMPI_setPageProtection(mem, bytesPerAlloc, false , true );
            freeCodeChunk(mem, bytesPerAlloc);
            totalAllocated -= bytesPerAlloc;
            b = next;
        }
        NanoAssert(!totalAllocated);
        heapblocks = availblocks = 0;
    }

    CodeList* CodeAlloc::firstBlock(CodeList* term) {
        
        uintptr_t end = (uintptr_t)alignUp(term, bytesPerPage);
        return (CodeList*) (end - (uintptr_t)bytesPerAlloc);
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
        
        if (availblocks) {
            CodeList* b = removeBlock(availblocks);
            b->isFree = false;
            start = b->start();
            end = b->end;
            if (verbose)
                avmplus::AvmLog("alloc %p-%p %d\n", start, end, int(end-start));
            return;
        }
        
        void *mem = allocCodeChunk(bytesPerAlloc); 
        totalAllocated += bytesPerAlloc;
        NanoAssert(mem != NULL); 
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
        NanoAssert(heapblocks);
        CodeList *blk = getBlock(start, end);
        if (verbose)
            avmplus::AvmLog("free %p-%p %d\n", start, end, (int)blk->size());

        AvmAssert(!blk->isFree);

        
        bool already_on_avail_list;

        if (blk->lower && blk->lower->isFree) {
            
            CodeList* lower = blk->lower;
            CodeList* higher = blk->higher;
            already_on_avail_list = lower->size() >= minAllocSize;
            lower->higher = higher;
            higher->lower = lower;
            blk = lower;
        }
        else
            already_on_avail_list = false;

        
        
        if (blk->higher->isFree) {
            CodeList *higher = blk->higher->higher;
            CodeList *coalescedBlock = blk->higher;

            if ( coalescedBlock->size() >= minAllocSize ) {
                
                if ( availblocks == coalescedBlock ) {
                    removeBlock(availblocks);
                }
                else {
                    CodeList* free_block = availblocks;
                    while ( free_block && free_block->next != coalescedBlock) {
                        NanoAssert(free_block->size() >= minAllocSize);
                        NanoAssert(free_block->isFree);
                        NanoAssert(free_block->next);
                        free_block = free_block->next;
                    }
                    NanoAssert(free_block && free_block->next == coalescedBlock);
                    free_block->next = coalescedBlock->next;
                }
            }

            
            blk->higher = higher;
            higher->lower = blk;
        }
        blk->isFree = true;
        NanoAssert(!blk->lower || !blk->lower->isFree);
        NanoAssert(blk->higher && !blk->higher->isFree);
        
        if ( !already_on_avail_list && blk->size() >= minAllocSize )
            addBlock(availblocks, blk);

        NanoAssert(heapblocks);
        debug_only(sanity_check();)
    }

    void CodeAlloc::sweep() {
        debug_only(sanity_check();)

        
        CodeList** prev = &availblocks;
        for (CodeList* ab = availblocks; ab != 0; ab = *prev) {
            NanoAssert(ab->higher != 0);
            NanoAssert(ab->isFree);
            if (!ab->higher->higher && !ab->lower) {
                *prev = ab->next;
                debug_only(ab->next = 0;)
            } else {
                prev = &ab->next;
            }
        }

        
        prev = &heapblocks;
        for (CodeList* hb = heapblocks; hb != 0; hb = *prev) {
            NanoAssert(hb->lower != 0);
            if (!hb->lower->lower && hb->lower->isFree) {
                NanoAssert(!hb->lower->next);
                
                void* mem = hb->lower;
                *prev = hb->next;
                _nvprof("free page",1);
                VMPI_setPageProtection(mem, bytesPerAlloc, false , true );
                freeCodeChunk(mem, bytesPerAlloc);
                totalAllocated -= bytesPerAlloc;
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

#if defined NANOJIT_ARM && defined UNDER_CE
    
    
    void CodeAlloc::flushICache(CodeList* &) {
        FlushInstructionCache(GetCurrentProcess(), NULL, NULL);
    }
#else
    void CodeAlloc::flushICache(CodeList* &blocks) {
        for (CodeList *b = blocks; b != 0; b = b->next)
            flushICache(b->start(), b->size());
    }
#endif

#if defined(AVMPLUS_UNIX) && defined(NANOJIT_ARM)
#include <asm/unistd.h>
extern "C" void __clear_cache(char *BEG, char *END);
#endif

#ifdef AVMPLUS_SPARC
#ifdef __linux__  
void sync_instruction_memory(caddr_t v, u_int len)
{
	caddr_t end = v + len;
	caddr_t p = v;
	while (p < end) {
		asm("flush %0" : : "r" (p));
		p += 32;
	}
}
#else
extern  "C" void sync_instruction_memory(caddr_t v, u_int len);
#endif
#endif

#if defined NANOJIT_IA32 || defined NANOJIT_X64
    
    void CodeAlloc::flushICache(void *start, size_t len) {
        
        
        (void)start;
        (void)len;
        VALGRIND_DISCARD_TRANSLATIONS(start, len);
    }

#elif defined NANOJIT_ARM && defined UNDER_CE
    
    
    
    void CodeAlloc::flushICache(void *, size_t) {
        FlushInstructionCache(GetCurrentProcess(), NULL, NULL);
    }

#elif defined AVMPLUS_MAC && defined NANOJIT_PPC

#  ifdef NANOJIT_64BIT
    extern "C" void sys_icache_invalidate(const void*, size_t len);
    extern "C" void sys_dcache_flush(const void*, size_t len);

    
    void CodeAlloc::flushICache(void *start, size_t len) {
        sys_dcache_flush(start, len);
        sys_icache_invalidate(start, len);
    }
#  else
    
    
    
    void CodeAlloc::flushICache(void *start, size_t len) {
        MakeDataExecutable(start, len);
    }
#  endif

#elif defined AVMPLUS_SPARC
    
    void CodeAlloc::flushICache(void *start, size_t len) {
            sync_instruction_memory((char*)start, len);
    }

#elif defined AVMPLUS_UNIX
    #ifdef ANDROID
    void CodeAlloc::flushICache(void *start, size_t len) {
        cacheflush((int)start, (int)start + len, 0);
    }
	#else
    
    void CodeAlloc::flushICache(void *start, size_t len) {
        __clear_cache((char*)start, (char*)start + len);
    }
	#endif
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
        NanoAssert(b);
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

    size_t CodeAlloc::size() {
        return totalAllocated;
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
        for (CodeList* avail = this->availblocks; avail; avail = avail->next) {
            NanoAssert(avail->isFree && avail->size() >= minAllocSize);
        }

        #if CROSS_CHECK_FREE_LIST
        for(CodeList* term = heapblocks; term; term = term->next) {
            for(CodeList* hb = term->lower; hb; hb = hb->lower) {
                if (hb->isFree && hb->size() >= minAllocSize) {
                    bool found_on_avail = false;
                    for (CodeList* avail = this->availblocks; !found_on_avail && avail; avail = avail->next) {
                        found_on_avail = avail == hb;
                    }

                    NanoAssert(found_on_avail);
                }
            }
        }
        for (CodeList* avail = this->availblocks; avail; avail = avail->next) {
            bool found_in_heapblocks = false;
            for(CodeList* term = heapblocks; !found_in_heapblocks && term; term = term->next) {
                for(CodeList* hb = term->lower; !found_in_heapblocks && hb; hb = hb->lower) {
                    found_in_heapblocks = hb == avail;
                }
            }
            NanoAssert(found_in_heapblocks);
        }
        #endif 
    }
    #endif
}
#endif 
