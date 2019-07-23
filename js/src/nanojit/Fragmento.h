









































#ifndef __nanojit_Fragmento__
#define __nanojit_Fragmento__

namespace nanojit
{
    struct GuardRecord;

    






    class Fragment
    {
        public:
            Fragment(const void*);

            NIns*           code()                          { return _code; }
            void            setCode(NIns* codee)            { _code = codee; }
            int32_t&        hits()                          { return _hits; }
            bool            isRoot() { return root == this; }

            Fragment*      root;
            LirBuffer*     lirbuf;
            LIns*          lastIns;

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
