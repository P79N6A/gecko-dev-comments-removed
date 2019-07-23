





































#ifndef __nanojit_LIR__
#define __nanojit_LIR__

namespace avmplus { class RegionTracker; }    

namespace nanojit
{
	#define is_trace_skip_tramp(op) ((op) <= LIR_tramp)
	
	enum LOpcode __msvc_only(: unsigned)	
	{
		
		LIR64	= 0x40,			
		
		
		LIR_trace = 2,	
		LIR_skip = 3,
		LIR_tramp	= 4,

		
		LIR_arg		= 9,
		LIR_param	= 10,
		LIR_st		= 11,
		LIR_ld		= 12,
		LIR_ref  	= 13, 
        LIR_sti     = 14,
		LIR_call	= 18,
			
		
		LIR_loop    = 19, 
		LIR_x		= 20, 

		

		LIR_feq		= 26,
		LIR_flt		= 27,
		LIR_fgt		= 28,
		LIR_fle		= 29,
		LIR_fge		= 30,
        LIR_cmov    = 31, 
		LIR_short   = 32,
		LIR_int		= 33,
		LIR_ldc     = 34, 
		LIR_2       = 35, 
		LIR_neg		= 36,					
		LIR_add		= 37,					
		LIR_sub		= 38,
		LIR_mul		= 39,
        LIR_callh   = 40,
		LIR_and		= 41,
		LIR_or		= 42,
		LIR_xor		= 43,
		LIR_not		= 44,
		LIR_lsh		= 45,
		LIR_rsh		= 46,	
		LIR_ush		= 47,	
        
		LIR_xt		= 48, 
		LIR_xf		= 49, 
		LIR_qlo		= 50,
		LIR_qhi		= 51,
		LIR_ldcb    = 52, 

        LIR_ov      = 53,
        LIR_cs      = 54,
        LIR_eq      = 55,
        
		LIR_lt      = 56, 
		LIR_gt      = 57, 
		LIR_le		= 58, 
		LIR_ge		= 59, 
		LIR_ult		= 60, 
		LIR_ugt		= 61, 
		LIR_ule		= 62, 
		LIR_uge		= 63, 

		


		LIR_stq		= LIR_st | LIR64,
		LIR_stqi	= LIR_sti | LIR64,
		LIR_quad    = LIR_int | LIR64,
		LIR_ldq		= LIR_ld    | LIR64,

		LIR_farg	= LIR_arg   | LIR64,
        LIR_fcall   = LIR_call  | LIR64,
		LIR_fneg	= LIR_neg  | LIR64,
		LIR_fadd	= LIR_add  | LIR64,
		LIR_fsub	= LIR_sub  | LIR64,
		LIR_fmul	= LIR_mul  | LIR64,
		LIR_fdiv	= 40        | LIR64,

		LIR_qjoin	= 41 | LIR64,
		LIR_i2f		= 42 | LIR64,
		LIR_u2f		= 43 | LIR64
	};

    struct SideExit;
    struct Page;
    struct CallInfo;

	
	
	class LIns
	{
		
		struct u_type
		{
			LOpcode			code:8;
			uint32_t		oprnd_1:8;  
			uint32_t		oprnd_2:8;  
			uint32_t		oprnd_3:8;	
		};

        struct sti_type
        {
			LOpcode			code:8;
			uint32_t		oprnd_1:8;  
			uint32_t		oprnd_2:8;  
			int32_t	    	disp:8;
        };

		
		struct c_type
		{
			LOpcode			code:8;
			uint32_t		imm8a:8;
			uint32_t		imm8b:8;  
			uint32_t		resv:8;  
		};

		
		struct i_type
		{
			LOpcode			code:8;
			int32_t			imm16:16;
			uint32_t		resv:8;  
		};

		
		struct t_type
		{
			LOpcode			code:8;
			int32_t			imm24:24;	
		};

		
		struct g_type
		{
			LOpcode			code:8;
			uint32_t		unused:16;
			uint32_t		resv:8;   
		};

		








		union 
		{
			u_type u;
			c_type c;
			i_type i;
			t_type t;
			g_type g;
            sti_type sti;
		};

		uint32_t reference(LIns*);

	public:
		LIns*		FASTCALL oprnd1() const;
		LIns*		FASTCALL oprnd2() const;
		LIns*		FASTCALL oprnd3() const;

		inline LOpcode	opcode() const	{ return u.code; }
		inline uint8_t	imm8()	 const	{ return c.imm8a; }
		inline uint8_t	imm8b()	 const	{ return c.imm8b; }
		inline int16_t	imm16()	 const	{ return i.imm16; }
		inline int32_t	imm24()	 const	{ return t.imm24; }
		inline int32_t	imm32()	 const	{ return *(int32_t*)(this-1); }
		inline uint8_t	resv()	 const  { return g.resv; }
        void*	payload() const;
        inline Page*	page()			{ return (Page*) alignTo(this,NJ_PAGE_SIZE); }

        inline int32_t  immdisp()const 
		{
            return (u.code&~LIR64) == LIR_sti ? sti.disp : oprnd3()->constval();
        }
    
		inline static bool sameop(LIns* a, LIns* b)
		{
			
			union { 
				uint32_t x; 
				u_type u;
			} tmp;
			tmp.x = *(uint32_t*)a ^ *(uint32_t*)b;
			return tmp.u.code == 0;
		}

		inline int32_t constval() const
		{
			NanoAssert(isconst());
			return isop(LIR_short) ? imm16() : imm32();
		}

		inline uint64_t constvalq() const
		{
			NanoAssert(isconstq());
		#ifdef AVMPLUS_UNALIGNED_ACCESS
			return *(const uint64_t*)(this-2);
		#else
			uint64_t tmp;
			const int32_t* src = (const int32_t*)(this-2);
			int32_t* dst = (int32_t*)&tmp;
			dst[0] = src[0];
			dst[1] = src[1];
			return tmp;
		#endif
		}
		
		inline double constvalf() const
		{
			NanoAssert(isconstq());
		#ifdef AVMPLUS_UNALIGNED_ACCESS
			return *(const double*)(this-2);
		#else
			union { uint64_t tmp; double tmpf; } u;
			const int32_t* src = (const int32_t*)(this-2);
			int32_t* dst = (int32_t*)&u;
			dst[0] = src[0];
			dst[1] = src[1];
			return u.tmpf;
		#endif
		}

		bool isCse(const CallInfo *functions) const;
		bool isop(LOpcode o) const { return u.code == o; }
		bool isQuad() const { return (u.code & LIR64) != 0; }
		bool isArg() const { return (u.code & ~LIR64)==LIR_arg || u.code == LIR_ref; }
		bool isCond() const;
		bool isCmp() const;
		bool isCall() const;
        bool isStore() const;
        bool isLoad() const;
		bool isGuard() const;
		bool isconst() const;
		bool isconstval(int32_t val) const;
		bool isconstq() const;

		void setimm8(int32_t a, int32_t b);
		void setimm16(int32_t i);
		void setimm24(int32_t i);
		void setresv(uint32_t resv);
		void initOpcode(LOpcode);
		void setOprnd1(LIns*);
		void setOprnd2(LIns*);
		void setOprnd3(LIns*);
        void setDisp(int8_t d);

        SideExit *exit();
	};
	typedef LIns*		LInsp;

	bool FASTCALL isCse(LOpcode v);
	bool FASTCALL isCmp(LOpcode v);
	bool FASTCALL isCond(LOpcode v);
	LIns* FASTCALL callArgN(LInsp i, uint32_t n);
	extern const uint8_t operandCount[];

	class Fragmento;	
	class LirFilter;
	struct CallInfo;

	class LirWriter
	{
	protected:
		LirWriter *out;
	public:
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
		virtual LInsp insGuard(LOpcode v, LIns *c, SideExit *x) {
			return out->insGuard(v, c, x);
		}
		virtual LInsp insImm8(LOpcode v, int32_t a, int32_t b) {
			return out->insImm8(v, a, b);
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
		virtual LInsp insStore(LIns* value, LIns* base, LIns* disp) {
			return out->insStore(value, base, disp);
		}
		virtual LInsp insStorei(LIns* value, LIns* base, int32_t d) {
			return isS8(d) ? out->insStorei(value, base, d)
				: out->insStore(value, base, insImm(d));
		}
		virtual LInsp insCall(int32_t fid, LInsp args[]) {
			return out->insCall(fid, args);
		}

		
	    LIns*		insLoadi(LIns *base, int disp);
	    LIns*		insLoad(LOpcode op, LIns *base, int disp);
	    LIns*		ins_choose(LIns* cond, LIns* iftrue, LIns* iffalse, bool);
	    LIns*		ins_eq0(LIns* oprnd1);
        LIns*       ins2i(LOpcode op, LIns *oprnd1, int32_t);
		LIns*		qjoin(LInsp lo, LInsp hi);
		LIns*		insImmPtr(const void *ptr);
	};

#ifdef NJ_VERBOSE
	extern const char* lirNames[];

	


    class LabelMap
    {
		LabelMap* parent;
		class Entry {
		public:
			Entry(int) : name(0), size(0), align(0) {}
			Entry(avmplus::String *n, size_t s, size_t a) : name(n),size(s),align(a) {}
			DRCWB(avmplus::String*) name;
			size_t size:29, align:3;
		};
        avmplus::SortedMap<const void*, Entry*, avmplus::LIST_GCObjects> names;
		bool addrs, pad[3];
		char buf[1000], *end;
        void formatAddr(const void *p, char *buf);
    public:
		AvmCore *core;
        LabelMap(AvmCore *, LabelMap* parent);
        void add(const void *p, size_t size, size_t align, const char *name);
		void add(const void *p, size_t size, size_t align, avmplus::String*);
		const char *dup(const char *);
		const char *format(const void *p);
    };

	class LirNameMap
	{
		class CountMap: public avmplus::SortedMap<int, int, avmplus::LIST_NonGCObjects> {
		public:
			CountMap(GC*gc) : avmplus::SortedMap<int, int, avmplus::LIST_NonGCObjects>(gc) {};
			int add(int i) {
				int c = 1;
				if (containsKey(i)) {
					c = 1+get(i);
				}
				put(i,c);
				return c;
			}
		} lircounts, funccounts;
		class Entry {
		public:
			Entry(int) : name(0) {}
			Entry(avmplus::String *n) : name(n) {}
			DRCWB(avmplus::String*) name;
		};
		avmplus::SortedMap<LInsp, Entry*, avmplus::LIST_GCObjects> names;
		const CallInfo *_functions;
		LabelMap *labels;
		void formatImm(int32_t c, char *buf);
	public:

		LirNameMap(GC *gc, const CallInfo *_functions, LabelMap *r) 
			: lircounts(gc),
			funccounts(gc),
			names(gc),
			_functions(_functions),
			labels(r)
		{}

		void addName(LInsp i, const char *s);
		void addName(LInsp i, avmplus::String *s);
		void copyName(LInsp i, const char *s, int suffix);
        const char *formatRef(LIns *ref);
		const char *formatIns(LInsp i);
	};

	class VerboseWriter : public LirWriter
	{
		avmplus::List<LInsp, avmplus::LIST_NonGCObjects> code;
		LirNameMap *names;
    public:
		VerboseWriter(GC *gc, LirWriter *out, LirNameMap* names) 
			: LirWriter(out), code(gc), names(names) 
		{}

		LInsp add(LInsp i) {
			code.add(i);
			return i;
		}

		void flush()
		{
			for (int j=0, n=code.size(); j < n; j++)
				printf("    %s\n",names->formatIns(code[j]));
			code.clear();
			printf("\n");
		}

		LIns* insGuard(LOpcode op, LInsp cond, SideExit *x) {
			LInsp i = add(out->insGuard(op,cond,x));
			if (i)
				flush();
			return i;
		}

		LIns* ins0(LOpcode v) {
			LInsp i = add(out->ins0(v));
			if (i)
				flush();
			return i;
		}

		LIns* ins1(LOpcode v, LInsp a) {
			return add(out->ins1(v, a));
		}
		LIns* ins2(LOpcode v, LInsp a, LInsp b) {
			return v == LIR_2 ? out->ins2(v,a,b) : add(out->ins2(v, a, b));
		}
		LIns* insCall(int32_t fid, LInsp args[]) {
			return add(out->insCall(fid, args));
		}
		LIns* insImm8(LOpcode v, int32_t a, int32_t b) {
			return add(out->insImm8(v, a, b));
		}
		LIns* insLoad(LOpcode v, LInsp base, LInsp disp) {
			return add(out->insLoad(v, base, disp));
		}
		LIns* insStore(LInsp v, LInsp b, LInsp d) {
			return add(out->insStore(v, b, d));
		}
		LIns* insStorei(LInsp v, LInsp b, int32_t d) {
			return add(out->insStorei(v, b, d));
		}
    };

#endif

	class ExprFilter: public LirWriter
	{
	public:
		ExprFilter(LirWriter *out) : LirWriter(out) {}
		LIns* ins1(LOpcode v, LIns* a);
	    LIns* ins2(LOpcode v, LIns* a, LIns* b);
		LIns* insGuard(LOpcode v, LIns *c, SideExit *x);
	};

	
	class LInsHashSet
	{
		
		
		
		static const uint32_t kInitialCap = 2048;	

		InsList m_list;
		uint32_t m_used;
		GC* m_gc;

		static uint32_t FASTCALL hashcode(LInsp i);
		uint32_t FASTCALL find(LInsp name, uint32_t hash, const InsList& list, uint32_t cap);
		static bool FASTCALL equals(LInsp a, LInsp b);
		void FASTCALL grow();

	public:

		LInsHashSet(GC* gc);
		LInsp find32(int32_t a, uint32_t &i);
		LInsp find64(uint64_t a, uint32_t &i);
		LInsp find1(LOpcode v, LInsp a, uint32_t &i);
		LInsp find2(LOpcode v, LInsp a, LInsp b, uint32_t &i);
		LInsp findcall(int32_t fid, uint32_t argc, LInsp args[], uint32_t &i);
		LInsp add(LInsp i, uint32_t k);
		void replace(LInsp i);

		static uint32_t FASTCALL hashimm(int32_t);
		static uint32_t FASTCALL hashimmq(uint64_t);
		static uint32_t FASTCALL hash1(LOpcode v, LInsp);
		static uint32_t FASTCALL hash2(LOpcode v, LInsp, LInsp);
		static uint32_t FASTCALL hashcall(int32_t fid, uint32_t argc, LInsp args[]);
	};

	class CseFilter: public LirWriter
	{
	public:
		LInsHashSet exprs;
		CseFilter(LirWriter *out, GC *gc);
	    LIns* insImm(int32_t imm);
	    LIns* insImmq(uint64_t q);
		LIns* ins1(LOpcode v, LInsp);
		LIns* ins2(LOpcode v, LInsp, LInsp);
		LIns* insLoad(LOpcode v, LInsp b, LInsp d);
		LIns* insCall(int32_t fid, LInsp args[]);
		LIns* insGuard(LOpcode op, LInsp cond, SideExit *x);
	};

	struct Page;
	class LirBuffer : public GCFinalizedObject
	{
		public:
			DWB(Fragmento*)		_frago;
		public:
			LirBuffer(Fragmento* frago, const CallInfo* functions);
			virtual ~LirBuffer();
			
			uint32_t            size();	
			void                clear();

			
			LInsp		next();
			LInsp		commit(uint32_t count);
			bool		addPage();
			bool		outOmem() { return _noMem != 0; }
			debug_only (void		validate() const;)
			verbose_only(DWB(LirNameMap*) names);

			
			struct 
			{
				uint32_t lir;		
				uint32_t pages;	
			}
			_stats;

			const CallInfo* _functions;
			
		private:
			Page*		pageAlloc();

			Page*				_start;		
			LInsp				_unused;	
			int					_noMem;		
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
			LInsp	insStore(LInsp o1, LInsp o2, LInsp o3);
			LInsp	insStorei(LInsp o1, LInsp o2, int32_t imm);
			LInsp	ins0(LOpcode op);
			LInsp	ins1(LOpcode op, LInsp o1);
			LInsp	ins2(LOpcode op, LInsp o1, LInsp o2);
			LInsp	insImm8(LOpcode op, int32_t a, int32_t b);
			LInsp	insImm(int32_t imm);
			LInsp	insImmq(uint64_t imm);
		    LInsp	insCall(int32_t fid, LInsp args[]);
			LInsp	insGuard(LOpcode op, LInsp cond, SideExit *x);

			
			LInsp	skip(size_t);
			LInsp	insFar(LOpcode op, int32_t imm);
			LInsp	ensureReferenceable(LInsp i, int32_t addedDistance);
			bool	ensureRoom(uint32_t count);
			bool	canReference(LInsp from, LInsp to) {
				return isU8(from-to-1);
			}
	};

	class LirFilter
	{
	public:
		LirFilter *in;
		LirFilter(LirFilter *in) : in(in) {}
		virtual ~LirFilter() {}

		virtual LInsp read() {
			return in->read();
		}
		virtual LInsp pos() {
			return in->pos();
		}

		LInsp previous() {
			read();
			return pos();
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
	};

    class Assembler;

    void compile(Assembler *assm, Fragment *frag);
    verbose_only( void printTracker(const char* s, avmplus::RegionTracker& trk, Assembler* assm); )
	verbose_only(void live(GC *gc, Assembler *assm, Fragment *frag);)

	class StoreFilter: public LirFilter
	{
		GC *gc;
		LInsp param0, sp, rp;
		avmplus::BitSet rstk, stk;
        int stop, rtop;
	public:
		StoreFilter(LirFilter *in, GC *gc, LInsp p0, LInsp sp, LInsp rp); 
		virtual ~StoreFilter() {}
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
}
#endif
