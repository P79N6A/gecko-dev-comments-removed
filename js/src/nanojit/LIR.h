





































#ifndef __nanojit_LIR__
#define __nanojit_LIR__










namespace nanojit
{
	enum LOpcode
#if defined(_MSC_VER) && _MSC_VER >= 1400
          : unsigned
#endif
	{
		
		LIR64	= 0x40,			
		
#define OPDEF(op, number, args) \
        LIR_##op = (number),
#define OPDEF64(op, number, args) \
        LIR_##op = ((number) | LIR64),
#include "LIRopcode.tbl"
        LIR_sentinel
#undef OPDEF
#undef OPDEF64
	};

	#if defined NANOJIT_64BIT
	#define LIR_ldp     LIR_ldq
	#define LIR_stp     LIR_stq
    #define LIR_piadd   LIR_qiadd
    #define LIR_piand   LIR_qiand
    #define LIR_pilsh   LIR_qilsh
	#define LIR_pcmov	LIR_qcmov
    #define LIR_pior    LIR_qior
	#else
	#define LIR_ldp     LIR_ld
	#define LIR_stp     LIR_st
    #define LIR_piadd   LIR_add
    #define LIR_piand   LIR_and
    #define LIR_pilsh   LIR_lsh
	#define LIR_pcmov	LIR_cmov
    #define LIR_pior    LIR_or
	#endif

	struct GuardRecord;
    struct SideExit;
    struct Page;

    enum AbiKind {
        ABI_FASTCALL,
        ABI_THISCALL,
		ABI_STDCALL,
        ABI_CDECL
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
		uintptr_t	_address;
        uint32_t	_argtypes:18;	
        uint8_t		_cse:1;			
        uint8_t		_fold:1;		
        AbiKind     _abi:3;
		verbose_only ( const char* _name; )
		
		uint32_t FASTCALL _count_args(uint32_t mask) const;
        uint32_t get_sizes(ArgSize*) const;

        inline bool isInterface() const {
            return _address == 2 || _address == 3; 
        }
        inline bool isIndirect() const {
            return _address < 256;
        }
		inline uint32_t FASTCALL count_args() const {
            return _count_args(_ARGSIZE_MASK_ANY) + isIndirect();
        }
		inline uint32_t FASTCALL count_iargs() const {
            return _count_args(_ARGSIZE_MASK_INT);
        }
		
	};

	


	struct SwitchInfo
	{
		NIns**      table;       
		uint32_t    count;       
		
		
		
		uint32_t    index;
	};

    inline bool isCseOpcode(LOpcode op) {
        op = LOpcode(op & ~LIR64);
        return op >= LIR_ldcs && op <= LIR_uge;
    }
    inline bool isRetOpcode(LOpcode op) {
        return (op & ~LIR64) == LIR_ret;
    }

	
	#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
	#define _sign_int signed int
	#else
	#define _sign_int int32_t
	#endif

    
    
    
    struct Reservation
    {
        uint32_t arIndex:16;    
        Register reg:7;         
        uint32_t used:1;        
        LOpcode  code:8;
	};

    
    
    
    
	class LIns
	{
        friend class LirBufWriter;

        
        
		struct u_type
		{
            
            
            LIns*       oprnd_1;

            LIns*       oprnd_2;  
		};

        
        struct sti_type
        {
            
            
            LIns*       oprnd_1;

            LIns*       oprnd_2;  

            int32_t     disp;
        };

        
		struct c_type
		{
            uintptr_t   imm8a:8;    
            uintptr_t   imm8b:8;    

            const CallInfo* ci;     
		};

        
		struct i_type
		{
            int32_t     imm32;
		};

        
        struct i64_type
		{
            int32_t     imm64_0;
            int32_t     imm64_1;
		};

        #undef _sign_int
		
        
        
        Reservation firstWord;

        
		union
		{
            u_type      u;
            c_type      c;
            i_type      i;
            i64_type    i64;
            sti_type    sti;
		};

	public:
        LIns* oprnd1() const { return u.oprnd_1; }
        LIns* oprnd2() const { return u.oprnd_2; }

        inline LOpcode opcode()   const { return firstWord.code; }
        inline uint8_t imm8()     const { return c.imm8a; }
        inline uint8_t imm8b()    const { return c.imm8b; }
        inline int32_t imm32()    const { NanoAssert(isconst());  return i.imm32; }
        inline int32_t imm64_0()  const { NanoAssert(isconstq()); return i64.imm64_0; }
        inline int32_t imm64_1()  const { NanoAssert(isconstq()); return i64.imm64_1; }
        uint64_t       imm64()    const;
        double         imm64f()   const;
        Reservation*   resv()           { return &firstWord; }
        void*	payload() const;
        inline Page*	page()			{ return (Page*) alignTo(this,NJ_PAGE_SIZE); }
        inline int32_t  size() const {
            NanoAssert(isop(LIR_alloc));
            return i.imm32<<2;
        }
        inline void setSize(int32_t bytes) {
            NanoAssert(isop(LIR_alloc) && (bytes&3)==0 && isU16(bytes>>2));
            i.imm32 = bytes>>2;
        }

		LIns* arg(uint32_t i);

        inline int32_t immdisp() const 
        {
            NanoAssert(isStore());
            return sti.disp;
        }
    
		inline void* constvalp() const
		{
        #ifdef AVMPLUS_64BIT
		    return (void*)imm64();
		#else
		    return (void*)imm32();
        #endif      
		}
		
		bool isCse(const CallInfo *functions) const;
        bool isRet() const { return nanojit::isRetOpcode(firstWord.code); }
		bool isop(LOpcode o) const { return firstWord.code == o; }
		bool isQuad() const;
		bool isCond() const;
        bool isFloat() const;
		bool isCmp() const;
        bool isCall() const { 
            LOpcode op = LOpcode(firstWord.code & ~LIR64);
            return op == LIR_call || op == LIR_calli;
        }
        bool isStore() const {
            LOpcode op = LOpcode(firstWord.code & ~LIR64);
            return op == LIR_sti;
        }
        bool isLoad() const { 
            LOpcode op = firstWord.code;
            return op == LIR_ldq  || op == LIR_ld || op == LIR_ldc || 
                   op == LIR_ldqc || op == LIR_ldcs;
        }
        bool isGuard() const {
            LOpcode op = firstWord.code;
            return op == LIR_x || op == LIR_xf || op == LIR_xt || 
                   op == LIR_loop || op == LIR_xbarrier || op == LIR_xtbl;
        }
		
        bool isconst() const { return firstWord.code == LIR_int; }
		
		
		bool isconstval(int32_t val) const;
		
		bool isconstq() const;
		
		bool isconstp() const;
		bool isBranch() const {
			return isop(LIR_jt) || isop(LIR_jf) || isop(LIR_j);
		}
        void setimm32(int32_t x) { i.imm32 = x; }
        
        void initOpcodeAndClearResv(LOpcode);
        Reservation* initResv();
        void         clearResv();

		
        void setOprnd1(LIns* r) { u.oprnd_1 = r; }
        void setOprnd2(LIns* r) { u.oprnd_2 = r; }
        void setDisp(int32_t d) { sti.disp = d; }
		void setTarget(LIns* t);
		LIns* getTarget();

        GuardRecord *record();

		inline uint32_t argc() const {
			NanoAssert(isCall());
			return c.imm8b;
		}
        size_t callInsSlots() const;
		const CallInfo *callInfo() const;
	};
	typedef LIns*		LInsp;

	LIns* FASTCALL callArgN(LInsp i, uint32_t n);
	extern const uint8_t operandCount[];

	class Fragmento;	
	class LirFilter;

	
	class LirWriter : public avmplus::GCObject
	{
	public:
		LirWriter *out;
        const CallInfo *_functions;

		virtual ~LirWriter() {}
		LirWriter(LirWriter* out) 
			: out(out), _functions(out?out->_functions : 0) {}

		virtual LInsp ins0(LOpcode v) {
			return out->ins0(v);
		}
		virtual LInsp ins1(LOpcode v, LIns* a) {
			return out->ins1(v, a);
		}
		virtual LInsp ins2(LOpcode v, LIns* a, LIns* b) {
			return out->ins2(v, a, b);
		}
		virtual LInsp insGuard(LOpcode v, LIns *c, LIns *x) {
			return out->insGuard(v, c, x);
		}
		virtual LInsp insBranch(LOpcode v, LInsp condition, LInsp to) {
			return out->insBranch(v, condition, to);
		}
        
        
		virtual LInsp insParam(int32_t arg, int32_t kind) {
			return out->insParam(arg, kind);
		}
		virtual LInsp insImm(int32_t imm) {
			return out->insImm(imm);
		}
		virtual LInsp insImmq(uint64_t imm) {
			return out->insImmq(imm);
		}
		virtual LInsp insLoad(LOpcode op, LIns* base, LIns* d) {
			return out->insLoad(op, base, d);
		}
		virtual LInsp insStorei(LIns* value, LIns* base, int32_t d) {
			return out->insStorei(value, base, d);
		}
		virtual LInsp insCall(const CallInfo *call, LInsp args[]) {
			return out->insCall(call, args);
		}
		virtual LInsp insAlloc(int32_t size) {
			return out->insAlloc(size);
		}
		virtual LInsp insSkip(size_t size) {
			return out->insSkip(size);
		}

		
	    LIns*		insLoadi(LIns *base, int disp);
	    LIns*		insLoad(LOpcode op, LIns *base, int disp);
		
		
	    LIns*		ins_choose(LIns* cond, LIns* iftrue, LIns* iffalse);
	    
	    LIns*		ins_eq0(LIns* oprnd1);
		
		
        LIns*       ins2i(LOpcode op, LIns *oprnd1, int32_t);
		LIns*		qjoin(LInsp lo, LInsp hi);
		LIns*		insImmPtr(const void *ptr);
		LIns*		insImmf(double f);
	};


	
	
	
	
	
	
	
#define MAX_SKIP_BYTES (NJ_PAGE_SIZE			\
						- sizeof(PageHeader)	\
						- 6*sizeof(LIns))

#ifdef NJ_VERBOSE
	extern const char* lirNames[];

	


    class LabelMap MMGC_SUBCLASS_DECL
    {
		LabelMap* parent;
		class Entry MMGC_SUBCLASS_DECL
		{
		public:
			Entry(int) : name(0), size(0), align(0) {}
			Entry(avmplus::String *n, size_t s, size_t a) : name(n),size(s),align(a) {}
            ~Entry(); 
			DRCWB(avmplus::String*) name;
			size_t size:29, align:3;
		};
        avmplus::SortedMap<const void*, Entry*, avmplus::LIST_GCObjects> names;
		bool addrs, pad[3];
		char buf[1000], *end;
        void formatAddr(const void *p, char *buf);
    public:
        avmplus::AvmCore *core;
        LabelMap(avmplus::AvmCore *, LabelMap* parent);
        ~LabelMap();
        void add(const void *p, size_t size, size_t align, const char *name);
		void add(const void *p, size_t size, size_t align, avmplus::String*);
		const char *dup(const char *);
		const char *format(const void *p);
		void promoteAll(const void *newbase);
		void clear();
    };

	class LirNameMap MMGC_SUBCLASS_DECL
	{
		template <class Key>
		class CountMap: public avmplus::SortedMap<Key, int, avmplus::LIST_NonGCObjects> {
		public:
			CountMap(avmplus::GC*gc) : avmplus::SortedMap<Key, int, avmplus::LIST_NonGCObjects>(gc) {}
			int add(Key k) {
				int c = 1;
				if (containsKey(k)) {
					c = 1+get(k);
				}
				put(k,c);
				return c;
			}
		};
		CountMap<int> lircounts;
		CountMap<const CallInfo *> funccounts;

		class Entry MMGC_SUBCLASS_DECL 
		{
		public:
			Entry(int) : name(0) {}
			Entry(avmplus::String *n) : name(n) {}
            ~Entry();
			DRCWB(avmplus::String*) name;
		};
		avmplus::SortedMap<LInsp, Entry*, avmplus::LIST_GCObjects> names;
		LabelMap *labels;
		void formatImm(int32_t c, char *buf);
	public:

		LirNameMap(avmplus::GC *gc, LabelMap *r) 
			: lircounts(gc),
			funccounts(gc),
			names(gc),
			labels(r)
		{}
        ~LirNameMap();

		void addName(LInsp i, const char *s);
		bool addName(LInsp i, avmplus::String *s);
		void copyName(LInsp i, const char *s, int suffix);
        const char *formatRef(LIns *ref);
		const char *formatIns(LInsp i);
		void formatGuard(LInsp i, char *buf);
	};


	class VerboseWriter : public LirWriter
	{
		InsList code;
		DWB(LirNameMap*) names;
    public:
		VerboseWriter(avmplus::GC *gc, LirWriter *out, LirNameMap* names) 
			: LirWriter(out), code(gc), names(names)
		{}

		LInsp add(LInsp i) {
            if (i)
                code.add(i);
			return i;
		}

        LInsp add_flush(LInsp i) {
            if ((i = add(i)) != 0) 
                flush();
            return i;
        }

		void flush()
		{
            int n = code.size();
            if (n) {
			    for (int i=0; i < n; i++)
				    nj_dprintf("    %s\n",names->formatIns(code[i]));
			    code.clear();
                if (n > 1)
        			nj_dprintf("\n");
            }
		}

		LIns* insGuard(LOpcode op, LInsp cond, LIns *x) {
			return add_flush(out->insGuard(op,cond,x));
		}

		LIns* insBranch(LOpcode v, LInsp condition, LInsp to) {
			return add_flush(out->insBranch(v, condition, to));
		}

		LIns* ins0(LOpcode v) {
            if (v == LIR_label || v == LIR_start) {
                flush();
            }
			return add(out->ins0(v));
		}

		LIns* ins1(LOpcode v, LInsp a) {
            return isRetOpcode(v) ? add_flush(out->ins1(v, a)) : add(out->ins1(v, a));
		}
		LIns* ins2(LOpcode v, LInsp a, LInsp b) {
			return v == LIR_2 ? out->ins2(v,a,b) : add(out->ins2(v, a, b));
		}
		LIns* insCall(const CallInfo *call, LInsp args[]) {
			return add_flush(out->insCall(call, args));
		}
		LIns* insParam(int32_t i, int32_t kind) {
			return add(out->insParam(i, kind));
		}
		LIns* insLoad(LOpcode v, LInsp base, LInsp disp) {
			return add(out->insLoad(v, base, disp));
		}
		LIns* insStorei(LInsp v, LInsp b, int32_t d) {
			return add(out->insStorei(v, b, d));
		}
        LIns* insAlloc(int32_t size) {
            return add(out->insAlloc(size));
        }
        LIns* insImm(int32_t imm) {
            return add(out->insImm(imm));
        }
        LIns* insImmq(uint64_t imm) {
            return add(out->insImmq(imm));
        }
    };

#endif

	class ExprFilter: public LirWriter
	{
	public:
		ExprFilter(LirWriter *out) : LirWriter(out) {}
		LIns* ins1(LOpcode v, LIns* a);
	    LIns* ins2(LOpcode v, LIns* a, LIns* b);
		LIns* insGuard(LOpcode, LIns *cond, LIns *);
        LIns* insBranch(LOpcode, LIns *cond, LIns *target);
	};

	
	class LInsHashSet
	{
		
		
		
		static const uint32_t kInitialCap = 64;	

		LInsp *m_list; 
		uint32_t m_used, m_cap;
		avmplus::GC* m_gc;

		static uint32_t FASTCALL hashcode(LInsp i);
		uint32_t FASTCALL find(LInsp name, uint32_t hash, const LInsp *list, uint32_t cap);
		static bool FASTCALL equals(LInsp a, LInsp b);
		void FASTCALL grow();

	public:

		LInsHashSet(avmplus::GC* gc);
		~LInsHashSet();
		LInsp find32(int32_t a, uint32_t &i);
		LInsp find64(uint64_t a, uint32_t &i);
		LInsp find1(LOpcode v, LInsp a, uint32_t &i);
		LInsp find2(LOpcode v, LInsp a, LInsp b, uint32_t &i);
		LInsp findcall(const CallInfo *call, uint32_t argc, LInsp args[], uint32_t &i);
		LInsp add(LInsp i, uint32_t k);
		void replace(LInsp i);
        void clear();

		static uint32_t FASTCALL hashimm(int32_t);
		static uint32_t FASTCALL hashimmq(uint64_t);
		static uint32_t FASTCALL hash1(LOpcode v, LInsp);
		static uint32_t FASTCALL hash2(LOpcode v, LInsp, LInsp);
		static uint32_t FASTCALL hashcall(const CallInfo *call, uint32_t argc, LInsp args[]);
	};

	class CseFilter: public LirWriter
	{
	public:
		LInsHashSet exprs;
		CseFilter(LirWriter *out, avmplus::GC *gc);
	    LIns* insImm(int32_t imm);
	    LIns* insImmq(uint64_t q);
	    LIns* ins0(LOpcode v);
		LIns* ins1(LOpcode v, LInsp);
		LIns* ins2(LOpcode v, LInsp, LInsp);
		LIns* insLoad(LOpcode v, LInsp b, LInsp d);
		LIns* insCall(const CallInfo *call, LInsp args[]);
		LIns* insGuard(LOpcode op, LInsp cond, LIns *x);
	};

	class LirBuffer : public avmplus::GCFinalizedObject
	{
		public:
			DWB(Fragmento*)		_frago;
			LirBuffer(Fragmento* frago, const CallInfo* functions);
			virtual ~LirBuffer();
			void        clear();
            void        rewind();
			LInsp		next();
			bool		outOMem() { return _noMem != 0; }
			
			debug_only (void validate() const;)
			verbose_only(DWB(LirNameMap*) names;)
			
			int32_t insCount();
			int32_t byteCount();

			
			struct 
			{
				uint32_t lir;	
			}
			_stats;

			const CallInfo* _functions;
            AbiKind abi;
            LInsp state,param1,sp,rp;
            LInsp savedRegs[NumSavedRegs];
            bool explicitSavedRegs;
			
		protected:
			friend class LirBufWriter;

			LInsp		commit(uint32_t count);
			Page*		pageAlloc();

			PageList	_pages;
			Page*		_nextPage; 
			LInsp		_unused;	
			int			_noMem;		
	};	

	class LirBufWriter : public LirWriter
	{
		DWB(LirBuffer*)	_buf;		

        public:			
			LirBufWriter(LirBuffer* buf)
				: LirWriter(0), _buf(buf) {
				_functions = buf->_functions;
			}

			
			LInsp   insLoad(LOpcode op, LInsp base, LInsp off);
			LInsp	insStorei(LInsp o1, LInsp o2, int32_t imm);
			LInsp	ins0(LOpcode op);
			LInsp	ins1(LOpcode op, LInsp o1);
			LInsp	ins2(LOpcode op, LInsp o1, LInsp o2);
			LInsp	insParam(int32_t i, int32_t kind);
			LInsp	insImm(int32_t imm);
			LInsp	insImmq(uint64_t imm);
		    LInsp	insCall(const CallInfo *call, LInsp args[]);
			LInsp	insGuard(LOpcode op, LInsp cond, LIns *x);
			LInsp	insBranch(LOpcode v, LInsp condition, LInsp to);
            LInsp   insAlloc(int32_t size);
            LInsp   insSkip(size_t);

		protected:
			void	ensureRoom(uint32_t count);
			
		private:
            LInsp   insSkipWithoutBuffer(LInsp to);     
	};

	class LirFilter
	{
	public:
		LirFilter *in;
		LirFilter(LirFilter *in) : in(in) {}
        virtual ~LirFilter(){}

		virtual LInsp read() {
			return in->read();
		}
		virtual LInsp pos() {
			return in->pos();
		}
	};

	
	class LirReader : public LirFilter
	{
		LInsp _i; 

	public:
		LirReader(LirBuffer* buf) : LirFilter(0), _i(buf->next()-1) { }
		LirReader(LInsp i) : LirFilter(0), _i(i) { }
		virtual ~LirReader() {}

		
		LInsp read(); 
		LInsp pos() {
			return _i;
		}
        void setpos(LIns *i) {
            _i = i;
        }
	};

    class Assembler;

    void compile(Assembler *assm, Fragment *frag);
	verbose_only(void live(avmplus::GC *gc, LirBuffer *lirbuf);)

	class StackFilter: public LirFilter
	{
	    avmplus::GC *gc;
		LirBuffer *lirbuf;
		LInsp sp;
		avmplus::BitSet stk;
        int top;
		int getTop(LInsp br);
	public:
	    StackFilter(LirFilter *in, avmplus::GC *gc, LirBuffer *lirbuf, LInsp sp); 
		virtual ~StackFilter() {}
		LInsp read();
	};

	class CseReader: public LirFilter
	{
		LInsHashSet *exprs;
		const CallInfo *functions;
	public:
		CseReader(LirFilter *in, LInsHashSet *exprs, const CallInfo*);
		LInsp read();
	};

    
    class LoadFilter: public LirWriter
    {
    public:
        LInsp sp, rp;
        LInsHashSet exprs;
        void clear(LInsp p);
    public:
        LoadFilter(LirWriter *out, avmplus::GC *gc)
            : LirWriter(out), exprs(gc) { }

        LInsp ins0(LOpcode);
        LInsp insLoad(LOpcode, LInsp base, LInsp disp);
        LInsp insStorei(LInsp v, LInsp b, int32_t d);
        LInsp insCall(const CallInfo *call, LInsp args[]);
    };	
}
#endif 
