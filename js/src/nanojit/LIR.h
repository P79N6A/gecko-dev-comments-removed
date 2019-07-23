






































#ifndef __nanojit_LIR__
#define __nanojit_LIR__

namespace nanojit
{
    enum LOpcode
#if defined(_MSC_VER) && _MSC_VER >= 1400
#pragma warning(disable:4480) // nonstandard extension used: specifying underlying type for enum
          : unsigned
#endif
    {
#define OP___(op, number, repKind, retType, isCse) \
        LIR_##op = (number),
#include "LIRopcode.tbl"
        LIR_sentinel,
#undef OP___

#ifdef NANOJIT_64BIT
#  define PTR_SIZE(a,b)  b
#else
#  define PTR_SIZE(a,b)  a
#endif

        
        LIR_ldp     = PTR_SIZE(LIR_ld,     LIR_ldq),
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
        LIR_param   = PTR_SIZE(LIR_iparam, LIR_qparam),
        LIR_plive   = PTR_SIZE(LIR_live,   LIR_qlive),
        LIR_pret    = PTR_SIZE(LIR_ret,    LIR_qret)
    };

    
    
    NanoStaticAssert(LIR_eq + 1 == LIR_lt  &&
                     LIR_eq + 2 == LIR_gt  &&
                     LIR_eq + 3 == LIR_le  &&
                     LIR_eq + 4 == LIR_ge  &&
                     LIR_eq + 5 == LIR_ult &&
                     LIR_eq + 6 == LIR_ugt &&
                     LIR_eq + 7 == LIR_ule &&
                     LIR_eq + 8 == LIR_uge);
#ifdef NANOJIT_64BIT
    NanoStaticAssert(LIR_qeq + 1 == LIR_qlt  &&
                     LIR_qeq + 2 == LIR_qgt  &&
                     LIR_qeq + 3 == LIR_qle  &&
                     LIR_qeq + 4 == LIR_qge  &&
                     LIR_qeq + 5 == LIR_qult &&
                     LIR_qeq + 6 == LIR_qugt &&
                     LIR_qeq + 7 == LIR_qule &&
                     LIR_qeq + 8 == LIR_quge);
#endif
    NanoStaticAssert(LIR_feq + 1 == LIR_flt &&
                     LIR_feq + 2 == LIR_fgt &&
                     LIR_feq + 3 == LIR_fle &&
                     LIR_feq + 4 == LIR_fge);

    
    
    
    NanoStaticAssert((LIR_jt^1) == LIR_jf && (LIR_jf^1) == LIR_jt);

    NanoStaticAssert((LIR_xt^1) == LIR_xf && (LIR_xf^1) == LIR_xt);

    NanoStaticAssert((LIR_lt^1)  == LIR_gt  && (LIR_gt^1)  == LIR_lt);
    NanoStaticAssert((LIR_le^1)  == LIR_ge  && (LIR_ge^1)  == LIR_le);
    NanoStaticAssert((LIR_ult^1) == LIR_ugt && (LIR_ugt^1) == LIR_ult);
    NanoStaticAssert((LIR_ule^1) == LIR_uge && (LIR_uge^1) == LIR_ule);
    
#ifdef NANOJIT_64BIT
    NanoStaticAssert((LIR_qlt^1)  == LIR_qgt  && (LIR_qgt^1)  == LIR_qlt);
    NanoStaticAssert((LIR_qle^1)  == LIR_qge  && (LIR_qge^1)  == LIR_qle);
    NanoStaticAssert((LIR_qult^1) == LIR_qugt && (LIR_qugt^1) == LIR_qult);
    NanoStaticAssert((LIR_qule^1) == LIR_quge && (LIR_quge^1) == LIR_qule);
#endif
    
    NanoStaticAssert((LIR_flt^1) == LIR_fgt && (LIR_fgt^1) == LIR_flt);
    NanoStaticAssert((LIR_fle^1) == LIR_fge && (LIR_fge^1) == LIR_fle);


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
#ifdef NANOJIT_64BIT
        ARGSIZE_Q = 3,      
#endif
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

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    typedef uint8_t AccSet;

    
    
    
    
    static const AccSet ACC_READONLY = 1 << 0;      
    static const AccSet ACC_STACK    = 1 << 1;      
    static const AccSet ACC_OTHER    = 1 << 2;      

    
    
    
    
    
    
    
    
    
    static const AccSet ACC_NONE         = 0x0;
    static const AccSet ACC_ALL_WRITABLE = ACC_STACK | ACC_OTHER;
    static const AccSet ACC_ALL          = ACC_READONLY | ACC_ALL_WRITABLE;
    static const AccSet ACC_LOAD_ANY     = ACC_ALL;            
    static const AccSet ACC_STORE_ANY    = ACC_ALL_WRITABLE;   


    struct CallInfo
    {
        uintptr_t   _address;
        uint32_t    _argtypes:27;    
        AbiKind     _abi:3;
        uint8_t     _isPure:1;      
        AccSet      _storeAccSet;   
        verbose_only ( const char* _name; )

        uint32_t _count_args(uint32_t mask) const;
        
        uint32_t get_sizes(ArgSize* sizes) const;

        inline ArgSize returnType() const {
            return ArgSize(_argtypes & ARGSIZE_MASK_ANY);
        }

        
        
        
        inline ArgSize argType(uint32_t arg) const {
            return ArgSize((_argtypes >> (ARGSIZE_SHIFT * (arg+1))) & ARGSIZE_MASK_ANY);
        }

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

    
    extern const int8_t isCses[];       

    inline bool isCseOpcode(LOpcode op) {
        NanoAssert(isCses[op] != -1);   
        return isCses[op] == 1;
    }
    inline bool isRetOpcode(LOpcode op) {
        return 
#if defined NANOJIT_64BIT
            op == LIR_qret ||
#endif
            op == LIR_ret || op == LIR_fret;
    }
    inline bool isCmovOpcode(LOpcode op) {
        return 
#if defined NANOJIT_64BIT
            op == LIR_qcmov ||
#endif
            op == LIR_cmov;
    }
    inline bool isICmpOpcode(LOpcode op) {
        return LIR_eq <= op && op <= LIR_uge;
    }
    inline bool isSICmpOpcode(LOpcode op) {
        return LIR_eq <= op && op <= LIR_ge;
    }
    inline bool isUICmpOpcode(LOpcode op) {
        return LIR_eq == op || (LIR_ult <= op && op <= LIR_uge);
    }
#ifdef NANOJIT_64BIT
    inline bool isQCmpOpcode(LOpcode op) {
        return LIR_qeq <= op && op <= LIR_quge;
    }
    inline bool isSQCmpOpcode(LOpcode op) {
        return LIR_qeq <= op && op <= LIR_qge;
    }
    inline bool isUQCmpOpcode(LOpcode op) {
        return LIR_qeq == op || (LIR_qult <= op && op <= LIR_quge);
    }
#endif
    inline bool isFCmpOpcode(LOpcode op) {
        return LIR_feq <= op && op <= LIR_fge;
    }

    inline LOpcode invertCondJmpOpcode(LOpcode op) {
        NanoAssert(op == LIR_jt || op == LIR_jf);
        return LOpcode(op ^ 1);
    }
    inline LOpcode invertCondGuardOpcode(LOpcode op) {
        NanoAssert(op == LIR_xt || op == LIR_xf);
        return LOpcode(op ^ 1);
    }
    inline LOpcode invertICmpOpcode(LOpcode op) {
        NanoAssert(isICmpOpcode(op));
        return LOpcode(op ^ 1);
    }
#ifdef NANOJIT_64BIT
    inline LOpcode invertQCmpOpcode(LOpcode op) {
        NanoAssert(isQCmpOpcode(op));
        return LOpcode(op ^ 1);
    }
#endif
    inline LOpcode invertFCmpOpcode(LOpcode op) {
        NanoAssert(isFCmpOpcode(op));
        return LOpcode(op ^ 1);
    }

    inline LOpcode getCallOpcode(const CallInfo* ci) {
        LOpcode op = LIR_pcall;
        switch (ci->returnType()) {
        case ARGSIZE_NONE:  op = LIR_pcall; break;
        case ARGSIZE_I:
        case ARGSIZE_U:     op = LIR_icall; break;
        case ARGSIZE_F:     op = LIR_fcall; break;
#ifdef NANOJIT_64BIT
        case ARGSIZE_Q:     op = LIR_qcall; break;
#endif
        default:            NanoAssert(0);  break;
        }
        return op;
    }

    LOpcode f64arith_to_i32arith(LOpcode op);
#ifdef NANOJIT_64BIT
    LOpcode i32cmp_to_i64cmp(LOpcode op);
#endif

    
    extern const uint8_t repKinds[];

    enum LTy {
        LTy_Void,   
        LTy_I32,    
#ifdef NANOJIT_64BIT
        LTy_I64,    
#endif
        LTy_F64,    

        LTy_Ptr  = PTR_SIZE(LTy_I32, LTy_I64)   
    };

    
    extern const LTy retTypes[];

    inline RegisterMask rmask(Register r)
    {
        return RegisterMask(1) << r;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

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
        LRK_N64,
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
    class LInsN64;
    class LInsJtbl;

    class LIns
    {
    private:
        
        
        
        
        
        
        
        struct LastWord {
            uint32_t inReg:1;       
            Register reg:7;
            uint32_t inAr:1;        
            uint32_t arIndex:15;    

            LOpcode  opcode:8;      
        };

        union {
            LastWord lastWord;
            
            
            
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
        inline LInsN64* toLInsN64() const;
        inline LInsJtbl*toLInsJtbl()const;

        void staticSanityCheck();

    public:
        
        inline void initLInsOp0(LOpcode opcode);
        inline void initLInsOp1(LOpcode opcode, LIns* oprnd1);
        inline void initLInsOp2(LOpcode opcode, LIns* oprnd1, LIns* oprnd2);
        inline void initLInsOp3(LOpcode opcode, LIns* oprnd1, LIns* oprnd2, LIns* oprnd3);
        inline void initLInsLd(LOpcode opcode, LIns* val, int32_t d, AccSet accSet);
        inline void initLInsSti(LOpcode opcode, LIns* val, LIns* base, int32_t d, AccSet accSet);
        inline void initLInsSk(LIns* prevLIns);
        
        
        inline void initLInsC(LOpcode opcode, LIns** args, const CallInfo* ci);
        inline void initLInsP(int32_t arg, int32_t kind);
        inline void initLInsI(LOpcode opcode, int32_t imm32);
        inline void initLInsN64(LOpcode opcode, int64_t imm64);
        inline void initLInsJtbl(LIns* index, uint32_t size, LIns** table);

        LOpcode opcode() const { return lastWord.opcode; }

        
        
        
        
        
        
        void deprecated_markAsClear() {
            lastWord.inReg = 0;
            lastWord.inAr = 0;
        }
        bool deprecated_hasKnownReg() {
            NanoAssert(isUsed());
            return isInReg();
        }
        Register deprecated_getReg() {
            NanoAssert(isUsed());
            return ( isInReg() ? lastWord.reg : deprecated_UnknownReg );
        }
        uint32_t deprecated_getArIndex() {
            NanoAssert(isUsed());
            return ( isInAr() ? lastWord.arIndex : 0 );
        }

        
        bool isUsed() {
            return isInReg() || isInAr();
        }
        bool isInReg() {
            return lastWord.inReg;
        }
        bool isInRegMask(RegisterMask allow) {
            return isInReg() && (rmask(getReg()) & allow);
        }
        Register getReg() {
            NanoAssert(isInReg());
            return lastWord.reg;
        }
        void setReg(Register r) {
            lastWord.inReg = 1;
            lastWord.reg = r;
        }
        void clearReg() {
            lastWord.inReg = 0;
        }
        bool isInAr() {
            return lastWord.inAr;
        }
        uint32_t getArIndex() {
            NanoAssert(isInAr());
            return lastWord.arIndex;
        }
        void setArIndex(uint32_t arIndex) {
            lastWord.inAr = 1;
            lastWord.arIndex = arIndex;
        }
        void clearArIndex() {
            lastWord.inAr = 0;
        }

        
        inline LIns*    oprnd1() const;
        inline LIns*    oprnd2() const;
        inline LIns*    oprnd3() const;

        
        inline LIns*    getTarget() const;
        inline void     setTarget(LIns* label);

        
        inline GuardRecord* record() const;

        
        inline int32_t  disp() const;
        inline AccSet   accSet() const;

        
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
        bool isLInsN64() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_N64 == repKinds[opcode()];
        }
        bool isLInsJtbl() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_Jtbl == repKinds[opcode()];
        }

        
        bool isop(LOpcode o) const {
            return opcode() == o;
        }
        bool isRet() const {
            return isRetOpcode(opcode());
        }
        bool isLive() const {
            return isop(LIR_live) ||
#if defined NANOJIT_64BIT
                   isop(LIR_qlive) ||
#endif
                   isop(LIR_flive);
        }
        bool isCmp() const {
            LOpcode op = opcode();
            return isICmpOpcode(op) ||
#if defined NANOJIT_64BIT
                   isQCmpOpcode(op) ||
#endif
                   isFCmpOpcode(op);
        }
        bool isCall() const {
            return isop(LIR_icall) ||
#if defined NANOJIT_64BIT
                   isop(LIR_qcall) ||
#endif
                   isop(LIR_fcall);
        }
        bool isCmov() const {
            return isCmovOpcode(opcode());
        }
        bool isStore() const {
            return isLInsSti();
        }
        bool isLoad() const {
            return isLInsLd();
        }
        bool isGuard() const {
            return isop(LIR_x) || isop(LIR_xf) || isop(LIR_xt) ||
                   isop(LIR_xbarrier) || isop(LIR_xtbl) ||
                   isop(LIR_addxov) || isop(LIR_subxov) || isop(LIR_mulxov);
        }
        
        bool isconst() const {
            return isop(LIR_int);
        }
        
        
        bool isconstval(int32_t val) const {
            return isconst() && imm32()==val;
        }
        
        bool isconstq() const {
            return
#ifdef NANOJIT_64BIT
                isop(LIR_quad) || 
#endif
                isop(LIR_float);
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
            return isop(LIR_float);
        }

        bool isBranch() const {
            return isop(LIR_jt) || isop(LIR_jf) || isop(LIR_j) || isop(LIR_jtbl);
        }

        LTy retType() const {
            return retTypes[opcode()];
        }
        bool isVoid() const {
            return retType() == LTy_Void;
        }
        bool isI32() const {
            return retType() == LTy_I32;
        }
#ifdef NANOJIT_64BIT
        bool isI64() const {
            return retType() == LTy_I64;
        }
#endif
        bool isF64() const {
            return retType() == LTy_F64;
        }
        bool isN64() const {
            return 
#ifdef NANOJIT_64BIT
                isI64() ||       
#endif
                isF64();
        }
        bool isPtr() const {
#ifdef NANOJIT_64BIT
            return isI64();
#else
            return isI32();
#endif
        }

        
        
        
        
        
        
        bool isStmt() {
            NanoAssert(!isop(LIR_start) && !isop(LIR_skip));
            
            
            if (isCall())
                return !callInfo()->_isPure;
            else
                return isVoid();
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
    typedef SeqBuilder<char*> StringList;


    
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

        
        
        
        
        int16_t     disp;
        AccSet      accSet;

        LIns*       oprnd_1;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    class LInsSti
    {
    private:
        friend class LIns;

        int16_t     disp;
        AccSet      accSet;

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

    
    class LInsN64
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
    LInsN64* LIns::toLInsN64() const { return (LInsN64*)( uintptr_t(this+1) - sizeof(LInsN64) ); }
    LInsJtbl*LIns::toLInsJtbl()const { return (LInsJtbl*)(uintptr_t(this+1) - sizeof(LInsJtbl)); }

    void LIns::initLInsOp0(LOpcode opcode) {
        clearReg();
        clearArIndex();
        lastWord.opcode = opcode;
        NanoAssert(isLInsOp0());
    }
    void LIns::initLInsOp1(LOpcode opcode, LIns* oprnd1) {
        clearReg();
        clearArIndex();
        lastWord.opcode = opcode;
        toLInsOp1()->oprnd_1 = oprnd1;
        NanoAssert(isLInsOp1());
    }
    void LIns::initLInsOp2(LOpcode opcode, LIns* oprnd1, LIns* oprnd2) {
        clearReg();
        clearArIndex();
        lastWord.opcode = opcode;
        toLInsOp2()->oprnd_1 = oprnd1;
        toLInsOp2()->oprnd_2 = oprnd2;
        NanoAssert(isLInsOp2());
    }
    void LIns::initLInsOp3(LOpcode opcode, LIns* oprnd1, LIns* oprnd2, LIns* oprnd3) {
        clearReg();
        clearArIndex();
        lastWord.opcode = opcode;
        toLInsOp3()->oprnd_1 = oprnd1;
        toLInsOp3()->oprnd_2 = oprnd2;
        toLInsOp3()->oprnd_3 = oprnd3;
        NanoAssert(isLInsOp3());
    }
    void LIns::initLInsLd(LOpcode opcode, LIns* val, int32_t d, AccSet accSet) {
        clearReg();
        clearArIndex();
        lastWord.opcode = opcode;
        toLInsLd()->oprnd_1 = val;
        NanoAssert(d == int16_t(d));
        toLInsLd()->disp = int16_t(d);
        toLInsLd()->accSet = accSet;
        NanoAssert(isLInsLd());
    }
    void LIns::initLInsSti(LOpcode opcode, LIns* val, LIns* base, int32_t d, AccSet accSet) {
        clearReg();
        clearArIndex();
        lastWord.opcode = opcode;
        toLInsSti()->oprnd_1 = val;
        toLInsSti()->oprnd_2 = base;
        NanoAssert(d == int16_t(d));
        toLInsSti()->disp = int16_t(d);
        toLInsSti()->accSet = accSet;
        NanoAssert(isLInsSti());
    }
    void LIns::initLInsSk(LIns* prevLIns) {
        clearReg();
        clearArIndex();
        lastWord.opcode = LIR_skip;
        toLInsSk()->prevLIns = prevLIns;
        NanoAssert(isLInsSk());
    }
    void LIns::initLInsC(LOpcode opcode, LIns** args, const CallInfo* ci) {
        clearReg();
        clearArIndex();
        lastWord.opcode = opcode;
        toLInsC()->args = args;
        toLInsC()->ci = ci;
        NanoAssert(isLInsC());
    }
    void LIns::initLInsP(int32_t arg, int32_t kind) {
        clearReg();
        clearArIndex();
        lastWord.opcode = LIR_param;
        NanoAssert(isU8(arg) && isU8(kind));
        toLInsP()->arg = arg;
        toLInsP()->kind = kind;
        NanoAssert(isLInsP());
    }
    void LIns::initLInsI(LOpcode opcode, int32_t imm32) {
        clearReg();
        clearArIndex();
        lastWord.opcode = opcode;
        toLInsI()->imm32 = imm32;
        NanoAssert(isLInsI());
    }
    void LIns::initLInsN64(LOpcode opcode, int64_t imm64) {
        clearReg();
        clearArIndex();
        lastWord.opcode = opcode;
        toLInsN64()->imm64_0 = int32_t(imm64);
        toLInsN64()->imm64_1 = int32_t(imm64 >> 32);
        NanoAssert(isLInsN64());
    }
    void LIns::initLInsJtbl(LIns* index, uint32_t size, LIns** table) {
        clearReg();
        clearArIndex();
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
        switch (opcode()) {
        case LIR_x:
        case LIR_xt:
        case LIR_xf:
        case LIR_xtbl:
        case LIR_xbarrier:
            return (GuardRecord*)oprnd2();

        case LIR_addxov:
        case LIR_subxov:
        case LIR_mulxov:
            return (GuardRecord*)oprnd3();

        default:
            NanoAssert(0);
            return NULL;
        }
    }

    int32_t LIns::disp() const {
        if (isLInsSti()) {
            return toLInsSti()->disp;
        } else {
            NanoAssert(isLInsLd());
            return toLInsLd()->disp;
        }
    }

    AccSet LIns::accSet() const {
        if (isLInsSti()) {
            return toLInsSti()->accSet;
        } else {
            NanoAssert(isLInsLd());
            return toLInsLd()->accSet;
        }
    }

    LIns* LIns::prevLIns() const {
        NanoAssert(isLInsSk());
        return toLInsSk()->prevLIns;
    }

    inline uint8_t LIns::paramArg()  const { NanoAssert(isop(LIR_param)); return toLInsP()->arg; }
    inline uint8_t LIns::paramKind() const { NanoAssert(isop(LIR_param)); return toLInsP()->kind; }

    inline int32_t LIns::imm32()     const { NanoAssert(isconst());  return toLInsI()->imm32; }

    inline int32_t LIns::imm64_0()   const { NanoAssert(isconstq()); return toLInsN64()->imm64_0; }
    inline int32_t LIns::imm64_1()   const { NanoAssert(isconstq()); return toLInsN64()->imm64_1; }
    uint64_t       LIns::imm64()     const {
        NanoAssert(isconstq());
        return (uint64_t(toLInsN64()->imm64_1) << 32) | uint32_t(toLInsN64()->imm64_0);
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
        virtual LInsp insGuardXov(LOpcode v, LIns *a, LIns* b, GuardRecord *gr) {
            return out->insGuardXov(v, a, b, gr);
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
#ifdef NANOJIT_64BIT
        virtual LInsp insImmq(uint64_t imm) {
            return out->insImmq(imm);
        }
#endif
        virtual LInsp insImmf(double d) {
            return out->insImmf(d);
        }
        virtual LInsp insLoad(LOpcode op, LIns* base, int32_t d, AccSet accSet) {
            return out->insLoad(op, base, d, accSet);
        }
        virtual LInsp insStore(LOpcode op, LIns* value, LIns* base, int32_t d, AccSet accSet) {
            return out->insStore(op, value, base, d, accSet);
        }
        
        virtual LInsp insCall(const CallInfo *call, LInsp args[]) {
            return out->insCall(call, args);
        }
        virtual LInsp insAlloc(int32_t size) {
            NanoAssert(size != 0);
            return out->insAlloc(size);
        }
        virtual LInsp insJtbl(LIns* index, uint32_t size) {
            return out->insJtbl(index, size);
        }

        

        
        
        LIns* ins_choose(LIns* cond, LIns* iftrue, LIns* iffalse, bool use_cmov);

        
        LIns* ins_eq0(LIns* oprnd1);

        
        LIns* ins_peq0(LIns* oprnd1);

        
        
        LIns* ins2i(LOpcode op, LIns *oprnd1, int32_t);

#if NJ_SOFTFLOAT_SUPPORTED
        LIns* qjoin(LInsp lo, LInsp hi);
#endif
        LIns* insImmPtr(const void *ptr);
        LIns* insImmWord(intptr_t ptr);

        
        LIns* ins_i2p(LIns* intIns);
        LIns* ins_u2p(LIns* uintIns);

        
        LIns* insStorei(LIns* value, LIns* base, int32_t d, AccSet accSet);

        
        LIns* insLoad(LOpcode op, LIns* base, int32_t d) {
            return insLoad(op, base, d, ACC_LOAD_ANY);
        }
        LIns* insStore(LOpcode op, LIns* value, LIns* base, int32_t d) {
            return insStore(op, value, base, d, ACC_STORE_ANY);
        }
        LIns* insStorei(LIns* value, LIns* base, int32_t d) {
            return insStorei(value, base, d, ACC_STORE_ANY);
        }
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
        void formatImm(int32_t c, char *buf);
        void formatImmq(uint64_t c, char *buf);

    public:
        LabelMap *labels;
        LirNameMap(Allocator& alloc, LabelMap *lm)
            : alloc(alloc),
            lircounts(alloc),
            funccounts(alloc),
            names(alloc),
            labels(lm)
        {}

        void addName(LInsp i, const char *s);
        void copyName(LInsp i, const char *s, int suffix);
        char* formatAccSet(LInsp ins, bool isLoad, char* buf);
        const char *formatRef(LIns *ref);
        const char *formatIns(LInsp i);
        void formatGuard(LInsp i, char *buf);
        void formatGuardXov(LInsp i, char *buf);
    };


    class VerboseWriter : public LirWriter
    {
        InsList code;
        LirNameMap* names;
        LogControl* logc;
        const char* const prefix;
        bool const always_flush;
    public:
        VerboseWriter(Allocator& alloc, LirWriter *out,
                      LirNameMap* names, LogControl* logc, const char* prefix = "", bool always_flush = false)
            : LirWriter(out), code(alloc), names(names), logc(logc), prefix(prefix), always_flush(always_flush)
        {}

        LInsp add(LInsp i) {
            if (i) {
                code.add(i);
                if (always_flush)
                    flush();
            }
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
                    logc->printf("%s    %s\n",prefix,names->formatIns(p->head));
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

        LIns* insGuardXov(LOpcode op, LInsp a, LInsp b, GuardRecord *gr) {
            return add_flush(out->insGuardXov(op,a,b,gr));
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
        LIns* insLoad(LOpcode v, LInsp base, int32_t disp, AccSet accSet) {
            return add(out->insLoad(v, base, disp, accSet));
        }
        LIns* insStore(LOpcode op, LInsp v, LInsp b, int32_t d, AccSet accSet) {
            return add(out->insStore(op, v, b, d, accSet));
        }
        LIns* insAlloc(int32_t size) {
            return add(out->insAlloc(size));
        }
        LIns* insImm(int32_t imm) {
            return add(out->insImm(imm));
        }
#ifdef NANOJIT_64BIT
        LIns* insImmq(uint64_t imm) {
            return add(out->insImmq(imm));
        }
#endif
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
        LIns* insGuardXov(LOpcode, LIns* a, LIns* b, GuardRecord *);
        LIns* insBranch(LOpcode, LIns *cond, LIns *target);
        LIns* insLoad(LOpcode op, LInsp base, int32_t off, AccSet accSet);
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
        LInsLast = 7,
        
        LInsInvalid = 8
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
        static uint32_t hash1(LOpcode v, LInsp);
        static uint32_t hash2(LOpcode v, LInsp, LInsp);
        static uint32_t hash3(LOpcode v, LInsp, LInsp, LInsp);
        static uint32_t hashLoad(LOpcode v, LInsp, int32_t);
        static uint32_t hashCall(const CallInfo *call, uint32_t argc, LInsp args[]);

        
        
        uint32_t findImm(LInsp ins);
#ifdef NANOJIT_64BIT
        uint32_t findImmq(LInsp ins);
#endif
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
#ifdef NANOJIT_64BIT
        LInsp findImmq(uint64_t a, uint32_t &k);
#endif
        LInsp findImmf(uint64_t d, uint32_t &k);
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
#ifdef NANOJIT_64BIT
        LIns* insImmq(uint64_t q);
#endif
        LIns* insImmf(double d);
        LIns* ins0(LOpcode v);
        LIns* ins1(LOpcode v, LInsp);
        LIns* ins2(LOpcode v, LInsp, LInsp);
        LIns* ins3(LOpcode v, LInsp, LInsp, LInsp);
        LIns* insLoad(LOpcode op, LInsp cond, int32_t d, AccSet accSet);
        LIns* insCall(const CallInfo *call, LInsp args[]);
        LIns* insGuard(LOpcode op, LInsp cond, GuardRecord *gr);
        LIns* insGuardXov(LOpcode op, LInsp a, LInsp b, GuardRecord *gr);
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

            
            void        chunkAlloc();
            void        moveToNewChunk(uintptr_t addrOfLastLInsOnCurrentChunk);

            Allocator&  _allocator;
            uintptr_t   _unused;   
            uintptr_t   _limit;    
            size_t      _bytesAllocated;
    };

    class LirBufWriter : public LirWriter
    {
        LirBuffer*              _buf;        
        const Config&           _config;

        public:
            LirBufWriter(LirBuffer* buf, const Config& config)
                : LirWriter(0), _buf(buf), _config(config) {
            }

            
            LInsp   insLoad(LOpcode op, LInsp base, int32_t disp, AccSet accSet);
            LInsp   insStore(LOpcode op, LInsp o1, LInsp o2, int32_t disp, AccSet accSet);
            LInsp   ins0(LOpcode op);
            LInsp   ins1(LOpcode op, LInsp o1);
            LInsp   ins2(LOpcode op, LInsp o1, LInsp o2);
            LInsp   ins3(LOpcode op, LInsp o1, LInsp o2, LInsp o3);
            LInsp   insParam(int32_t i, int32_t kind);
            LInsp   insImm(int32_t imm);
#ifdef NANOJIT_64BIT
            LInsp   insImmq(uint64_t imm);
#endif
            LInsp   insImmf(double d);
            LInsp   insCall(const CallInfo *call, LInsp args[]);
            LInsp   insGuard(LOpcode op, LInsp cond, GuardRecord *gr);
            LInsp   insGuardXov(LOpcode op, LInsp a, LInsp b, GuardRecord *gr);
            LInsp   insBranch(LOpcode v, LInsp condition, LInsp to);
            LInsp   insAlloc(int32_t size);
            LInsp   insJtbl(LIns* index, uint32_t size);
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
        LirReader(LInsp i) : LirFilter(0), _i(i)
        {
            
            
            
            
            
            
            
            NanoAssert(i && !i->isop(LIR_skip));
        }
        virtual ~LirReader() {}

        
        
        LInsp read();

        
        LInsp pos() {
            return _i;
        }
    };

    verbose_only(void live(LirFilter* in, Allocator& alloc, Fragment* frag, LogControl*);)

    class StackFilter: public LirFilter
    {
        LInsp sp;
        LInsp rp;
        BitSet spStk;
        BitSet rpStk;
        int spTop;
        int rpTop;
        void getTops(LInsp br, int& spTop, int& rpTop);

    public:
        StackFilter(LirFilter *in, Allocator& alloc, LInsp sp, LInsp rp);
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
        LInsp insLoad(LOpcode op, LInsp base, int32_t disp, AccSet accSet);
        LInsp insStore(LOpcode op, LInsp value, LInsp base, int32_t disp, AccSet accSet);
        LInsp insCall(const CallInfo *call, LInsp args[]);
    };

    struct SoftFloatOps
    {
        const CallInfo* opmap[LIR_sentinel];
        SoftFloatOps();
    };

    extern const SoftFloatOps softFloatOps;

    
    
    class SoftFloatFilter: public LirWriter
    {
    public:
        static const CallInfo* opmap[LIR_sentinel];

        SoftFloatFilter(LirWriter *out);
        LIns *split(LIns *a);
        LIns *split(const CallInfo *call, LInsp args[]);
        LIns *fcall1(const CallInfo *call, LIns *a);
        LIns *fcall2(const CallInfo *call, LIns *a, LIns *b);
        LIns *fcmp(const CallInfo *call, LIns *a, LIns *b);
        LIns *ins1(LOpcode op, LIns *a);
        LIns *ins2(LOpcode op, LIns *a, LIns *b);
        LIns *insCall(const CallInfo *ci, LInsp args[]);
    };


#ifdef DEBUG
    
    
    
    
    
    
    
    
    
    
    
    
    class ValidateWriter : public LirWriter
    {
    private:
        const char* _whereInPipeline;

        const char* type2string(LTy type);
        void typeCheckArgs(LOpcode op, int nArgs, LTy formals[], LIns* args[]);
        void errorStructureShouldBe(LOpcode op, const char* argDesc, int argN, LIns* arg,
                                    const char* shouldBeDesc);
        void errorAccSetShould(const char* what, AccSet accSet, const char* shouldDesc);
        void checkLInsHasOpcode(LOpcode op, int argN, LIns* ins, LOpcode op2);
        void checkLInsIsACondOrConst(LOpcode op, int argN, LIns* ins);
        void checkLInsIsNull(LOpcode op, int argN, LIns* ins);

    public:
        ValidateWriter(LirWriter* out, const char* stageName);
        LIns* insLoad(LOpcode op, LIns* base, int32_t d, AccSet accSet);
        LIns* insStore(LOpcode op, LIns* value, LIns* base, int32_t d, AccSet accSet);
        LIns* ins0(LOpcode v);
        LIns* ins1(LOpcode v, LIns* a);
        LIns* ins2(LOpcode v, LIns* a, LIns* b);
        LIns* ins3(LOpcode v, LIns* a, LIns* b, LIns* c);
        LIns* insParam(int32_t arg, int32_t kind);
        LIns* insImm(int32_t imm);
#ifdef NANOJIT_64BIT
        LIns* insImmq(uint64_t imm);
#endif
        LIns* insImmf(double d);
        LIns* insCall(const CallInfo *call, LIns* args[]);
        LIns* insGuard(LOpcode v, LIns *c, GuardRecord *gr);
        LIns* insGuardXov(LOpcode v, LIns* a, LIns* b, GuardRecord* gr);
        LIns* insBranch(LOpcode v, LIns* condition, LIns* to);
        LIns* insAlloc(int32_t size);
        LIns* insJtbl(LIns* index, uint32_t size);
    };

    
    
    class ValidateReader: public LirFilter {
    public:
        ValidateReader(LirFilter* in);
        LIns* read();
    };
#endif

#ifdef NJ_VERBOSE
    




    class ReverseLister : public LirFilter
    {
        Allocator&   _alloc;
        LirNameMap*  _names;
        const char*  _title;
        StringList   _strs;
        LogControl*  _logc;
    public:
        ReverseLister(LirFilter* in, Allocator& alloc,
                      LirNameMap* names, LogControl* logc, const char* title)
            : LirFilter(in)
            , _alloc(alloc)
            , _names(names)
            , _title(title)
            , _strs(alloc)
            , _logc(logc)
        { }

        void finish();
        LInsp read();
    };
#endif

}
#endif 
