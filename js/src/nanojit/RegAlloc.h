







































#ifndef __nanojit_RegAlloc__
#define __nanojit_RegAlloc__


namespace nanojit
{
    inline RegisterMask rmask(Register r)
    {
        return 1 << r;
    }

    class RegAlloc
    {
        public:
            RegAlloc() { clear(); }
            void    clear();
            bool    isFree(Register r);
            void    addFree(Register r);
            void    addActive(Register r, LIns* ins);
            void    useActive(Register r);
            void    removeActive(Register r);
            void    retire(Register r);
            bool    isValid() {
                return (free|used) != 0;
            }

            int32_t getPriority(Register r) {
                NanoAssert(r != UnknownReg && active[r]);
                return usepri[r];
            }

            LIns* getActive(Register r) {
                NanoAssert(r != UnknownReg);
                return active[r];
            }

            debug_only( uint32_t    countFree(); )
            debug_only( uint32_t    countActive(); )
            debug_only( void        checkCount(); )
            debug_only( bool        isConsistent(Register r, LIns* v); )
            debug_only( uint32_t    count; )
            debug_only( RegisterMask managed; )    

            LIns*    active[LastReg + 1];  
            int32_t usepri[LastReg + 1]; 
            RegisterMask    free;
            RegisterMask    used;
            int32_t         priority;

            verbose_only( static void formatRegisters(RegAlloc& regs, char* s, Fragment*); )

            DECLARE_PLATFORM_REGALLOC()
    };
}
#endif 
