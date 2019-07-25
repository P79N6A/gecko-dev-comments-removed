






































#ifndef __nanojit_CodeAlloc__
#define __nanojit_CodeAlloc__

namespace nanojit
{
    




    class CodeList
    {
        friend class CodeAlloc;

        
        CodeList* next;

        


        CodeList* lower;

        
        CodeList* terminator;

        
        bool isFree;

        

        bool isExec;

        union {
            
            
            CodeList* higher;   
            NIns* end;          
        };

        

        NIns  code[1]; 

        
        NIns* start() { return &code[0]; }

        
        size_t size() const { return uintptr_t(end) - uintptr_t(&code[0]); }

        
        size_t blockSize() const { return uintptr_t(end) - uintptr_t(this); }

    public:
        
        bool isInBlock(NIns* n) { return (n >= this->start() && n < this->end); }
    };

    



















    class CodeAlloc
    {
        static const size_t sizeofMinBlock = offsetof(CodeList, code);
        static const size_t minAllocSize = LARGEST_UNDERRUN_PROT;

        
        static size_t headerSpaceFor(uint32_t nbrBlks)  { return nbrBlks * sizeofMinBlock; }

        
        static size_t blkSpaceFor(uint32_t nbrBlks)     { return (nbrBlks * minAllocSize) + headerSpaceFor(nbrBlks); }

        


        CodeList* heapblocks;

        
        CodeList* availblocks;
        size_t totalAllocated;

        
        const size_t bytesPerPage;

        
        const size_t bytesPerAlloc;

        
        static CodeList* removeBlock(CodeList* &list);

        
        static void addBlock(CodeList* &blocks, CodeList* b);

        
        static CodeList* getBlock(NIns* start, NIns* end);

        
        void addMem();

        
        void sanity_check();

        
        CodeList* firstBlock(CodeList* term);

        
        
        
        
        

        
        void* allocCodeChunk(size_t nbytes);

        




        void freeCodeChunk(void* addr, size_t nbytes);

        
        void markCodeChunkExec(void* addr, size_t nbytes);

        
        void markCodeChunkWrite(void* addr, size_t nbytes);

    public:
        CodeAlloc();
        ~CodeAlloc();

        
        void reset();

        
        void alloc(NIns* &start, NIns* &end, size_t byteLimit);

        
        void free(NIns* start, NIns* end);

        
        void freeAll(CodeList* &code);

        
        static void flushICache(CodeList* &blocks);

        
        static void flushICache(void *start, size_t len);

        

        void addRemainder(CodeList* &code, NIns* start, NIns* end, NIns* holeStart, NIns* holeEnd);

        
        static void add(CodeList* &code, NIns* start, NIns* end);

        
#ifdef PERFM
        static size_t size(const CodeList* code);
#endif

        
        size_t size();

        
        void logStats();

        
        void markAllExec();

        
        void markExec(CodeList* &blocks);

        
        void markChunkExec(CodeList* term);

        
        void markBlockWrite(CodeList* b);
    };
}

#endif 
