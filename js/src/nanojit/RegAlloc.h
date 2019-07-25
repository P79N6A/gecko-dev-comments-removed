







































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
            NanoAssert(active[REGNUM(r)] == NULL);
            active[REGNUM(r)] = v;
            useActive(r);
        }

        void useActive(Register r)
        {
            NanoAssert(r != deprecated_UnknownReg);
            NanoAssert(active[REGNUM(r)] != NULL);
            usepri[REGNUM(r)] = priority++;
        }

        void removeActive(Register r)
        {
            
            NanoAssert(r != deprecated_UnknownReg);
            NanoAssert(active[REGNUM(r)] != NULL);

            
            active[REGNUM(r)] = NULL;
        }

        void retire(Register r)
        {
            NanoAssert(r != deprecated_UnknownReg);
            NanoAssert(active[REGNUM(r)] != NULL);
            active[REGNUM(r)] = NULL;
            free |= rmask(r);
        }

        int32_t getPriority(Register r) {
            NanoAssert(r != deprecated_UnknownReg && active[REGNUM(r)]);
            return usepri[REGNUM(r)];
        }

        LIns* getActive(Register r) const {
            NanoAssert(r != deprecated_UnknownReg);
            return active[REGNUM(r)];
        }

        
        
        RegisterMask activeMask() const {
            return ~free & managed;
        }

        debug_only( bool        isConsistent(Register r, LIns* v) const; )

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        LIns*           active[LastRegNum + 1]; 
        int32_t         usepri[LastRegNum + 1]; 
        RegisterMask    free;       
        RegisterMask    managed;    
        int32_t         priority;

        DECLARE_PLATFORM_REGALLOC()
    };

    
    inline Register lsReg(RegisterMask mask) {
        
        
        Register r = { (sizeof(RegisterMask) == 4) ? lsbSet32(mask) : lsbSet64(mask) };
        return r;
    }

    
    inline Register msReg(RegisterMask mask) {
        
        
        Register r = { (sizeof(RegisterMask) == 4) ? msbSet32(mask) : msbSet64(mask) };
        return r;
    }

    
    inline Register nextLsReg(RegisterMask& mask, Register r) {
        return lsReg(mask &= ~rmask(r));
    }

    
    inline Register nextMsReg(RegisterMask& mask, Register r) {
        return msReg(mask &= ~rmask(r));
    }
}
#endif
