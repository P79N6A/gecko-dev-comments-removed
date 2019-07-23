






































#ifndef __nanojit_Assembler__
#define __nanojit_Assembler__


namespace nanojit
{
	





















	#define STACK_GRANULARITY		sizeof(void *)

	


    struct Reservation
	{
		uint32_t arIndex:16;	
		Register reg:15;			
        uint32_t used:1;
	};

	struct AR
	{
		LIns*			entry[ NJ_MAX_STACK_ENTRY ];	
		uint32_t		tos;							
		uint32_t		highwatermark;					
		uint32_t		lowwatermark;					
	};

	#ifdef AVMPLUS_WIN32
		#define AVMPLUS_ALIGN16(type) __declspec(align(16)) type
	#else
		#define AVMPLUS_ALIGN16(type) type __attribute__ ((aligned (16)))
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

	class Fragmento;
	
	
	enum AssmError
	{
		 None = 0
		,OutOMem
		,StackFull
		,ResvFull
		,RegionFull
        ,MaxLength
        ,MaxExit
        ,MaxXJump
        ,UnknownPrim
        ,UnknownBranch
	};

	typedef avmplus::List<NIns*, avmplus::LIST_NonGCObjects> NInsList;
	typedef avmplus::SortedMap<LIns*,NIns*,avmplus::LIST_NonGCObjects> InsMap;
	typedef avmplus::SortedMap<NIns*,LIns*,avmplus::LIST_NonGCObjects> NInsMap;

    class LabelState MMGC_SUBCLASS_DECL
    {
    public:
        RegAlloc regs;
        NIns *addr;
        LabelState(NIns *a, RegAlloc &r) : regs(r), addr(a)
        {}
    };

    class LabelStateMap
    {
        GC *gc;
        avmplus::SortedMap<LIns*, LabelState*, avmplus::LIST_GCObjects> labels;
    public:
        LabelStateMap(GC *gc) : gc(gc), labels(gc)
        {}

        void clear() { labels.clear(); }
        void add(LIns *label, NIns *addr, RegAlloc &regs);
        LabelState *get(LIns *);
    };
    






	class Assembler MMGC_SUBCLASS_DECL
	{
		friend class DeadCodeFilter;
		friend class VerboseBlockReader;
		public:
			#ifdef NJ_VERBOSE
			static char  outline[8192]; 
			static char* outputAlign(char* s, int col); 

			void FASTCALL output(const char* s); 
			void FASTCALL outputf(const char* format, ...); 
			void FASTCALL output_asm(const char* s); 
			
			bool _verbose, vpad[3];
			void printActivationState();

			StringList* _outputCache;
			#endif

			Assembler(Fragmento* frago);
            ~Assembler() {}

			void		assemble(Fragment* frag, NInsList& loopJumps);
			void		endAssembly(Fragment* frag, NInsList& loopJumps);
			void		beginAssembly(Fragment *frag, RegAllocMap* map);
			void		copyRegisters(RegAlloc* copyTo);
			void		releaseRegisters();
            void        patch(GuardRecord *lr);
            void        patch(SideExit *exit);
			void        disconnectLoop(GuardRecord *lr);
			void        reconnectLoop(GuardRecord *lr);
			AssmError   error()	{ return _err; }
			void		setError(AssmError e) { _err = e; }
			void		setCallTable(const CallInfo *functions);
			void		pageReset();
			int32_t		codeBytes();
			Page*		handoverPages(bool exitPages=false);

			debug_only ( void		pageValidate(); )
			debug_only ( bool		onPage(NIns* where, bool exitPages=false); )
			
			
			debug_only( void		resourceConsistencyCheck(); )
			debug_only( void		registerConsistencyCheck(); )
			
			Stats		_stats;		
            int hasLoop;

		private:
			
			void		gen(LirFilter* toCompile, NInsList& loopJumps);
			NIns*		genPrologue();
			NIns*		genEpilogue();

			uint32_t	arReserve(LIns* l);
			void    	arFree(uint32_t idx);
			void		arReset();

			Register	registerAlloc(RegisterMask allow);
			void		registerResetAll();
			void		evictRegs(RegisterMask regs);
            void        evictScratchRegs();
			void		intersectRegisterState(RegAlloc& saved);
			void		unionRegisterState(RegAlloc& saved);
            void        assignSaved(RegAlloc &saved, RegisterMask skip);
	        LInsp       findVictim(RegAlloc& regs, RegisterMask allow);

            Register    getBaseReg(LIns *i, int &d, RegisterMask allow);
            int			findMemFor(LIns* i);
			Register	findRegFor(LIns* i, RegisterMask allow);
			void		findRegFor2(RegisterMask allow, LIns* ia, Reservation* &ra, LIns *ib, Reservation* &rb);
			Register	findSpecificRegFor(LIns* i, Register w);
			Register	prepResultReg(LIns *i, RegisterMask allow);
			void		freeRsrcOf(LIns *i, bool pop);
			void		evict(Register r);
			RegisterMask hint(LIns*i, RegisterMask allow);

			NIns*		pageAlloc(bool exitPage=false);
			void		pagesFree(Page*& list);
			void		internalReset();
            bool        canRemat(LIns*);

			Reservation* reserveAlloc(LInsp i);
			void		reserveFree(LInsp i);
			void		reserveReset();

			Reservation* getresv(LIns *x) {
                uint32_t resv_index = x->resv();
                return resv_index ? &_resvTable[resv_index] : 0;
            }

			DWB(Fragmento*)		_frago;
            GC*					_gc;
            DWB(Fragment*)		_thisfrag;
			RegAllocMap*		_branchStateMap;
		
			const CallInfo	*_functions;
			
			NIns*		_nIns;			
			NIns*		_nExitIns;		
			NIns*       _epilogue;
			Page*		_nativePages;	
			Page*		_nativeExitPages; 
			AssmError	_err;			

			AR			_activation;
			RegAlloc	_allocator;

			LabelStateMap	_labels; 
			NInsMap		_patches;
			Reservation _resvTable[ NJ_MAX_STACK_ENTRY ]; 
			uint32_t	_resvFree;
			bool		_inExit, vpad2[3];
            avmplus::List<LIns*, avmplus::LIST_GCObjects> pending_lives;

			void		asm_cmp(LIns *cond);
			void		asm_fcmp(LIns *cond);
            void        asm_setcc(Register res, LIns *cond);
            NIns *      asm_jmpcc(bool brOnFalse, LIns *cond, NIns *target);
			void		asm_mmq(Register rd, int dd, Register rs, int ds);
            NIns*       asm_exit(LInsp guard);
			NIns*		asm_leave_trace(LInsp guard);
            void        asm_qjoin(LIns *ins);
            void        asm_store32(LIns *val, int d, LIns *base);
            void        asm_store64(LIns *val, int d, LIns *base);
			void		asm_restore(LInsp, Reservation*, Register);
			void		asm_load(int d, Register r);
			void		asm_spilli(LInsp i, Reservation *resv, bool pop);
			void		asm_spill(Register rr, int d, bool pop, bool quad);
			void		asm_load64(LInsp i);
			void		asm_pusharg(LInsp p);
			void		asm_quad(LInsp i);
			void		asm_loop(LInsp i, NInsList& loopJumps);
			void		asm_fcond(LInsp i);
			void		asm_cond(LInsp i);
			void		asm_arith(LInsp i);
			void		asm_neg_not(LInsp i);
			void		asm_ld(LInsp i);
			void		asm_cmov(LInsp i);
			void		asm_param(LInsp i);
			void		asm_int(LInsp i);
			void		asm_short(LInsp i);
			void		asm_qlo(LInsp i);
			void		asm_qhi(LInsp i);
			void		asm_fneg(LInsp ins);
			void		asm_fop(LInsp ins);
			void		asm_i2f(LInsp ins);
			void		asm_u2f(LInsp ins);
			Register	asm_prep_fcall(Reservation *rR, LInsp ins);
			void		asm_nongp_copy(Register r, Register s);
			void		asm_call(LInsp);
            void        asm_arg(ArgSize, LInsp, Register);
			Register	asm_binop_rhs_reg(LInsp ins);
			NIns*		asm_branch(bool branchOnFalse, LInsp cond, NIns* targ, bool far);
            void        assignSavedRegs();
            void        reserveSavedRegs();
            void        assignParamRegs();
            void        handleLoopCarriedExprs();

			
			void		nInit(uint32_t flags);
			void		nInit(AvmCore *);
			Register	nRegisterAllocFromSet(int32_t set);
			void		nRegisterResetAll(RegAlloc& a);
			void		nMarkExecute(Page* page, int32_t count=1, bool enable=true);
			void		nFrameRestore(RegisterMask rmask);
			NIns*    	nPatchBranch(NIns* branch, NIns* location);
			void		nFragExit(LIns* guard);

			
        public:
			const static Register savedRegs[NumSavedRegs];
			DECLARE_PLATFORM_ASSEMBLER()

		private:
			debug_only( int32_t	_fpuStkDepth; )
			debug_only( int32_t	_sv_fpuStkDepth; )

			
			inline void fpu_push() {
				debug_only( ++_fpuStkDepth;  NanoAssert(_fpuStkDepth<=0); )
			} 
			inline void fpu_pop() { 
				debug_only( --_fpuStkDepth;  NanoAssert(_fpuStkDepth<=0); )
			}
	#ifdef AVMPLUS_PORTING_API
			
			
			
			void* _endJit1Addr;
			void* _endJit2Addr;
	#endif 
	};

	inline int32_t disp(Reservation* r) 
	{
		return stack_direction((int32_t)STACK_GRANULARITY) * int32_t(r->arIndex) + NJ_STACK_OFFSET; 
	}
}
#endif 
