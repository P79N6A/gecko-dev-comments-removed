









































#ifndef __nanojit_Fragmento__
#define __nanojit_Fragmento__

namespace nanojit
{
    struct GuardRecord;
    class Assembler;

    typedef avmplus::GCSortedMap<const void*, uint32_t, avmplus::LIST_NonGCObjects> BlockSortedMap;
    class BlockHist: public BlockSortedMap
    {
    public:
        BlockHist(avmplus::GC*gc) : BlockSortedMap(gc)
        {
        }
        uint32_t count(const void *p) {
            uint32_t c = 1+get(p);
            put(p, c);
            return c;
        }
    };

    struct fragstats;
    



    class Fragmento : public avmplus::GCFinalizedObject
    {
        public:
            Fragmento(AvmCore* core, LogControl* logc, uint32_t cacheSizeLog2, CodeAlloc *codeAlloc);
            ~Fragmento();

            AvmCore*    core();

            Fragment*   getLoop(const void* ip);
            Fragment*   getAnchor(const void* ip);
            
            
            
            
            void        clearFrags();    
            Fragment*   createBranch(SideExit *exit, const void* ip);
            Fragment*   newFrag(const void* ip);
            Fragment*   newBranch(Fragment *from, const void* ip);

            verbose_only ( uint32_t pageCount(); )
            verbose_only( void addLabel(Fragment* f, const char *prefix, int id); )

            
            struct
            {
                uint32_t    pages;                    
                uint32_t    flushes, ilsize, abcsize, compiles, totalCompiles;
            }
            _stats;

            verbose_only( DWB(BlockHist*)        enterCounts; )
            verbose_only( DWB(BlockHist*)        mergeCounts; )
            verbose_only( LabelMap*        labels; )

            #ifdef AVMPLUS_VERBOSE
            void    drawTrees(char *fileName);
            #endif

            void        clearFragment(Fragment *f);
        private:
            AvmCore*        _core;
            CodeAlloc*      _codeAlloc;
            FragmentMap     _frags;        

            const uint32_t _max_pages;
            uint32_t _pagesGrowth;
    };

    enum TraceKind {
        LoopTrace,
        BranchTrace,
        MergeTrace
    };

    






    class Fragment : public avmplus::GCFinalizedObject
    {
        public:
            Fragment(const void*);
            ~Fragment();

            NIns*            code()                            { return _code; }
            void            setCode(NIns* codee)               { _code = codee; }
            int32_t&        hits()                             { return _hits; }
            void            blacklist();
            bool            isBlacklisted()        { return _hits < 0; }
            void            releaseLirBuffer();
            void            releaseCode(CodeAlloc *alloc);
            void            releaseTreeMem(CodeAlloc *alloc);
            bool            isAnchor() { return anchor == this; }
            bool            isRoot() { return root == this; }
            void            onDestroy();

            verbose_only( uint32_t        _called; )
            verbose_only( uint32_t        _native; )
            verbose_only( uint32_t      _exitNative; )
            verbose_only( uint32_t        _lir; )
            verbose_only( uint32_t        _lirbytes; )
            verbose_only( const char*    _token; )
            verbose_only( uint64_t      traceTicks; )
            verbose_only( uint64_t      interpTicks; )
            verbose_only( DWB(Fragment*) eot_target; )
            verbose_only( uint32_t        sid;)
            verbose_only( uint32_t        compileNbr;)

            DWB(Fragment*) treeBranches;
            DWB(Fragment*) branches;
            DWB(Fragment*) nextbranch;
            DWB(Fragment*) anchor;
            DWB(Fragment*) root;
            DWB(Fragment*) parent;
            DWB(Fragment*) first;
            DWB(Fragment*) peer;
            LirBuffer*     lirbuf;
            LIns*          lastIns;
            SideExit*      spawnedFrom;
            GuardRecord*   outbound;

            TraceKind kind;
            const void* ip;
            uint32_t guardCount;
            uint32_t xjumpCount;
            uint32_t recordAttempts;
            int32_t blacklistLevel;
            NIns* fragEntry;
            NIns* loopEntry;
            void* vmprivate;
            CodeList* codeList;

        private:
            NIns*            _code;        
            int32_t          _hits;
    };
}
#endif 
