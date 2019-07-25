






































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

        

        LIR_paramp  = PTR_SIZE(LIR_parami,  LIR_paramq),

        LIR_retp    = PTR_SIZE(LIR_reti,    LIR_retq),

        LIR_livep   = PTR_SIZE(LIR_livei,   LIR_liveq),

        LIR_ldp     = PTR_SIZE(LIR_ldi,     LIR_ldq),

        LIR_stp     = PTR_SIZE(LIR_sti,     LIR_stq),

        LIR_callp   = PTR_SIZE(LIR_calli,   LIR_callq),

        LIR_eqp     = PTR_SIZE(LIR_eqi,     LIR_eqq),
        LIR_ltp     = PTR_SIZE(LIR_lti,     LIR_ltq),
        LIR_gtp     = PTR_SIZE(LIR_gti,     LIR_gtq),
        LIR_lep     = PTR_SIZE(LIR_lei,     LIR_leq),
        LIR_gep     = PTR_SIZE(LIR_gei,     LIR_geq),
        LIR_ltup    = PTR_SIZE(LIR_ltui,    LIR_ltuq),
        LIR_gtup    = PTR_SIZE(LIR_gtui,    LIR_gtuq),
        LIR_leup    = PTR_SIZE(LIR_leui,    LIR_leuq),
        LIR_geup    = PTR_SIZE(LIR_geui,    LIR_geuq),

        LIR_addp    = PTR_SIZE(LIR_addi,    LIR_addq),
        LIR_subp    = PTR_SIZE(LIR_subi,    LIR_subq),
        LIR_addjovp = PTR_SIZE(LIR_addjovi, LIR_addjovq),

        LIR_andp    = PTR_SIZE(LIR_andi,    LIR_andq),
        LIR_orp     = PTR_SIZE(LIR_ori,     LIR_orq),
        LIR_xorp    = PTR_SIZE(LIR_xori,    LIR_xorq),

        LIR_lshp    = PTR_SIZE(LIR_lshi,    LIR_lshq),
        LIR_rshp    = PTR_SIZE(LIR_rshi,    LIR_rshq),
        LIR_rshup   = PTR_SIZE(LIR_rshui,   LIR_rshuq),

        LIR_cmovp   = PTR_SIZE(LIR_cmovi,   LIR_cmovq)
    };

    
    
    NanoStaticAssert(LIR_eqi + 1 == LIR_lti  &&
                     LIR_eqi + 2 == LIR_gti  &&
                     LIR_eqi + 3 == LIR_lei  &&
                     LIR_eqi + 4 == LIR_gei  &&
                     LIR_eqi + 5 == LIR_ltui &&
                     LIR_eqi + 6 == LIR_gtui &&
                     LIR_eqi + 7 == LIR_leui &&
                     LIR_eqi + 8 == LIR_geui);
#ifdef NANOJIT_64BIT
    NanoStaticAssert(LIR_eqq + 1 == LIR_ltq  &&
                     LIR_eqq + 2 == LIR_gtq  &&
                     LIR_eqq + 3 == LIR_leq  &&
                     LIR_eqq + 4 == LIR_geq  &&
                     LIR_eqq + 5 == LIR_ltuq &&
                     LIR_eqq + 6 == LIR_gtuq &&
                     LIR_eqq + 7 == LIR_leuq &&
                     LIR_eqq + 8 == LIR_geuq);
#endif
    NanoStaticAssert(LIR_eqd + 1 == LIR_ltd &&
                     LIR_eqd + 2 == LIR_gtd &&
                     LIR_eqd + 3 == LIR_led &&
                     LIR_eqd + 4 == LIR_ged);

    
    
    
    NanoStaticAssert((LIR_jt^1) == LIR_jf && (LIR_jf^1) == LIR_jt);

    NanoStaticAssert((LIR_xt^1) == LIR_xf && (LIR_xf^1) == LIR_xt);

    NanoStaticAssert((LIR_lti^1)  == LIR_gti  && (LIR_gti^1)  == LIR_lti);
    NanoStaticAssert((LIR_lei^1)  == LIR_gei  && (LIR_gei^1)  == LIR_lei);
    NanoStaticAssert((LIR_ltui^1) == LIR_gtui && (LIR_gtui^1) == LIR_ltui);
    NanoStaticAssert((LIR_leui^1) == LIR_geui && (LIR_geui^1) == LIR_leui);

#ifdef NANOJIT_64BIT
    NanoStaticAssert((LIR_ltq^1)  == LIR_gtq  && (LIR_gtq^1)  == LIR_ltq);
    NanoStaticAssert((LIR_leq^1)  == LIR_geq  && (LIR_geq^1)  == LIR_leq);
    NanoStaticAssert((LIR_ltuq^1) == LIR_gtuq && (LIR_gtuq^1) == LIR_ltuq);
    NanoStaticAssert((LIR_leuq^1) == LIR_geuq && (LIR_geuq^1) == LIR_leuq);
#endif

    NanoStaticAssert((LIR_ltd^1) == LIR_gtd && (LIR_gtd^1) == LIR_ltd);
    NanoStaticAssert((LIR_led^1) == LIR_ged && (LIR_ged^1) == LIR_led);


    struct GuardRecord;
    struct SideExit;

    enum AbiKind {
        ABI_FASTCALL,
        ABI_THISCALL,
        ABI_STDCALL,
        ABI_CDECL
    };

    
    
    
    
    
    enum ArgType {
        ARGTYPE_V  = 0,     
        ARGTYPE_I  = 1,     
        ARGTYPE_UI = 2,     
#ifdef NANOJIT_64BIT
        ARGTYPE_Q  = 3,     
#endif
        ARGTYPE_D  = 4,     

        
        ARGTYPE_P = PTR_SIZE(ARGTYPE_I, ARGTYPE_Q), 
        ARGTYPE_B = ARGTYPE_I   
    };

    
    static const int ARGTYPE_SHIFT = 3;
    static const int ARGTYPE_MASK = 0x7;

    enum IndirectCall {
        CALL_INDIRECT = 0
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    typedef uint8_t AccSet;

    
    
    
    
    
    static const AccSet ACC_READONLY = 1 << 0;      
    static const AccSet ACC_STACK    = 1 << 1;      
    static const AccSet ACC_RSTACK   = 1 << 2;      
    static const AccSet ACC_OTHER    = 1 << 3;      

    
    
    
    
    
    
    
    
    
    static const AccSet ACC_NONE         = 0x0;
    static const AccSet ACC_ALL_STORABLE = ACC_STACK | ACC_RSTACK | ACC_OTHER;
    static const AccSet ACC_ALL          = ACC_READONLY | ACC_ALL_STORABLE;
    static const AccSet ACC_LOAD_ANY     = ACC_ALL;            
    static const AccSet ACC_STORE_ANY    = ACC_ALL_STORABLE;   

    struct CallInfo
    {
    private:

    public:
        uintptr_t   _address;
        uint32_t    _typesig:27;     
        AbiKind     _abi:3;
        uint32_t    _isPure:1;      
        AccSet      _storeAccSet;   
        verbose_only ( const char* _name; )

        uint32_t count_args() const;
        uint32_t count_int32_args() const;
        
        uint32_t getArgTypes(ArgType* types) const;

        inline ArgType returnType() const {
            return ArgType(_typesig & ARGTYPE_MASK);
        }

        inline bool isIndirect() const {
            return _address < 256;
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
    inline bool isLiveOpcode(LOpcode op) {
        return
#if defined NANOJIT_64BIT
               op == LIR_liveq ||
#endif
               op == LIR_livei || op == LIR_lived;
    }
    inline bool isRetOpcode(LOpcode op) {
        return
#if defined NANOJIT_64BIT
            op == LIR_retq ||
#endif
            op == LIR_reti || op == LIR_retd;
    }
    inline bool isCmovOpcode(LOpcode op) {
        return
#if defined NANOJIT_64BIT
            op == LIR_cmovq ||
#endif
            op == LIR_cmovi;
    }
    inline bool isCmpIOpcode(LOpcode op) {
        return LIR_eqi <= op && op <= LIR_geui;
    }
    inline bool isCmpSIOpcode(LOpcode op) {
        return LIR_eqi <= op && op <= LIR_gei;
    }
    inline bool isCmpUIOpcode(LOpcode op) {
        return LIR_eqi == op || (LIR_ltui <= op && op <= LIR_geui);
    }
#ifdef NANOJIT_64BIT
    inline bool isCmpQOpcode(LOpcode op) {
        return LIR_eqq <= op && op <= LIR_geuq;
    }
    inline bool isCmpSQOpcode(LOpcode op) {
        return LIR_eqq <= op && op <= LIR_geq;
    }
    inline bool isCmpUQOpcode(LOpcode op) {
        return LIR_eqq == op || (LIR_ltuq <= op && op <= LIR_geuq);
    }
#endif
    inline bool isCmpDOpcode(LOpcode op) {
        return LIR_eqd <= op && op <= LIR_ged;
    }

    inline LOpcode invertCondJmpOpcode(LOpcode op) {
        NanoAssert(op == LIR_jt || op == LIR_jf);
        return LOpcode(op ^ 1);
    }
    inline LOpcode invertCondGuardOpcode(LOpcode op) {
        NanoAssert(op == LIR_xt || op == LIR_xf);
        return LOpcode(op ^ 1);
    }
    inline LOpcode invertCmpIOpcode(LOpcode op) {
        NanoAssert(isCmpIOpcode(op));
        return LOpcode(op ^ 1);
    }
#ifdef NANOJIT_64BIT
    inline LOpcode invertCmpQOpcode(LOpcode op) {
        NanoAssert(isCmpQOpcode(op));
        return LOpcode(op ^ 1);
    }
#endif
    inline LOpcode invertCmpDOpcode(LOpcode op) {
        NanoAssert(isCmpDOpcode(op));
        return LOpcode(op ^ 1);
    }

    inline LOpcode getCallOpcode(const CallInfo* ci) {
        LOpcode op = LIR_callp;
        switch (ci->returnType()) {
        case ARGTYPE_V: op = LIR_callp; break;
        case ARGTYPE_I:
        case ARGTYPE_UI: op = LIR_calli; break;
        case ARGTYPE_D: op = LIR_calld; break;
#ifdef NANOJIT_64BIT
        case ARGTYPE_Q: op = LIR_callq; break;
#endif
        default:        NanoAssert(0);  break;
        }
        return op;
    }

    LOpcode arithOpcodeD2I(LOpcode op);
#ifdef NANOJIT_64BIT
    LOpcode cmpOpcodeI2Q(LOpcode op);
#endif
    LOpcode cmpOpcodeD2I(LOpcode op);
    LOpcode cmpOpcodeD2UI(LOpcode op);

    
    extern const uint8_t repKinds[];

    enum LTy {
        LTy_V,  
        LTy_I,  
#ifdef NANOJIT_64BIT
        LTy_Q,  
#endif
        LTy_D,  

        LTy_P  = PTR_SIZE(LTy_I, LTy_Q)   
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
        LRK_St,
        LRK_Sk,
        LRK_C,
        LRK_P,
        LRK_I,
        LRK_QorD,
        LRK_Jtbl,
        LRK_None    
    };

    class LInsOp0;
    class LInsOp1;
    class LInsOp2;
    class LInsOp3;
    class LInsLd;
    class LInsSt;
    class LInsSk;
    class LInsC;
    class LInsP;
    class LInsI;
    class LInsQorD;
    class LInsJtbl;

    class LIns
    {
    private:
        
        
        
        
        
        
        
        
        
        
        
        
        struct SharedFields {
            uint32_t inReg:1;           
            Register reg:7;
            uint32_t inAr:1;            
            uint32_t isResultLive:1;    

            uint32_t arIndex:14;        

            LOpcode  opcode:8;          
        };

        union {
            SharedFields sharedFields;
            
            
            
            void* wholeWord;
        };

        inline void initSharedFields(LOpcode opcode)
        {
            
            
            wholeWord = 0;
            sharedFields.opcode = opcode;
        }

        
        inline LInsOp0* toLInsOp0() const;
        inline LInsOp1* toLInsOp1() const;
        inline LInsOp2* toLInsOp2() const;
        inline LInsOp3* toLInsOp3() const;
        inline LInsLd*  toLInsLd()  const;
        inline LInsSt*  toLInsSt()  const;
        inline LInsSk*  toLInsSk()  const;
        inline LInsC*   toLInsC()   const;
        inline LInsP*   toLInsP()   const;
        inline LInsI*   toLInsI()   const;
        inline LInsQorD* toLInsQorD() const;
        inline LInsJtbl*toLInsJtbl()const;

        void staticSanityCheck();

    public:
        
        inline void initLInsOp0(LOpcode opcode);
        inline void initLInsOp1(LOpcode opcode, LIns* oprnd1);
        inline void initLInsOp2(LOpcode opcode, LIns* oprnd1, LIns* oprnd2);
        inline void initLInsOp3(LOpcode opcode, LIns* oprnd1, LIns* oprnd2, LIns* oprnd3);
        inline void initLInsLd(LOpcode opcode, LIns* val, int32_t d, AccSet accSet);
        inline void initLInsSt(LOpcode opcode, LIns* val, LIns* base, int32_t d, AccSet accSet);
        inline void initLInsSk(LIns* prevLIns);
        
        
        inline void initLInsC(LOpcode opcode, LIns** args, const CallInfo* ci);
        inline void initLInsP(int32_t arg, int32_t kind);
        inline void initLInsI(LOpcode opcode, int32_t immI);
        inline void initLInsQorD(LOpcode opcode, uint64_t immQorD);
        inline void initLInsJtbl(LIns* index, uint32_t size, LIns** table);

        LOpcode opcode() const { return sharedFields.opcode; }

        
        
        
        bool isLive() const {
            return isV() ||
                   sharedFields.isResultLive ||
                   (isCall() && !callInfo()->_isPure) ||    
                   isop(LIR_paramp);                        
        }
        void setResultLive() {
            NanoAssert(!isV());
            sharedFields.isResultLive = 1;
        }

        
        
        
        
        
        
        void deprecated_markAsClear() {
            sharedFields.inReg = 0;
            sharedFields.inAr = 0;
        }
        bool deprecated_hasKnownReg() {
            NanoAssert(isExtant());
            return isInReg();
        }
        Register deprecated_getReg() {
            NanoAssert(isExtant());
            return ( isInReg() ? sharedFields.reg : deprecated_UnknownReg );
        }
        uint32_t deprecated_getArIndex() {
            NanoAssert(isExtant());
            return ( isInAr() ? sharedFields.arIndex : 0 );
        }

        
        
        
        
        
        
        bool isExtant() {
            return isInReg() || isInAr();
        }
        bool isInReg() {
            return sharedFields.inReg;
        }
        bool isInRegMask(RegisterMask allow) {
            return isInReg() && (rmask(getReg()) & allow);
        }
        Register getReg() {
            NanoAssert(isInReg());
            return sharedFields.reg;
        }
        void setReg(Register r) {
            sharedFields.inReg = 1;
            sharedFields.reg = r;
        }
        void clearReg() {
            sharedFields.inReg = 0;
        }
        bool isInAr() {
            return sharedFields.inAr;
        }
        uint32_t getArIndex() {
            NanoAssert(isInAr());
            return sharedFields.arIndex;
        }
        void setArIndex(uint32_t arIndex) {
            sharedFields.inAr = 1;
            sharedFields.arIndex = arIndex;
        }
        void clearArIndex() {
            sharedFields.inAr = 0;
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

        
        inline int32_t  immI() const;

        
#ifdef NANOJIT_64BIT
        inline int32_t  immQlo() const;
        inline uint64_t immQ() const;
#endif
        inline int32_t  immDlo() const;
        inline int32_t  immDhi() const;
        inline double   immD() const;
        inline uint64_t immDasQ() const;

        
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
        bool isLInsSt() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_St == repKinds[opcode()];
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
        bool isLInsQorD() const {
            NanoAssert(LRK_None != repKinds[opcode()]);
            return LRK_QorD == repKinds[opcode()];
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
        bool isCmp() const {
            LOpcode op = opcode();
            return isCmpIOpcode(op) ||
#if defined NANOJIT_64BIT
                   isCmpQOpcode(op) ||
#endif
                   isCmpDOpcode(op);
        }
        bool isCall() const {
            return isop(LIR_calli) ||
#if defined NANOJIT_64BIT
                   isop(LIR_callq) ||
#endif
                   isop(LIR_calld);
        }
        bool isCmov() const {
            return isCmovOpcode(opcode());
        }
        bool isStore() const {
            return isLInsSt();
        }
        bool isLoad() const {
            return isLInsLd();
        }
        bool isGuard() const {
            return isop(LIR_x) || isop(LIR_xf) || isop(LIR_xt) ||
                   isop(LIR_xbarrier) || isop(LIR_xtbl) ||
                   isop(LIR_addxovi) || isop(LIR_subxovi) || isop(LIR_mulxovi);
        }
        bool isJov() const {
            return
#ifdef NANOJIT_64BIT
                isop(LIR_addjovq) || isop(LIR_subjovq) ||
#endif
                isop(LIR_addjovi) || isop(LIR_subjovi) || isop(LIR_muljovi);
        }
        
        bool isImmI() const {
            return isop(LIR_immi);
        }
        
        
        bool isImmI(int32_t val) const {
            return isImmI() && immI()==val;
        }
#ifdef NANOJIT_64BIT
        
        bool isImmQ() const {
            return isop(LIR_immq);
        }
#endif
        
        bool isImmP() const
        {
#ifdef NANOJIT_64BIT
            return isImmQ();
#else
            return isImmI();
#endif
        }
        
        bool isImmD() const {
            return isop(LIR_immd);
        }
        
        bool isImmQorD() const {
            return
#ifdef NANOJIT_64BIT
                isImmQ() ||
#endif
                isImmD();
        }
        
        bool isImmAny() const {
            return isImmI() || isImmQorD();
        }

        bool isBranch() const {
            return isop(LIR_jt) || isop(LIR_jf) || isop(LIR_j) || isop(LIR_jtbl) || isJov();
        }

        LTy retType() const {
            return retTypes[opcode()];
        }
        bool isV() const {
            return retType() == LTy_V;
        }
        bool isI() const {
            return retType() == LTy_I;
        }
#ifdef NANOJIT_64BIT
        bool isQ() const {
            return retType() == LTy_Q;
        }
#endif
        bool isD() const {
            return retType() == LTy_D;
        }
        bool isQorD() const {
            return
#ifdef NANOJIT_64BIT
                isQ() ||
#endif
                isD();
        }
        bool isP() const {
#ifdef NANOJIT_64BIT
            return isQ();
#else
            return isI();
#endif
        }

        inline void* immP() const
        {
        #ifdef NANOJIT_64BIT
            return (void*)immQ();
        #else
            return (void*)immI();
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

    
    class LInsSt
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

        int32_t     immI;

        LIns        ins;

    public:
        LIns* getLIns() { return &ins; };
    };

    
    class LInsQorD
    {
    private:
        friend class LIns;

        int32_t     immQorDlo;

        int32_t     immQorDhi;

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

    LInsOp0*  LIns::toLInsOp0()  const { return (LInsOp0* )(uintptr_t(this+1) - sizeof(LInsOp0 )); }
    LInsOp1*  LIns::toLInsOp1()  const { return (LInsOp1* )(uintptr_t(this+1) - sizeof(LInsOp1 )); }
    LInsOp2*  LIns::toLInsOp2()  const { return (LInsOp2* )(uintptr_t(this+1) - sizeof(LInsOp2 )); }
    LInsOp3*  LIns::toLInsOp3()  const { return (LInsOp3* )(uintptr_t(this+1) - sizeof(LInsOp3 )); }
    LInsLd*   LIns::toLInsLd()   const { return (LInsLd*  )(uintptr_t(this+1) - sizeof(LInsLd  )); }
    LInsSt*   LIns::toLInsSt()   const { return (LInsSt*  )(uintptr_t(this+1) - sizeof(LInsSt  )); }
    LInsSk*   LIns::toLInsSk()   const { return (LInsSk*  )(uintptr_t(this+1) - sizeof(LInsSk  )); }
    LInsC*    LIns::toLInsC()    const { return (LInsC*   )(uintptr_t(this+1) - sizeof(LInsC   )); }
    LInsP*    LIns::toLInsP()    const { return (LInsP*   )(uintptr_t(this+1) - sizeof(LInsP   )); }
    LInsI*    LIns::toLInsI()    const { return (LInsI*   )(uintptr_t(this+1) - sizeof(LInsI   )); }
    LInsQorD* LIns::toLInsQorD() const { return (LInsQorD*)(uintptr_t(this+1) - sizeof(LInsQorD)); }
    LInsJtbl* LIns::toLInsJtbl() const { return (LInsJtbl*)(uintptr_t(this+1) - sizeof(LInsJtbl)); }

    void LIns::initLInsOp0(LOpcode opcode) {
        initSharedFields(opcode);
        NanoAssert(isLInsOp0());
    }
    void LIns::initLInsOp1(LOpcode opcode, LIns* oprnd1) {
        initSharedFields(opcode);
        toLInsOp1()->oprnd_1 = oprnd1;
        NanoAssert(isLInsOp1());
    }
    void LIns::initLInsOp2(LOpcode opcode, LIns* oprnd1, LIns* oprnd2) {
        initSharedFields(opcode);
        toLInsOp2()->oprnd_1 = oprnd1;
        toLInsOp2()->oprnd_2 = oprnd2;
        NanoAssert(isLInsOp2());
    }
    void LIns::initLInsOp3(LOpcode opcode, LIns* oprnd1, LIns* oprnd2, LIns* oprnd3) {
        initSharedFields(opcode);
        toLInsOp3()->oprnd_1 = oprnd1;
        toLInsOp3()->oprnd_2 = oprnd2;
        toLInsOp3()->oprnd_3 = oprnd3;
        NanoAssert(isLInsOp3());
    }
    void LIns::initLInsLd(LOpcode opcode, LIns* val, int32_t d, AccSet accSet) {
        initSharedFields(opcode);
        toLInsLd()->oprnd_1 = val;
        NanoAssert(d == int16_t(d));
        toLInsLd()->disp = int16_t(d);
        toLInsLd()->accSet = accSet;
        NanoAssert(isLInsLd());
    }
    void LIns::initLInsSt(LOpcode opcode, LIns* val, LIns* base, int32_t d, AccSet accSet) {
        initSharedFields(opcode);
        toLInsSt()->oprnd_1 = val;
        toLInsSt()->oprnd_2 = base;
        NanoAssert(d == int16_t(d));
        toLInsSt()->disp = int16_t(d);
        toLInsSt()->accSet = accSet;
        NanoAssert(isLInsSt());
    }
    void LIns::initLInsSk(LIns* prevLIns) {
        initSharedFields(LIR_skip);
        toLInsSk()->prevLIns = prevLIns;
        NanoAssert(isLInsSk());
    }
    void LIns::initLInsC(LOpcode opcode, LIns** args, const CallInfo* ci) {
        initSharedFields(opcode);
        toLInsC()->args = args;
        toLInsC()->ci = ci;
        NanoAssert(isLInsC());
    }
    void LIns::initLInsP(int32_t arg, int32_t kind) {
        initSharedFields(LIR_paramp);
        NanoAssert(isU8(arg) && isU8(kind));
        toLInsP()->arg = arg;
        toLInsP()->kind = kind;
        NanoAssert(isLInsP());
    }
    void LIns::initLInsI(LOpcode opcode, int32_t immI) {
        initSharedFields(opcode);
        toLInsI()->immI = immI;
        NanoAssert(isLInsI());
    }
    void LIns::initLInsQorD(LOpcode opcode, uint64_t immQorD) {
        initSharedFields(opcode);
        toLInsQorD()->immQorDlo = int32_t(immQorD);
        toLInsQorD()->immQorDhi = int32_t(immQorD >> 32);
        NanoAssert(isLInsQorD());
    }
    void LIns::initLInsJtbl(LIns* index, uint32_t size, LIns** table) {
        initSharedFields(LIR_jtbl);
        toLInsJtbl()->oprnd_1 = index;
        toLInsJtbl()->table = table;
        toLInsJtbl()->size = size;
        NanoAssert(isLInsJtbl());
    }

    LIns* LIns::oprnd1() const {
        NanoAssert(isLInsOp1() || isLInsOp2() || isLInsOp3() || isLInsLd() || isLInsSt() || isLInsJtbl());
        return toLInsOp2()->oprnd_1;
    }
    LIns* LIns::oprnd2() const {
        NanoAssert(isLInsOp2() || isLInsOp3() || isLInsSt());
        return toLInsOp2()->oprnd_2;
    }
    LIns* LIns::oprnd3() const {
        NanoAssert(isLInsOp3());
        return toLInsOp3()->oprnd_3;
    }

    LIns* LIns::getTarget() const {
        NanoAssert(isBranch() && !isop(LIR_jtbl));
        if (isJov())
            return oprnd3();
        else
            return oprnd2();
    }

    void LIns::setTarget(LIns* label) {
        NanoAssert(label && label->isop(LIR_label));
        NanoAssert(isBranch() && !isop(LIR_jtbl));
        if (isJov())
            toLInsOp3()->oprnd_3 = label;
        else
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

        case LIR_addxovi:
        case LIR_subxovi:
        case LIR_mulxovi:
            return (GuardRecord*)oprnd3();

        default:
            NanoAssert(0);
            return NULL;
        }
    }

    int32_t LIns::disp() const {
        if (isLInsSt()) {
            return toLInsSt()->disp;
        } else {
            NanoAssert(isLInsLd());
            return toLInsLd()->disp;
        }
    }

    AccSet LIns::accSet() const {
        if (isLInsSt()) {
            return toLInsSt()->accSet;
        } else {
            NanoAssert(isLInsLd());
            return toLInsLd()->accSet;
        }
    }

    LIns* LIns::prevLIns() const {
        NanoAssert(isLInsSk());
        return toLInsSk()->prevLIns;
    }

    inline uint8_t LIns::paramArg()  const { NanoAssert(isop(LIR_paramp)); return toLInsP()->arg; }
    inline uint8_t LIns::paramKind() const { NanoAssert(isop(LIR_paramp)); return toLInsP()->kind; }

    inline int32_t LIns::immI()     const { NanoAssert(isImmI());  return toLInsI()->immI; }

#ifdef NANOJIT_64BIT
    inline int32_t LIns::immQlo()   const { NanoAssert(isImmQ()); return toLInsQorD()->immQorDlo; }
    uint64_t       LIns::immQ()     const {
        NanoAssert(isImmQ());
        return (uint64_t(toLInsQorD()->immQorDhi) << 32) | uint32_t(toLInsQorD()->immQorDlo);
    }
#endif
    inline int32_t LIns::immDlo() const { NanoAssert(isImmD()); return toLInsQorD()->immQorDlo; }
    inline int32_t LIns::immDhi() const { NanoAssert(isImmD()); return toLInsQorD()->immQorDhi; }
    double         LIns::immD()    const {
        NanoAssert(isImmD());
        union {
            double f;
            uint64_t q;
        } u;
        u.q = immDasQ();
        return u.f;
    }
    uint64_t       LIns::immDasQ()  const {
        NanoAssert(isImmD());
        return (uint64_t(toLInsQorD()->immQorDhi) << 32) | uint32_t(toLInsQorD()->immQorDlo);
    }

    int32_t LIns::size() const {
        NanoAssert(isop(LIR_allocp));
        return toLInsI()->immI << 2;
    }

    void LIns::setSize(int32_t nbytes) {
        NanoAssert(isop(LIR_allocp));
        NanoAssert(nbytes > 0);
        toLInsI()->immI = (nbytes+3)>>2; 
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
        virtual LInsp insBranch(LOpcode v, LIns* condition, LIns* to) {
            return out->insBranch(v, condition, to);
        }
        virtual LInsp insBranchJov(LOpcode v, LIns* a, LIns* b, LIns* to) {
            return out->insBranchJov(v, a, b, to);
        }
        
        
        virtual LInsp insParam(int32_t arg, int32_t kind) {
            return out->insParam(arg, kind);
        }
        virtual LInsp insImmI(int32_t imm) {
            return out->insImmI(imm);
        }
#ifdef NANOJIT_64BIT
        virtual LInsp insImmQ(uint64_t imm) {
            return out->insImmQ(imm);
        }
#endif
        virtual LInsp insImmD(double d) {
            return out->insImmD(d);
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

        

        
        
        LIns* insChoose(LIns* cond, LIns* iftrue, LIns* iffalse, bool use_cmov);

        
        LIns* insEqI_0(LIns* oprnd1) {
            return ins2ImmI(LIR_eqi, oprnd1, 0);
        }

        
        LIns* insEqP_0(LIns* oprnd1) {
            return ins2(LIR_eqp, oprnd1, insImmWord(0));
        }

        
        
        LIns* ins2ImmI(LOpcode v, LIns* oprnd1, int32_t imm) {
            return ins2(v, oprnd1, insImmI(imm));
        }

        LIns* insImmP(const void *ptr) {
#ifdef NANOJIT_64BIT
            return insImmQ((uint64_t)ptr);
#else
            return insImmI((int32_t)ptr);
#endif
        }

        LIns* insImmWord(intptr_t value) {
#ifdef NANOJIT_64BIT
            return insImmQ(value);
#else
            return insImmI(value);
#endif
        }

        
        LIns* insI2P(LIns* intIns) {
#ifdef NANOJIT_64BIT
            return ins1(LIR_i2q, intIns);
#else
            return intIns;
#endif
        }

        
        LIns* insUI2P(LIns* uintIns) {
    #ifdef NANOJIT_64BIT
            return ins1(LIR_ui2uq, uintIns);
    #else
            return uintIns;
    #endif
        }

        
        LIns* insStore(LIns* value, LIns* base, int32_t d, AccSet accSet);
    };


#ifdef NJ_VERBOSE
    extern const char* lirNames[];

    
    class AddrNameMap
    {
        Allocator& allocator;
        class Entry
        {
        public:
            Entry(int) : name(0), size(0), align(0) {}
            Entry(char *n, size_t s, size_t a) : name(n), size(s), align(a) {}
            char* name;
            size_t size:29, align:3;
        };
        TreeMap<const void*, Entry*> names;     
    public:
        AddrNameMap(Allocator& allocator);
        void addAddrRange(const void *p, size_t size, size_t align, const char *name);
        void lookupAddr(void *p, char*& name, int32_t& offset);
    };

    
    class LirNameMap
    {
    private:
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
        CountMap<const char *> namecounts;

        void addNameWithSuffix(LInsp i, const char *s, int suffix, bool ignoreOneSuffix);

        class Entry
        {
        public:
            Entry(int) : name(0) {}
            Entry(char* n) : name(n) {}
            char* name;
        };

        HashMap<LInsp, Entry*> names;

    public:
        LirNameMap(Allocator& alloc)
            : alloc(alloc),
            lircounts(alloc),
            funccounts(alloc),
            namecounts(alloc),
            names(alloc)
        {}

        void        addName(LInsp ins, const char *s);  
        const char* createName(LInsp ins);              
        const char* lookupName(LInsp ins);
    };

    
    
    
    
    class InsBuf {
    public:
        static const size_t len = 1000;
        char buf[len];
    };
    class RefBuf {
    public:
        static const size_t len = 200;
        char buf[len];
    };

    class LInsPrinter
    {
    private:
        Allocator& alloc;

        char *formatImmI(RefBuf* buf, int32_t c);
        char *formatImmQ(RefBuf* buf, uint64_t c);
        char *formatImmD(RefBuf* buf, double c);
        void formatGuard(InsBuf* buf, LInsp ins);
        void formatGuardXov(InsBuf* buf, LInsp ins);

    public:
        LInsPrinter(Allocator& alloc)
            : alloc(alloc)
        {
            addrNameMap = new (alloc) AddrNameMap(alloc);
            lirNameMap = new (alloc) LirNameMap(alloc);
        }

        char *formatAddr(RefBuf* buf, void* p);
        char *formatRef(RefBuf* buf, LInsp ref, bool showImmValue = true);
        char *formatIns(InsBuf* buf, LInsp ins);
        char *formatAccSet(RefBuf* buf, AccSet accSet);

        AddrNameMap* addrNameMap;
        LirNameMap* lirNameMap;
    };


    class VerboseWriter : public LirWriter
    {
        InsList code;
        LInsPrinter* printer;
        LogControl* logc;
        const char* const prefix;
        bool const always_flush;
    public:
        VerboseWriter(Allocator& alloc, LirWriter *out, LInsPrinter* printer, LogControl* logc,
                      const char* prefix = "", bool always_flush = false)
            : LirWriter(out), code(alloc), printer(printer), logc(logc), prefix(prefix), always_flush(always_flush)
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
                InsBuf b;
                int32_t count = 0;
                for (Seq<LIns*>* p = code.get(); p != NULL; p = p->tail) {
                    logc->printf("%s    %s\n", prefix, printer->formatIns(&b, p->head));
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

        LIns* insBranchJov(LOpcode v, LInsp a, LInsp b, LInsp to) {
            return add_flush(out->insBranchJov(v, a, b, to));
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
        LIns* insImmI(int32_t imm) {
            return add(out->insImmI(imm));
        }
#ifdef NANOJIT_64BIT
        LIns* insImmQ(uint64_t imm) {
            return add(out->insImmQ(imm));
        }
#endif
        LIns* insImmD(double d) {
            return add(out->insImmD(d));
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
        LIns* insGuard(LOpcode, LIns* cond, GuardRecord *);
        LIns* insGuardXov(LOpcode, LIns* a, LIns* b, GuardRecord *);
        LIns* insBranch(LOpcode, LIns* cond, LIns* target);
        LIns* insBranchJov(LOpcode, LIns* a, LIns* b, LIns* target);
        LIns* insLoad(LOpcode op, LInsp base, int32_t off, AccSet accSet);
    private:
        LIns* simplifyOverflowArith(LOpcode op, LInsp *opnd1, LInsp *opnd2);
    };

    class CseFilter: public LirWriter
    {
        enum LInsHashKind {
            
            
            LInsImmI = 0,
            LInsImmQ = 1,   
            LInsImmD = 2,
            LIns1    = 3,
            LIns2    = 4,
            LIns3    = 5,
            LInsCall = 6,

            
            
            
            
            
            
            
            
            LInsLoadReadOnly = 7,
            LInsLoadStack    = 8,
            LInsLoadRStack   = 9,
            LInsLoadOther    = 10,
            LInsLoadMultiple = 11,

            LInsFirst = 0,
            LInsLast = 11,
            
            LInsInvalid = 12
        };
        #define nextKind(kind)  LInsHashKind(kind+1)

        
        
        
        
        
        
        
        
        
        LInsp*      m_list[LInsLast + 1];
        uint32_t    m_cap[LInsLast + 1];
        uint32_t    m_used[LInsLast + 1];
        typedef uint32_t (CseFilter::*find_t)(LInsp);
        find_t      m_find[LInsLast + 1];

        AccSet      storesSinceLastLoad;    

        Allocator& alloc;

        static uint32_t hash8(uint32_t hash, const uint8_t data);
        static uint32_t hash32(uint32_t hash, const uint32_t data);
        static uint32_t hashptr(uint32_t hash, const void* data);
        static uint32_t hashfinish(uint32_t hash);

        static uint32_t hashImmI(int32_t);
        static uint32_t hashImmQorD(uint64_t);     
        static uint32_t hash1(LOpcode op, LInsp);
        static uint32_t hash2(LOpcode op, LInsp, LInsp);
        static uint32_t hash3(LOpcode op, LInsp, LInsp, LInsp);
        static uint32_t hashLoad(LOpcode op, LInsp, int32_t, AccSet);
        static uint32_t hashCall(const CallInfo *call, uint32_t argc, LInsp args[]);

        
        LInsp findImmI(int32_t a, uint32_t &k);
#ifdef NANOJIT_64BIT
        LInsp findImmQ(uint64_t a, uint32_t &k);
#endif
        LInsp findImmD(uint64_t d, uint32_t &k);
        LInsp find1(LOpcode v, LInsp a, uint32_t &k);
        LInsp find2(LOpcode v, LInsp a, LInsp b, uint32_t &k);
        LInsp find3(LOpcode v, LInsp a, LInsp b, LInsp c, uint32_t &k);
        LInsp findLoad(LOpcode v, LInsp a, int32_t b, AccSet accSet, LInsHashKind kind,
                       uint32_t &k);
        LInsp findCall(const CallInfo *call, uint32_t argc, LInsp args[], uint32_t &k);

        
        
        
        uint32_t findImmI(LInsp ins);
#ifdef NANOJIT_64BIT
        uint32_t findImmQ(LInsp ins);
#endif
        uint32_t findImmD(LInsp ins);
        uint32_t find1(LInsp ins);
        uint32_t find2(LInsp ins);
        uint32_t find3(LInsp ins);
        uint32_t findCall(LInsp ins);
        uint32_t findLoadReadOnly(LInsp ins);
        uint32_t findLoadStack(LInsp ins);
        uint32_t findLoadRStack(LInsp ins);
        uint32_t findLoadOther(LInsp ins);
        uint32_t findLoadMultiple(LInsp ins);

        void grow(LInsHashKind kind);

        
        void add(LInsHashKind kind, LInsp ins, uint32_t k);

        void clear();               
        void clear(LInsHashKind);   

    public:
        CseFilter(LirWriter *out, Allocator&);

        LIns* insImmI(int32_t imm);
#ifdef NANOJIT_64BIT
        LIns* insImmQ(uint64_t q);
#endif
        LIns* insImmD(double d);
        LIns* ins0(LOpcode v);
        LIns* ins1(LOpcode v, LInsp);
        LIns* ins2(LOpcode v, LInsp, LInsp);
        LIns* ins3(LOpcode v, LInsp, LInsp, LInsp);
        LIns* insLoad(LOpcode op, LInsp base, int32_t d, AccSet accSet);
        LIns* insStore(LOpcode op, LInsp value, LInsp base, int32_t d, AccSet accSet);
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
            verbose_only(LInsPrinter* printer;)

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
            LInsp   insImmI(int32_t imm);
#ifdef NANOJIT_64BIT
            LInsp   insImmQ(uint64_t imm);
#endif
            LInsp   insImmD(double d);
            LInsp   insCall(const CallInfo *call, LInsp args[]);
            LInsp   insGuard(LOpcode op, LInsp cond, GuardRecord *gr);
            LInsp   insGuardXov(LOpcode op, LInsp a, LInsp b, GuardRecord *gr);
            LInsp   insBranch(LOpcode v, LInsp condition, LInsp to);
            LInsp   insBranchJov(LOpcode v, LInsp a, LInsp b, LInsp to);
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
        virtual LInsp finalIns() {
            return in->finalIns();
        }
    };

    
    class LirReader : public LirFilter
    {
        LInsp _ins;         
        LInsp _finalIns;    

    public:
        LirReader(LInsp ins) : LirFilter(0), _ins(ins), _finalIns(ins)
        {
            
            
            
            
            
            
            
            NanoAssert(ins && !ins->isop(LIR_skip));
        }
        virtual ~LirReader() {}

        
        
        LInsp read();

        LInsp finalIns() {
            return _finalIns;
        }
    };

    verbose_only(void live(LirFilter* in, Allocator& alloc, Fragment* frag, LogControl*);)

    
    
    
    class StackFilter: public LirFilter
    {
        LInsp sp;
        BitSet stk;
        int top;
        int getTop(LInsp br);

    public:
        StackFilter(LirFilter *in, Allocator& alloc, LInsp sp);
        LInsp read();
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
        LIns *callD1(const CallInfo *call, LIns *a);
        LIns *callD2(const CallInfo *call, LIns *a, LIns *b);
        LIns *cmpD(const CallInfo *call, LIns *a, LIns *b);
        LIns *ins1(LOpcode op, LIns *a);
        LIns *ins2(LOpcode op, LIns *a, LIns *b);
        LIns *insCall(const CallInfo *ci, LInsp args[]);
    };


#ifdef DEBUG
    
    
    
    
    
    
    
    
    
    
    
    
    class ValidateWriter : public LirWriter
    {
    private:
        LInsPrinter* printer;
        const char* whereInPipeline;

        const char* type2string(LTy type);
        void typeCheckArgs(LOpcode op, int nArgs, LTy formals[], LIns* args[]);
        void errorStructureShouldBe(LOpcode op, const char* argDesc, int argN, LIns* arg,
                                    const char* shouldBeDesc);
        void errorAccSet(const char* what, AccSet accSet, const char* shouldDesc);
        void checkLInsHasOpcode(LOpcode op, int argN, LIns* ins, LOpcode op2);
        void checkLInsIsACondOrConst(LOpcode op, int argN, LIns* ins);
        void checkLInsIsNull(LOpcode op, int argN, LIns* ins);
        void checkAccSet(LOpcode op, LInsp base, AccSet accSet, AccSet maxAccSet);

        LInsp sp, rp;

    public:
        ValidateWriter(LirWriter* out, LInsPrinter* printer, const char* where);
        void setSp(LInsp ins) { sp = ins; }
        void setRp(LInsp ins) { rp = ins; }

        LIns* insLoad(LOpcode op, LIns* base, int32_t d, AccSet accSet);
        LIns* insStore(LOpcode op, LIns* value, LIns* base, int32_t d, AccSet accSet);
        LIns* ins0(LOpcode v);
        LIns* ins1(LOpcode v, LIns* a);
        LIns* ins2(LOpcode v, LIns* a, LIns* b);
        LIns* ins3(LOpcode v, LIns* a, LIns* b, LIns* c);
        LIns* insParam(int32_t arg, int32_t kind);
        LIns* insImmI(int32_t imm);
#ifdef NANOJIT_64BIT
        LIns* insImmQ(uint64_t imm);
#endif
        LIns* insImmD(double d);
        LIns* insCall(const CallInfo *call, LIns* args[]);
        LIns* insGuard(LOpcode v, LIns *c, GuardRecord *gr);
        LIns* insGuardXov(LOpcode v, LIns* a, LIns* b, GuardRecord* gr);
        LIns* insBranch(LOpcode v, LIns* condition, LIns* to);
        LIns* insBranchJov(LOpcode v, LIns* a, LIns* b, LIns* to);
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
        LInsPrinter* _printer;
        const char*  _title;
        StringList   _strs;
        LogControl*  _logc;
        LIns*        _prevIns;
    public:
        ReverseLister(LirFilter* in, Allocator& alloc,
                      LInsPrinter* printer, LogControl* logc, const char* title)
            : LirFilter(in)
            , _alloc(alloc)
            , _printer(printer)
            , _title(title)
            , _strs(alloc)
            , _logc(logc)
            , _prevIns(NULL)
        { }

        void finish();
        LInsp read();
    };
#endif

}
#endif 
