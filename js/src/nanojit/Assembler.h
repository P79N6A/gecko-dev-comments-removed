







































#ifndef __nanojit_Assembler__
#define __nanojit_Assembler__


namespace nanojit
{
    

















    #define STACK_GRANULARITY        sizeof(void *)

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    struct AR
    {
        LIns*           entry[ NJ_MAX_STACK_ENTRY ];    
        uint32_t        tos;                            
        uint32_t        lowwatermark;                   
    };

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
    };

    typedef SeqBuilder<NIns*> NInsList;
    typedef HashMap<NIns*, LIns*> NInsMap;

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

    typedef SeqBuilder<char*> StringList;

    

    typedef HashMap<SideExit*, RegAlloc*> RegAllocMap;

    






    class Assembler
    {
        friend class VerboseBlockReader;
        public:
            #ifdef NJ_VERBOSE
            static char  outline[8192];
            static char  outlineEOL[512];  
            static char* outputAlign(char* s, int col);

            void outputForEOL(const char* format, ...);
            void output(const char* s);
            void outputf(const char* format, ...);
            void output_asm(const char* s);

            bool outputAddr, vpad[3];  
            void printActivationState(const char* what);

            StringList* _outputCache;

            
            
            LogControl* _logc;
            size_t codeBytes;
            size_t exitBytes;
            #endif 

            #ifdef VTUNE
            avmplus::CodegenLIR *cgen;
            #endif

            Assembler(CodeAlloc& codeAlloc, Allocator& dataAlloc, Allocator& alloc, AvmCore* core, LogControl* logc);

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

            Stats       _stats;
            CodeList*   codeList;                   

        private:

            void        gen(LirFilter* toCompile);
            NIns*       genPrologue();
            NIns*       genEpilogue();

            uint32_t    arReserve(LIns* l);
            void        arFree(uint32_t idx);
            void        arReset();

            Register    registerAlloc(RegisterMask allow);
            void        registerResetAll();
            void        evictAllActiveRegs();
            void        evictSomeActiveRegs(RegisterMask regs);
            void        evictScratchRegs();
            void        intersectRegisterState(RegAlloc& saved);
            void        unionRegisterState(RegAlloc& saved);
            void        assignSaved(RegAlloc &saved, RegisterMask skip);
            LInsp       findVictim(RegisterMask allow);

            Register    getBaseReg(LOpcode op, LIns *i, int &d, RegisterMask allow);
            int         findMemFor(LIns* i);
            Register    findRegFor(LIns* i, RegisterMask allow);
            void        findRegFor2(RegisterMask allow, LIns* ia, Reservation* &resva, LIns *ib, Reservation* &resvb);
            void        findRegFor2b(RegisterMask allow, LIns* ia, Register &ra, LIns *ib, Register &rb);
            Register    findSpecificRegFor(LIns* i, Register r);
            Register    findSpecificRegForUnallocated(LIns* i, Register r);
            Register    prepResultReg(LIns *i, RegisterMask allow);
            void        freeRsrcOf(LIns *i, bool pop);
            void        evictIfActive(Register r);
            void        evict(Register r, LIns* vic);
            RegisterMask hint(LIns*i, RegisterMask allow);

            void        codeAlloc(NIns *&start, NIns *&end, NIns *&eip
                                  verbose_only(, size_t &nBytes));
            bool        canRemat(LIns*);

            bool isKnownReg(Register r) {
                return r != UnknownReg;
            }

            Reservation* getresv(LIns *x) {
                Reservation* r = x->resv();
                return r->used ? r : 0;
            }

            Allocator&          alloc;              
            CodeAlloc&          _codeAlloc;         
            Allocator&          _dataAlloc;         
            Fragment*           _thisfrag;
            RegAllocMap         _branchStateMap;
            NInsMap             _patches;
            LabelStateMap       _labels;

            NIns        *codeStart, *codeEnd;       
            NIns        *exitStart, *exitEnd;       
            NIns*       _nIns;          
            NIns*       _nExitIns;      
            NIns*       _epilogue;
            AssmError   _err;           
        #if PEDANTIC
            NIns*       pedanticTop;
        #endif

            AR          _activation;
            RegAlloc    _allocator;

            bool        _inExit, vpad2[3];

            verbose_only( void asm_inc_m32(uint32_t*); )
            void        asm_mmq(Register rd, int dd, Register rs, int ds);
            NIns*       asm_exit(LInsp guard);
            NIns*       asm_leave_trace(LInsp guard);
            void        asm_qjoin(LIns *ins);
            void        asm_store32(LIns *val, int d, LIns *base);
            void        asm_store64(LIns *val, int d, LIns *base);
            void        asm_restore(LInsp, Reservation*, Register);
            void        asm_load(int d, Register r);
            void        asm_spilli(LInsp i, bool pop);
            void        asm_spill(Register rr, int d, bool pop, bool quad);
            void        asm_load64(LInsp i);
            void        asm_ret(LInsp p);
            void        asm_quad(LInsp i);
            void        asm_fcond(LInsp i);
            void        asm_cond(LInsp i);
            void        asm_arith(LInsp i);
            void        asm_neg_not(LInsp i);
            void        asm_ld(LInsp i);
            void        asm_cmov(LInsp i);
            void        asm_param(LInsp i);
            void        asm_int(LInsp i);
            void        asm_qlo(LInsp i);
            void        asm_qhi(LInsp i);
            void        asm_fneg(LInsp ins);
            void        asm_fop(LInsp ins);
            void        asm_i2f(LInsp ins);
            void        asm_u2f(LInsp ins);
            void        asm_promote(LIns *ins);
            Register    asm_prep_fcall(Reservation *rR, LInsp ins);
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
                debug_only( ++_fpuStkDepth;  NanoAssert(_fpuStkDepth<=0); )
            }
            inline void fpu_pop() {
                debug_only( --_fpuStkDepth;  NanoAssert(_fpuStkDepth<=0); )
            }
#endif
            avmplus::Config &config;
    };

    inline int32_t disp(Reservation* r)
    {
        
        return stack_direction(4 * int32_t(r->arIndex));
    }
    inline int32_t disp(LIns* ins)
    {
        
        return stack_direction(4 * int32_t(ins->getArIndex()));
    }
}
#endif 
