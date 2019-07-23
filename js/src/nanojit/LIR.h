






































#ifndef __nanojit_LIR__
#define __nanojit_LIR__










namespace nanojit
{
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

    
    extern const int8_t operandCount[];

    
    extern const uint8_t repKinds[];

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

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
        LRK_Jtbl,
        LRK_None    
    };

    class LInsOp0;
    class LInsOp1;
    class LInsOp2;
    class LInsOp3;
    class LInsLd;
    class LInsSti;
    class LInsSk;
    class LInsC;
    class LInsP;
    class LInsI;
    class LInsI64;
    class LInsJtbl;

    class LIns
    {
    private:
        
        
        union {
            Reservation lastWord;
            
            
            
            void* dummy;
        };

        
        inline LInsOp0* toLInsOp0() const;
        inline LInsOp1* toLInsOp1() const;
        inline LInsOp2* toLInsOp2() const;
        inline LInsOp3* toLInsOp3() const;
        inline LInsLd*  toLInsLd()  const;
        inline LInsSti* toLInsSti() const;
        inline LInsSk*  toLInsSk()  const;
        inline LInsC*   toLInsC()   const;
        inline LInsP*   toLInsP()   const;
        inline LInsI*   toLInsI()   const;
        inline LInsI64* toLInsI64() const;
        inline LInsJtbl*toLInsJtbl()const;

        void staticSanityCheck();

    public:
        
        inline void initLInsOp0(LOpcode opcode);
        inline void initLInsOp1(LOpcode opcode, LIns* oprnd1);
        inline void initLInsOp2(LOpcode opcode, LIns* oprnd1, LIns* oprnd2);
        inline void initLInsOp3(LOpcode opcode, LIns* oprnd1, LIns* oprnd2, LIns* oprnd3);
        inline void initLInsLd(LOpcode opcode, LIns* val, int32_t d);
        inline void initLInsSti(LOpcode opcode, LIns* val, LIns* base, int32_t d);
        inline void initLInsSk(LIns* prevLIns);
        
        
        inline void initLInsC(LOpcode opcode, LIns** args, const CallInfo* ci);
        inline void initLInsP(int32_t arg, int32_t kind);
        inline void initLInsI(LOpcode opcode, int32_t imm32);
        inline void initLInsI64(LOpcode opcode, int64_t imm64);
        inline void initLInsJtbl(LIns* index, uint32_t size, LIns** table);

        LOpcode opcode() const { return lastWord.opcode; }

        
        Reservation* resv() {
            return &lastWord;
        }
        
        Reservation* resvUsed() {
            Reservation* r = resv();
            NanoAssert(r->used);
            return r;
        }
        void markAsUsed() {
            lastWord.init();
        }
        void markAsClear() {
            lastWord.clear();
        }
        bool isUsed() {
            return lastWord.used;
        }
        bool hasKnownReg() {
            NanoAssert(isUsed());
            return getReg() != UnknownReg;
        }
        Register getReg() {
            NanoAssert(isUsed());
            return lastWord.reg;
        }
        void setReg(Register r) {
            NanoAssert(isUsed());
            lastWord.reg = r;
        }
        uint32_t getArIndex() {
            NanoAssert(isUsed());
            return lastWord.arIndex;
        }
        void setArIndex(uint32_t arIndex) {
            NanoAssert(isUsed());
            lastWord.arIndex = arIndex;
        }
        bool isUnusedOrHasUnknownReg() {
            return !isUsed() || !hasKnownReg();
        }

        
        inline LIns*    oprnd1() const;
        inline LIns*    oprnd2() const;
        inline LIns*    oprnd3() const;

        
        inline LIns*    getTarget() const;
        inline void     setTarget(LIns* label);

        
        inline GuardRecord* record() const;

        
        inline int32_t  disp() const;

        
        inline void*    payload()  const;
        inline LIns*    prevLIns() const;

        
        inline uint8_t  paramArg()  const;
        inline uint8_t  paramKind() const;

        
        inline int32_t  imm32() const;

        
        inline int32_t  imm64_0() const;
        inline int32_t  imm64_1() const;
        inline uint64_t imm64()   const;
        inline double   imm64f()  const;

        
        inline int32_t  size()    const;
        inline void     setSize(int32_t nbytes);

        
        inline LIns*    arg(uint32_t i)         const;
        inline uint32_t argc()                  const;
        inline LIns*    callArgN(uint32_t n)    const;
        inline const CallInfo* callInfo()       const;

        
        inline uint32_t getTableSize() const;
        inline LIns* getTarget(uint32_t index) const;
        inline void setTarget(uint32_t index, LIns* label) const;

        
        
        
        
        
        
        bool isLInsOp0() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_Op0 == repKinds[opcode()];
        }
        bool isLInsOp1() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_Op1 == repKinds[opcode()];
        }
        bool isLInsOp2() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_Op2 == repKinds[opcode()];
        }
        bool isLInsOp3() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_Op3 == repKinds[opcode()];
        }
        bool isLInsLd() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_Ld == repKinds[opcode()];
        }
        bool isLInsSti() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_Sti == repKinds[opcode()];
        }
        bool isLInsSk() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_Sk == repKinds[opcode()];
        }
        bool isLInsC() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_C == repKinds[opcode()];
        }
        bool isLInsP() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_P == repKinds[opcode()];
        }
        bool isLInsI() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_I == repKinds[opcode()];
        }
        bool isLInsI64() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_I64 == repKinds[opcode()];
        }
        bool isLInsJtbl() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_Jtbl == repKinds[opcode()];
        }

        
        bool isCse() const {
            return isCseOpcode(opcode()) || (isCall() && callInfo()->_cse);
        }
        bool isRet() const {
            return isRetOpcode(opcode());
        }
        bool isop(LOpcode o) const {
            return opcode() == o;
        }
        bool isQuad() const {
            LOpcode op = opcode();
#ifdef NANOJIT_64BIT
            
            return (!(op >= LIR_qeq && op <= LIR_quge) && (op & LIR64) != 0) ||
                   op == LIR_callh;
#else
            
            return (op & LIR64) != 0;
#endif
        }
        bool isCond() const {
            LOpcode op = opcode();
            return (op == LIR_ov) || isCmp();
        }
        bool isFloat() const;   
        bool isCmp() const {
            LOpcode op = opcode();
            return (op >= LIR_eq && op <= LIR_uge) ||
                   (op >= LIR_qeq && op <= LIR_quge) ||
                   (op >= LIR_feq && op <= LIR_fge);
        }
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
        
        bool isconst() const {
            return opcode() == LIR_int;
        }
        
        
        bool isconstval(int32_t val) const {
            return isconst() && imm32()==val;
        }
        
        bool isconstq() const {
            return opcode() == LIR_quad || opcode() == LIR_float;
        }
        
        bool isconstp() const
        {
#ifdef NANOJIT_64BIT
            return isconstq();
#else
            return isconst();
#endif
        }
        
        bool isconstf() const {
            return opcode() == LIR_float;
        }

        bool isBranch() const {
            return isop(LIR_jt) || isop(LIR_jf) || isop(LIR_j) || isop(LIR_jtbl);
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
                   isop(LIR_label) || isop(LIR_live) || isop(LIR_flive) ||
                   isop(LIR_regfence) ||
                   isRet();
        }

        inline void* constvalp() const
        {
        #ifdef NANOJIT_64BIT
            return (void*)imm64();
        #else
            return (void*)imm32();
        #endif
        }
    };

    typedef LIns* LInsp;
    typedef SeqBuilder<LIns*> InsList;


    
    class LInsOp0
    {
    private:
        friend class LIns;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    
    class LInsOp1
    {
    private:
        friend class LIns;

        LIns*       oprnd_1;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    
    class LInsOp2
    {
    private:
        friend class LIns;

        LIns*       oprnd_2;

        LIns*       oprnd_1;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    class LInsOp3
    {
    private:
        friend class LIns;

        LIns*       oprnd_3;

        LIns*       oprnd_2;

        LIns*       oprnd_1;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    class LInsLd
    {
    private:
        friend class LIns;

        int32_t     disp;

        LIns*       oprnd_1;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    class LInsSti
    {
    private:
        friend class LIns;

        int32_t     disp;

        LIns*       oprnd_2;

        LIns*       oprnd_1;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    class LInsSk
    {
    private:
        friend class LIns;

        LIns*       prevLIns;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    class LInsC
    {
    private:
        friend class LIns;

        
        
        
        LIns**      args;

        const CallInfo* ci;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    class LInsP
    {
    private:
        friend class LIns;

        uintptr_t   arg:8;
        uintptr_t   kind:8;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    class LInsI
    {
    private:
        friend class LIns;

        int32_t     imm32;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    class LInsI64
    {
    private:
        friend class LIns;

        int32_t     imm64_0;

        int32_t     imm64_1;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    
    class LInsJtbl
    {
    private:
        friend class LIns;

        uint32_t    size;     
        LIns**      table;    
        LIns*       oprnd_1;  

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; }
    };

    
    
    class LInsNone
    {
    };

    LInsOp0* LIns::toLInsOp0() const { return (LInsOp0*)( uintptr_t(this+1) - sizeof(LInsOp0) ); }
    LInsOp1* LIns::toLInsOp1() const { return (LInsOp1*)( uintptr_t(this+1) - sizeof(LInsOp1) ); }
    LInsOp2* LIns::toLInsOp2() const { return (LInsOp2*)( uintptr_t(this+1) - sizeof(LInsOp2) ); }
    LInsOp3* LIns::toLInsOp3() const { return (LInsOp3*)( uintptr_t(this+1) - sizeof(LInsOp3) ); }
    LInsLd*  LIns::toLInsLd()  const { return (LInsLd* )( uintptr_t(this+1) - sizeof(LInsLd ) ); }
    LInsSti* LIns::toLInsSti() const { return (LInsSti*)( uintptr_t(this+1) - sizeof(LInsSti) ); }
    LInsSk*  LIns::toLInsSk()  const { return (LInsSk* )( uintptr_t(this+1) - sizeof(LInsSk ) ); }
    LInsC*   LIns::toLInsC()   const { return (LInsC*  )( uintptr_t(this+1) - sizeof(LInsC  ) ); }
    LInsP*   LIns::toLInsP()   const { return (LInsP*  )( uintptr_t(this+1) - sizeof(LInsP  ) ); }
    LInsI*   LIns::toLInsI()   const { return (LInsI*  )( uintptr_t(this+1) - sizeof(LInsI  ) ); }
    LInsI64* LIns::toLInsI64() const { return (LInsI64*)( uintptr_t(this+1) - sizeof(LInsI64) ); }
    LInsJtbl*LIns::toLInsJtbl()const { return (LInsJtbl*)(uintptr_t(this+1) - sizeof(LInsJtbl)); }

    void LIns::initLInsOp0(LOpcode opcode) {
        lastWord.clear();
        lastWord.opcode = opcode;
        NanoAssert(isLInsOp0());
    }
    void LIns::initLInsOp1(LOpcode opcode, LIns* oprnd1) {
        lastWord.clear();
        lastWord.opcode = opcode;
        toLInsOp1()->oprnd_1 = oprnd1;
        NanoAssert(isLInsOp1());
    }
    void LIns::initLInsOp2(LOpcode opcode, LIns* oprnd1, LIns* oprnd2) {
        lastWord.clear();
        lastWord.opcode = opcode;
        toLInsOp2()->oprnd_1 = oprnd1;
        toLInsOp2()->oprnd_2 = oprnd2;
        NanoAssert(isLInsOp2());
    }
    void LIns::initLInsOp3(LOpcode opcode, LIns* oprnd1, LIns* oprnd2, LIns* oprnd3) {
        lastWord.clear();
        lastWord.opcode = opcode;
        toLInsOp3()->oprnd_1 = oprnd1;
        toLInsOp3()->oprnd_2 = oprnd2;
        toLInsOp3()->oprnd_3 = oprnd3;
        NanoAssert(isLInsOp3());
    }
    void LIns::initLInsLd(LOpcode opcode, LIns* val, int32_t d) {
        lastWord.clear();
        lastWord.opcode = opcode;
        toLInsLd()->oprnd_1 = val;
        toLInsLd()->disp = d;
        NanoAssert(isLInsLd());
    }
    void LIns::initLInsSti(LOpcode opcode, LIns* val, LIns* base, int32_t d) {
        lastWord.clear();
        lastWord.opcode = opcode;
        toLInsSti()->oprnd_1 = val;
        toLInsSti()->oprnd_2 = base;
        toLInsSti()->disp = d;
        NanoAssert(isLInsSti());
    }
    void LIns::initLInsSk(LIns* prevLIns) {
        lastWord.clear();
        lastWord.opcode = LIR_skip;
        toLInsSk()->prevLIns = prevLIns;
        NanoAssert(isLInsSk());
    }
    void LIns::initLInsC(LOpcode opcode, LIns** args, const CallInfo* ci) {
        lastWord.clear();
        lastWord.opcode = opcode;
        toLInsC()->args = args;
        toLInsC()->ci = ci;
        NanoAssert(isLInsC());
    }
    void LIns::initLInsP(int32_t arg, int32_t kind) {
        lastWord.clear();
        lastWord.opcode = LIR_param;
        NanoAssert(isU8(arg) && isU8(kind));
        toLInsP()->arg = arg;
        toLInsP()->kind = kind;
        NanoAssert(isLInsP());
    }
    void LIns::initLInsI(LOpcode opcode, int32_t imm32) {
        lastWord.clear();
        lastWord.opcode = opcode;
        toLInsI()->imm32 = imm32;
        NanoAssert(isLInsI());
    }
    void LIns::initLInsI64(LOpcode opcode, int64_t imm64) {
        lastWord.clear();
        lastWord.opcode = opcode;
        toLInsI64()->imm64_0 = int32_t(imm64);
        toLInsI64()->imm64_1 = int32_t(imm64 >> 32);
        NanoAssert(isLInsI64());
    }
    void LIns::initLInsJtbl(LIns* index, uint32_t size, LIns** table) {
        lastWord.clear();
        lastWord.opcode = LIR_jtbl;
        toLInsJtbl()->oprnd_1 = index;
        toLInsJtbl()->table = table;
        toLInsJtbl()->size = size;
        NanoAssert(isLInsJtbl());
    }

    LIns* LIns::oprnd1() const {
        NanoAssert(isLInsOp1() || isLInsOp2() || isLInsOp3() || isLInsLd() || isLInsSti() || isLInsJtbl());
        return toLInsOp2()->oprnd_1;
    }
    LIns* LIns::oprnd2() const {
        NanoAssert(isLInsOp2() || isLInsOp3() || isLInsSti());
        return toLInsOp2()->oprnd_2;
    }
    LIns* LIns::oprnd3() const {
        NanoAssert(isLInsOp3());
        return toLInsOp3()->oprnd_3;
    }

    LIns* LIns::getTarget() const {
        NanoAssert(isBranch() && !isop(LIR_jtbl));
        return oprnd2();
    }

    void LIns::setTarget(LIns* label) {
        NanoAssert(label && label->isop(LIR_label));
        NanoAssert(isBranch() && !isop(LIR_jtbl));
        toLInsOp2()->oprnd_2 = label;
    }

    LIns* LIns::getTarget(uint32_t index) const {
        NanoAssert(isop(LIR_jtbl));
        NanoAssert(index < toLInsJtbl()->size);
        return toLInsJtbl()->table[index];
    }

    void LIns::setTarget(uint32_t index, LIns* label) const {
        NanoAssert(label && label->isop(LIR_label));
        NanoAssert(isop(LIR_jtbl));
        NanoAssert(index < toLInsJtbl()->size);
        toLInsJtbl()->table[index] = label;
    }

    GuardRecord *LIns::record() const {
        NanoAssert(isGuard());
        return (GuardRecord*)oprnd2();
    }

    int32_t LIns::disp() const {
        if (isLInsSti()) {
            return toLInsSti()->disp;
        } else {
            NanoAssert(isLInsLd());
            return toLInsLd()->disp;
        }
    }

    void* LIns::payload() const {
        NanoAssert(isop(LIR_skip));
        
        
        return (void*) (uintptr_t(prevLIns()) + sizeof(LIns));
    }

    LIns* LIns::prevLIns() const {
        NanoAssert(isLInsSk());
        return toLInsSk()->prevLIns;
    }

    inline uint8_t LIns::paramArg()  const { NanoAssert(isop(LIR_param)); return toLInsP()->arg; }
    inline uint8_t LIns::paramKind() const { NanoAssert(isop(LIR_param)); return toLInsP()->kind; }

    inline int32_t LIns::imm32()     const { NanoAssert(isconst());  return toLInsI()->imm32; }

    inline int32_t LIns::imm64_0()   const { NanoAssert(isconstq()); return toLInsI64()->imm64_0; }
    inline int32_t LIns::imm64_1()   const { NanoAssert(isconstq()); return toLInsI64()->imm64_1; }
    uint64_t       LIns::imm64()     const {
        NanoAssert(isconstq());
        return (uint64_t(toLInsI64()->imm64_1) << 32) | uint32_t(toLInsI64()->imm64_0);
    }
    double         LIns::imm64f()    const {
        union {
            double f;
            uint64_t q;
        } u;
        u.q = imm64();
        return u.f;
    }

    int32_t LIns::size() const {
        NanoAssert(isop(LIR_alloc));
        return toLInsI()->imm32 << 2;
    }

    void LIns::setSize(int32_t nbytes) {
        NanoAssert(isop(LIR_alloc));
        NanoAssert(nbytes > 0);
        toLInsI()->imm32 = (nbytes+3)>>2; 
    }

    
    
    LIns* LIns::arg(uint32_t i) const
    {
        NanoAssert(isCall());
        NanoAssert(i < callInfo()->count_args());
        return toLInsC()->args[i];  
    }

    uint32_t LIns::argc() const {
        return callInfo()->count_args();
    }

    LIns* LIns::callArgN(uint32_t n) const
    {
        return arg(argc()-n-1);
    }

    const CallInfo* LIns::callInfo() const
    {
        NanoAssert(isCall());
        return toLInsC()->ci;
    }

    uint32_t LIns::getTableSize() const
    {
        NanoAssert(isLInsJtbl());
        return toLInsJtbl()->size;
    }

    class LirWriter
    {
    protected:
        LInsp insDisp(LOpcode op, LInsp base, int32_t& d) {
            if (!isValidDisplacement(op, d)) {
                base = ins2i(LIR_piadd, base, d);
                d = 0;
            }
            return base;
        }
    public:
        LirWriter *out;

        LirWriter(LirWriter* out)
            : out(out) {}
        virtual ~LirWriter() {}

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
        virtual LInsp insGuard(LOpcode v, LIns *c, GuardRecord *gr) {
            return out->insGuard(v, c, gr);
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
        virtual LInsp insJtbl(LIns* index, uint32_t size) {
            return out->insJtbl(index, size);
        }

        

        
        
        LIns*        ins_choose(LIns* cond, LIns* iftrue, LIns* iffalse, bool use_cmov);
        
        LIns*        ins_eq0(LIns* oprnd1);
        
        LIns*        ins_peq0(LIns* oprnd1);
        
        
        LIns*        ins2i(LOpcode op, LIns *oprnd1, int32_t);
        LIns*        qjoin(LInsp lo, LInsp hi);
        LIns*        insImmPtr(const void *ptr);
        LIns*        insImmWord(intptr_t ptr);
        
        LIns*        ins_i2p(LIns* intIns);
        LIns*        ins_u2p(LIns* uintIns);
    };


#ifdef NJ_VERBOSE
    extern const char* lirNames[];

    


    class LabelMap
    {
        Allocator& allocator;
        class Entry
        {
        public:
            Entry(int) : name(0), size(0), align(0) {}
            Entry(char *n, size_t s, size_t a) : name(n),size(s),align(a) {}
            char* name;
            size_t size:29, align:3;
        };
        TreeMap<const void*, Entry*> names;
        LogControl *logc;
        char buf[5000], *end;
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
            Entry(int) : name(0) {}
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

        LIns* insGuard(LOpcode op, LInsp cond, GuardRecord *gr) {
            return add_flush(out->insGuard(op,cond,gr));
        }

        LIns* insBranch(LOpcode v, LInsp condition, LInsp to) {
            return add_flush(out->insBranch(v, condition, to));
        }

        LIns* insJtbl(LIns* index, uint32_t size) {
            return add_flush(out->insJtbl(index, size));
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
        LIns* insGuard(LOpcode, LIns *cond, GuardRecord *);
        LIns* insBranch(LOpcode, LIns *cond, LIns *target);
        LIns* insLoad(LOpcode op, LInsp base, int32_t off);
    };

    enum LInsHashKind {
        
        
        
        LInsImm   = 0,
        LInsImmq  = 1,
        LInsImmf  = 2,
        LIns1     = 3,
        LIns2     = 4,
        LIns3     = 5,
        LInsLoad  = 6,
        LInsCall  = 7,

        LInsFirst = 0,
        LInsLast = 7
    };
    #define nextKind(kind)  LInsHashKind(kind+1)

    
    class LInsHashSet
    {
        
        
        
        static const uint32_t kInitialCap[LInsLast + 1];

        
        
        
        
        LInsp *m_list[LInsLast + 1];
        uint32_t m_cap[LInsLast + 1];
        uint32_t m_used[LInsLast + 1];
        typedef uint32_t (LInsHashSet::*find_t)(LInsp);
        find_t m_find[LInsLast + 1];
        Allocator& alloc;

        static uint32_t hashImm(int32_t);
        static uint32_t hashImmq(uint64_t);
        static uint32_t hashImmf(double);
        static uint32_t hash1(LOpcode v, LInsp);
        static uint32_t hash2(LOpcode v, LInsp, LInsp);
        static uint32_t hash3(LOpcode v, LInsp, LInsp, LInsp);
        static uint32_t hashLoad(LOpcode v, LInsp, int32_t);
        static uint32_t hashCall(const CallInfo *call, uint32_t argc, LInsp args[]);

        
        
        uint32_t findImm(LInsp ins);
        uint32_t findImmq(LInsp ins);
        uint32_t findImmf(LInsp ins);
        uint32_t find1(LInsp ins);
        uint32_t find2(LInsp ins);
        uint32_t find3(LInsp ins);
        uint32_t findLoad(LInsp ins);
        uint32_t findCall(LInsp ins);

        void grow(LInsHashKind kind);

    public:
        
        LInsHashSet(Allocator&, uint32_t kInitialCaps[]);

        
        LInsp findImm(int32_t a, uint32_t &k);
        LInsp findImmq(uint64_t a, uint32_t &k);
        LInsp findImmf(double d, uint32_t &k);
        LInsp find1(LOpcode v, LInsp a, uint32_t &k);
        LInsp find2(LOpcode v, LInsp a, LInsp b, uint32_t &k);
        LInsp find3(LOpcode v, LInsp a, LInsp b, LInsp c, uint32_t &k);
        LInsp findLoad(LOpcode v, LInsp a, int32_t b, uint32_t &k);
        LInsp findCall(const CallInfo *call, uint32_t argc, LInsp args[], uint32_t &k);

        
        LInsp add(LInsHashKind kind, LInsp ins, uint32_t k);

        void clear();
    };

    class CseFilter: public LirWriter
    {
    private:
        LInsHashSet* exprs;

    public:
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
        LIns* insGuard(LOpcode op, LInsp cond, GuardRecord *gr);
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

        protected:
            friend class LirBufWriter;

            


            static const size_t CHUNK_SZB = 8000;

            


            static const size_t MAX_LINS_SZB = CHUNK_SZB - sizeof(LInsSk);

            


            static const size_t MAX_SKIP_PAYLOAD_SZB = MAX_LINS_SZB - sizeof(LInsSk);

            
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
            LInsp    insGuard(LOpcode op, LInsp cond, GuardRecord *gr);
            LInsp    insBranch(LOpcode v, LInsp condition, LInsp to);
            LInsp    insAlloc(int32_t size);
            LInsp    insSkip(size_t);
            LInsp    insJtbl(LIns* index, uint32_t size);
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
    };

    class Assembler;

    void compile(Assembler *assm, Fragment *frag, Allocator& alloc verbose_only(, LabelMap*));
    verbose_only(void live(Allocator& alloc, Fragment* frag, LogControl*);)

    class StackFilter: public LirFilter
    {
        LirBuffer *lirbuf;
        LInsp sp;
        LInsp rp;
        BitSet spStk;
        BitSet rpStk;
        int spTop;
        int rpTop;
        void getTops(LInsp br, int& spTop, int& rpTop);

    public:
        StackFilter(LirFilter *in, Allocator& alloc, LirBuffer *lirbuf, LInsp sp, LInsp rp);
        bool ignoreStore(LInsp ins, int top, BitSet* stk);
        LInsp read();
    };

    
    class LoadFilter: public LirWriter
    {
    public:
        LInsp sp, rp;
        LInsHashSet* exprs;

        void clear(LInsp p);

    public:
        LoadFilter(LirWriter *out, Allocator& alloc)
            : LirWriter(out), sp(NULL), rp(NULL)
        { 
            uint32_t kInitialCaps[LInsLast + 1];
            kInitialCaps[LInsImm]   = 1;
            kInitialCaps[LInsImmq]  = 1;
            kInitialCaps[LInsImmf]  = 1;
            kInitialCaps[LIns1]     = 1;
            kInitialCaps[LIns2]     = 1;
            kInitialCaps[LIns3]     = 1;
            kInitialCaps[LInsLoad]  = 64;
            kInitialCaps[LInsCall]  = 1;
            exprs = new (alloc) LInsHashSet(alloc, kInitialCaps);
        }

        LInsp ins0(LOpcode);
        LInsp insLoad(LOpcode, LInsp base, int32_t disp);
        LInsp insStorei(LInsp v, LInsp b, int32_t d);
        LInsp insCall(const CallInfo *call, LInsp args[]);
    };

#ifdef DEBUG
    class SanityFilter : public LirWriter
    {
    public:
        SanityFilter(LirWriter* out) : LirWriter(out)
        { }
    public:
        LIns* ins1(LOpcode v, LIns* s0);
        LIns* ins2(LOpcode v, LIns* s0, LIns* s1);
        LIns* ins3(LOpcode v, LIns* s0, LIns* s1, LIns* s2);
    };
#endif
}
#endif 
