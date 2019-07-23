






































#ifndef __nanojit_Assembler__
#define __nanojit_Assembler__


namespace nanojit
{
	





















	#define STACK_GRANULARITY		sizeof(void *)

	


    struct Reservation
	{
		uint32_t arIndex:16;	
		Register reg:8;			
        int cost:8;
	};

	struct AR
	{
		LIns*			entry[ NJ_MAX_STACK_ENTRY ];	
		uint32_t		tos;							
		uint32_t		highwatermark;					
		uint32_t		lowwatermark;					
		LIns*			parameter[ NJ_MAX_PARAMETERS ]; 
	};

    enum ArgSize {
	    ARGSIZE_NONE = 0,
	    ARGSIZE_F = 1,
	    ARGSIZE_LO = 2,
	    ARGSIZE_Q = 3,
	    _ARGSIZE_MASK_INT = 2, 
        _ARGSIZE_MASK_ANY = 3
    };

	struct CallInfo
	{
		intptr_t	_address;
		uint16_t	_argtypes;		
		uint8_t		_cse;			
		uint8_t		_fold;			
		verbose_only ( const char* _name; )
		
		uint32_t FASTCALL _count_args(uint32_t mask) const;
        uint32_t get_sizes(ArgSize*) const;

		inline uint32_t FASTCALL count_args() const { return _count_args(_ARGSIZE_MASK_ANY); }
		inline uint32_t FASTCALL count_iargs() const { return _count_args(_ARGSIZE_MASK_INT); }
		
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
	};

	typedef avmplus::List<NIns*, avmplus::LIST_NonGCObjects> NInsList;

    






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
			void		unpatch(GuardRecord *lr);
			AssmError   error()	{ return _err; }
			void		setError(AssmError e) { _err = e; }
			void		setCallTable(const CallInfo *functions);
			void		pageReset();
			Page*		handoverPages(bool exitPages=false);

			debug_only ( void		pageValidate(); )
			debug_only ( bool		onPage(NIns* where, bool exitPages=false); )
			
			
			debug_only( void		resourceConsistencyCheck(); )
			debug_only( void		registerConsistencyCheck(LIns** resv); )
			
			Stats		_stats;		

		private:
			
			void		gen(LirFilter* toCompile, NInsList& loopJumps);
			NIns*		genPrologue(RegisterMask);
			NIns*		genEpilogue(RegisterMask);

			bool		ignoreInstruction(LInsp ins);

			GuardRecord* placeGuardRecord(LInsp guard);
			void		initGuardRecord(LInsp guard, GuardRecord*);

			uint32_t	arReserve(LIns* l);
			uint32_t	arFree(uint32_t idx);
			void		arReset();

			Register	registerAlloc(RegisterMask allow);
			void		registerResetAll();
			void		restoreCallerSaved();
			void		mergeRegisterState(RegAlloc& saved);
	        LInsp       findVictim(RegAlloc& regs, RegisterMask allow, RegisterMask prefer);
		
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

			Reservation* reserveAlloc(LInsp i);
			void		reserveFree(LInsp i);
			void		reserveReset();

			Reservation* getresv(LIns *x) { return x->resv() ? &_resvTable[x->resv()] : 0; }

			DWB(Fragmento*)		_frago;
            GC*					_gc;
            DWB(Fragment*)		_thisfrag;
			RegAllocMap*		_branchStateMap;
			GuardRecord*		_latestGuard;
		
			const CallInfo	*_functions;
			
			NIns*		_nIns;			
			NIns*		_nExitIns;		
			NIns*       _epilogue;
			Page*		_nativePages;	
			Page*		_nativeExitPages; 
			AssmError	_err;			

			AR			_activation;
			RegAlloc	_allocator;

			Reservation _resvTable[ NJ_MAX_STACK_ENTRY ]; 
			uint32_t	_resvFree;
			bool		_inExit,vpad2[3];

			void		asm_cmp(LIns *cond);
#ifndef NJ_SOFTFLOAT
			void		asm_fcmp(LIns *cond);
#endif
			void		asm_mmq(Register rd, int dd, Register rs, int ds);
            NIns*       asm_exit(LInsp guard);
			NIns*		asm_leave_trace(LInsp guard);
            void        asm_qjoin(LIns *ins);
            void        asm_store32(LIns *val, int d, LIns *base);
            void        asm_store64(LIns *val, int d, LIns *base);
			void		asm_restore(LInsp, Reservation*, Register);
			void		asm_spill(LInsp i, Reservation *resv, bool pop);
			void		asm_load64(LInsp i);
			void		asm_pusharg(LInsp p);
			NIns*		asm_adjustBranch(NIns* at, NIns* target);
			void		asm_quad(LInsp i);
			bool		asm_qlo(LInsp ins, LInsp q);
			void		asm_fneg(LInsp ins);
			void		asm_fop(LInsp ins);
			void		asm_i2f(LInsp ins);
			void		asm_u2f(LInsp ins);
			Register	asm_prep_fcall(Reservation *rR, LInsp ins);
			void		asm_nongp_copy(Register r, Register s);
			void		asm_bailout(LInsp guard, Register state);
			void		asm_call(LInsp);
            void        asm_arg(ArgSize, LInsp, Register);
			Register	asm_binop_rhs_reg(LInsp ins);

			
			void		nInit(uint32_t flags);
			void		nInit(AvmCore *);
			Register	nRegisterAllocFromSet(int32_t set);
			void		nRegisterResetAll(RegAlloc& a);
			void		nMarkExecute(Page* page, int32_t count=1, bool enable=true);
			void		nFrameRestore(RegisterMask rmask);
			static void	nPatchBranch(NIns* branch, NIns* location);
			void		nFragExit(LIns* guard);

			
        public:
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
