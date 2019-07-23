






































#ifndef __nanojit_CodeAlloc__
#define __nanojit_CodeAlloc__

namespace nanojit
{
    
    using namespace MMgc;

    
    inline bool containsPtr(const NIns* start, const NIns* end, const NIns* ptr) {
        return ptr >= start && ptr < end;
    }

    




    class CodeList
    {
        friend class CodeAlloc;

        
        CodeList* next;

        


        CodeList* lower;

        
        bool isFree;
        union {
            
            
            CodeList* higher;   
            NIns* end;          
        };

        

        NIns  code[1]; 

        
        NIns* start() { return &code[0]; }

        
        size_t size() const { return uintptr_t(end) - uintptr_t(&code[0]); }

        
        size_t blockSize() const { return uintptr_t(end) - uintptr_t(this); }

        
        bool contains(NIns* p) const  { return containsPtr(&code[0], end, p); }
    };

    








    class CodeAlloc
    {
        static const size_t sizeofMinBlock = offsetof(CodeList, code);
        static const size_t minAllocSize = LARGEST_UNDERRUN_PROT;

        


        CodeList* heapblocks;

        
        CodeList* availblocks;
        size_t totalAllocated;

        
        static CodeList* removeBlock(CodeList* &list);

        
        static void addBlock(CodeList* &blocks, CodeList* b);

        
        static CodeList* getBlock(NIns* start, NIns* end);

        
        CodeList* addMem(void* mem, size_t bytes);

        
        void sanity_check();

        
        static CodeList* firstBlock(CodeList* term);

        
        
        
        
        

        
        void* allocCodeChunk(size_t nbytes);

        


        void freeCodeChunk(void* addr, size_t nbytes);

    public:
        CodeAlloc();
        ~CodeAlloc();

        
        void alloc(NIns* &start, NIns* &end);

        
        void free(NIns* start, NIns* end);

        
        void freeAll(CodeList* &code);

        
        void flushICache(CodeList* &blocks);

        

        void addRemainder(CodeList* &code, NIns* start, NIns* end, NIns* holeStart, NIns* holeEnd);

        
        static void add(CodeList* &code, NIns* start, NIns* end);

        
        static void moveAll(CodeList* &to, CodeList* &from);

        
        static bool contains(const CodeList* code, NIns* p);

        
        static size_t size(const CodeList* code);

        
        size_t size();

        
        void logStats();

        enum CodePointerKind {
            kUnknown, kFree, kUsed
        };

        
        CodePointerKind classifyPtr(NIns *p);

        
        void sweep();
    };
}

#endif 
