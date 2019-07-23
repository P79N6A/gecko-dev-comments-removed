






































#ifndef __nanojit_LIR__
#define __nanojit_LIR__










namespace nanojit
{
    using namespace MMgc;

    enum LOpcode
#if defined(_MSC_VER) && _MSC_VER >= 1400
#pragma warning(disable:4480) // nonstandard extension used: specifying underlying type for enum
          : unsigned
#endif
    {
        
        LIR64    = 0x40,            

#define OPDEF(op, number, args, repkind) \
        LIR_##op = (number),
#define OPDEF64(op, number, args, repkind) \
        LIR_##op = ((number) | LIR64),
#include "LIRopcode.tbl"
        LIR_sentinel,
#undef OPDEF
#undef OPDEF64

#ifdef NANOJIT_64BIT
#  define PTR_SIZE(a,b)  b
#else
#  define PTR_SIZE(a,b)  a
#endif

        
        LIR_ldp     = PTR_SIZE(LIR_ld,     LIR_ldq),
        LIR_ldcp    = PTR_SIZE(LIR_ldc,    LIR_ldqc),
        LIR_stpi    = PTR_SIZE(LIR_sti,    LIR_stqi),
        LIR_piadd   = PTR_SIZE(LIR_add,    LIR_qiadd),
        LIR_piand   = PTR_SIZE(LIR_and,    LIR_qiand),
        LIR_pilsh   = PTR_SIZE(LIR_lsh,    LIR_qilsh),
        LIR_pirsh   = PTR_SIZE(LIR_rsh,    LIR_qirsh),
        LIR_pursh   = PTR_SIZE(LIR_ush,    LIR_qursh),
        LIR_pcmov   = PTR_SIZE(LIR_cmov,   LIR_qcmov),
        LIR_pior    = PTR_SIZE(LIR_or,     LIR_qior),
        LIR_pxor    = PTR_SIZE(LIR_xor,    LIR_qxor),
        LIR_addp    = PTR_SIZE(LIR_iaddp,  LIR_qaddp),
        LIR_peq     = PTR_SIZE(LIR_eq,     LIR_qeq),
        LIR_plt     = PTR_SIZE(LIR_lt,     LIR_qlt),
        LIR_pgt     = PTR_SIZE(LIR_gt,     LIR_qgt),
        LIR_ple     = PTR_SIZE(LIR_le,     LIR_qle),
        LIR_pge     = PTR_SIZE(LIR_ge,     LIR_qge),
        LIR_pult    = PTR_SIZE(LIR_ult,    LIR_qult),
        LIR_pugt    = PTR_SIZE(LIR_ugt,    LIR_qugt),
        LIR_pule    = PTR_SIZE(LIR_ule,    LIR_qule),
        LIR_puge    = PTR_SIZE(LIR_uge,    LIR_quge),
        LIR_alloc   = PTR_SIZE(LIR_ialloc, LIR_qalloc),
        LIR_pcall   = PTR_SIZE(LIR_icall,  LIR_qcall),
        LIR_param   = PTR_SIZE(LIR_iparam, LIR_qparam)
    };

    struct GuardRecord;
    struct SideExit;

    enum AbiKind {
        ABI_FASTCALL,
        ABI_THISCALL,
        ABI_STDCALL,
        ABI_CDECL
    };

    enum ArgSize {
        ARGSIZE_NONE = 0,
        ARGSIZE_F = 1,      
        ARGSIZE_I = 2,      
        ARGSIZE_Q = 3,      
        ARGSIZE_U = 6,      
        ARGSIZE_MASK_ANY = 7,
        ARGSIZE_MASK_INT = 2,
        ARGSIZE_SHIFT = 3,

        
        ARGSIZE_P = PTR_SIZE(ARGSIZE_I, ARGSIZE_Q), 
        ARGSIZE_LO = ARGSIZE_I, 
        ARGSIZE_B = ARGSIZE_I, 
        ARGSIZE_V = ARGSIZE_NONE  
    };

    enum IndirectCall {
        CALL_INDIRECT = 0
    };

    struct CallInfo
    {
        uintptr_t   _address;
        uint32_t    _argtypes:27;    
        uint8_t     _cse:1;          
        uint8_t     _fold:1;         
        AbiKind     _abi:3;
        verbose_only ( const char* _name; )

        uint32_t _count_args(uint32_t mask) const;
        uint32_t get_sizes(ArgSize*) const;

        inline bool isIndirect() const {
            return _address < 256;
        }
        inline uint32_t count_args() const {
            return _count_args(ARGSIZE_MASK_ANY);
        }
        inline uint32_t count_iargs() const {
            return _count_args(ARGSIZE_MASK_INT);
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
        return op >= LIR_int && op <= LIR_uge;
    }
    inline bool isRetOpcode(LOpcode op) {
        return (op & ~LIR64) == LIR_ret;
    }

    
    
    
    struct Reservation
    {
        uint32_t arIndex:16;    
        Register reg:7;         
        uint32_t used:1;        
        LOpcode  opcode:8;

        inline void init() {
            reg = UnknownReg;
            arIndex = 0;
            used = 1;
        }

        inline void clear() {
            used = 0;
        }
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    enum LInsRepKind {
        
        LRK_Op0,
        LRK_Op1,
        LRK_Op2,
        LRK_Op3,
        LRK_Ld,
        LRK_Sti,
        LRK_Sk,
        LRK_C,
        LRK_P,
        LRK_I,
        LRK_I64,
        LRK_None    
    };

    class LIns;

    
    class LInsOp0
    {
    private:
        friend class LIns;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    
    class LInsOp1
    {
    private:
        friend class LIns;

        LIns*       oprnd_1;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    
    class LInsOp2
    {
    private:
        friend class LIns;

        LIns*       oprnd_2;

        LIns*       oprnd_1;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    class LInsOp3
    {
    private:
        friend class LIns;

        LIns*       oprnd_3;

        LIns*       oprnd_2;

        LIns*       oprnd_1;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    class LInsLd
    {
    private:
        friend class LIns;

        int32_t     disp;

        LIns*       oprnd_1;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    class LInsSti
    {
    private:
        friend class LIns;

        int32_t     disp;

        LIns*       oprnd_2;

        LIns*       oprnd_1;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    class LInsSk
    {
    private:
        friend class LIns;

        LIns*       prevLIns;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    class LInsC
    {
    private:
        friend class LIns;

        uintptr_t   argc:8;

        const CallInfo* ci;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    class LInsP
    {
    private:
        friend class LIns;

        uintptr_t   arg:8;
        uintptr_t   kind:8;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    class LInsI
    {
    private:
        friend class LIns;

        int32_t     imm32;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    class LInsI64
    {
    private:
        friend class LIns;

        int32_t     imm64_0;

        int32_t     imm64_1;

        void*       ins;

    public:
        LIns* getLIns() { return (LIns*)&ins; };
    };

    
    
    class LInsNone
    {
    };

    class LIns
    {
    private:
        
        
        union {
            Reservation lastWord;
            
            
            
            void* dummy;
        };

        
        LInsOp0* toLInsOp0() const { return (LInsOp0*)( uintptr_t(this+1) - sizeof(LInsOp0) ); }
        LInsOp1* toLInsOp1() const { return (LInsOp1*)( uintptr_t(this+1) - sizeof(LInsOp1) ); }
        LInsOp2* toLInsOp2() const { return (LInsOp2*)( uintptr_t(this+1) - sizeof(LInsOp2) ); }
        LInsOp3* toLInsOp3() const { return (LInsOp3*)( uintptr_t(this+1) - sizeof(LInsOp3) ); }
        LInsLd*  toLInsLd()  const { return (LInsLd* )( uintptr_t(this+1) - sizeof(LInsLd ) ); }
        LInsSti* toLInsSti() const { return (LInsSti*)( uintptr_t(this+1) - sizeof(LInsSti) ); }
        LInsSk*  toLInsSk()  const { return (LInsSk* )( uintptr_t(this+1) - sizeof(LInsSk ) ); }
        LInsC*   toLInsC()   const { return (LInsC*  )( uintptr_t(this+1) - sizeof(LInsC  ) ); }
        LInsP*   toLInsP()   const { return (LInsP*  )( uintptr_t(this+1) - sizeof(LInsP  ) ); }
        LInsI*   toLInsI()   const { return (LInsI*  )( uintptr_t(this+1) - sizeof(LInsI  ) ); }
        LInsI64* toLInsI64() const { return (LInsI64*)( uintptr_t(this+1) - sizeof(LInsI64) ); }

        
        
        void staticSanityCheck()
        {
            
            NanoStaticAssert(sizeof(LIns) == 1*sizeof(void*));

            
            NanoStaticAssert(sizeof(LInsOp0) == 1*sizeof(void*));
            NanoStaticAssert(sizeof(LInsOp1) == 2*sizeof(void*));
            NanoStaticAssert(sizeof(LInsOp2) == 3*sizeof(void*));
            NanoStaticAssert(sizeof(LInsOp3) == 4*sizeof(void*));
            NanoStaticAssert(sizeof(LInsLd)  == 3*sizeof(void*));
            NanoStaticAssert(sizeof(LInsSti) == 4*sizeof(void*));
            NanoStaticAssert(sizeof(LInsSk)  == 2*sizeof(void*));
            NanoStaticAssert(sizeof(LInsC)   == 3*sizeof(void*));
            NanoStaticAssert(sizeof(LInsP)   == 2*sizeof(void*));
            NanoStaticAssert(sizeof(LInsI)   == 2*sizeof(void*));
        #if defined NANOJIT_64BIT
            NanoStaticAssert(sizeof(LInsI64) == 2*sizeof(void*));
        #else
            NanoStaticAssert(sizeof(LInsI64) == 3*sizeof(void*));
        #endif

            
            
            NanoStaticAssert( (offsetof(LInsOp1, ins) - offsetof(LInsOp1, oprnd_1)) ==
                              (offsetof(LInsOp2, ins) - offsetof(LInsOp2, oprnd_1)) );
            NanoStaticAssert( (offsetof(LInsOp2, ins) - offsetof(LInsOp2, oprnd_1)) ==
                              (offsetof(LInsOp3, ins) - offsetof(LInsOp3, oprnd_1)) );
            NanoStaticAssert( (offsetof(LInsOp3, ins) - offsetof(LInsOp3, oprnd_1)) ==
                              (offsetof(LInsLd,  ins) - offsetof(LInsLd,  oprnd_1)) );
            NanoStaticAssert( (offsetof(LInsLd,  ins) - offsetof(LInsLd,  oprnd_1)) ==
                              (offsetof(LInsSti, ins) - offsetof(LInsSti, oprnd_1)) );

            
            
            NanoStaticAssert( (offsetof(LInsOp2, ins) - offsetof(LInsOp2, oprnd_2)) ==
                              (offsetof(LInsOp3, ins) - offsetof(LInsOp3, oprnd_2)) );
            NanoStaticAssert( (offsetof(LInsOp3, ins) - offsetof(LInsOp3, oprnd_2)) ==
                              (offsetof(LInsSti, ins) - offsetof(LInsSti, oprnd_2)) );
        }

    public:
        void initLInsOp0(LOpcode opcode) {
            lastWord.clear();
            lastWord.opcode = opcode;
            NanoAssert(isLInsOp0());
        }
        void initLInsOp1(LOpcode opcode, LIns* oprnd1) {
            lastWord.clear();
            lastWord.opcode = opcode;
            toLInsOp1()->oprnd_1 = oprnd1;
            NanoAssert(isLInsOp1());
        }
        void initLInsOp2(LOpcode opcode, LIns* oprnd1, LIns* oprnd2) {
            lastWord.clear();
            lastWord.opcode = opcode;
            toLInsOp2()->oprnd_1 = oprnd1;
            toLInsOp2()->oprnd_2 = oprnd2;
            NanoAssert(isLInsOp2());
        }
        void initLInsOp3(LOpcode opcode, LIns* oprnd1, LIns* oprnd2, LIns* oprnd3) {
            lastWord.clear();
            lastWord.opcode = opcode;
            toLInsOp3()->oprnd_1 = oprnd1;
            toLInsOp3()->oprnd_2 = oprnd2;
            toLInsOp3()->oprnd_3 = oprnd3;
            NanoAssert(isLInsOp3());
        }
        void initLInsLd(LOpcode opcode, LIns* val, int32_t d) {
            lastWord.clear();
            lastWord.opcode = opcode;
            toLInsLd()->oprnd_1 = val;
            toLInsLd()->disp = d;
            NanoAssert(isLInsLd());
        }
        void initLInsSti(LOpcode opcode, LIns* val, LIns* base, int32_t d) {
            lastWord.clear();
            lastWord.opcode = opcode;
            toLInsSti()->oprnd_1 = val;
            toLInsSti()->oprnd_2 = base;
            toLInsSti()->disp = d;
            NanoAssert(isLInsSti());
        }
        void initLInsSk(LIns* prevLIns) {
            lastWord.clear();
            lastWord.opcode = LIR_skip;
            toLInsSk()->prevLIns = prevLIns;
            NanoAssert(isLInsSk());
        }
        
        
        void initLInsC(LOpcode opcode, int32_t argc, const CallInfo* ci) {
            NanoAssert(isU8(argc));
            lastWord.clear();
            lastWord.opcode = opcode;
            toLInsC()->argc = argc;
            toLInsC()->ci = ci;
            NanoAssert(isLInsC());
        }
        void initLInsP(int32_t arg, int32_t kind) {
            lastWord.clear();
            lastWord.opcode = LIR_param;
            NanoAssert(isU8(arg) && isU8(kind));
            toLInsP()->arg = arg;
            toLInsP()->kind = kind;
            NanoAssert(isLInsP());
        }
        void initLInsI(LOpcode opcode, int32_t imm32) {
            lastWord.clear();
            lastWord.opcode = opcode;
            toLInsI()->imm32 = imm32;
            NanoAssert(isLInsI());
        }
        void initLInsI64(LOpcode opcode, int64_t imm64) {
            lastWord.clear();
            lastWord.opcode = opcode;
            toLInsI64()->imm64_0 = int32_t(imm64);
            toLInsI64()->imm64_1 = int32_t(imm64 >> 32);
            NanoAssert(isLInsI64());
        }

        LIns* oprnd1() const {
            NanoAssert(isLInsOp1() || isLInsOp2() || isLInsOp3() || isLInsLd() || isLInsSti());
            return toLInsOp2()->oprnd_1;
        }
        LIns* oprnd2() const {
            NanoAssert(isLInsOp2() || isLInsOp3() || isLInsSti());
            return toLInsOp2()->oprnd_2;
        }
        LIns* oprnd3() const {
            NanoAssert(isLInsOp3());
            return toLInsOp3()->oprnd_3;
        }

        LIns* prevLIns() const {
            NanoAssert(isLInsSk());
            return toLInsSk()->prevLIns;
        }

        inline LOpcode opcode()    const { return lastWord.opcode; }
        inline uint8_t paramArg()  const { NanoAssert(isop(LIR_param)); return toLInsP()->arg; }
        inline uint8_t paramKind() const { NanoAssert(isop(LIR_param)); return toLInsP()->kind; }
        inline int32_t imm32()     const { NanoAssert(isconst());  return toLInsI()->imm32; }
        inline int32_t imm64_0()   const { NanoAssert(isconstq()); return toLInsI64()->imm64_0; }
        inline int32_t imm64_1()   const { NanoAssert(isconstq()); return toLInsI64()->imm64_1; }
        uint64_t       imm64()     const;
        double         imm64f()    const;
        Reservation*   resv()            { return &lastWord; }
        
        Reservation*   resvUsed() {
            Reservation* r = resv();
            NanoAssert(r->used);
            return r;
        }
        void*          payload()   const;
        inline int32_t size()      const {
            NanoAssert(isop(LIR_alloc));
            return toLInsI()->imm32 << 2;
        }
        void           setSize(int32_t nbytes);

        LIns* arg(uint32_t i);

        inline int32_t disp() const
        {
            if (isLInsSti()) {
                return toLInsSti()->disp;
            } else {
                NanoAssert(isLInsLd());
                return toLInsLd()->disp;
            }
        }

        inline void* constvalp() const
        {
        #ifdef NANOJIT_64BIT
            return (void*)imm64();
        #else
            return (void*)imm32();
        #endif
        }

        bool isCse() const;
        bool isRet() const { return nanojit::isRetOpcode(opcode()); }
        bool isop(LOpcode o) const { return opcode() == o; }
        
        
        
        
        
        
        bool isLInsOp0() const;
        bool isLInsOp1() const;
        bool isLInsOp2() const;
        bool isLInsOp3() const;
        bool isLInsSti() const;
        bool isLInsLd()  const;
        bool isLInsSk()  const;
        bool isLInsC()   const;
        bool isLInsP()   const;
        bool isLInsI()   const;
        bool isLInsI64() const;
        bool isQuad() const;
        bool isCond() const;
        bool isFloat() const;
        bool isCmp() const;
        bool isCall() const {
            LOpcode op = opcode();
            return (op & ~LIR64) == LIR_icall || op == LIR_qcall;
        }
        bool isStore() const {
            LOpcode op = LOpcode(opcode() & ~LIR64);
            return op == LIR_sti;
        }
        bool isLoad() const {
            LOpcode op = opcode();
            return op == LIR_ldq  || op == LIR_ld || op == LIR_ldc ||
                   op == LIR_ldqc || op == LIR_ldcs || op == LIR_ldcb;
        }
        bool isGuard() const {
            LOpcode op = opcode();
            return op == LIR_x || op == LIR_xf || op == LIR_xt ||
                   op == LIR_xbarrier || op == LIR_xtbl;
        }
        
        bool isconst() const { return opcode() == LIR_int; }
        
        
        bool isconstval(int32_t val) const;
        
        bool isconstq() const;
        
        bool isconstp() const;
        
        bool isconstf() const;
        bool isBranch() const {
            return isop(LIR_jt) || isop(LIR_jf) || isop(LIR_j);
        }

        bool isPtr() {
#ifdef NANOJIT_64BIT
            return isQuad();
#else
            return !isQuad();
#endif
        }

        
        
        
        
        
        
        bool isStmt() {
            return isGuard() || isBranch() ||
                   (isCall() && !isCse()) ||
                   isStore() ||
                   isop(LIR_label) || isop(LIR_live) || isop(LIR_regfence) ||
                   isRet();
        }

        void setTarget(LIns* t);
        LIns* getTarget();

        GuardRecord *record();

        inline uint32_t argc() const {
            NanoAssert(isCall());
            return toLInsC()->argc;
        }
        const CallInfo *callInfo() const;
    };

    typedef LIns* LInsp;
    typedef SeqBuilder<LIns*> InsList;

    inline LIns* callArgN(LInsp i, uint32_t n) {
        return i->arg(i->argc()-n-1);
    }

    extern const int8_t operandCount[];

    
    class LirWriter : public GCObject
    {
    public:
        LirWriter *out;

        virtual ~LirWriter() {}
        LirWriter(LirWriter* out)
            : out(out) {}

        virtual LInsp ins0(LOpcode v) {
            return out->ins0(v);
        }
        virtual LInsp ins1(LOpcode v, LIns* a) {
            return out->ins1(v, a);
        }
        virtual LInsp ins2(LOpcode v, LIns* a, LIns* b) {
            return out->ins2(v, a, b);
        }
        virtual LInsp ins3(LOpcode v, LIns* a, LIns* b, LIns* c) {
            return out->ins3(v, a, b, c);
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
        virtual LInsp insImmf(double d) {
            return out->insImmf(d);
        }
        virtual LInsp insLoad(LOpcode op, LIns* base, int32_t d) {
            return out->insLoad(op, base, d);
        }
        virtual LInsp insStorei(LIns* value, LIns* base, int32_t d) {
            return out->insStorei(value, base, d);
        }
        virtual LInsp insCall(const CallInfo *call, LInsp args[]) {
            return out->insCall(call, args);
        }
        virtual LInsp insAlloc(int32_t size) {
            NanoAssert(size != 0);
            return out->insAlloc(size);
        }
        virtual LInsp insSkip(size_t size) {
            return out->insSkip(size);
        }

        

        
        
        LIns*        ins_choose(LIns* cond, LIns* iftrue, LIns* iffalse);
        
        LIns*        ins_eq0(LIns* oprnd1);
        
        
        LIns*        ins2i(LOpcode op, LIns *oprnd1, int32_t);
        LIns*        qjoin(LInsp lo, LInsp hi);
        LIns*        insImmPtr(const void *ptr);
    };


#ifdef NJ_VERBOSE
    extern const char* lirNames[];

    


    class LabelMap
    {
        Allocator& allocator;
        class Entry
        {
        public:
            Entry(char *n, size_t s, size_t a) : name(n),size(s),align(a) {}
            char* name;
            size_t size:29, align:3;
        };
        TreeMap<const void*, Entry*> names;
        LogControl *logc;
        char buf[1000], *end;
        void formatAddr(const void *p, char *buf);
    public:
        LabelMap(Allocator& allocator, LogControl* logc);
        void add(const void *p, size_t size, size_t align, const char *name);
        const char *dup(const char *);
        const char *format(const void *p);
    };

    class LirNameMap
    {
        Allocator& alloc;

        template <class Key>
        class CountMap: public HashMap<Key, int> {
        public:
            CountMap(Allocator& alloc) : HashMap<Key, int>(alloc) {}
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

        class Entry
        {
        public:
            Entry(char* n) : name(n) {}
            char* name;
        };
        HashMap<LInsp, Entry*> names;
        LabelMap *labels;
        void formatImm(int32_t c, char *buf);
    public:

        LirNameMap(Allocator& alloc, LabelMap *lm)
            : alloc(alloc),
            lircounts(alloc),
            funccounts(alloc),
            names(alloc),
            labels(lm)
        {}

        void addName(LInsp i, const char *s);
        void copyName(LInsp i, const char *s, int suffix);
        const char *formatRef(LIns *ref);
        const char *formatIns(LInsp i);
        void formatGuard(LInsp i, char *buf);
    };


    class VerboseWriter : public LirWriter
    {
        InsList code;
        LirNameMap* names;
        LogControl* logc;
    public:
        VerboseWriter(Allocator& alloc, LirWriter *out,
                      LirNameMap* names, LogControl* logc)
            : LirWriter(out), code(alloc), names(names), logc(logc)
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
            if (!code.isEmpty()) {
                int32_t count = 0;
                for (Seq<LIns*>* p = code.get(); p != NULL; p = p->tail) {
                    logc->printf("    %s\n",names->formatIns(p->head));
                    count++;
                }
                code.clear();
                if (count > 1)
                    logc->printf("\n");
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
            return add(out->ins2(v, a, b));
        }
        LIns* ins3(LOpcode v, LInsp a, LInsp b, LInsp c) {
            return add(out->ins3(v, a, b, c));
        }
        LIns* insCall(const CallInfo *call, LInsp args[]) {
            return add_flush(out->insCall(call, args));
        }
        LIns* insParam(int32_t i, int32_t kind) {
            return add(out->insParam(i, kind));
        }
        LIns* insLoad(LOpcode v, LInsp base, int32_t disp) {
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
        LIns* insImmf(double d) {
            return add(out->insImmf(d));
        }
    };

#endif

    class ExprFilter: public LirWriter
    {
    public:
        ExprFilter(LirWriter *out) : LirWriter(out) {}
        LIns* ins1(LOpcode v, LIns* a);
        LIns* ins2(LOpcode v, LIns* a, LIns* b);
        LIns* ins3(LOpcode v, LIns* a, LIns* b, LIns* c);
        LIns* insGuard(LOpcode, LIns *cond, LIns *);
        LIns* insBranch(LOpcode, LIns *cond, LIns *target);
    };

    
    class LInsHashSet
    {
        
        
        
        static const uint32_t kInitialCap = 64;

        LInsp *m_list;
        uint32_t m_used, m_cap;
        Allocator& alloc;

        static uint32_t hashcode(LInsp i);
        uint32_t find(LInsp name, uint32_t hash, const LInsp *list, uint32_t cap);
        static bool equals(LInsp a, LInsp b);
        void grow();

    public:

        LInsHashSet(Allocator&);
        LInsp find32(int32_t a, uint32_t &i);
        LInsp find64(LOpcode v, uint64_t a, uint32_t &i);
        LInsp find1(LOpcode v, LInsp a, uint32_t &i);
        LInsp find2(LOpcode v, LInsp a, LInsp b, uint32_t &i);
        LInsp find3(LOpcode v, LInsp a, LInsp b, LInsp c, uint32_t &i);
        LInsp findLoad(LOpcode v, LInsp a, int32_t b, uint32_t &i);
        LInsp findcall(const CallInfo *call, uint32_t argc, LInsp args[], uint32_t &i);
        LInsp add(LInsp i, uint32_t k);
        void clear();

        static uint32_t hashimm(int32_t);
        static uint32_t hashimmq(uint64_t);
        static uint32_t hash1(LOpcode v, LInsp);
        static uint32_t hash2(LOpcode v, LInsp, LInsp);
        static uint32_t hash3(LOpcode v, LInsp, LInsp, LInsp);
        static uint32_t hashLoad(LOpcode v, LInsp, int32_t);
        static uint32_t hashcall(const CallInfo *call, uint32_t argc, LInsp args[]);
    };

    class CseFilter: public LirWriter
    {
    public:
        LInsHashSet exprs;
        CseFilter(LirWriter *out, Allocator&);
        LIns* insImm(int32_t imm);
        LIns* insImmq(uint64_t q);
        LIns* insImmf(double d);
        LIns* ins0(LOpcode v);
        LIns* ins1(LOpcode v, LInsp);
        LIns* ins2(LOpcode v, LInsp, LInsp);
        LIns* ins3(LOpcode v, LInsp, LInsp, LInsp);
        LIns* insLoad(LOpcode op, LInsp cond, int32_t d);
        LIns* insCall(const CallInfo *call, LInsp args[]);
        LIns* insGuard(LOpcode op, LInsp cond, LIns *x);
    };

    class LirBuffer
    {
        public:
            LirBuffer(Allocator& alloc);
            void        clear();
            uintptr_t   makeRoom(size_t szB);   

            debug_only (void validate() const;)
            verbose_only(LirNameMap* names;)

            int32_t insCount();
            size_t  byteCount();

            
            struct
            {
                uint32_t lir;    
            }
            _stats;

            AbiKind abi;
            LInsp state,param1,sp,rp;
            LInsp savedRegs[NumSavedRegs];

            


            static const size_t CHUNK_SZB = 8000;

            


            static const size_t MAX_LINS_SZB = CHUNK_SZB - sizeof(LInsSk);

            


            static const size_t MAX_SKIP_PAYLOAD_SZB = MAX_LINS_SZB - sizeof(LInsSk);

        protected:
            friend class LirBufWriter;

            
            void        chunkAlloc();
            void        moveToNewChunk(uintptr_t addrOfLastLInsOnCurrentChunk);

            Allocator&  _allocator;
            uintptr_t   _unused;   
            uintptr_t   _limit;    
            size_t      _bytesAllocated;
    };

    class LirBufWriter : public LirWriter
    {
        LirBuffer*    _buf;        

        public:
            LirBufWriter(LirBuffer* buf)
                : LirWriter(0), _buf(buf) {
            }

            
            LInsp    insLoad(LOpcode op, LInsp base, int32_t disp);
            LInsp    insStorei(LInsp o1, LInsp o2, int32_t disp);
            LInsp    ins0(LOpcode op);
            LInsp    ins1(LOpcode op, LInsp o1);
            LInsp    ins2(LOpcode op, LInsp o1, LInsp o2);
            LInsp    ins3(LOpcode op, LInsp o1, LInsp o2, LInsp o3);
            LInsp    insParam(int32_t i, int32_t kind);
            LInsp    insImm(int32_t imm);
            LInsp    insImmq(uint64_t imm);
            LInsp    insImmf(double d);
            LInsp    insCall(const CallInfo *call, LInsp args[]);
            LInsp    insGuard(LOpcode op, LInsp cond, LIns *x);
            LInsp    insBranch(LOpcode v, LInsp condition, LInsp to);
            LInsp   insAlloc(int32_t size);
            LInsp   insSkip(size_t);
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
        LirReader(LInsp i) : LirFilter(0), _i(i) {
            NanoAssert(_i);
        }
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

    void compile(Assembler *assm, Fragment *frag, Allocator& alloc verbose_only(, LabelMap*));
    verbose_only(void live(Allocator& alloc, Fragment *frag, LirBuffer *lirbuf);)

    class StackFilter: public LirFilter
    {
        LirBuffer *lirbuf;
        LInsp sp;
        BitSet stk;
        int top;
        int getTop(LInsp br);
    public:
        StackFilter(LirFilter *in, Allocator& alloc, LirBuffer *lirbuf, LInsp sp);
        LInsp read();
    };

    
    class LoadFilter: public LirWriter
    {
    public:
        LInsp sp, rp;
        LInsHashSet exprs;

        void clear(LInsp p);
    public:
        LoadFilter(LirWriter *out, Allocator& alloc)
            : LirWriter(out), sp(NULL), rp(NULL), exprs(alloc)
        { }

        LInsp ins0(LOpcode);
        LInsp insLoad(LOpcode, LInsp base, int32_t disp);
        LInsp insStorei(LInsp v, LInsp b, int32_t d);
        LInsp insCall(const CallInfo *call, LInsp args[]);
    };
}
#endif 
