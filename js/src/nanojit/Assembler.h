







































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
        if (ins->isop(LIR_allocp)) {
            n = ins->size() >> 2;
        } else {
            switch (ins->retType()) {
            case LTy_I:   n = 1;          break;
            CASE64(LTy_Q:)
            case LTy_D:   n = 2;          break;
            case LTy_V:  NanoAssert(0);  break;
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
        #ifdef _MSC_VER
            #define AVMPLUS_ALIGN16(type) __declspec(align(16)) type
        #else
            #define AVMPLUS_ALIGN16(type) type __attribute__ ((aligned (16)))
        #endif
    #endif

    class Noise
    {
        public:
            virtual ~Noise() {}

            
            virtual uint32_t getValue(uint32_t maxValue) = 0;
    };

    
    enum AssmError
    {
         None = 0
        ,StackFull
        ,UnknownBranch
        ,BranchTooFar
    };

    typedef SeqBuilder<NIns*> NInsList;
    typedef HashMap<NIns*, LIns*> NInsMap;
#if NJ_USES_IMMD_POOL
    typedef HashMap<uint64_t, uint64_t*> ImmDPoolMap;
#endif

#ifdef VMCFG_VTUNE
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

    





    struct Branches 
    {
        NIns* const branch1;
        NIns* const branch2;
        inline explicit Branches(NIns* b1 = NULL, NIns* b2 = NULL) 
            : branch1(b1)
            , branch2(b2)
        {
        }
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
            #ifdef VMCFG_VTUNE
            void* vtuneHandle;
            #endif

            Assembler(CodeAlloc& codeAlloc, Allocator& dataAlloc, Allocator& alloc, LogControl* logc, const Config& config);

            void        compile(Fragment *frag, Allocator& alloc, bool optimize
                                verbose_only(, LInsPrinter*));

            void        endAssembly(Fragment* frag);
            void        assemble(Fragment* frag, LirFilter* reader);
            void        beginAssembly(Fragment *frag);

            void        setNoiseGenerator(Noise* noise)  { _noise = noise; } 

            void        releaseRegisters();
            void        patch(GuardRecord *lr);
            void        patch(SideExit *exit);
            AssmError   error()               { return _err; }
            void        setError(AssmError e) { _err = e; }
            void        cleanupAfterError();
            void        clearNInsPtrs();
            void        reset();

            debug_only ( void       pageValidate(); )

            
            debug_only( void        resourceConsistencyCheck(); )
            debug_only( void        registerConsistencyCheck(); )

        private:
            void        gen(LirFilter* toCompile);
            NIns*       genPrologue();
            NIns*       genEpilogue();

            uint32_t    arReserve(LIns* ins);
            void        arFree(LIns* ins);
            void        arReset();

            Register    registerAlloc(LIns* ins, RegisterMask allow, RegisterMask prefer);
            Register    registerAllocTmp(RegisterMask allow);
            void        registerResetAll();
            void        evictAllActiveRegs() {
                
                
                evictSomeActiveRegs(~RegisterMask(0));
            }
            void        evictSomeActiveRegs(RegisterMask regs);
            void        evictScratchRegsExcept(RegisterMask ignore);
            void        intersectRegisterState(RegAlloc& saved);
            void        unionRegisterState(RegAlloc& saved);
            void        assignSaved(RegAlloc &saved, RegisterMask skip);
            LIns*       findVictim(RegisterMask allow);

            Register    getBaseReg(LIns *ins, int &d, RegisterMask allow);
            void        getBaseReg2(RegisterMask allowValue, LIns* value, Register& rv,
                                    RegisterMask allowBase, LIns* base, Register& rb, int &d);
#if NJ_USES_IMMD_POOL
            const uint64_t*
                        findImmDFromPool(uint64_t q);
#endif
            int         findMemFor(LIns* ins);
            Register    findRegFor(LIns* ins, RegisterMask allow);
            void        findRegFor2(RegisterMask allowa, LIns* ia, Register &ra,
                                    RegisterMask allowb, LIns *ib, Register &rb);
            Register    findSpecificRegFor(LIns* ins, Register r);
            Register    findSpecificRegForUnallocated(LIns* ins, Register r);
            Register    deprecated_prepResultReg(LIns *ins, RegisterMask allow);
            Register    prepareResultReg(LIns *ins, RegisterMask allow);
            void        deprecated_freeRsrcOf(LIns *ins);
            void        freeResourcesOf(LIns *ins);
            void        evictIfActive(Register r);
            void        evict(LIns* vic);
            RegisterMask hint(LIns* ins);

            void        getBaseIndexScale(LIns* addp, LIns** base, LIns** index, int* scale);

            void        codeAlloc(NIns *&start, NIns *&end, NIns *&eip
                                  verbose_only(, size_t &nBytes)
                                  , size_t byteLimit=0);

            
            
            
            
            
            
            
            
            
            static bool canRemat(LIns*);

            bool deprecated_isKnownReg(Register r) {
                return r != deprecated_UnknownReg;
            }

            Allocator&          alloc;              
            CodeAlloc&          _codeAlloc;         
            Allocator&          _dataAlloc;         
            Fragment*           _thisfrag;
            RegAllocMap         _branchStateMap;
            NInsMap             _patches;
            LabelStateMap       _labels;
            Noise*              _noise;             
        #if NJ_USES_IMMD_POOL
            ImmDPoolMap     _immDPool;
        #endif

            
            
            
            
            
            
            
            
            
            CodeList*   codeList;               
            bool        _inExit, vpad2[3];
            NIns        *codeStart, *codeEnd;   
            NIns        *exitStart, *exitEnd;   
            NIns*       _nIns;                  
            NIns*       _nExitIns;              
                                                
        #ifdef NJ_VERBOSE
            NIns*       _nInsAfter;             
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

            
            LIns*       currIns;

            AR          _activation;
            RegAlloc    _allocator;

            verbose_only( void asm_inc_m32(uint32_t*); )
            void        asm_mmq(Register rd, int dd, Register rs, int ds);
            void        asm_jmp(LIns* ins, InsList& pending_lives);
            void        asm_jcc(LIns* ins, InsList& pending_lives);
            void        asm_jov(LIns* ins, InsList& pending_lives);
            void        asm_x(LIns* ins);
            void        asm_xcc(LIns* ins);
            NIns*       asm_exit(LIns* guard);
            NIns*       asm_leave_trace(LIns* guard);
            void        asm_store32(LOpcode op, LIns *val, int d, LIns *base);
            void        asm_store64(LOpcode op, LIns *val, int d, LIns *base);

            
            
            
            void        asm_restore(LIns*, Register);

            bool        asm_maybe_spill(LIns* ins, bool pop);
#ifdef NANOJIT_IA32
            void        asm_spill(Register rr, int d, bool pop);
#else
            void        asm_spill(Register rr, int d, bool quad);
#endif
            void        asm_load64(LIns* ins);
            void        asm_ret(LIns* ins);
#ifdef NANOJIT_64BIT
            void        asm_immq(LIns* ins);
#endif
            void        asm_immd(LIns* ins);
            void        asm_condd(LIns* ins);
            void        asm_cond(LIns* ins);
            void        asm_arith(LIns* ins);
            void        asm_neg_not(LIns* ins);
            void        asm_load32(LIns* ins);
            void        asm_cmov(LIns* ins);
            void        asm_param(LIns* ins);
            void        asm_immi(LIns* ins);
#if NJ_SOFTFLOAT_SUPPORTED
            void        asm_qlo(LIns* ins);
            void        asm_qhi(LIns* ins);
            void        asm_qjoin(LIns *ins);
#endif
            void        asm_fneg(LIns* ins);
            void        asm_fop(LIns* ins);
            void        asm_i2d(LIns* ins);
            void        asm_ui2d(LIns* ins);
            void        asm_d2i(LIns* ins);
#ifdef NANOJIT_64BIT
            void        asm_q2i(LIns* ins);
            void        asm_ui2uq(LIns *ins);
            void        asm_dasq(LIns *ins);
            void        asm_qasd(LIns *ins);
#endif
            void        asm_nongp_copy(Register r, Register s);
            void        asm_call(LIns*);
            Register    asm_binop_rhs_reg(LIns* ins);
            Branches    asm_branch(bool branchOnFalse, LIns* cond, NIns* targ);
            NIns*       asm_branch_ov(LOpcode op, NIns* targ);
            void        asm_jtbl(LIns* ins, NIns** table);
            void        asm_insert_random_nop();
            void        assignSavedRegs();
            void        reserveSavedRegs();
            void        assignParamRegs();
            void        handleLoopCarriedExprs(InsList& pending_lives);

            
            void        nInit();
            void        nBeginAssembly();
            Register    nRegisterAllocFromSet(RegisterMask set);
            void        nRegisterResetAll(RegAlloc& a);
            void        nPatchBranch(NIns* branch, NIns* location);
            void        nFragExit(LIns* guard);

            RegisterMask nHints[LIR_sentinel+1];
            RegisterMask nHint(LIns* ins);

            
            
            
            static const RegisterMask PREFER_SPECIAL = 0xffffffff;

            
        public:
            const static Register savedRegs[NumSavedRegs+1]; 
            DECLARE_PLATFORM_ASSEMBLER()

        private:
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
