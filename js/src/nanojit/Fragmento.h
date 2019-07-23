









































#ifndef __nanojit_Fragmento__
#define __nanojit_Fragmento__

namespace nanojit
{
    struct GuardRecord;

    enum TraceKind {
        LoopTrace,
        BranchTrace,
        MergeTrace
    };

    






    class Fragment
    {
        public:
            Fragment(const void*);

            NIns*            code()                            { return _code; }
            void            setCode(NIns* codee)               { _code = codee; }
            int32_t&        hits()                             { return _hits; }
            bool            isAnchor() { return anchor == this; }
            bool            isRoot() { return root == this; }

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
            NIns* fragEntry;
            NIns* loopEntry;
            void* vmprivate;

        private:
            NIns*            _code;        
            int32_t          _hits;
    };
}
#endif 
