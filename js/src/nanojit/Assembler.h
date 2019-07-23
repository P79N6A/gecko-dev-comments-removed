







































#ifndef __nanojit_Assembler__
#define __nanojit_Assembler__


namespace nanojit
{
    

















    #define STACK_GRANULARITY        sizeof(void *)

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    class AR
    {
    private:
        uint32_t        _highWaterMark;                 
        LIns*           _entries[ NJ_MAX_STACK_ENTRY ]; 


        #ifdef _DEBUG
        static LIns* const BAD_ENTRY;
        #endif

        bool isEmptyRange(uint32_t start, uint32_t nStackSlots) const;
        static uint32_t nStackSlotsFor(LIns* ins);

    public:
        AR();

        uint32_t stackSlotsNeeded() const;

        void clear();
        void freeEntryAt(uint32_t i);
        uint32_t reserveEntry(LIns* ins); 

        #ifdef _DEBUG
        void validateQuick();
        void validateFull();
        void validate();
        bool isValidEntry(uint32_t idx, LIns* ins) const; 
        void checkForResourceConsistency(const RegAlloc& regs);
        void checkForResourceLeaks() const;
        #endif

        class Iter
        {
        private:
            const AR& _ar;
            uint32_t _i;
        public:
            inline Iter(const AR& ar) : _ar(ar), _i(1) { }
            bool next(LIns*& ins, uint32_t& nStackSlots, int32_t& offset);             
        };
    };

    inline AR::AR()
    {
         _entries[0] = NULL;
         clear();
    }

    inline  uint32_t AR::nStackSlotsFor(LIns* ins)
    {
        uint32_t n = 0;
        if (ins->isop(LIR_alloc)) {
            n = ins->size() >> 2;
        } else {
            switch (ins->retType()) {
            case LTy_I32:   n = 1;          break;
            CASE64(LTy_I64:)
            case LTy_F64:   n = 2;          break;
            case LTy_Void:  NanoAssert(0);  break;
            default:        NanoAssert(0);  break;
            }
        }
        return n;
    }

    inline uint32_t AR::stackSlotsNeeded() const
    {
        
        return _highWaterMark+1;
    }

    #ifndef AVMPLUS_ALIGN16
        #ifdef AVMPLUS_WIN32
            #define AVMPLUS_ALIGN16(type) __declspec(align(16)) type
        #else
            #define AVMPLUS_ALIGN16(type) type __attribute__ ((aligned (16)))
        #endif
    #endif

    struct Stats
    {
        counter_define(steals;)
        counter_define(remats;)
        counter_define(spills;)
        counter_define(native;)
        counter_define(exitnative;)

        int32_t pages;
        NIns* codeStart;
        NIns* codeExitStart;

        DECLARE_PLATFORM_STATS()
#ifdef __GNUC__
        
        bool pad[4];
#endif
    };

    
    enum AssmError
    {
         None = 0
        ,StackFull
        ,UnknownBranch
        ,ConditionalBranchTooFar
    };

    typedef SeqBuilder<NIns*> NInsList;
    typedef HashMap<NIns*, LIns*> NInsMap;
#if NJ_USES_QUAD_CONSTANTS
    typedef HashMap<uint64_t, uint64_t*> QuadConstantMap;
#endif

#ifdef VTUNE
    class avmplus::CodegenLIR;
#endif

    class LabelState
    {
    public:
        RegAlloc regs;
        NIns *addr;
        LabelState(NIns *a, RegAlloc &r) : regs(r), addr(a)
        {}
    };

    class LabelStateMap
    {
        Allocator& alloc;
        HashMap<LIns*, LabelState*> labels;
    public:
        LabelStateMap(Allocator& alloc) : alloc(alloc), labels(alloc)
        {}

        void clear() { labels.clear(); }
        void add(LIns *label, NIns *addr, RegAlloc &regs);
        LabelState *get(LIns *);
    };

    

    typedef HashMap<SideExit*, RegAlloc*> RegAllocMap;

    






    class Assembler
    {
        friend class VerboseBlockReader;
            #ifdef NJ_VERBOSE
        public:
            
            StringList* _outputCache;

            
            
            void outputf(const char* format, ...);

        private:
            
            
            LogControl* _logc;

            
            
            
            static char  outline[8192];
            
            
            static char  outlineEOL[512];

            
            
            
            void output();

            
            void setOutputForEOL(const char* format, ...);

            void printRegState();
            void printActivationState();
            #endif 

        public:
            #ifdef VTUNE
            avmplus::CodegenLIR *cgen;
            #endif

            Assembler(CodeAlloc& codeAlloc, Allocator& dataAlloc, Allocator& alloc, AvmCore* core, LogControl* logc, const Config& config);

            void        compile(Fragment *frag, Allocator& alloc, bool optimize
                                verbose_only(, LabelMap*));

            void        endAssembly(Fragment* frag);
            void        assemble(Fragment* frag, LirFilter* reader);
            void        beginAssembly(Fragment *frag);

            void        releaseRegisters();
            void        patch(GuardRecord *lr);
            void        patch(SideExit *exit);
#ifdef NANOJIT_IA32
            void        patch(SideExit *exit, SwitchInfo* si);
#endif
            AssmError   error()    { return _err; }
            void        setError(AssmError e) { _err = e; }

            void        reset();

            debug_only ( void       pageValidate(); )

            
            debug_only( void        resourceConsistencyCheck(); )
            debug_only( void        registerConsistencyCheck(); )

            CodeList*   codeList;                   

        private:
            Stats       _stats;

            void        gen(LirFilter* toCompile);
            NIns*       genPrologue();
            NIns*       genEpilogue();

            uint32_t    arReserve(LIns* ins);
            void        arFree(LIns* ins);
            void        arReset();

            Register    registerAlloc(LIns* ins, RegisterMask allow, RegisterMask prefer);
            Register    registerAllocTmp(RegisterMask allow);
            void        registerResetAll();
            void        evictAllActiveRegs();
            void        evictSomeActiveRegs(RegisterMask regs);
            void        evictScratchRegs();
            void        intersectRegisterState(RegAlloc& saved);
            void        unionRegisterState(RegAlloc& saved);
            void        assignSaved(RegAlloc &saved, RegisterMask skip);
            LInsp       findVictim(RegisterMask allow);

            Register    getBaseReg(LIns *i, int &d, RegisterMask allow);
            void        getBaseReg2(RegisterMask allowValue, LIns* value, Register& rv,
                                    RegisterMask allowBase, LIns* base, Register& rb, int &d);
#if NJ_USES_QUAD_CONSTANTS
            const uint64_t*
                        findQuadConstant(uint64_t q);
#endif
            int         findMemFor(LIns* i);
            Register    findRegFor(LIns* i, RegisterMask allow);
            void        findRegFor2(RegisterMask allowa, LIns* ia, Register &ra,
                                    RegisterMask allowb, LIns *ib, Register &rb);
            Register    findSpecificRegFor(LIns* i, Register r);
            Register    findSpecificRegForUnallocated(LIns* i, Register r);
            Register    deprecated_prepResultReg(LIns *i, RegisterMask allow);
            Register    prepareResultReg(LIns *i, RegisterMask allow);
            void        deprecated_freeRsrcOf(LIns *i, bool pop);
            void        freeResourcesOf(LIns *ins);
            void        evictIfActive(Register r);
            void        evict(LIns* vic);
            RegisterMask hint(LIns* ins);   

            void        codeAlloc(NIns *&start, NIns *&end, NIns *&eip
                                  verbose_only(, size_t &nBytes));
            bool        canRemat(LIns*);

            bool isKnownReg(Register r) {
                return r != deprecated_UnknownReg;
            }

            Allocator&          alloc;              
            CodeAlloc&          _codeAlloc;         
            Allocator&          _dataAlloc;         
            Fragment*           _thisfrag;
            RegAllocMap         _branchStateMap;
            NInsMap             _patches;
            LabelStateMap       _labels;
        #if NJ_USES_QUAD_CONSTANTS
            QuadConstantMap     _quadConstants;
        #endif

            
            
            
            
            
            
            
            
            
            bool        _inExit, vpad2[3];
            NIns        *codeStart, *codeEnd;   
            NIns        *exitStart, *exitEnd;   
            NIns*       _nIns;                  
            NIns*       _nExitIns;              
                                                
        #ifdef NJ_VERBOSE
            size_t      codeBytes;              
            size_t      exitBytes;              
        #endif

            #define     SWAP(t, a, b)   do { t tmp = a; a = b; b = tmp; } while (0)
            void        swapCodeChunks();

            NIns*       _epilogue;
            AssmError   _err;           
        #if PEDANTIC
            NIns*       pedanticTop;
        #endif

            AR          _activation;
            RegAlloc    _allocator;

            verbose_only( void asm_inc_m32(uint32_t*); )
            void        asm_mmq(Register rd, int dd, Register rs, int ds);
            NIns*       asm_exit(LInsp guard);
            NIns*       asm_leave_trace(LInsp guard);
            void        asm_store32(LOpcode op, LIns *val, int d, LIns *base);
            void        asm_store64(LOpcode op, LIns *val, int d, LIns *base);
            void        asm_restore(LInsp, Register);
            void        asm_spilli(LInsp i, bool pop);
            void        asm_spill(Register rr, int d, bool pop, bool quad);
            void        asm_load64(LInsp i);
            void        asm_ret(LInsp p);
            void        asm_quad(LInsp i);
            void        asm_fcond(LInsp i);
            void        asm_cond(LInsp i);
            void        asm_arith(LInsp i);
            void        asm_neg_not(LInsp i);
            void        asm_load32(LInsp i);
            void        asm_cmov(LInsp i);
            void        asm_param(LInsp i);
            void        asm_int(LInsp i);
#if NJ_SOFTFLOAT_SUPPORTED
            void        asm_qlo(LInsp i);
            void        asm_qhi(LInsp i);
            void        asm_qjoin(LIns *ins);
#endif
            void        asm_fneg(LInsp ins);
            void        asm_fop(LInsp ins);
            void        asm_i2f(LInsp ins);
            void        asm_u2f(LInsp ins);
            void        asm_f2i(LInsp ins);
#ifdef NANOJIT_64BIT
            void        asm_q2i(LInsp ins);
            void        asm_promote(LIns *ins);
#endif
            void        asm_nongp_copy(Register r, Register s);
            void        asm_call(LInsp);
            Register    asm_binop_rhs_reg(LInsp ins);
            NIns*       asm_branch(bool branchOnFalse, LInsp cond, NIns* targ);
            void        asm_switch(LIns* ins, NIns* target);
            void        asm_jtbl(LIns* ins, NIns** table);
            void        emitJumpTable(SwitchInfo* si, NIns* target);
            void        assignSavedRegs();
            void        reserveSavedRegs();
            void        assignParamRegs();
            void        handleLoopCarriedExprs(InsList& pending_lives);

            
            void        nInit(AvmCore *);
            void        nBeginAssembly();
            Register    nRegisterAllocFromSet(RegisterMask set);
            void        nRegisterResetAll(RegAlloc& a);
            static void nPatchBranch(NIns* branch, NIns* location);
            void        nFragExit(LIns* guard);

            
        public:
            const static Register savedRegs[NumSavedRegs];
            DECLARE_PLATFORM_ASSEMBLER()

        private:
#ifdef NANOJIT_IA32
            debug_only( int32_t _fpuStkDepth; )
            debug_only( int32_t _sv_fpuStkDepth; )

            
            inline void fpu_push() {
                debug_only( ++_fpuStkDepth; NanoAssert(_fpuStkDepth<=0); )
            }
            inline void fpu_pop() {
                debug_only( --_fpuStkDepth; NanoAssert(_fpuStkDepth<=0); )
            }
#endif
            const Config& _config;
    };

    inline int32_t arDisp(LIns* ins)
    {
        
        return -4 * int32_t(ins->getArIndex());
    }
    
    inline int32_t deprecated_disp(LIns* ins)
    {
        
        return -4 * int32_t(ins->deprecated_getArIndex());
    }
}
#endif 
