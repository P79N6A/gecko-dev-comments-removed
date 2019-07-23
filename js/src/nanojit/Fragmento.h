









































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

            NIns*           code()                          { return _code; }
            void            setCode(NIns* codee)            { _code = codee; }
            int32_t&        hits()                          { return _hits; }
            bool            isAnchor() { return anchor == this; }
            bool            isRoot() { return root == this; }

            Fragment*      anchor;
            Fragment*      root;
            Fragment*      parent;
            Fragment*      first;
            Fragment*      peer;
            LirBuffer*     lirbuf;
            LIns*          lastIns;

            TraceKind kind;
            const void* ip;
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
