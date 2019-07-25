







































#ifndef __nanojit_RegAlloc__
#define __nanojit_RegAlloc__


namespace nanojit
{
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

        bool isFree(Register r) const
        {
            NanoAssert(r != deprecated_UnknownReg);
            return (free & rmask(r)) != 0;
        }

        void addFree(Register r)
        {
            NanoAssert(!isFree(r));
            free |= rmask(r);
        }

        void removeFree(Register r)
        {
            NanoAssert(isFree(r));
            free &= ~rmask(r);
        }

        void addActive(Register r, LIns* v)
        {
            
            NanoAssert(v);
            NanoAssert(r != deprecated_UnknownReg);
            NanoAssert(active[r] == NULL);
            active[r] = v;
            useActive(r);
        }

        void useActive(Register r)
        {
            NanoAssert(r != deprecated_UnknownReg);
            NanoAssert(active[r] != NULL);
            usepri[r] = priority++;
        }

        void removeActive(Register r)
        {
            
            NanoAssert(r != deprecated_UnknownReg);
            NanoAssert(active[r] != NULL);

            
            active[r] = NULL;
        }

        void retire(Register r)
        {
            NanoAssert(r != deprecated_UnknownReg);
            NanoAssert(active[r] != NULL);
            active[r] = NULL;
            free |= rmask(r);
        }

        int32_t getPriority(Register r) {
            NanoAssert(r != deprecated_UnknownReg && active[r]);
            return usepri[r];
        }

        LIns* getActive(Register r) const {
            NanoAssert(r != deprecated_UnknownReg);
            return active[r];
        }

        
        
        RegisterMask activeMask() const {
            return ~free & managed;
        }

        debug_only( bool        isConsistent(Register r, LIns* v) const; )

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        LIns*           active[LastReg + 1];    
        int32_t         usepri[LastReg + 1];    
        RegisterMask    free;       
        RegisterMask    managed;    
        int32_t         priority;

        DECLARE_PLATFORM_REGALLOC()
    };

    
    inline Register lsReg(RegisterMask mask) {
        
        
        if (sizeof(RegisterMask) == 4)
            return (Register) lsbSet32(mask);
        else
            return (Register) lsbSet64(mask);
    }

    
    inline Register msReg(RegisterMask mask) {
        
        
        if (sizeof(RegisterMask) == 4)
            return (Register) msbSet32(mask);
        else
            return (Register) msbSet64(mask);
    }

    
    inline Register nextLsReg(RegisterMask& mask, Register r) {
        return lsReg(mask &= ~rmask(r));
    }

    
    inline Register nextMsReg(RegisterMask& mask, Register r) {
        return msReg(mask &= ~rmask(r));
    }
}
#endif
