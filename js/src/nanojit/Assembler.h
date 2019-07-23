





































#ifndef __nanojit_Assembler__
#define __nanojit_Assembler__


namespace avmplus { class InterpState; }

namespace nanojit
{
	





















	



	struct GuardRecord
	{
		GuardRecord* next;			
        Fragment *   from;
        Fragment *   target;
		NIns*		 jmp;
		NIns*        origTarget;
		int32_t		 calldepth;
		SideExit*	 exit;
		GuardRecord* outgoing;			
		verbose_only( uint32_t gid; )
		verbose_only( uint32_t compileNbr; )
	};

	


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

	const uint32_t ARGSIZE_NONE = 0;
	const uint32_t ARGSIZE_F = 1;
	const uint32_t ARGSIZE_LO = 2;
	const uint32_t ARGSIZE_Q = 3;
	const uint32_t _ARGSIZE_MASK_INT = 2;
	const uint32_t _ARGSIZE_MASK_ANY = 3;

	struct CallInfo
	{
		intptr_t	_address;
		uint16_t	_argtypes;		
		uint8_t		_cse;			
		uint8_t		_fold;			
		verbose_only ( const char* _name; )
		
		uint32_t FASTCALL _count_args(uint32_t mask) const;

		inline uint32_t FASTCALL count_args() const { return _count_args(_ARGSIZE_MASK_ANY); }
		inline uint32_t FASTCALL count_iargs() const { return _count_args(_ARGSIZE_MASK_INT); }
		
	};

	#define FUNCTIONID(name) CI_avmplus_##name

	#define INTERP_FOPCODE_LIST_BEGIN											enum FunctionID {
	#define INTERP_FOPCODE_LIST_ENTRY_PRIM(nm)									
	#define INTERP_FOPCODE_LIST_ENTRY_FUNCPRIM(nm,argtypes,cse,fold,ret,args)	FUNCTIONID(nm),
	#define INTERP_FOPCODE_LIST_ENTRY_SUPER(nm,off)								
	#define INTERP_FOPCODE_LIST_ENTRY_EXTERN(nm,off)							
	#define INTERP_FOPCODE_LIST_ENTRY_LITC(nm,i)								
	#define INTERP_FOPCODE_LIST_END												CI_Max } ;
	#include "vm_fops.h"
	#undef INTERP_FOPCODE_LIST_BEGIN
	#undef INTERP_FOPCODE_LIST_ENTRY_PRIM
	#undef INTERP_FOPCODE_LIST_ENTRY_FUNCPRIM
	#undef INTERP_FOPCODE_LIST_ENTRY_SUPER
	#undef INTERP_FOPCODE_LIST_ENTRY_EXTERN
	#undef INTERP_FOPCODE_LIST_ENTRY_LITC
	#undef INTERP_FOPCODE_LIST_END

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
	 
    






	class Assembler
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

			LInsp		begin(LirWriter *writer);	

			NIns*		assemble(Fragment* frag);
			NIns*		endAssembly(Fragment* frag, NInsList& loopJumps);
			NIns*		beginAssembly(RegAllocMap* map);
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

			const CallInfo* callInfoFor(int32_t fid);
			const CallInfo* callInfoFor(LInsp call)
			{
				return callInfoFor(call->imm8());
			}

		private:
			
			NIns*		gen(LirFilter* toCompile);
			NIns*		genPrologue(RegisterMask);
			NIns*		genEpilogue(RegisterMask);

			bool		ignoreInstruction(LInsp ins);

			GuardRecord* placeGuardRecord(SideExit *exit);

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
			verbose_only( uint32_t gid;)
			bool		_inExit,vpad2[3];

			void		asm_cmp(LIns *cond);
#ifndef NJ_SOFTFLOAT
			void		asm_fcmp(LIns *cond);
#endif
			void		asm_mmq(Register rd, int dd, Register rs, int ds);
            NIns*       asm_exit(SideExit *exit);
			NIns*		asm_leave_trace(SideExit* exit);
            void        asm_qjoin(LIns *ins);
            void        asm_store32(LIns *val, int d, LIns *base);
            void        asm_store64(LIns *val, int d, LIns *base);
			void		asm_restore(LInsp, Reservation*, Register);
			void		asm_spill(LInsp i, Reservation *resv, bool pop);
			void		asm_load64(LInsp i);
			void		asm_pusharg(LInsp p);
			NIns*		asm_adjustBranch(NIns* at, NIns* target);

			
			void		nInit(uint32_t flags);
			void		nInit(AvmCore *);
			Register	nRegisterAllocFromSet(int32_t set);
			void		nRegisterResetAll(RegAlloc& a);
			void		nMarkExecute(Page* page, int32_t count=1, bool enable=true);
			void		nPostCallCleanup(const CallInfo* call);
			void		nArgEmitted(const CallInfo* call, uint32_t stackSlotCount, uint32_t iargs, uint32_t fargs);
			void		nFrameRestore(RegisterMask rmask);
			static void	nPatchBranch(NIns* branch, NIns* location);
			GuardRecord *nFragExit(SideExit *exit);

			
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
	};

	inline int32_t disp(Reservation* r) 
	{
		return stack_direction(4) * int32_t(r->arIndex) + NJ_STACK_OFFSET; 
	}
}
#endif 
