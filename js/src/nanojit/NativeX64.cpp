






































#include "nanojit.h"



#include "../vprof/vprof.h"

#if defined FEATURE_NANOJIT && defined NANOJIT_X64























namespace nanojit
{
    const Register Assembler::retRegs[] = { RAX };
#ifdef _WIN64
    const Register Assembler::argRegs[] = { RCX, RDX, R8, R9 };
    const Register Assembler::savedRegs[] = { RBX, RSI, RDI, R12, R13, R14, R15 };
#else
    const Register Assembler::argRegs[] = { RDI, RSI, RDX, RCX, R8, R9 };
    const Register Assembler::savedRegs[] = { RBX, R12, R13, R14, R15 };
#endif

    const char *regNames[] = {
        "rax",  "rcx",  "rdx",   "rbx",   "rsp",   "rbp",   "rsi",   "rdi",
        "r8",   "r9",   "r10",   "r11",   "r12",   "r13",   "r14",   "r15",
        "xmm0", "xmm1", "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
        "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"
    };

    const char *gpRegNames32[] = {
        "eax", "ecx", "edx",  "ebx",  "esp",  "ebp",  "esi",  "edi",
        "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
    };

    const char *gpRegNames8[] = {
        "al",  "cl",  "dl",   "bl",   "spl",  "bpl",  "sil",  "dil",
        "r8l", "r9l", "r10l", "r11l", "r12l", "r13l", "r14l", "r15l"
    };

    const char *gpRegNames8hi[] = {
        "ah", "ch", "dh", "bh"
    };

#ifdef _DEBUG
    #define TODO(x) todo(#x)
    static void todo(const char *s) {
        verbose_only( avmplus::AvmLog("%s",s); )
        NanoAssertMsgf(false, "%s", s);
    }
#else
    #define TODO(x)
#endif

    
    
    
    
    
    

    
    const RegisterMask BaseRegs = GpRegs & ~rmask(R12);

    static inline int oplen(uint64_t op) {
        return op & 255;
    }

    
    static inline uint64_t rexrb(uint64_t op, Register r, Register b) {
        int shift = 64 - 8*oplen(op);
        uint64_t rex = ((op >> shift) & 255) | ((r&8)>>1) | ((b&8)>>3);
        return rex != 0x40 ? op | rex << shift : op - 1;
    }

    
    static inline uint64_t rexrxb(uint64_t op, Register r, Register x, Register b) {
        int shift = 64 - 8*oplen(op);
        uint64_t rex = ((op >> shift) & 255) | ((r&8)>>1) | ((x&8)>>2) | ((b&8)>>3);
        return rex != 0x40 ? op | rex << shift : op - 1;
    }

    
    
    static inline uint64_t rexrb8(uint64_t op, Register r, Register b) {
        int shift = 64 - 8*oplen(op);
        uint64_t rex = ((op >> shift) & 255) | ((r&8)>>1) | ((b&8)>>3);
        return ((rex | (b & ~3)) != 0x40) ? (op | (rex << shift)) : op - 1;
    }

    
    
    static inline uint64_t rexprb(uint64_t op, Register r, Register b) {
        int shift = 64 - 8*oplen(op) + 8;
        uint64_t rex = ((op >> shift) & 255) | ((r&8)>>1) | ((b&8)>>3);
        
        return rex != 0x40 ? op | rex << shift :
            ((op & ~(255LL<<shift)) | (op>>(shift-8)&255) << shift) - 1;
    }

    
    static inline uint64_t mod_rr(uint64_t op, Register r, Register b) {
        return op | uint64_t((r&7)<<3 | (b&7))<<56;
    }

    
    static inline uint64_t mod_rxb(uint64_t op, Register r, Register x, Register b) {
        return op | uint64_t((r&7)<<3)<<48 | uint64_t((x&7)<<3|(b&7))<<56;
    }

    static inline uint64_t mod_disp32(uint64_t op, Register r, Register b, int32_t d) {
        NanoAssert(IsGpReg(r) && IsGpReg(b));
        NanoAssert((b & 7) != 4); 
        if (isS8(d)) {
            
            NanoAssert((((op>>24)&255)>>6) == 2); 
            int len = oplen(op);
            op = (op & ~0xff000000LL) | (0x40 | (r&7)<<3 | (b&7))<<24; 
            return op<<24 | int64_t(d)<<56 | (len-3); 
        } else {
            
            return op | int64_t(d)<<32 | uint64_t((r&7)<<3 | (b&7))<<24;
        }
    }

    
    

    void Assembler::emit(uint64_t op) {
        int len = oplen(op);
        
        
        
        underrunProtect(8);
        ((int64_t*)_nIns)[-1] = op;
        _nIns -= len; 
        _nvprof("x64-bytes", len);
    }

    void Assembler::emit8(uint64_t op, int64_t v) {
        NanoAssert(isS8(v));
        emit(op | uint64_t(v)<<56);
    }

    void Assembler::emit_target8(size_t underrun, uint64_t op, NIns* target) {
        underrunProtect(underrun); 
        
        int64_t offset = target - _nIns;
        NanoAssert(isS8(offset));
        emit(op | uint64_t(offset)<<56);
    }

    void Assembler::emit_target32(size_t underrun, uint64_t op, NIns* target) {
        underrunProtect(underrun); 
        
        
        
        
        
        
        int64_t offset = target ? target - _nIns : 0;
        NanoAssert(isS32(offset));
        emit(op | uint64_t(uint32_t(offset))<<32);
    }

    
    void Assembler::emitrxb(uint64_t op, Register r, Register x, Register b) {
        emit(rexrxb(mod_rxb(op, r, x, b), r, x, b));
    }

    
    void Assembler::emitrr(uint64_t op, Register r, Register b) {
        emit(rexrb(mod_rr(op, r, b), r, b));
    }

    
    void Assembler::emitrr8(uint64_t op, Register r, Register b) {
        emit(rexrb8(mod_rr(op, r, b), r, b));
    }

    
    void Assembler::emitprr(uint64_t op, Register r, Register b) {
        emit(rexprb(mod_rr(op, r, b), r, b));
    }

    
    void Assembler::emitrm(uint64_t op, Register r, int32_t d, Register b) {
        emit(rexrb(mod_disp32(op, r, b, d), r, b));
    }

    
    uint64_t Assembler::emit_disp32(uint64_t op, int32_t d) {
        if (isS8(d)) {
            NanoAssert(((op>>56)&0xC0) == 0x80); 
            underrunProtect(1+8);
            *(--_nIns) = (NIns) d;
            _nvprof("x64-bytes", 1);
            op ^= 0xC000000000000000LL; 
        } else {
            underrunProtect(4+8); 
            *((int32_t*)(_nIns -= 4)) = d;
            _nvprof("x64-bytes", 4);
        }
        return op;
    }

    
    void Assembler::emitrm_wide(uint64_t op, Register r, int32_t d, Register b) {
        op = emit_disp32(op, d);
        emitrr(op, r, b);
    }

    
    
    void Assembler::emitprm(uint64_t op, Register r, int32_t d, Register b) {
        op = emit_disp32(op, d);
        emitprr(op, r, b);
    }

    void Assembler::emitrr_imm(uint64_t op, Register r, Register b, int32_t imm) {
        NanoAssert(IsGpReg(r) && IsGpReg(b));
        underrunProtect(4+8); 
        *((int32_t*)(_nIns -= 4)) = imm;
        _nvprof("x86-bytes", 4);
        emitrr(op, r, b);
    }

    void Assembler::emitr_imm64(uint64_t op, Register r, uint64_t imm64) {
        underrunProtect(8+8); 
        *((uint64_t*)(_nIns -= 8)) = imm64;
        _nvprof("x64-bytes", 8);
        emitr(op, r);
    }

    void Assembler::emitrxb_imm(uint64_t op, Register r, Register x, Register b, int32_t imm) {
        NanoAssert(IsGpReg(r) && IsGpReg(x) && IsGpReg(b));
        underrunProtect(4+8); 
        *((int32_t*)(_nIns -= 4)) = imm;
        _nvprof("x86-bytes", 4);
        emitrxb(op, r, x, b);
    }

    
    void Assembler::emitr_imm8(uint64_t op, Register b, int32_t imm8) {
        NanoAssert(IsGpReg(b) && isS8(imm8));
        op |= uint64_t(imm8)<<56 | uint64_t(b&7)<<48;  
        emit(rexrb(op, (Register)0, b));
    }

    void Assembler::emitxm_abs(uint64_t op, Register r, int32_t addr32)
    {
        underrunProtect(4+8);
        *((int32_t*)(_nIns -= 4)) = addr32;
        _nvprof("x64-bytes", 4);
        op = op | uint64_t((r&7)<<3)<<48; 
        op = rexrb(op, r, (Register)0);   
        emit(op);
    }

    void Assembler::emitxm_rel(uint64_t op, Register r, NIns* addr64)
    {
        underrunProtect(4+8);
        int32_t d = (int32_t)(addr64 - _nIns);
        *((int32_t*)(_nIns -= 4)) = d;
        _nvprof("x64-bytes", 4);
        emitrr(op, r, (Register)0);
    }

    
    
    bool Assembler::isTargetWithinS8(NIns* target)
    {
        NanoAssert(target);
        
        
        underrunProtect(8);
        return isS8(target - _nIns);
    }

    
    bool Assembler::isTargetWithinS32(NIns* target)
    {
        NanoAssert(target);
        underrunProtect(8);
        return isS32(target - _nIns);
    }

#define RB(r)       gpRegNames8[(r)]
#define RBhi(r)     gpRegNames8hi[(r)]
#define RL(r)       gpRegNames32[(r)]
#define RQ(r)       gpn(r)

#define R           Register
#define I           int
#define I32         int32_t
#define U64         uint64_t
#define S           size_t

    void Assembler::PUSHR(R r)  { emitr(X64_pushr,r); asm_output("push %s", RQ(r)); }
    void Assembler::POPR( R r)  { emitr(X64_popr, r); asm_output("pop %s",  RQ(r)); }
    void Assembler::NOT(  R r)  { emitr(X64_not,  r); asm_output("notl %s", RL(r)); }
    void Assembler::NEG(  R r)  { emitr(X64_neg,  r); asm_output("negl %s", RL(r)); }
    void Assembler::IDIV( R r)  { emitr(X64_idiv, r); asm_output("idivl edx:eax, %s",RL(r)); }

    void Assembler::SHR( R r)   { emitr(X64_shr,  r); asm_output("shrl %s, ecx", RL(r)); }
    void Assembler::SAR( R r)   { emitr(X64_sar,  r); asm_output("sarl %s, ecx", RL(r)); }
    void Assembler::SHL( R r)   { emitr(X64_shl,  r); asm_output("shll %s, ecx", RL(r)); }
    void Assembler::SHRQ(R r)   { emitr(X64_shrq, r); asm_output("shrq %s, ecx", RQ(r)); }
    void Assembler::SARQ(R r)   { emitr(X64_sarq, r); asm_output("sarq %s, ecx", RQ(r)); }
    void Assembler::SHLQ(R r)   { emitr(X64_shlq, r); asm_output("shlq %s, ecx", RQ(r)); }

    void Assembler::SHRI( R r, I i)   { emit8(rexrb(X64_shri  | U64(r&7)<<48, (R)0, r), i); asm_output("shrl %s, %d", RL(r), i); }
    void Assembler::SARI( R r, I i)   { emit8(rexrb(X64_sari  | U64(r&7)<<48, (R)0, r), i); asm_output("sarl %s, %d", RL(r), i); }
    void Assembler::SHLI( R r, I i)   { emit8(rexrb(X64_shli  | U64(r&7)<<48, (R)0, r), i); asm_output("shll %s, %d", RL(r), i); }
    void Assembler::SHRQI(R r, I i)   { emit8(rexrb(X64_shrqi | U64(r&7)<<48, (R)0, r), i); asm_output("shrq %s, %d", RQ(r), i); }
    void Assembler::SARQI(R r, I i)   { emit8(rexrb(X64_sarqi | U64(r&7)<<48, (R)0, r), i); asm_output("sarq %s, %d", RQ(r), i); }
    void Assembler::SHLQI(R r, I i)   { emit8(rexrb(X64_shlqi | U64(r&7)<<48, (R)0, r), i); asm_output("shlq %s, %d", RQ(r), i); }

    void Assembler::SETE( R r)  { emitr8(X64_sete, r); asm_output("sete %s", RB(r)); }
    void Assembler::SETL( R r)  { emitr8(X64_setl, r); asm_output("setl %s", RB(r)); }
    void Assembler::SETLE(R r)  { emitr8(X64_setle,r); asm_output("setle %s",RB(r)); }
    void Assembler::SETG( R r)  { emitr8(X64_setg, r); asm_output("setg %s", RB(r)); }
    void Assembler::SETGE(R r)  { emitr8(X64_setge,r); asm_output("setge %s",RB(r)); }
    void Assembler::SETB( R r)  { emitr8(X64_setb, r); asm_output("setb %s", RB(r)); }
    void Assembler::SETBE(R r)  { emitr8(X64_setbe,r); asm_output("setbe %s",RB(r)); }
    void Assembler::SETA( R r)  { emitr8(X64_seta, r); asm_output("seta %s", RB(r)); }
    void Assembler::SETAE(R r)  { emitr8(X64_setae,r); asm_output("setae %s",RB(r)); }
    void Assembler::SETO( R r)  { emitr8(X64_seto, r); asm_output("seto %s", RB(r)); }

    void Assembler::ADDRR(R l, R r)     { emitrr(X64_addrr,l,r); asm_output("addl %s, %s", RL(l),RL(r)); }
    void Assembler::SUBRR(R l, R r)     { emitrr(X64_subrr,l,r); asm_output("subl %s, %s", RL(l),RL(r)); }
    void Assembler::ANDRR(R l, R r)     { emitrr(X64_andrr,l,r); asm_output("andl %s, %s", RL(l),RL(r)); }
    void Assembler::ORLRR(R l, R r)     { emitrr(X64_orlrr,l,r); asm_output("orl %s, %s",  RL(l),RL(r)); }
    void Assembler::XORRR(R l, R r)     { emitrr(X64_xorrr,l,r); asm_output("xorl %s, %s", RL(l),RL(r)); }
    void Assembler::IMUL( R l, R r)     { emitrr(X64_imul, l,r); asm_output("imull %s, %s",RL(l),RL(r)); }
    void Assembler::CMPLR(R l, R r)     { emitrr(X64_cmplr,l,r); asm_output("cmpl %s, %s", RL(l),RL(r)); }
    void Assembler::MOVLR(R l, R r)     { emitrr(X64_movlr,l,r); asm_output("movl %s, %s", RL(l),RL(r)); }

    void Assembler::ADDQRR( R l, R r)   { emitrr(X64_addqrr, l,r); asm_output("addq %s, %s",  RQ(l),RQ(r)); }
    void Assembler::SUBQRR( R l, R r)   { emitrr(X64_subqrr, l,r); asm_output("subq %s, %s",  RQ(l),RQ(r)); }
    void Assembler::ANDQRR( R l, R r)   { emitrr(X64_andqrr, l,r); asm_output("andq %s, %s",  RQ(l),RQ(r)); }
    void Assembler::ORQRR(  R l, R r)   { emitrr(X64_orqrr,  l,r); asm_output("orq %s, %s",   RQ(l),RQ(r)); }
    void Assembler::XORQRR( R l, R r)   { emitrr(X64_xorqrr, l,r); asm_output("xorq %s, %s",  RQ(l),RQ(r)); }
    void Assembler::CMPQR(  R l, R r)   { emitrr(X64_cmpqr,  l,r); asm_output("cmpq %s, %s",  RQ(l),RQ(r)); }
    void Assembler::MOVQR(  R l, R r)   { emitrr(X64_movqr,  l,r); asm_output("movq %s, %s",  RQ(l),RQ(r)); }
    void Assembler::MOVAPSR(R l, R r)   { emitrr(X64_movapsr,l,r); asm_output("movaps %s, %s",RQ(l),RQ(r)); }

    void Assembler::CMOVNO( R l, R r)   { emitrr(X64_cmovno, l,r); asm_output("cmovlno %s, %s",  RL(l),RL(r)); }
    void Assembler::CMOVNE( R l, R r)   { emitrr(X64_cmovne, l,r); asm_output("cmovlne %s, %s",  RL(l),RL(r)); }
    void Assembler::CMOVNL( R l, R r)   { emitrr(X64_cmovnl, l,r); asm_output("cmovlnl %s, %s",  RL(l),RL(r)); }
    void Assembler::CMOVNLE(R l, R r)   { emitrr(X64_cmovnle,l,r); asm_output("cmovlnle %s, %s", RL(l),RL(r)); }
    void Assembler::CMOVNG( R l, R r)   { emitrr(X64_cmovng, l,r); asm_output("cmovlng %s, %s",  RL(l),RL(r)); }
    void Assembler::CMOVNGE(R l, R r)   { emitrr(X64_cmovnge,l,r); asm_output("cmovlnge %s, %s", RL(l),RL(r)); }
    void Assembler::CMOVNB( R l, R r)   { emitrr(X64_cmovnb, l,r); asm_output("cmovlnb %s, %s",  RL(l),RL(r)); }
    void Assembler::CMOVNBE(R l, R r)   { emitrr(X64_cmovnbe,l,r); asm_output("cmovlnbe %s, %s", RL(l),RL(r)); }
    void Assembler::CMOVNA( R l, R r)   { emitrr(X64_cmovna, l,r); asm_output("cmovlna %s, %s",  RL(l),RL(r)); }
    void Assembler::CMOVNAE(R l, R r)   { emitrr(X64_cmovnae,l,r); asm_output("cmovlnae %s, %s", RL(l),RL(r)); }

    void Assembler::CMOVQNO( R l, R r)  { emitrr(X64_cmovqno, l,r); asm_output("cmovqno %s, %s",  RQ(l),RQ(r)); }
    void Assembler::CMOVQNE( R l, R r)  { emitrr(X64_cmovqne, l,r); asm_output("cmovqne %s, %s",  RQ(l),RQ(r)); }
    void Assembler::CMOVQNL( R l, R r)  { emitrr(X64_cmovqnl, l,r); asm_output("cmovqnl %s, %s",  RQ(l),RQ(r)); }
    void Assembler::CMOVQNLE(R l, R r)  { emitrr(X64_cmovqnle,l,r); asm_output("cmovqnle %s, %s", RQ(l),RQ(r)); }
    void Assembler::CMOVQNG( R l, R r)  { emitrr(X64_cmovqng, l,r); asm_output("cmovqng %s, %s",  RQ(l),RQ(r)); }
    void Assembler::CMOVQNGE(R l, R r)  { emitrr(X64_cmovqnge,l,r); asm_output("cmovqnge %s, %s", RQ(l),RQ(r)); }
    void Assembler::CMOVQNB( R l, R r)  { emitrr(X64_cmovqnb, l,r); asm_output("cmovqnb %s, %s",  RQ(l),RQ(r)); }
    void Assembler::CMOVQNBE(R l, R r)  { emitrr(X64_cmovqnbe,l,r); asm_output("cmovqnbe %s, %s", RQ(l),RQ(r)); }
    void Assembler::CMOVQNA( R l, R r)  { emitrr(X64_cmovqna, l,r); asm_output("cmovqna %s, %s",  RQ(l),RQ(r)); }
    void Assembler::CMOVQNAE(R l, R r)  { emitrr(X64_cmovqnae,l,r); asm_output("cmovqnae %s, %s", RQ(l),RQ(r)); }

    void Assembler::MOVSXDR(R l, R r)   { emitrr(X64_movsxdr,l,r); asm_output("movsxd %s, %s",RQ(l),RL(r)); }

    void Assembler::MOVZX8(R l, R r)    { emitrr8(X64_movzx8,l,r); asm_output("movzx %s, %s",RQ(l),RB(r)); }




    void Assembler::XORPS(        R r)  { emitprr(X64_xorps,   r,r); asm_output("xorps %s, %s",   RQ(r),RQ(r)); }
    void Assembler::DIVSD(   R l, R r)  { emitprr(X64_divsd,   l,r); asm_output("divsd %s, %s",   RQ(l),RQ(r)); }
    void Assembler::MULSD(   R l, R r)  { emitprr(X64_mulsd,   l,r); asm_output("mulsd %s, %s",   RQ(l),RQ(r)); }
    void Assembler::ADDSD(   R l, R r)  { emitprr(X64_addsd,   l,r); asm_output("addsd %s, %s",   RQ(l),RQ(r)); }
    void Assembler::SUBSD(   R l, R r)  { emitprr(X64_subsd,   l,r); asm_output("subsd %s, %s",   RQ(l),RQ(r)); }
    void Assembler::CVTSQ2SD(R l, R r)  { emitprr(X64_cvtsq2sd,l,r); asm_output("cvtsq2sd %s, %s",RQ(l),RQ(r)); }
    void Assembler::CVTSI2SD(R l, R r)  { emitprr(X64_cvtsi2sd,l,r); asm_output("cvtsi2sd %s, %s",RQ(l),RL(r)); }
    void Assembler::UCOMISD( R l, R r)  { emitprr(X64_ucomisd, l,r); asm_output("ucomisd %s, %s", RQ(l),RQ(r)); }
    void Assembler::MOVQRX(  R l, R r)  { emitprr(X64_movqrx,  r,l); asm_output("movq %s, %s",    RQ(l),RQ(r)); } 
    void Assembler::MOVQXR(  R l, R r)  { emitprr(X64_movqxr,  l,r); asm_output("movq %s, %s",    RQ(l),RQ(r)); }


    void Assembler::MOVI(  R r, I32 i32)    { emitr_imm(X64_movi,  r,i32); asm_output("movl %s, %d",RL(r),i32); }
    void Assembler::ADDLRI(R r, I32 i32)    { emitr_imm(X64_addlri,r,i32); asm_output("addl %s, %d",RL(r),i32); }
    void Assembler::SUBLRI(R r, I32 i32)    { emitr_imm(X64_sublri,r,i32); asm_output("subl %s, %d",RL(r),i32); }
    void Assembler::ANDLRI(R r, I32 i32)    { emitr_imm(X64_andlri,r,i32); asm_output("andl %s, %d",RL(r),i32); }
    void Assembler::ORLRI( R r, I32 i32)    { emitr_imm(X64_orlri, r,i32); asm_output("orl %s, %d", RL(r),i32); }
    void Assembler::XORLRI(R r, I32 i32)    { emitr_imm(X64_xorlri,r,i32); asm_output("xorl %s, %d",RL(r),i32); }
    void Assembler::CMPLRI(R r, I32 i32)    { emitr_imm(X64_cmplri,r,i32); asm_output("cmpl %s, %d",RL(r),i32); }

    void Assembler::ADDQRI( R r, I32 i32)   { emitr_imm(X64_addqri, r,i32); asm_output("addq %s, %d",   RQ(r),i32); }
    void Assembler::SUBQRI( R r, I32 i32)   { emitr_imm(X64_subqri, r,i32); asm_output("subq %s, %d",   RQ(r),i32); }
    void Assembler::ANDQRI( R r, I32 i32)   { emitr_imm(X64_andqri, r,i32); asm_output("andq %s, %d",   RQ(r),i32); }
    void Assembler::ORQRI(  R r, I32 i32)   { emitr_imm(X64_orqri,  r,i32); asm_output("orq %s, %d",    RQ(r),i32); }
    void Assembler::XORQRI( R r, I32 i32)   { emitr_imm(X64_xorqri, r,i32); asm_output("xorq %s, %d",   RQ(r),i32); }
    void Assembler::CMPQRI( R r, I32 i32)   { emitr_imm(X64_cmpqri, r,i32); asm_output("cmpq %s, %d",   RQ(r),i32); }
    void Assembler::MOVQI32(R r, I32 i32)   { emitr_imm(X64_movqi32,r,i32); asm_output("movqi32 %s, %d",RQ(r),i32); }

    void Assembler::ADDLR8(R r, I32 i8)     { emitr_imm8(X64_addlr8,r,i8); asm_output("addl %s, %d", RL(r),i8); }
    void Assembler::SUBLR8(R r, I32 i8)     { emitr_imm8(X64_sublr8,r,i8); asm_output("subl %s, %d", RL(r),i8); }
    void Assembler::ANDLR8(R r, I32 i8)     { emitr_imm8(X64_andlr8,r,i8); asm_output("andl %s, %d", RL(r),i8); }
    void Assembler::ORLR8( R r, I32 i8)     { emitr_imm8(X64_orlr8, r,i8); asm_output("orl %s, %d",  RL(r),i8); }
    void Assembler::XORLR8(R r, I32 i8)     { emitr_imm8(X64_xorlr8,r,i8); asm_output("xorl %s, %d", RL(r),i8); }
    void Assembler::CMPLR8(R r, I32 i8)     { emitr_imm8(X64_cmplr8,r,i8); asm_output("cmpl %s, %d", RL(r),i8); }

    void Assembler::ADDQR8(R r, I32 i8)     { emitr_imm8(X64_addqr8,r,i8); asm_output("addq %s, %d",RQ(r),i8); }
    void Assembler::SUBQR8(R r, I32 i8)     { emitr_imm8(X64_subqr8,r,i8); asm_output("subq %s, %d",RQ(r),i8); }
    void Assembler::ANDQR8(R r, I32 i8)     { emitr_imm8(X64_andqr8,r,i8); asm_output("andq %s, %d",RQ(r),i8); }
    void Assembler::ORQR8( R r, I32 i8)     { emitr_imm8(X64_orqr8, r,i8); asm_output("orq %s, %d", RQ(r),i8); }
    void Assembler::XORQR8(R r, I32 i8)     { emitr_imm8(X64_xorqr8,r,i8); asm_output("xorq %s, %d",RQ(r),i8); }
    void Assembler::CMPQR8(R r, I32 i8)     { emitr_imm8(X64_cmpqr8,r,i8); asm_output("cmpq %s, %d",RQ(r),i8); }

    void Assembler::IMULI(R l, R r, I32 i32)    { emitrr_imm(X64_imuli,l,r,i32); asm_output("imuli %s, %s, %d",RL(l),RL(r),i32); }

    void Assembler::MOVQI(R r, U64 u64)         { emitr_imm64(X64_movqi,r,u64); asm_output("movq %s, %p",RQ(r),(void*)u64); }

    void Assembler::LEARIP(R r, I32 d)          { emitrm(X64_learip,r,d,(Register)0); asm_output("lea %s, %d(rip)",RQ(r),d); }

    void Assembler::LEAQRM(R r1, I d, R r2)     { emitrm(X64_leaqrm,r1,d,r2); asm_output("leaq %s, %d(%s)",RQ(r1),d,RQ(r2)); }
    void Assembler::MOVLRM(R r1, I d, R r2)     { emitrm(X64_movlrm,r1,d,r2); asm_output("movl %s, %d(%s)",RL(r1),d,RQ(r2)); }
    void Assembler::MOVQRM(R r1, I d, R r2)     { emitrm(X64_movqrm,r1,d,r2); asm_output("movq %s, %d(%s)",RQ(r1),d,RQ(r2)); }
    void Assembler::MOVLMR(R r1, I d, R r2)     { emitrm(X64_movlmr,r1,d,r2); asm_output("movl %d(%s), %s",d,RQ(r1),RL(r2)); }
    void Assembler::MOVQMR(R r1, I d, R r2)     { emitrm(X64_movqmr,r1,d,r2); asm_output("movq %d(%s), %s",d,RQ(r1),RQ(r2)); }

    void Assembler::MOVZX8M( R r1, I d, R r2)   { emitrm_wide(X64_movzx8m, r1,d,r2); asm_output("movzxb %s, %d(%s)",RQ(r1),d,RQ(r2)); }
    void Assembler::MOVZX16M(R r1, I d, R r2)   { emitrm_wide(X64_movzx16m,r1,d,r2); asm_output("movzxs %s, %d(%s)",RQ(r1),d,RQ(r2)); }

    void Assembler::MOVSDRM(R r1, I d, R r2)    { emitprm(X64_movsdrm,r1,d,r2); asm_output("movsd %s, %d(%s)",RQ(r1),d,RQ(r2)); }
    void Assembler::MOVSDMR(R r1, I d, R r2)    { emitprm(X64_movsdmr,r1,d,r2); asm_output("movsd %d(%s), %s",d,RQ(r1),RQ(r2)); }

    void Assembler::JMP8( S n, NIns* t)    { emit_target8(n, X64_jmp8,t); asm_output("jmp %p", t); }

    void Assembler::JMP32(S n, NIns* t)    { emit_target32(n,X64_jmp, t); asm_output("jmp %p", t); }

    void Assembler::JO( S n, NIns* t)      { emit_target32(n,X64_jo,  t); asm_output("jo %p", t); }
    void Assembler::JE( S n, NIns* t)      { emit_target32(n,X64_je,  t); asm_output("je %p", t); }
    void Assembler::JL( S n, NIns* t)      { emit_target32(n,X64_jl,  t); asm_output("jl %p", t); }
    void Assembler::JLE(S n, NIns* t)      { emit_target32(n,X64_jle, t); asm_output("jle %p",t); }
    void Assembler::JG( S n, NIns* t)      { emit_target32(n,X64_jg,  t); asm_output("jg %p", t); }
    void Assembler::JGE(S n, NIns* t)      { emit_target32(n,X64_jge, t); asm_output("jge %p",t); }
    void Assembler::JB( S n, NIns* t)      { emit_target32(n,X64_jb,  t); asm_output("jb %p", t); }
    void Assembler::JBE(S n, NIns* t)      { emit_target32(n,X64_jbe, t); asm_output("jbe %p",t); }
    void Assembler::JA( S n, NIns* t)      { emit_target32(n,X64_ja,  t); asm_output("ja %p", t); }
    void Assembler::JAE(S n, NIns* t)      { emit_target32(n,X64_jae, t); asm_output("jae %p",t); }
    void Assembler::JP( S n, NIns* t)      { emit_target32(n,X64_jp,  t); asm_output("jp  %p",t); }

    void Assembler::JNO( S n, NIns* t)     { emit_target32(n,X64_jo ^X64_jneg, t); asm_output("jno %p", t); }
    void Assembler::JNE( S n, NIns* t)     { emit_target32(n,X64_je ^X64_jneg, t); asm_output("jne %p", t); }
    void Assembler::JNL( S n, NIns* t)     { emit_target32(n,X64_jl ^X64_jneg, t); asm_output("jnl %p", t); }
    void Assembler::JNLE(S n, NIns* t)     { emit_target32(n,X64_jle^X64_jneg, t); asm_output("jnle %p",t); }
    void Assembler::JNG( S n, NIns* t)     { emit_target32(n,X64_jg ^X64_jneg, t); asm_output("jng %p", t); }
    void Assembler::JNGE(S n, NIns* t)     { emit_target32(n,X64_jge^X64_jneg, t); asm_output("jnge %p",t); }
    void Assembler::JNB( S n, NIns* t)     { emit_target32(n,X64_jb ^X64_jneg, t); asm_output("jnb %p", t); }
    void Assembler::JNBE(S n, NIns* t)     { emit_target32(n,X64_jbe^X64_jneg, t); asm_output("jnbe %p",t); }
    void Assembler::JNA( S n, NIns* t)     { emit_target32(n,X64_ja ^X64_jneg, t); asm_output("jna %p", t); }
    void Assembler::JNAE(S n, NIns* t)     { emit_target32(n,X64_jae^X64_jneg, t); asm_output("jnae %p",t); }

    void Assembler::JO8( S n, NIns* t)     { emit_target8(n,X64_jo8,  t); asm_output("jo %p", t); }
    void Assembler::JE8( S n, NIns* t)     { emit_target8(n,X64_je8,  t); asm_output("je %p", t); }
    void Assembler::JL8( S n, NIns* t)     { emit_target8(n,X64_jl8,  t); asm_output("jl %p", t); }
    void Assembler::JLE8(S n, NIns* t)     { emit_target8(n,X64_jle8, t); asm_output("jle %p",t); }
    void Assembler::JG8( S n, NIns* t)     { emit_target8(n,X64_jg8,  t); asm_output("jg %p", t); }
    void Assembler::JGE8(S n, NIns* t)     { emit_target8(n,X64_jge8, t); asm_output("jge %p",t); }
    void Assembler::JB8( S n, NIns* t)     { emit_target8(n,X64_jb8,  t); asm_output("jb %p", t); }
    void Assembler::JBE8(S n, NIns* t)     { emit_target8(n,X64_jbe8, t); asm_output("jbe %p",t); }
    void Assembler::JA8( S n, NIns* t)     { emit_target8(n,X64_ja8,  t); asm_output("ja %p", t); }
    void Assembler::JAE8(S n, NIns* t)     { emit_target8(n,X64_jae8, t); asm_output("jae %p",t); }
    void Assembler::JP8( S n, NIns* t)     { emit_target8(n,X64_jp8,  t); asm_output("jp  %p",t); }

    void Assembler::JNO8( S n, NIns* t)    { emit_target8(n,X64_jo8 ^X64_jneg8, t); asm_output("jno %p", t); }
    void Assembler::JNE8( S n, NIns* t)    { emit_target8(n,X64_je8 ^X64_jneg8, t); asm_output("jne %p", t); }
    void Assembler::JNL8( S n, NIns* t)    { emit_target8(n,X64_jl8 ^X64_jneg8, t); asm_output("jnl %p", t); }
    void Assembler::JNLE8(S n, NIns* t)    { emit_target8(n,X64_jle8^X64_jneg8, t); asm_output("jnle %p",t); }
    void Assembler::JNG8( S n, NIns* t)    { emit_target8(n,X64_jg8 ^X64_jneg8, t); asm_output("jng %p", t); }
    void Assembler::JNGE8(S n, NIns* t)    { emit_target8(n,X64_jge8^X64_jneg8, t); asm_output("jnge %p",t); }
    void Assembler::JNB8( S n, NIns* t)    { emit_target8(n,X64_jb8 ^X64_jneg8, t); asm_output("jnb %p", t); }
    void Assembler::JNBE8(S n, NIns* t)    { emit_target8(n,X64_jbe8^X64_jneg8, t); asm_output("jnbe %p",t); }
    void Assembler::JNA8( S n, NIns* t)    { emit_target8(n,X64_ja8 ^X64_jneg8, t); asm_output("jna %p", t); }
    void Assembler::JNAE8(S n, NIns* t)    { emit_target8(n,X64_jae8^X64_jneg8, t); asm_output("jnae %p",t); }

    void Assembler::CALL( S n, NIns* t)    { emit_target32(n,X64_call,t); asm_output("call %p",t); }

    void Assembler::CALLRAX()       { emit(X64_callrax); asm_output("call (rax)"); }
    void Assembler::RET()           { emit(X64_ret);     asm_output("ret");        }

    void Assembler::MOVQSPR(I d, R r)   { emit(X64_movqspr | U64(d) << 56 | U64((r&7)<<3) << 40 | U64((r&8)>>1) << 24); asm_output("movq %d(rsp), %s", d, RQ(r)); }    

    void Assembler::XORPSA(R r, I32 i32)    { emitxm_abs(X64_xorpsa, r, i32); asm_output("xorps %s, (0x%x)",RQ(r), i32); }
    void Assembler::XORPSM(R r, NIns* a64)  { emitxm_rel(X64_xorpsm, r, a64); asm_output("xorps %s, (%p)",  RQ(r), a64); }

    void Assembler::X86_AND8R(R r)  { emit(X86_and8r | U64(r<<3|(r|4))<<56); asm_output("andb %s, %s", RB(r), RBhi(r)); }
    void Assembler::X86_SETNP(R r)  { emit(X86_setnp | U64(r|4)<<56); asm_output("setnp %s", RBhi(r)); }
    void Assembler::X86_SETE(R r)   { emit(X86_sete  | U64(r)<<56);   asm_output("sete %s", RB(r)); }

#undef R
#undef I
#undef I32
#undef U64
#undef S

    void Assembler::MR(Register d, Register s) {
        NanoAssert(IsGpReg(d) && IsGpReg(s));
        MOVQR(d, s);
    }

    
    
    
    void Assembler::JMPl(NIns* target) {
        JMP32(8, target);
    }

    void Assembler::JMP(NIns *target) {
        if (!target || isTargetWithinS32(target)) {
            if (target && isTargetWithinS8(target)) {
                JMP8(8, target);
            } else {
                JMP32(8, target);
            }
        } else {
            TODO(jmp64);
        }
    }

    
    void Assembler::regalloc_binary(LIns *ins, RegisterMask allow, Register &rr, Register &ra, Register &rb) {
#ifdef _DEBUG
        RegisterMask originalAllow = allow;
#endif
        rb = UnknownReg;
        LIns *a = ins->oprnd1();
        LIns *b = ins->oprnd2();
        if (a != b) {
            rb = findRegFor(b, allow);
            allow &= ~rmask(rb);
        }
        rr = prepResultReg(ins, allow);
        
        if (a->isUnusedOrHasUnknownReg()) {
            ra = findSpecificRegForUnallocated(a, rr);
        } else if (!(allow & rmask(a->getReg()))) {
            
            
            
            
            NanoAssert(a->isop(LIR_quad) || a->isop(LIR_ldq) || a->isop(LIR_ldqc)|| a->isop(LIR_u2f) || a->isop(LIR_float));
            allow &= ~rmask(rr);
            ra = findRegFor(a, allow);
        } else {
            ra = a->getReg();
        }
        if (a == b) {
            rb = ra;
        }
        NanoAssert(originalAllow & rmask(rr));
        NanoAssert(originalAllow & rmask(ra));
        NanoAssert(originalAllow & rmask(rb));
    }

    void Assembler::asm_qbinop(LIns *ins) {
        asm_arith(ins);
    }

    void Assembler::asm_shift(LIns *ins) {
        
        LIns *b = ins->oprnd2();
        if (b->isconst()) {
            asm_shift_imm(ins);
            return;
        }
        Register rr, ra;
        if (b != ins->oprnd1()) {
            findSpecificRegFor(b, RCX);
            regalloc_unary(ins, GpRegs & ~rmask(RCX), rr, ra);
        } else {
            
            regalloc_unary(ins, rmask(RCX), rr, ra);
        }
        switch (ins->opcode()) {
        default:
            TODO(asm_shift);
        case LIR_qursh: SHRQ(rr);   break;
        case LIR_qirsh: SARQ(rr);   break;
        case LIR_qilsh: SHLQ(rr);   break;
        case LIR_ush:   SHR( rr);   break;
        case LIR_rsh:   SAR( rr);   break;
        case LIR_lsh:   SHL( rr);   break;
        }
        if (rr != ra)
            MR(rr, ra);
    }

    void Assembler::asm_shift_imm(LIns *ins) {
        Register rr, ra;
        regalloc_unary(ins, GpRegs, rr, ra);
        int shift = ins->oprnd2()->imm32() & 63;
        switch (ins->opcode()) {
        default: TODO(shiftimm);
        case LIR_qursh: SHRQI(rr, shift);   break;
        case LIR_qirsh: SARQI(rr, shift);   break;
        case LIR_qilsh: SHLQI(rr, shift);   break;
        case LIR_ush:   SHRI( rr, shift);   break;
        case LIR_rsh:   SARI( rr, shift);   break;
        case LIR_lsh:   SHLI( rr, shift);   break;
        }
        if (rr != ra)
            MR(rr, ra);
    }

    static bool isImm32(LIns *ins) {
        return ins->isconst() || (ins->isconstq() && isS32(ins->imm64()));
    }
    static int32_t getImm32(LIns *ins) {
        return ins->isconst() ? ins->imm32() : int32_t(ins->imm64());
    }

    
    void Assembler::asm_arith_imm(LIns *ins) {
        LIns *b = ins->oprnd2();
        int32_t imm = getImm32(b);
        LOpcode op = ins->opcode();
        Register rr, ra;
        if (op == LIR_mul) {
            
            rr = prepResultReg(ins, GpRegs);
            LIns *a = ins->oprnd1();
            ra = findRegFor(a, GpRegs);
            IMULI(rr, ra, imm);
            return;
        }
        regalloc_unary(ins, GpRegs, rr, ra);
        if (isS8(imm)) {
            switch (ins->opcode()) {
            default: TODO(arith_imm8);
            case LIR_iaddp:
            case LIR_add:   ADDLR8(rr, imm);   break;
            case LIR_and:   ANDLR8(rr, imm);   break;
            case LIR_or:    ORLR8( rr, imm);   break;
            case LIR_sub:   SUBLR8(rr, imm);   break;
            case LIR_xor:   XORLR8(rr, imm);   break;
            case LIR_qiadd:
            case LIR_qaddp: ADDQR8(rr, imm);   break;
            case LIR_qiand: ANDQR8(rr, imm);   break;
            case LIR_qior:  ORQR8( rr, imm);   break;
            case LIR_qxor:  XORQR8(rr, imm);   break;
            }
        } else {
            switch (ins->opcode()) {
            default: TODO(arith_imm);
            case LIR_iaddp:
            case LIR_add:   ADDLRI(rr, imm);   break;
            case LIR_and:   ANDLRI(rr, imm);   break;
            case LIR_or:    ORLRI( rr, imm);   break;
            case LIR_sub:   SUBLRI(rr, imm);   break;
            case LIR_xor:   XORLRI(rr, imm);   break;
            case LIR_qiadd:
            case LIR_qaddp: ADDQRI(rr, imm);   break;
            case LIR_qiand: ANDQRI(rr, imm);   break;
            case LIR_qior:  ORQRI( rr, imm);   break;
            case LIR_qxor:  XORQRI(rr, imm);   break;
            }
        }
        if (rr != ra)
            MR(rr, ra);
    }

    void Assembler::asm_div_mod(LIns *ins) {
        LIns *div;
        if (ins->opcode() == LIR_mod) {
            
            div = ins->oprnd1();
            prepResultReg(ins, rmask(RDX));
        } else {
            div = ins;
            evictIfActive(RDX);
        }

        NanoAssert(div->isop(LIR_div));

        LIns *lhs = div->oprnd1();
        LIns *rhs = div->oprnd2();

        prepResultReg(div, rmask(RAX));

        Register rhsReg = findRegFor(rhs, (GpRegs ^ (rmask(RAX)|rmask(RDX))));
        Register lhsReg = lhs->isUnusedOrHasUnknownReg()
                          ? findSpecificRegForUnallocated(lhs, RAX)
                          : lhs->getReg();
        IDIV(rhsReg);
        SARI(RDX, 31);
        MR(RDX, RAX);
        if (RAX != lhsReg)
            MR(RAX, lhsReg);
    }

    
    void Assembler::asm_arith(LIns *ins) {
        Register rr, ra, rb;

        switch (ins->opcode() & ~LIR64) {
        case LIR_lsh:
        case LIR_rsh:
        case LIR_ush:
            asm_shift(ins);
            return;
        case LIR_mod:
        case LIR_div:
            asm_div_mod(ins);
            return;
        default:
            break;
        }

        LIns *b = ins->oprnd2();
        if (isImm32(b)) {
            asm_arith_imm(ins);
            return;
        }
        regalloc_binary(ins, GpRegs, rr, ra, rb);
        switch (ins->opcode()) {
        default:        TODO(asm_arith);
        case LIR_or:    ORLRR(rr, rb);  break;
        case LIR_sub:   SUBRR(rr, rb);  break;
        case LIR_iaddp:
        case LIR_add:   ADDRR(rr, rb);  break;
        case LIR_and:   ANDRR(rr, rb);  break;
        case LIR_xor:   XORRR(rr, rb);  break;
        case LIR_mul:   IMUL(rr, rb);   break;
        case LIR_qxor:  XORQRR(rr, rb); break;
        case LIR_qior:  ORQRR(rr, rb);  break;
        case LIR_qiand: ANDQRR(rr, rb); break;
        case LIR_qiadd:
        case LIR_qaddp: ADDQRR(rr, rb); break;
        }
        if (rr != ra)
            MR(rr,ra);
    }

    
    void Assembler::asm_fop(LIns *ins) {
        Register rr, ra, rb;
        regalloc_binary(ins, FpRegs, rr, ra, rb);
        switch (ins->opcode()) {
        default:       TODO(asm_fop);
        case LIR_fdiv: DIVSD(rr, rb); break;
        case LIR_fmul: MULSD(rr, rb); break;
        case LIR_fadd: ADDSD(rr, rb); break;
        case LIR_fsub: SUBSD(rr, rb); break;
        }
        if (rr != ra) {
            asm_nongp_copy(rr, ra);
        }
    }

    void Assembler::asm_neg_not(LIns *ins) {
        Register rr, ra;
        regalloc_unary(ins, GpRegs, rr, ra);
        NanoAssert(IsGpReg(ra));
        if (ins->isop(LIR_not))
            NOT(rr);
        else
            NEG(rr);
        if (rr != ra)
            MR(rr, ra);
    }

    void Assembler::asm_call(LIns *ins) {
        const CallInfo *call = ins->callInfo();
        ArgSize sizes[MAXARGS];
        int argc = call->get_sizes(sizes);

        bool indirect = call->isIndirect();
        if (!indirect) {
            verbose_only(if (_logc->lcbits & LC_Assembly)
                outputf("        %p:", _nIns);
            )
            NIns *target = (NIns*)call->_address;
            if (isTargetWithinS32(target)) {
                CALL(8, target);
            } else {
                
                CALLRAX();
                asm_quad(RAX, (uint64_t)target);
            }
        } else {
            
            
            
            asm_regarg(ARGSIZE_P, ins->arg(--argc), RAX);
            CALLRAX();
        }

    #ifdef _WIN64
        int stk_used = 32; 
    #else
        int stk_used = 0;
        Register fr = XMM0;
    #endif
        int arg_index = 0;
        for (int i = 0; i < argc; i++) {
            int j = argc - i - 1;
            ArgSize sz = sizes[j];
            LIns* arg = ins->arg(j);
            if ((sz & ARGSIZE_MASK_INT) && arg_index < NumArgRegs) {
                
                asm_regarg(sz, arg, argRegs[arg_index]);
                arg_index++;
            }
        #ifdef _WIN64
            else if (sz == ARGSIZE_F && arg_index < NumArgRegs) {
                
                asm_regarg(sz, arg, Register(XMM0+arg_index));
                arg_index++;
            }
        #else
            else if (sz == ARGSIZE_F && fr < XMM8) {
                
                asm_regarg(sz, arg, fr);
                fr = nextreg(fr);
            }
        #endif
            else {
                asm_stkarg(sz, arg, stk_used);
                stk_used += sizeof(void*);
            }
        }

        if (stk_used > max_stk_used)
            max_stk_used = stk_used;
    }

    void Assembler::asm_regarg(ArgSize sz, LIns *p, Register r) {
        if (sz == ARGSIZE_I) {
            NanoAssert(!p->isQuad());
            if (p->isconst()) {
                asm_quad(r, int64_t(p->imm32()));
                return;
            }
            
            MOVSXDR(r, r);
        } else if (sz == ARGSIZE_U) {
            NanoAssert(!p->isQuad());
            if (p->isconst()) {
                asm_quad(r, uint64_t(uint32_t(p->imm32())));
                return;
            }
            
            MOVLR(r, r);
        }
        






        findSpecificRegFor(p, r);
    }

    void Assembler::asm_stkarg(ArgSize sz, LIns *p, int stk_off) {
        NanoAssert(isS8(stk_off));
        if (sz & ARGSIZE_MASK_INT) {
            Register r = findRegFor(p, GpRegs);
            MOVQSPR(stk_off, r);    
            if (sz == ARGSIZE_I) {
                
                NanoAssert(!p->isQuad());
                MOVSXDR(r, r);
            } else if (sz == ARGSIZE_U) {
                
                NanoAssert(!p->isQuad());
                MOVLR(r, r);
            }
        } else {
            TODO(asm_stkarg_non_int);
        }
    }

    void Assembler::asm_promote(LIns *ins) {
        Register rr, ra;
        regalloc_unary(ins, GpRegs, rr, ra);
        NanoAssert(IsGpReg(ra));
        if (ins->isop(LIR_u2q)) {
            MOVLR(rr, ra);      
        } else {
            NanoAssert(ins->isop(LIR_i2q));
            MOVSXDR(rr, ra);    
        }
    }

    
    
    

    void Assembler::asm_i2f(LIns *ins) {
        Register r = prepResultReg(ins, FpRegs);
        Register b = findRegFor(ins->oprnd1(), GpRegs);
        CVTSI2SD(r, b);     
        XORPS(r);           
    }

    void Assembler::asm_u2f(LIns *ins) {
        Register r = prepResultReg(ins, FpRegs);
        Register b = findRegFor(ins->oprnd1(), GpRegs);
        NanoAssert(!ins->oprnd1()->isQuad());
        
        CVTSQ2SD(r, b);     
        XORPS(r);           
        MOVLR(b, b);        
    }

    void Assembler::asm_cmov(LIns *ins) {
        LIns* cond    = ins->oprnd1();
        LIns* iftrue  = ins->oprnd2();
        LIns* iffalse = ins->oprnd3();
        NanoAssert(cond->isCmp());
        NanoAssert((ins->isop(LIR_qcmov) && iftrue->isQuad() && iffalse->isQuad()) ||
                   (ins->isop(LIR_cmov) && !iftrue->isQuad() && !iffalse->isQuad()));

        
        
        const Register rr = prepResultReg(ins, GpRegs);
        const Register rf = findRegFor(iffalse, GpRegs & ~rmask(rr));

        LOpcode condop = cond->opcode();
        if (ins->opcode() == LIR_cmov) {
            switch (condop & ~LIR64) {
            case LIR_ov:  CMOVNO( rr, rf);  break;
            case LIR_eq:  CMOVNE( rr, rf);  break;
            case LIR_lt:  CMOVNL( rr, rf);  break;
            case LIR_gt:  CMOVNG( rr, rf);  break;
            case LIR_le:  CMOVNLE(rr, rf);  break;
            case LIR_ge:  CMOVNGE(rr, rf);  break;
            case LIR_ult: CMOVNB( rr, rf);  break;
            case LIR_ugt: CMOVNA( rr, rf);  break;
            case LIR_ule: CMOVNBE(rr, rf);  break;
            case LIR_uge: CMOVNAE(rr, rf);  break;
            default:      NanoAssert(0);    break;
            }
        } else {
            switch (condop & ~LIR64) {
            case LIR_ov:  CMOVQNO( rr, rf); break;
            case LIR_eq:  CMOVQNE( rr, rf); break;
            case LIR_lt:  CMOVQNL( rr, rf); break;
            case LIR_gt:  CMOVQNG( rr, rf); break;
            case LIR_le:  CMOVQNLE(rr, rf); break;
            case LIR_ge:  CMOVQNGE(rr, rf); break;
            case LIR_ult: CMOVQNB( rr, rf); break;
            case LIR_ugt: CMOVQNA( rr, rf); break;
            case LIR_ule: CMOVQNBE(rr, rf); break;
            case LIR_uge: CMOVQNAE(rr, rf); break;
            default:      NanoAssert(0);    break;
            }
        }
         findSpecificRegFor(iftrue, rr);
        asm_cmp(cond);
    }

    NIns* Assembler::asm_branch(bool onFalse, LIns *cond, NIns *target) {
        LOpcode condop = cond->opcode();
        if (condop >= LIR_feq && condop <= LIR_fge)
            return asm_fbranch(onFalse, cond, target);

        
        
        NanoAssert((condop & ~LIR64) >= LIR_ov);
        NanoAssert((condop & ~LIR64) <= LIR_uge);
        if (target && isTargetWithinS8(target)) {
            if (onFalse) {
                switch (condop & ~LIR64) {
                case LIR_ov:  JNO8( 8, target); break;
                case LIR_eq:  JNE8( 8, target); break;
                case LIR_lt:  JNL8( 8, target); break;
                case LIR_gt:  JNG8( 8, target); break;
                case LIR_le:  JNLE8(8, target); break;
                case LIR_ge:  JNGE8(8, target); break;
                case LIR_ult: JNB8( 8, target); break;
                case LIR_ugt: JNA8( 8, target); break;
                case LIR_ule: JNBE8(8, target); break;
                case LIR_uge: JNAE8(8, target); break;
                default:      NanoAssert(0);    break;
                }
            } else {
                switch (condop & ~LIR64) {
                case LIR_ov:  JO8( 8, target);  break;
                case LIR_eq:  JE8( 8, target);  break;
                case LIR_lt:  JL8( 8, target);  break;
                case LIR_gt:  JG8( 8, target);  break;
                case LIR_le:  JLE8(8, target);  break;
                case LIR_ge:  JGE8(8, target);  break;
                case LIR_ult: JB8( 8, target);  break;
                case LIR_ugt: JA8( 8, target);  break;
                case LIR_ule: JBE8(8, target);  break;
                case LIR_uge: JAE8(8, target);  break;
                default:      NanoAssert(0);    break;
                }
            }
        } else {
            if (onFalse) {
                switch (condop & ~LIR64) {
                case LIR_ov:  JNO( 8, target);  break;
                case LIR_eq:  JNE( 8, target);  break;
                case LIR_lt:  JNL( 8, target);  break;
                case LIR_gt:  JNG( 8, target);  break;
                case LIR_le:  JNLE(8, target);  break;
                case LIR_ge:  JNGE(8, target);  break;
                case LIR_ult: JNB( 8, target);  break;
                case LIR_ugt: JNA( 8, target);  break;
                case LIR_ule: JNBE(8, target);  break;
                case LIR_uge: JNAE(8, target);  break;
                default:      NanoAssert(0);    break;
                }
            } else {
                switch (condop & ~LIR64) {
                case LIR_ov:  JO( 8, target);   break;
                case LIR_eq:  JE( 8, target);   break;
                case LIR_lt:  JL( 8, target);   break;
                case LIR_gt:  JG( 8, target);   break;
                case LIR_le:  JLE(8, target);   break;
                case LIR_ge:  JGE(8, target);   break;
                case LIR_ult: JB( 8, target);   break;
                case LIR_ugt: JA( 8, target);   break;
                case LIR_ule: JBE(8, target);   break;
                case LIR_uge: JAE(8, target);   break;
                default:      NanoAssert(0);    break;
                }
            }
        }
        NIns *patch = _nIns;            
        asm_cmp(cond);
        return patch;
    }

    void Assembler::asm_cmp(LIns *cond) {
        
        if (cond->opcode() == LIR_ov)
            return;
        LIns *b = cond->oprnd2();
        if (isImm32(b)) {
            asm_cmp_imm(cond);
            return;
        }
        LIns *a = cond->oprnd1();
        Register ra, rb;
        if (a != b) {
            findRegFor2b(GpRegs, a, ra, b, rb);
        } else {
            
            ra = rb = findRegFor(a, GpRegs);
        }

        LOpcode condop = cond->opcode();
        if (condop & LIR64)
            CMPQR(ra, rb);
        else
            CMPLR(ra, rb);
    }

    void Assembler::asm_cmp_imm(LIns *cond) {
        LIns *a = cond->oprnd1();
        LIns *b = cond->oprnd2();
        Register ra = findRegFor(a, GpRegs);
        int32_t imm = getImm32(b);
        if (isS8(imm)) {
            if (cond->opcode() & LIR64)
                CMPQR8(ra, imm);
            else 
                CMPLR8(ra, imm);
        } else {
            if (cond->opcode() & LIR64)
                CMPQRI(ra, imm);
            else
                CMPLRI(ra, imm);
        }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    NIns* Assembler::asm_fbranch(bool onFalse, LIns *cond, NIns *target) {
        LOpcode condop = cond->opcode();
        NIns *patch;
        LIns *a = cond->oprnd1();
        LIns *b = cond->oprnd2();
        if (condop == LIR_feq) {
            if (onFalse) {
                
                JP(16, target);     
                JNE(0, target);     
                patch = _nIns;
            } else {
                
                
                
                underrunProtect(16); 
                NIns *skip = _nIns;
                JE(0, target);      
                patch = _nIns;
                JP8(0, skip);       
            }
        }
        else {
            if (condop == LIR_flt) {
                condop = LIR_fgt;
                LIns *t = a; a = b; b = t;
            } else if (condop == LIR_fle) {
                condop = LIR_fge;
                LIns *t = a; a = b; b = t;
            }
            if (condop == LIR_fgt) {
                if (onFalse)
                    JBE(8, target);
                else
                    JA(8, target);
            } else { 
                if (onFalse)
                    JB(8, target);
                else
                    JAE(8, target);
            }
            patch = _nIns;
        }
        fcmp(a, b);
        return patch;
    }

    void Assembler::asm_fcond(LIns *ins) {
        LOpcode op = ins->opcode();
        LIns *a = ins->oprnd1();
        LIns *b = ins->oprnd2();
        if (op == LIR_feq) {
            
            
            Register r = prepResultReg(ins, 1<<RAX|1<<RCX|1<<RDX|1<<RBX);
            MOVZX8(r, r);       
            X86_AND8R(r);       
            X86_SETNP(r);       
            X86_SETE(r);        
        } else {
            if (op == LIR_flt) {
                op = LIR_fgt;
                LIns *t = a; a = b; b = t;
            } else if (op == LIR_fle) {
                op = LIR_fge;
                LIns *t = a; a = b; b = t;
            }
            Register r = prepResultReg(ins, GpRegs); 
            MOVZX8(r, r);
            if (op == LIR_fgt) 
                SETA(r);
            else
                SETAE(r);
        }
        fcmp(a, b);
    }

    void Assembler::fcmp(LIns *a, LIns *b) {
        Register ra, rb;
        findRegFor2b(FpRegs, a, ra, b, rb);
        UCOMISD(ra, rb);
    }

    void Assembler::asm_restore(LIns *ins, Reservation *, Register r) {
        (void) r;
        if (ins->isop(LIR_alloc)) {
            int d = disp(ins);
            LEAQRM(r, d, FP);
        }
        else if (ins->isconst()) {
            if (!ins->getArIndex()) {
                ins->markAsClear();
            }
            
            MOVI(r, ins->imm32());
        }
        else if (ins->isconstq() && IsGpReg(r)) {
            if (!ins->getArIndex()) {
                ins->markAsClear();
            }
            
            asm_quad(r, ins->imm64());
        }
        else {
            int d = findMemFor(ins);
            if (IsFpReg(r)) {
                NanoAssert(ins->isQuad());
                
                MOVSDRM(r, d, FP);
            } else if (ins->isQuad()) {
                MOVQRM(r, d, FP);
            } else {
                MOVLRM(r, d, FP);
            }
        }
        verbose_only( if (_logc->lcbits & LC_RegAlloc) {
                        outputForEOL("  <= restore %s",
                        _thisfrag->lirbuf->names->formatRef(ins)); } )
    }

    void Assembler::asm_cond(LIns *ins) {
        LOpcode op = ins->opcode();
        
        Register r = prepResultReg(ins, GpRegs);
        
        MOVZX8(r, r);
        switch (op) {
        default:
            TODO(cond);
        case LIR_qeq:
        case LIR_eq:    SETE(r);    break;
        case LIR_qlt:
        case LIR_lt:    SETL(r);    break;
        case LIR_qle:
        case LIR_le:    SETLE(r);   break;
        case LIR_qgt:
        case LIR_gt:    SETG(r);    break;
        case LIR_qge:
        case LIR_ge:    SETGE(r);   break;
        case LIR_qult:
        case LIR_ult:   SETB(r);    break;
        case LIR_qule:
        case LIR_ule:   SETBE(r);   break;
        case LIR_qugt:
        case LIR_ugt:   SETA(r);    break;
        case LIR_quge:
        case LIR_uge:   SETAE(r);   break;
        case LIR_ov:    SETO(r);    break;
        }
        asm_cmp(ins);
    }

    void Assembler::asm_ret(LIns *ins) {
        genEpilogue();

        
        MR(RSP,FP);

        assignSavedRegs();
        LIns *value = ins->oprnd1();
        Register r = ins->isop(LIR_ret) ? RAX : XMM0;
        findSpecificRegFor(value, r);
    }

    void Assembler::asm_nongp_copy(Register d, Register s) {
        if (!IsFpReg(d) && IsFpReg(s)) {
            
            MOVQRX(d, s);
        } else if (IsFpReg(d) && IsFpReg(s)) {
            
            MOVAPSR(d, s);
        } else {
            
            MOVQXR(d, s);
        }
    }

    void Assembler::regalloc_load(LIns *ins, Register &rr, int32_t &dr, Register &rb) {
        dr = ins->disp();
        LIns *base = ins->oprnd1();
        rb = getBaseReg(ins->opcode(), base, dr, BaseRegs);
        if (ins->isUnusedOrHasUnknownReg()) {
            
            rr = prepResultReg(ins, GpRegs & ~rmask(rb));
        } else {
            
            rr = ins->getReg();
            freeRsrcOf(ins, false);
        }
    }

    void Assembler::asm_load64(LIns *ins) {
        Register rr, rb;
        int32_t dr;
        regalloc_load(ins, rr, dr, rb);
        if (IsGpReg(rr)) {
            
            MOVQRM(rr, dr, rb);
        } else {
            
            MOVSDRM(rr, dr, rb);
        }
    }

    void Assembler::asm_ld(LIns *ins) {
        NanoAssert(!ins->isQuad());
        Register r, b;
        int32_t d;
        regalloc_load(ins, r, d, b);
        LOpcode op = ins->opcode();
        switch (op) {
        case LIR_ldcb: MOVZX8M( r, d, b);   break;
        case LIR_ldcs: MOVZX16M(r, d, b);   break;
        default:       MOVLRM(  r, d, b);   break;
        }
    }

    void Assembler::asm_store64(LIns *value, int d, LIns *base) {
        NanoAssert(value->isQuad());
        Register b = getBaseReg(LIR_stqi, base, d, BaseRegs);

        
        Register r;
        if (value->isUnusedOrHasUnknownReg()) {
            RegisterMask allow;
            
            if (value->isFloat() || value->isop(LIR_float) || value->isop(LIR_fmod)) {
                allow = FpRegs;
            } else {
                allow = GpRegs;
            }
            r = findRegFor(value, allow & ~rmask(b));
        } else {
            r = value->getReg();
        }

        if (IsGpReg(r)) {
            
            MOVQMR(r, d, b);
        }
        else {
            
            MOVSDMR(r, d, b);
        }
    }

    void Assembler::asm_store32(LIns *value, int d, LIns *base) {
        NanoAssert(!value->isQuad());
        Register b = getBaseReg(LIR_sti, base, d, BaseRegs);
        Register r = findRegFor(value, GpRegs & ~rmask(b));

        
        MOVLMR(r, d, b);
    }

    
    void Assembler::asm_quad(Register r, uint64_t v) {
        NanoAssert(IsGpReg(r));
        if (isU32(v)) {
            MOVI(r, int32_t(v));
        } else if (isS32(v)) {
            
            MOVQI32(r, int32_t(v));
        } else if (isTargetWithinS32((NIns*)v)) {
            
            int32_t d = int32_t(int64_t(v)-int64_t(_nIns));
            LEARIP(r, d);
        } else {
            MOVQI(r, v);
        }
    }

    void Assembler::asm_int(LIns *ins) {
        Register r = prepResultReg(ins, GpRegs);
        int32_t v = ins->imm32();
        if (v == 0) {
            
            XORRR(r, r);
        } else {
            MOVI(r, v);
        }
    }

    void Assembler::asm_quad(LIns *ins) {
        uint64_t v = ins->imm64();
        RegisterMask allow = v == 0 ? GpRegs|FpRegs : GpRegs;
        Register r = prepResultReg(ins, allow);
        if (v == 0) {
            if (IsGpReg(r)) {
                
                XORRR(r, r);
            } else {
                
                XORPS(r);
            }
        } else {
            asm_quad(r, v);
        }
    }

    void Assembler::asm_qjoin(LIns*) {
        TODO(asm_qjoin);
    }

    Register Assembler::asm_prep_fcall(Reservation*, LIns *ins) {
        return prepResultReg(ins, rmask(XMM0));
    }

    void Assembler::asm_param(LIns *ins) {
        uint32_t a = ins->paramArg();
        uint32_t kind = ins->paramKind();
        if (kind == 0) {
            
            
            if (a < (uint32_t)NumArgRegs) {
                
                prepResultReg(ins, rmask(argRegs[a]));
            } else {
                
                
                TODO(asm_param_stk);
            }
        }
        else {
            
            prepResultReg(ins, rmask(savedRegs[a]));
        }
    }

    
    void Assembler::regalloc_unary(LIns *ins, RegisterMask allow, Register &rr, Register &ra) {
        LIns *a = ins->oprnd1();
        rr = prepResultReg(ins, allow);
        
        if (a->isUnusedOrHasUnknownReg()) {
            ra = findSpecificRegForUnallocated(a, rr);
        } else {
            
            
            ra = a->getReg();
        }
        NanoAssert(allow & rmask(rr));
    }

    static const AVMPLUS_ALIGN16(int64_t) negateMask[] = {0x8000000000000000LL,0};

    void Assembler::asm_fneg(LIns *ins) {
        Register rr, ra;
        if (isS32((uintptr_t)negateMask) || isTargetWithinS32((NIns*)negateMask)) {
            regalloc_unary(ins, FpRegs, rr, ra);
            if (isS32((uintptr_t)negateMask)) {
                
                XORPSA(rr, (int32_t)(uintptr_t)negateMask);
            } else {
                
                XORPSM(rr, (NIns*)negateMask);
            }
            if (ra != rr)
                asm_nongp_copy(rr,ra);
        } else {
            
            
            
            rr = prepResultReg(ins, GpRegs);
            ra = findRegFor(ins->oprnd1(), GpRegs & ~rmask(rr));
            XORQRR(rr, ra);                     
            asm_quad(rr, negateMask[0]);        
        }
    }

    void Assembler::asm_qhi(LIns*) {
        TODO(asm_qhi);
    }

    void Assembler::asm_qlo(LIns *ins) {
        Register rr, ra;
        regalloc_unary(ins, GpRegs, rr, ra);
        NanoAssert(IsGpReg(ra));
        MOVLR(rr, ra);  
    }

    void Assembler::asm_spill(Register rr, int d, bool , bool quad) {
        if (d) {
            if (!IsFpReg(rr)) {
                if (quad)
                    MOVQMR(rr, d, FP);
                else
                    MOVLMR(rr, d, FP);
            } else {
                
                NanoAssert(quad);
                MOVSDMR(rr, d, FP);
            }
        }
    }

    NIns* Assembler::genPrologue() {
        
        uint32_t stackNeeded = max_stk_used + _activation.tos * 4;

        uint32_t stackPushed =
            sizeof(void*) + 
            sizeof(void*); 
        uint32_t aligned = alignUp(stackNeeded + stackPushed, NJ_ALIGN_STACK);
        uint32_t amt = aligned - stackPushed;

        
        
        if (amt) {
            if (isS8(amt))
                SUBQR8(RSP, amt);
            else
                SUBQRI(RSP, amt);
        }

        verbose_only( outputAddr=true; asm_output("[patch entry]"); )
        NIns *patchEntry = _nIns;
        MR(FP, RSP);    
        PUSHR(FP);      

        return patchEntry;
    }

    NIns* Assembler::genEpilogue() {
        
        
        RET();
        POPR(RBP);
        return _nIns;
    }

    void Assembler::nRegisterResetAll(RegAlloc &a) {
        
        a.clear();
#ifdef _WIN64
        a.free = 0x001fffcf; 
#else
        a.free = 0xffffffff & ~(1<<RSP | 1<<RBP);
#endif
        debug_only( a.managed = a.free; )
    }

    void Assembler::nPatchBranch(NIns *patch, NIns *target) {
        NIns *next = 0;
        if (patch[0] == 0xE9) {
            
            next = patch+5;
        } else if (patch[0] == 0x0F && (patch[1] & 0xF0) == 0x80) {
            
            next = patch+6;
        } else {
            next = 0;
            TODO(unknown_patch);
        }
        
        
        NanoAssert(isS32(target - next));
        ((int32_t*)next)[-1] = int32_t(target - next);
        if (next[0] == 0x0F && next[1] == 0x8A) {
            
            
            next += 6;
            NanoAssert(((int32_t*)next)[-1] == 0);
            NanoAssert(isS32(target - next));
            ((int32_t*)next)[-1] = int32_t(target - next);
        }
    }

    Register Assembler::nRegisterAllocFromSet(RegisterMask set) {
    #if defined _MSC_VER
        DWORD tr;
        _BitScanForward(&tr, set);
        _allocator.free &= ~rmask((Register)tr);
        return (Register) tr;
    #else
        
        Register r;
        asm("bsf    %1, %%eax\n\t"
            "btr    %%eax, %2\n\t"
            "movl   %%eax, %0\n\t"
            : "=m"(r) : "m"(set), "m"(_allocator.free) : "%eax", "memory");
        (void)set;
        return r;
    #endif
    }

    void Assembler::nFragExit(LIns *guard) {
        SideExit *exit = guard->record()->exit;
        Fragment *frag = exit->target;
        GuardRecord *lr = 0;
        bool destKnown = (frag && frag->fragEntry);
        
        
        if (guard->isop(LIR_xtbl)) {
            NanoAssert(!guard->isop(LIR_xtbl));
        } else {
            
            if (destKnown) {
                JMP(frag->fragEntry);
                lr = 0;
            } else {  
                if (!_epilogue)
                    _epilogue = genEpilogue();
                lr = guard->record();
                JMPl(_epilogue);
                lr->jmp = _nIns;
            }
        }

        MR(RSP, RBP);

        
        asm_quad(RAX, uintptr_t(lr));
    }

    void Assembler::nInit(AvmCore*) {
    }

    void Assembler::nBeginAssembly() {
        max_stk_used = 0;
    }

    
    void Assembler::underrunProtect(ptrdiff_t bytes) {
        NanoAssertMsg(bytes<=LARGEST_UNDERRUN_PROT, "constant LARGEST_UNDERRUN_PROT is too small");
        NIns *pc = _nIns;
        NIns *top = codeStart;  

    #if PEDANTIC
        
        
        
        

        NanoAssert(pedanticTop >= top);
        if (pc - bytes < pedanticTop) {
            
            const int br_size = 8; 
            if (pc - bytes - br_size < top) {
                
                verbose_only(if (_logc->lcbits & LC_Assembly) outputf("newpage %p:", pc);)
                
                codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
            }
            
            
            pedanticTop = _nIns - br_size;
            JMP(pc);
            pedanticTop = _nIns - bytes;
        }
    #else
        if (pc - bytes < top) {
            verbose_only(if (_logc->lcbits & LC_Assembly) outputf("newpage %p:", pc);)
            
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
            
            
            JMP(pc);
        }
    #endif
    }

    RegisterMask Assembler::hint(LIns *, RegisterMask allow) {
        return allow;
    }

    void Assembler::nativePageSetup() {
        NanoAssert(!_inExit);
        if (!_nIns) {
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
            IF_PEDANTIC( pedanticTop = _nIns; )
        }
        if (!_nExitIns) {
            codeAlloc(exitStart, exitEnd, _nExitIns verbose_only(, exitBytes));
        }
    }

    void Assembler::nativePageReset()
    {}

    
    
    verbose_only(
    void Assembler::asm_inc_m32(uint32_t* )
    {
        
    }
    )

    void Assembler::asm_jtbl(LIns* ins, NIns** table)
    {
        
        
        Register indexreg = findRegFor(ins->oprnd1(), GpRegs & ~rmask(R12));
        if (isS32((intptr_t)table)) {
            
            
            emitrxb_imm(X64_jmpx, (Register)0, indexreg, (Register)5, (int32_t)(uintptr_t)table);
        } else {
            
            Register tablereg = registerAllocTmp(GpRegs & ~(rmask(indexreg)|rmask(R13)));
            
            emitxb(X64_jmpxb, indexreg, tablereg);
            
            asm_quad(tablereg, (uint64_t)table);
        }
    }

    void Assembler::swapCodeChunks() {
        SWAP(NIns*, _nIns, _nExitIns);
        SWAP(NIns*, codeStart, exitStart);
        SWAP(NIns*, codeEnd, exitEnd);
        verbose_only( SWAP(size_t, codeBytes, exitBytes); )
    }

} 

#endif 
