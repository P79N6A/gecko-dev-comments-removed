







































#ifndef __nanojit_RegAlloc__
#define __nanojit_RegAlloc__


namespace nanojit
{
    inline RegisterMask rmask(Register r)
    {
        return RegisterMask(1) << r;
    }

    class RegAlloc
    {
    public:
        RegAlloc()
        {
            clear();
        }

        void clear()
        {
            VMPI_memset(this, 0, sizeof(*this));
        }

        bool isFree(Register r)
        {
            NanoAssert(r != UnknownReg);
            return (free & rmask(r)) != 0;
        }

        void addFree(Register r)
        {
            NanoAssert(!isFree(r));
            free |= rmask(r);
        }

        void addActive(Register r, LIns* v)
        {
            
            NanoAssert(v);
            NanoAssert(r != UnknownReg);
            NanoAssert(active[r] == NULL);
            active[r] = v;
            useActive(r);
        }

        void useActive(Register r)
        {
            NanoAssert(r != UnknownReg);
            NanoAssert(active[r] != NULL);
            usepri[r] = priority++;
        }

        void removeActive(Register r)
        {
            
            NanoAssert(r != UnknownReg);
            NanoAssert(active[r] != NULL);

            
            active[r] = NULL;
        }

        void retire(Register r)
        {
            NanoAssert(r != UnknownReg);
            NanoAssert(active[r] != NULL);
            active[r] = NULL;
            free |= rmask(r);
        }

        int32_t getPriority(Register r) {
            NanoAssert(r != UnknownReg && active[r]);
            return usepri[r];
        }

        LIns* getActive(Register r) {
            NanoAssert(r != UnknownReg);
            return active[r];
        }

        debug_only( uint32_t    countActive(); )
        debug_only( bool        isConsistent(Register r, LIns* v); )
        debug_only( RegisterMask managed; )    

        LIns*           active[LastReg + 1];    
        int32_t         usepri[LastReg + 1];    
        RegisterMask    free;
        int32_t         priority;

        verbose_only( void formatRegisters(char* s, Fragment*); )

        DECLARE_PLATFORM_REGALLOC()
    };
}
#endif 
