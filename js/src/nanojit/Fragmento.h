









































#ifndef __nanojit_Fragmento__
#define __nanojit_Fragmento__

namespace nanojit
{
    struct GuardRecord;

    






    class Fragment
    {
        public:
            Fragment(const void*
                     verbose_only(, uint32_t profFragID));

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
            void* vmprivate;

            
            
            verbose_only( LIns*          loopLabel; ) 
            verbose_only( uint32_t       profFragID; )
            verbose_only( uint32_t       profCount; )
            verbose_only( uint32_t       nStaticExits; )
            verbose_only( size_t         nCodeBytes; )
            verbose_only( size_t         nExitBytes; )
            verbose_only( uint32_t       guardNumberer; )
            verbose_only( GuardRecord*   guardsForFrag; )

        private:
            NIns*            _code;        
            int32_t          _hits;
    };
}













































#endif 
