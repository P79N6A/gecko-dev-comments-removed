






































#include "nanojit.h"

#if defined FEATURE_NANOJIT && defined NANOJIT_MIPS

namespace nanojit
{
#ifdef NJ_VERBOSE
    const char *regNames[] = {
        "$zr", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
        "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
        "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
        "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",

        "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
        "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
        "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
        "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31"
    };

    const char *cname[16] = {
        "f",    "un",   "eq",   "ueq",
        "olt",  "ult",  "ole",  "ule",
        "sf",   "ngle", "seq",  "ngl",
        "lt",   "nge",  "le",   "ngt"
    };

    const char *fname[32] = {
        "resv", "resv", "resv", "resv",
        "resv", "resv", "resv", "resv",
        "resv", "resv", "resv", "resv",
        "resv", "resv", "resv", "resv",
        "s",    "d",    "resv", "resv",
        "w",    "l",    "ps",   "resv",
        "resv", "resv", "resv", "resv",
        "resv", "resv", "resv", "resv",
    };

    const char *oname[64] = {
        "special", "regimm", "j",    "jal",   "beq",      "bne",  "blez",  "bgtz",
        "addi",    "addiu",  "slti", "sltiu", "andi",     "ori",  "xori",  "lui",
        "cop0",    "cop1",   "cop2", "cop1x", "beql",     "bnel", "blezl", "bgtzl",
        "resv",    "resv",   "resv", "resv",  "special2", "jalx", "resv",  "special3",
        "lb",      "lh",     "lwl",  "lw",    "lbu",      "lhu",  "lwr",   "resv",
        "sb",      "sh",     "swl",  "sw",    "resv",     "resv", "swr",   "cache",
        "ll",      "lwc1",   "lwc2", "pref",  "resv",     "ldc1", "ldc2",  "resv",
        "sc",      "swc1",   "swc2", "resv",  "resv",     "sdc1", "sdc2",  "resv",
    };
#endif

    const Register Assembler::argRegs[] = { A0, A1, A2, A3 };
    const Register Assembler::retRegs[] = { V0, V1 };
    const Register Assembler::savedRegs[] = {
        S0, S1, S2, S3, S4, S5, S6, S7,
#ifdef FPCALLEESAVED
        FS0, FS1, FS2, FS3, FS4, FS5
#endif
    };

#define USE(x) (void)x
#define BADOPCODE(op) NanoAssertMsgf(false, "unexpected opcode %s", lirNames[op])

    
    static inline bool isLittleEndian(void)
    {
        const union {
            uint32_t      ival;
            unsigned char cval[4];
        } u = { 1 };
        return u.cval[0] == 1;
    }

    
    
    static inline int mswoff(void) {
        return isLittleEndian() ? 4 : 0;
    }

    static inline int lswoff(void) {
        return isLittleEndian() ? 0 : 4;
    }

    static inline Register mswregpair(Register r) {
        return Register(r + (isLittleEndian() ? 1 : 0));
    }

    static inline Register lswregpair(Register r) {
        return Register(r + (isLittleEndian() ? 0 : 1));
    }
















#ifdef DEBUG
    
    #define _CONST
#else
    #define _CONST const
#endif

#if NJ_SOFTFLOAT_SUPPORTED
    _CONST bool cpu_has_fpu = false;
#else
    _CONST bool cpu_has_fpu = true;
#endif

#if (__mips==4 || __mips==32 || __mips==64)
    _CONST bool cpu_has_cmov = true;
#else
    _CONST bool cpu_has_cmov = false;
#endif

#if __mips != 1
    _CONST bool cpu_has_lsdc1 = true;
#else
    _CONST bool cpu_has_lsdc1 = false;
#endif

#if (__mips==32 || __mips==64) && __mips_isa_rev>=2
    _CONST bool cpu_has_lsdxc1 = true;
#else
    _CONST bool cpu_has_lsdxc1 = false;
#endif

#if (__mips==1 || __mips==2 || __mips==3)
    _CONST bool cpu_has_fpuhazard = true;
#else
    _CONST bool cpu_has_fpuhazard = false;
#endif
#undef _CONST

    

    debug_only (
                
                static NIns *breakAddr;
                static void codegenBreak(NIns *genAddr)
                {
                    NanoAssert (breakAddr != genAddr);
                }
    )

    
    uint16_t hi(uint32_t v)
    {
        uint16_t r = v >> 16;
        if ((int16_t)(v) < 0)
            r += 1;
        return r;
    }

    int16_t lo(uint32_t v)
    {
        int16_t r = v;
        return r;
    }

    void Assembler::asm_li32(Register r, int32_t imm)
    {
        
        ADDIU(r, r, lo(imm));
        LUI(r, hi(imm));
    }

    void Assembler::asm_li(Register r, int32_t imm)
    {
#if !PEDANTIC
        if (isU16(imm)) {
            ORI(r, ZERO, imm);
            return;
        }
        if (isS16(imm)) {
            ADDIU(r, ZERO, imm);
            return;
        }
        if ((imm & 0xffff) == 0) {
            LUI(r, uint32_t(imm) >> 16);
            return;
        }
#endif
        asm_li32(r, imm);
    }

    
    void Assembler::asm_li_d(Register r, int32_t msw, int32_t lsw)
    {
        if (IsFpReg(r)) {
            NanoAssert(cpu_has_fpu);
            
            
            
            
            if (msw == 0)
                MTC1(ZERO, r+1);
            else {
                MTC1(AT, r+1);
                
                if (msw != lsw)
                    asm_li(AT, msw);
            }
            if (lsw == 0)
                MTC1(ZERO, r);
            else {
                MTC1(AT, r);
                asm_li(AT, lsw);
            }
        }
        else {
            



            if (msw == lsw)
                MOVE(mswregpair(r), lswregpair(r));
            else
                asm_li(mswregpair(r), msw);
            asm_li(lswregpair(r), lsw);
        }
    }

    void Assembler::asm_move(Register d, Register s)
    {
        MOVE(d, s);
    }

    
    void Assembler::asm_ldst(int op, Register rt, int dr, Register rbase)
    {
#if !PEDANTIC
        if (isS16(dr)) {
            LDST(op, rt, dr, rbase);
            return;
        }
#endif

        
        
        
        LDST(op, rt, lo(dr), AT);
        ADDU(AT, AT, rbase);
        LUI(AT, hi(dr));
    }

    void Assembler::asm_ldst64(bool store, Register r, int dr, Register rbase)
    {
#if !PEDANTIC
        if (isS16(dr) && isS16(dr+4)) {
            if (IsGpReg(r)) {
                LDST(store ? OP_SW : OP_LW, r+1, dr+4, rbase);
                LDST(store ? OP_SW : OP_LW, r,   dr, rbase);
            }
            else {
                NanoAssert(cpu_has_fpu);
                
                if (cpu_has_lsdc1 && ((dr & 7) == 0)) {
                    
                    LDST(store ? OP_SDC1 : OP_LDC1, r, dr, rbase);
                }
                else {
                    
                    
                    LDST(store ? OP_SWC1 : OP_LWC1, r+1, dr+mswoff(), rbase);
                    LDST(store ? OP_SWC1 : OP_LWC1, r,   dr+lswoff(), rbase);
                }
                return;
            }
        }
#endif

        if (IsGpReg(r)) {
            
            
            
            
            LDST(store ? OP_SW : OP_LW, r+1, lo(dr+4), AT);
            LDST(store ? OP_SW : OP_LW, r,   lo(dr), AT);
            ADDU(AT, AT, rbase);
            LUI(AT, hi(dr));
        }
        else {
            NanoAssert(cpu_has_fpu);
            if (cpu_has_lsdxc1) {
                
                
                if (store)
                    SDXC1(r, AT, rbase);
                else
                    LDXC1(r, AT, rbase);
                asm_li(AT, dr);
            }
            else if (cpu_has_lsdc1) {
                
                
                
                LDST(store ? OP_SDC1 : OP_LDC1, r, lo(dr), AT);
                ADDU(AT, AT, rbase);
                LUI(AT, hi(dr));
            }
            else {
                
                
                
                
                LDST(store ? OP_SWC1 : OP_LWC1, r+1, lo(dr+mswoff()), AT);
                LDST(store ? OP_SWC1 : OP_LWC1, r,   lo(dr+lswoff()), AT);
                ADDU(AT, AT, rbase);
                LUI(AT, hi(dr));
            }
        }
    }

    void Assembler::asm_store_imm64(LIns *value, int dr, Register rbase)
    {
        NanoAssert(value->isImmD());
        int32_t msw = value->immDhi();
        int32_t lsw = value->immDlo();

        
        
        
        

        NanoAssert(isS16(dr) && isS16(dr+4));

        if (lsw == 0)
            SW(ZERO, dr+lswoff(), rbase);
        else {
            SW(AT, dr+lswoff(), rbase);
            if (msw != lsw)
                asm_li(AT, lsw);
        }
        if (msw == 0)
            SW(ZERO, dr+mswoff(), rbase);
        else {
            SW(AT, dr+mswoff(), rbase);
            
            if (msw != lsw)
                asm_li(AT, msw);
        }
    }

    void Assembler::asm_regarg(ArgType ty, LIns* p, Register r)
    {
        NanoAssert(deprecated_isKnownReg(r));
        if (ty == ARGTYPE_I || ty == ARGTYPE_UI) {
            
            if (p->isImmI())
                asm_li(r, p->immI());
            else {
                if (p->isExtant()) {
                    if (!p->deprecated_hasKnownReg()) {
                        
                        int d = findMemFor(p);
                        if (p->isop(LIR_allocp))
                            ADDIU(r, FP, d);
                        else
                            asm_ldst(OP_LW, r, d, FP);
                    }
                    else
                        
                        MOVE(r, p->deprecated_getReg());
                }
                else {
                    
                    
                    findSpecificRegFor(p, r);
                }
            }
        }
        else {
            
            NanoAssert(false);
        }
    }

    void Assembler::asm_stkarg(LIns* arg, int stkd)
    {
        bool isF64 = arg->isD();
        Register rr;
        if (arg->isExtant() && (rr = arg->deprecated_getReg(), deprecated_isKnownReg(rr))) {
            
            
            if (!cpu_has_fpu || !isF64) {
                NanoAssert(IsGpReg(rr));
                SW(rr, stkd, SP);
            }
            else {
                NanoAssert(cpu_has_fpu);
                NanoAssert(IsFpReg(rr));
                NanoAssert((stkd & 7) == 0);
                asm_ldst64(true, rr, stkd, SP);
            }
        }
        else {
            
            
            int d = findMemFor(arg);
            if (!isF64) {
                SW(AT, stkd, SP);
                if (arg->isop(LIR_allocp))
                    ADDIU(AT, FP, d);
                else
                    LW(AT, d, FP);
            }
            else {
                NanoAssert((stkd & 7) == 0);
                SW(AT, stkd+4, SP);
                LW(AT, d+4,    FP);
                SW(AT, stkd,   SP);
                LW(AT, d,      FP);
            }
        }
    }

    
    
    
    void
    Assembler::asm_arg_64(LIns* arg, Register& r, Register& fr, int& stkd)
    {
        
        NanoAssert((stkd & 3) == 0);
#if NJ_SOFTFLOAT_SUPPORTED
        NanoAssert(arg->isop(LIR_ii2d));
#else
        NanoAssert(cpu_has_fpu);
#endif

        
        
        
        if (stkd & 4) {
            if (stkd < 16) {
                r = Register(r + 1);
                fr = Register(fr + 1);
            }
            stkd += 4;
        }

        if (stkd < 16) {
            NanoAssert(fr == FA0 || fr == FA1 || fr == A2);
            if (fr == FA0 || fr == FA1)
                findSpecificRegFor(arg, fr);
            else {
                findSpecificRegFor(arg, FA1);
                
                Register fpupair = arg->getReg();
                Register intpair = fr;
                MFC1(mswregpair(intpair), Register(fpupair + 1));  
                MFC1(lswregpair(intpair), fpupair);                
            }
            r = Register(r + 2);
            fr = Register(fr + 2);
        }
        else
            asm_stkarg(arg, stkd);

        stkd += 8;
    }

    

#define FRAMESIZE        8
#define RA_OFFSET        4
#define FP_OFFSET        0

    void Assembler::asm_store32(LOpcode op, LIns *value, int dr, LIns *base)
    {
        Register rt, rbase;
        getBaseReg2(GpRegs, value, rt, GpRegs, base, rbase, dr);

        switch (op) {
        case LIR_sti:
            asm_ldst(OP_SW, rt, dr, rbase);
            break;
        case LIR_sti2s:
            asm_ldst(OP_SH, rt, dr, rbase);
            break;
        case LIR_sti2c:
            asm_ldst(OP_SB, rt, dr, rbase);
            break;
        default:
            BADOPCODE(op);
        }

        TAG("asm_store32(value=%p{%s}, dr=%d, base=%p{%s})",
            value, lirNames[value->opcode()], dr, base, lirNames[base->opcode()]);
    }

    void Assembler::asm_ui2d(LIns *ins)
    {
        Register fr = deprecated_prepResultReg(ins, FpRegs);
        Register v = findRegFor(ins->oprnd1(), GpRegs);
        Register ft = registerAllocTmp(FpRegs & ~(rmask(fr)));    

        
        NanoAssert(deprecated_isKnownReg(v));

        
        
        
        
        
        
        
        

        underrunProtect(6*4);   
        NIns *here = _nIns;
        ADD_D(fr,fr,ft);
        MTC1(AT,ft+1);
        MTC1(ZERO,ft);
        LUI(AT,0x41f0);
        CVT_D_W(fr,ft);            
        BGEZ(v,here);
        MTC1(v,ft);

        TAG("asm_ui2d(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_d2i(LIns* ins)
    {
        NanoAssert(cpu_has_fpu);

        Register rr = deprecated_prepResultReg(ins, GpRegs);
        Register sr = findRegFor(ins->oprnd1(), FpRegs);
        
        
        MFC1(rr,sr);
        TRUNC_W_D(sr,sr);
        TAG("asm_d2i(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_fop(LIns *ins)
    {
        NanoAssert(cpu_has_fpu);
        if (cpu_has_fpu) {
            LIns* lhs = ins->oprnd1();
            LIns* rhs = ins->oprnd2();
            LOpcode op = ins->opcode();

            

            Register rr = deprecated_prepResultReg(ins, FpRegs);
            Register ra = findRegFor(lhs, FpRegs);
            Register rb = (rhs == lhs) ? ra : findRegFor(rhs, FpRegs & ~rmask(ra));

            switch (op) {
            case LIR_addd: ADD_D(rr, ra, rb); break;
            case LIR_subd: SUB_D(rr, ra, rb); break;
            case LIR_muld: MUL_D(rr, ra, rb); break;
            case LIR_divd: DIV_D(rr, ra, rb); break;
            default:
                BADOPCODE(op);
            }
        }
        TAG("asm_fop(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_fneg(LIns *ins)
    {
        NanoAssert(cpu_has_fpu);
        if (cpu_has_fpu) {
            LIns* lhs = ins->oprnd1();
            Register rr = deprecated_prepResultReg(ins, FpRegs);
            Register sr = ( !lhs->isInReg()
                            ? findRegFor(lhs, FpRegs)
                            : lhs->deprecated_getReg() );
            NEG_D(rr, sr);
        }
        TAG("asm_fneg(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_immd(LIns *ins)
    {
        int d = deprecated_disp(ins);
        Register rr = ins->deprecated_getReg();

        deprecated_freeRsrcOf(ins);

        if (cpu_has_fpu && deprecated_isKnownReg(rr)) {
            if (d)
                asm_spill(rr, d, true);
            asm_li_d(rr, ins->immDhi(), ins->immDlo());
        }
        else {
            NanoAssert(d);
            asm_store_imm64(ins, d, FP);
        }
        TAG("asm_immd(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

#ifdef NANOJIT_64BIT
    void
    Assembler::asm_q2i(LIns *)
    {
        NanoAssert(0);  
    }

    void Assembler::asm_ui2uq(LIns *ins)
    {
        USE(ins);
        TODO(asm_ui2uq);
        TAG("asm_ui2uq(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }
#endif

    void Assembler::asm_load64(LIns *ins)
    {
        NanoAssert(ins->isD());

        LIns* base = ins->oprnd1();
        int dr = ins->disp();

        Register rd = ins->deprecated_getReg();
        int ds = deprecated_disp(ins);

        Register rbase = findRegFor(base, GpRegs);
        NanoAssert(IsGpReg(rbase));
        deprecated_freeRsrcOf(ins);

        if (cpu_has_fpu && deprecated_isKnownReg(rd)) {
            NanoAssert(IsFpReg(rd));
            asm_ldst64 (false, rd, dr, rbase);
        }
        else {
            
            
            
            NanoAssert(!deprecated_isKnownReg(rd));
            NanoAssert(ds != 0);

            NanoAssert(isS16(dr) && isS16(dr+4));
            NanoAssert(isS16(ds) && isS16(ds+4));

            
            NanoAssert((ds & 0x7) == 0);

            
            
            
            
            
            

            SW(AT, ds+4, FP);
            LW(AT, dr+4, rbase);
            SW(AT, ds,   FP);
            LW(AT, dr,   rbase);
        }

        TAG("asm_load64(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_cond(LIns *ins)
    {
        Register r = deprecated_prepResultReg(ins, GpRegs);
        LOpcode op = ins->opcode();
        LIns *a = ins->oprnd1();
        LIns *b = ins->oprnd2();

        asm_cmp(op, a, b, r);

        TAG("asm_cond(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

#if NJ_SOFTFLOAT_SUPPORTED
    void Assembler::asm_qhi(LIns *ins)
    {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        LIns *q = ins->oprnd1();
        int d = findMemFor(q);
        LW(rr, d+mswoff(), FP);
        TAG("asm_qhi(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_qlo(LIns *ins)
    {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        LIns *q = ins->oprnd1();
        int d = findMemFor(q);
        LW(rr, d+lswoff(), FP);
        TAG("asm_qlo(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_qjoin(LIns *ins)
    {
        int d = findMemFor(ins);
        NanoAssert(d && isS16(d));
        LIns* lo = ins->oprnd1();
        LIns* hi = ins->oprnd2();

        Register r = findRegFor(hi, GpRegs);
        SW(r, d+mswoff(), FP);
        r = findRegFor(lo, GpRegs);             
        SW(r, d+lswoff(), FP);
        deprecated_freeRsrcOf(ins);             

        TAG("asm_qjoin(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

#endif

    void Assembler::asm_neg_not(LIns *ins)
    {
        LOpcode op = ins->opcode();
        Register rr = deprecated_prepResultReg(ins, GpRegs);

        LIns* lhs = ins->oprnd1();
        
        
        Register ra = !lhs->isInReg() ? findSpecificRegFor(lhs, rr) : lhs->deprecated_getReg();
        if (op == LIR_noti)
            NOT(rr, ra);
        else
            NEGU(rr, ra);
        TAG("asm_neg_not(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_immi(LIns *ins)
    {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        asm_li(rr, ins->immI());
        TAG("asm_immi(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_cmov(LIns *ins)
    {
        LIns* condval = ins->oprnd1();
        LIns* iftrue  = ins->oprnd2();
        LIns* iffalse = ins->oprnd3();

        NanoAssert(condval->isCmp());
        NanoAssert(ins->opcode() == LIR_cmovi && iftrue->isI() && iffalse->isI());

        const Register rr = deprecated_prepResultReg(ins, GpRegs);

        const Register iftruereg = findRegFor(iftrue, GpRegs & ~rmask(rr));
        MOVN(rr, iftruereg, AT);
         findSpecificRegFor(iffalse, rr);
        asm_cmp(condval->opcode(), condval->oprnd1(), condval->oprnd2(), AT);
        TAG("asm_cmov(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_condd(LIns *ins)
    {
        NanoAssert(cpu_has_fpu);
        if (cpu_has_fpu) {
            Register r = deprecated_prepResultReg(ins, GpRegs);
            LOpcode op = ins->opcode();
            LIns *a = ins->oprnd1();
            LIns *b = ins->oprnd2();

            if (cpu_has_cmov) {
                
                
                
                MOVF(r, ZERO, 0);
                ORI(r, ZERO, 1);
            }
            else {
                
                
                
                
                
                
                NIns *here = _nIns;
                verbose_only(verbose_outputf("%p:", here);)
                underrunProtect(3*4);
                MOVE(r, ZERO);
                ORI(r, ZERO, 1);        
                BC1T(here);
                if (cpu_has_fpuhazard)
                    NOP();
            }
            asm_cmp(op, a, b, r);
        }
        TAG("asm_condd(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_i2d(LIns *ins)
    {
        NanoAssert(cpu_has_fpu);
        if (cpu_has_fpu) {
            Register fr = deprecated_prepResultReg(ins, FpRegs);
            Register v = findRegFor(ins->oprnd1(), GpRegs);

            
            
            CVT_D_W(fr,fr);
            MTC1(v,fr);
        }
        TAG("asm_i2d(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_ret(LIns *ins)
    {
        genEpilogue();

        releaseRegisters();
        assignSavedRegs();

        LIns *value = ins->oprnd1();
        if (ins->isop(LIR_reti)) {
            findSpecificRegFor(value, V0);
        }
        else {
            NanoAssert(ins->isop(LIR_retd));
#if NJ_SOFTFLOAT_SUPPORTED
            NanoAssert(value->isop(LIR_ii2d));
            findSpecificRegFor(value->oprnd1(), V0); 
            findSpecificRegFor(value->oprnd2(), V1); 
#else
            findSpecificRegFor(value, FV0);
#endif
        }
        TAG("asm_ret(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_load32(LIns *ins)
    {
        LOpcode op = ins->opcode();
        LIns* base = ins->oprnd1();
        int d = ins->disp();

        Register rres = deprecated_prepResultReg(ins, GpRegs);
        Register rbase = getBaseReg(base, d, GpRegs);

        switch (op) {
        case LIR_lduc2ui:          
            asm_ldst(OP_LBU, rres, d, rbase);
            break;
        case LIR_ldus2ui:          
            asm_ldst(OP_LHU, rres, d, rbase);
            break;
        case LIR_ldc2i:          
            asm_ldst(OP_LB, rres, d, rbase);
            break;
        case LIR_lds2i:          
            asm_ldst(OP_LH, rres, d, rbase);
            break;
        case LIR_ldi:            
            asm_ldst(OP_LW, rres, d, rbase);
            break;
        default:
            BADOPCODE(op);
        }

        TAG("asm_load32(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_param(LIns *ins)
    {
        uint32_t a = ins->paramArg();
        uint32_t kind = ins->paramKind();

        if (kind == 0) {
            
            
            if (a < 4) {
                
                deprecated_prepResultReg(ins, rmask(argRegs[a]));
            } else {
                
                Register r = deprecated_prepResultReg(ins, GpRegs);
                TODO(Check stack offset);
                int d = FRAMESIZE + a * sizeof(intptr_t);
                LW(r, d, FP);
            }
        }
        else {
            
            deprecated_prepResultReg(ins, rmask(savedRegs[a]));
        }
        TAG("asm_param(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_arith(LIns *ins)
    {
        LOpcode op = ins->opcode();
        LIns* lhs = ins->oprnd1();
        LIns* rhs = ins->oprnd2();

        RegisterMask allow = GpRegs;

        
        Register rr = deprecated_prepResultReg(ins, allow);

        
        
        Register ra = !lhs->isInReg() ? findSpecificRegFor(lhs, rr) : lhs->deprecated_getReg();
        Register rb, t;

        
        NanoAssert(deprecated_isKnownReg(rr));
        NanoAssert(deprecated_isKnownReg(ra));
        allow &= ~rmask(rr);
        allow &= ~rmask(ra);

        if (rhs->isImmI()) {
            int32_t rhsc = rhs->immI();
            if (isS16(rhsc)) {
                
                switch (op) {
                case LIR_addxovi:
                case LIR_addjovi:
                    
                    

                    
                    
                    
                    
                    
                    
                    
                    

                    t = registerAllocTmp(allow);
                    SRL(AT, AT, 31);
                    if (rhsc < 0) {
                        AND(AT, AT, t);
                        NOT(t, rr);
                    }
                    else
                        AND(AT, AT, rr);
                    if (rr == ra)
                        XOR(AT, rr, t);
                    else
                        XOR(AT, rr, ra);
                    ADDIU(rr, ra, rhsc);
                    if (rr == ra)
                        MOVE(t, ra);
                    goto done;
                case LIR_addi:
                    ADDIU(rr, ra, rhsc);
                    goto done;
                case LIR_subxovi:
                case LIR_subjovi:
                    
                    

                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    if (isS16(-rhsc)) {
                        t = registerAllocTmp(allow);
                        SRL(AT,AT,31);
                        if (rhsc < 0) {
                            AND(AT, AT, t);
                            if (rr == ra)
                                NOT(t, t);
                            else
                                NOT(t, ra);
                        }
                        else {
                            if (rr == ra)
                                AND(AT, AT, t);
                            else
                                AND(AT, AT, ra);
                        }
                        if (rr == ra)
                            XOR(AT, rr, t);
                        else
                            XOR(AT, rr, ra);
                        ADDIU(rr, ra, -rhsc);
                        if (rr == ra)
                            MOVE(t, ra);
                        goto done;
                    }
                    break;
                case LIR_subi:
                    if (isS16(-rhsc)) {
                        ADDIU(rr, ra, -rhsc);
                        goto done;
                    }
                    break;
                case LIR_mulxovi:
                case LIR_muljovi:
                case LIR_muli:
                    
                    
                    
                    
                    break;
                default:
                    break;
                }
            }
            if (isU16(rhsc)) {
                
                switch (op) {
                case LIR_ori:
                    ORI(rr, ra, rhsc);
                    goto done;
                case LIR_andi:
                    ANDI(rr, ra, rhsc);
                    goto done;
                case LIR_xori:
                    XORI(rr, ra, rhsc);
                    goto done;
                default:
                    break;
                }
            }

            
            switch (op) {
            case LIR_lshi:
                SLL(rr, ra, rhsc&31);
                goto done;
            case LIR_rshui:
                SRL(rr, ra, rhsc&31);
                goto done;
            case LIR_rshi:
                SRA(rr, ra, rhsc&31);
                goto done;
            default:
                break;
            }
        }

        
        rb = (rhs == lhs) ? ra : findRegFor(rhs, allow);
        NanoAssert(deprecated_isKnownReg(rb));
        allow &= ~rmask(rb);

        
        
        
        
        
        NanoAssert(ra == rb || rr != rb);

        switch (op) {
            case LIR_addxovi:
            case LIR_addjovi:
                
                

                
                
                
                
                
                
                
                
                

                t = ZERO;
                if (rr == ra || ra != rb)
                    t = registerAllocTmp(allow);
                SRL(AT, AT, 31);
                if (ra != rb) {
                    AND(AT, AT, t);
                    XOR(t, rr, rb);
                }
                if (rr == ra)
                    XOR(AT, rr, t);
                else
                    XOR(AT, rr, ra);
                ADDU(rr, ra, rb);
                if (rr == ra)
                    MOVE(t, ra);
                break;
            case LIR_addi:
                ADDU(rr, ra, rb);
                break;
            case LIR_andi:
                AND(rr, ra, rb);
                break;
            case LIR_ori:
                OR(rr, ra, rb);
                break;
            case LIR_xori:
                XOR(rr, ra, rb);
                break;
            case LIR_subxovi:
            case LIR_subjovi:
                
                

                
                
                
                
                
                
                
                
                

                if (ra == rb) {
                    
                    MOVE(AT, ZERO);
                    SUBU(rr, ra, rb);
                }
                else {
                    t = registerAllocTmp(allow);
                    SRL(AT, AT, 31);
                    AND(AT, AT, t);
                    if (rr == ra)
                        XOR(t, rr, t);
                    else
                        XOR(t, rr, ra);
                    SUBU(rr, ra, rb);
                    XOR(AT, ra, rb);
                    if (rr == ra)
                        MOVE(t, ra);
                }
                break;
            case LIR_subi:
                SUBU(rr, ra, rb);
                break;
            case LIR_lshi:
                
                SLLV(rr, ra, rb);
                break;
            case LIR_rshi:
                
                SRAV(rr, ra, rb);
                break;
            case LIR_rshui:
                
                SRLV(rr, ra, rb);
                break;
            case LIR_mulxovi:
            case LIR_muljovi:
                t = registerAllocTmp(allow);
                
                
                
                
                
                
                
                
                XOR(AT, AT, t);
                MFHI(AT);
                SRA(t, rr, 31);
                MFLO(rr);
                MULT(ra, rb);
                break;
            case LIR_muli:
                MUL(rr, ra, rb);
                break;
            default:
                BADOPCODE(op);
        }
    done:
        TAG("asm_arith(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    void Assembler::asm_store64(LOpcode op, LIns *value, int dr, LIns *base)
    {
        
#if NANOJIT_64BIT
        NanoAssert (op == LIR_stq || op == LIR_std2f || op == LIR_std);
#else
        NanoAssert (op == LIR_std2f || op == LIR_std);
#endif

        switch (op) {
            case LIR_std:
                if (cpu_has_fpu) {
                    Register rbase = findRegFor(base, GpRegs);

                    if (value->isImmD())
                        asm_store_imm64(value, dr, rbase);
                    else {
                        Register fr = findRegFor(value, FpRegs);
                        asm_ldst64(true, fr, dr, rbase);
                    }
                }
                else {
                    Register rbase = findRegFor(base, GpRegs);
                    

                    int ds = findMemFor(value);

                    
                    
                    
                    
                    SW(AT, dr+4, rbase);
                    LW(AT, ds+4, FP);
                    SW(AT, dr,   rbase);
                    LW(AT, ds,   FP);
                }

                break;
            case LIR_std2f:
                NanoAssertMsg(0, "NJ_EXPANDED_LOADSTORE_SUPPORTED not yet supported for this architecture");
                return;
            default:
                BADOPCODE(op);
                return;
        }

        TAG("asm_store64(value=%p{%s}, dr=%d, base=%p{%s})",
            value, lirNames[value->opcode()], dr, base, lirNames[base->opcode()]);
    }

    bool Assembler::canRemat(LIns* ins)
    {
        return ins->isImmI() || ins->isop(LIR_allocp);
    }

    void Assembler::asm_restore(LIns *i, Register r)
    {
        int d;
        if (i->isop(LIR_allocp)) {
            d = deprecated_disp(i);
            if (isS16(d))
                ADDIU(r, FP, d);
            else {
                ADDU(r, FP, AT);
                asm_li(AT, d);
            }
        }
        else if (i->isImmI()) {
            asm_li(r, i->immI());
        }
        else {
            d = findMemFor(i);
            if (IsFpReg(r)) {
                asm_ldst64(false, r, d, FP);
            }
            else {
                asm_ldst(OP_LW, r, d, FP);
            }
        }
        TAG("asm_restore(i=%p{%s}, r=%d)", i, lirNames[i->opcode()], r);
    }

    void Assembler::asm_cmp(LOpcode condop, LIns *a, LIns *b, Register cr)
    {
        RegisterMask allow = isCmpDOpcode(condop) ? FpRegs : GpRegs;
        Register ra = findRegFor(a, allow);
        Register rb = (b==a) ? ra : findRegFor(b, allow & ~rmask(ra));

        

        
        switch (condop) {
        case LIR_eqi:
            SLTIU(cr,cr,1);
            XOR(cr,ra,rb);
            break;
        case LIR_lti:
            SLT(cr,ra,rb);
            break;
        case LIR_gti:
            SLT(cr,rb,ra);
            break;
        case LIR_lei:
            XORI(cr,cr,1);
            SLT(cr,rb,ra);
            break;
        case LIR_gei:
            XORI(cr,cr,1);
            SLT(cr,ra,rb);
            break;
        case LIR_ltui:
            SLTU(cr,ra,rb);
            break;
        case LIR_gtui:
            SLTU(cr,rb,ra);
            break;
        case LIR_leui:
            XORI(cr,cr,1);
            SLTU(cr,rb,ra);
            break;
        case LIR_geui:
            XORI(cr,cr,1);
            SLTU(cr,ra,rb);
            break;
        case LIR_eqd:
            C_EQ_D(ra,rb);
            break;
        case LIR_ltd:
            C_LT_D(ra,rb);
            break;
        case LIR_gtd:
            C_LT_D(rb,ra);
            break;
        case LIR_led:
            C_LE_D(ra,rb);
            break;
        case LIR_ged:
            C_LE_D(rb,ra);
            break;
        default:
            debug_only(outputf("%s",lirNames[condop]);)
            TODO(asm_cond);
        }
    }

#define SEG(addr) (uint32_t(addr) & 0xf0000000)
#define SEGOFFS(addr) (uint32_t(addr) & 0x0fffffff)


    
    
    
    NIns* Assembler::asm_branchtarget(NIns * const targ)
    {
        bool inrange;
        NIns *btarg = targ;

        
        
        if (targ)
            underrunProtect(2 * 4);    

        
        
        ptrdiff_t bd = BOFFSET(targ-1);

#if PEDANTIC
        inrange = false;
#else
        inrange = (targ && isS16(bd));
#endif

        
        
        
        if (inrange)
            NOP();
        else {
            NIns *tramp = _nSlot;
            if (targ) {
                
                if (SEG(targ) == SEG(tramp)) {
                    
                    
                    
                    
                    
                    
                    

                    underrunProtect(4 * 4);             

                    NOP();                              

                    
                    trampJ(targ);
                    trampNOP();                         

                }
                else {
                    
                    
                    
                    
                    
                    
                    
                    

                    underrunProtect(5 * 4);             

                    LUI(AT,hi(uint32_t(targ)));         

                    
                    trampADDIU(AT, AT, lo(uint32_t(targ)));
                    trampJR(AT);
                    trampNOP();                         

                }
            }
            else {
                
                
                
                

                underrunProtect(5 * 4);                 

                NOP();                                  

                trampNOP();
                trampNOP();
                trampNOP();

            }
            btarg = tramp;
        }

        return btarg;
    }


    NIns* Assembler::asm_bxx(bool branchOnFalse, LOpcode condop, Register ra, Register rb, NIns * const targ)
    {
        NIns *patch = NULL;
        NIns *btarg = asm_branchtarget(targ);

        if (cpu_has_fpu && isCmpDOpcode(condop)) {
            
            
            switch (condop) {
            case LIR_eqd:
                if (branchOnFalse)
                    BC1F(btarg);
                else
                    BC1T(btarg);
                patch = _nIns;
                if (cpu_has_fpuhazard)
                    NOP();
                C_EQ_D(ra, rb);
                break;
            case LIR_ltd:
                if (branchOnFalse)
                    BC1F(btarg);
                else
                    BC1T(btarg);
                patch = _nIns;
                if (cpu_has_fpuhazard)
                    NOP();
                C_LT_D(ra, rb);
                break;
            case LIR_gtd:
                if (branchOnFalse)
                    BC1F(btarg);
                else
                    BC1T(btarg);
                patch = _nIns;
                if (cpu_has_fpuhazard)
                    NOP();
                C_LT_D(rb, ra);
                break;
            case LIR_led:
                if (branchOnFalse)
                    BC1F(btarg);
                else
                    BC1T(btarg);
                patch = _nIns;
                if (cpu_has_fpuhazard)
                    NOP();
                C_LE_D(ra, rb);
                break;
            case LIR_ged:
                if (branchOnFalse)
                    BC1F(btarg);
                else
                    BC1T(btarg);
                patch = _nIns;
                if (cpu_has_fpuhazard)
                    NOP();
                C_LE_D(rb, ra);
                break;
            default:
                BADOPCODE(condop);
                break;
            }
        }
        else {
            
            
            
            switch (condop) {
            case LIR_eqi:
                
                
                if (branchOnFalse)
                    BNE(ra, rb, btarg);
                else {
                    if (ra == rb)
                        B(btarg);
                    else
                        BEQ(ra, rb, btarg);
                }
                patch = _nIns;
                break;
            case LIR_lti:
                if (branchOnFalse)
                    BEQ(AT, ZERO, btarg);
                else
                    BNE(AT, ZERO, btarg);
                patch = _nIns;
                SLT(AT, ra, rb);
                break;
            case LIR_gti:
                if (branchOnFalse)
                    BEQ(AT, ZERO, btarg);
                else
                    BNE(AT, ZERO, btarg);
                patch = _nIns;
                SLT(AT, rb, ra);
                break;
            case LIR_lei:
                if (branchOnFalse)
                    BNE(AT, ZERO, btarg);
                else
                    BEQ(AT, ZERO, btarg);
                patch = _nIns;
                SLT(AT, rb, ra);
                break;
            case LIR_gei:
                if (branchOnFalse)
                    BNE(AT, ZERO, btarg);
                else
                    BEQ(AT, ZERO, btarg);
                patch = _nIns;
                SLT(AT, ra, rb);
                break;
            case LIR_ltui:
                if (branchOnFalse)
                    BEQ(AT, ZERO, btarg);
                else
                    BNE(AT, ZERO, btarg);
                patch = _nIns;
                SLTU(AT, ra, rb);
                break;
            case LIR_gtui:
                if (branchOnFalse)
                    BEQ(AT, ZERO, btarg);
                else
                    BNE(AT, ZERO, btarg);
                patch = _nIns;
                SLTU(AT, rb, ra);
                break;
            case LIR_leui:
                if (branchOnFalse)
                    BNE(AT, ZERO, btarg);
                else
                    BEQ(AT, ZERO, btarg);
                patch = _nIns;
                SLT(AT, rb, ra);
                break;
            case LIR_geui:
                if (branchOnFalse)
                    BNE(AT, ZERO, btarg);
                else
                    BEQ(AT, ZERO, btarg);
                patch = _nIns;
                SLTU(AT, ra, rb);
                break;
            default:
                BADOPCODE(condop);
            }
        }
        TAG("asm_bxx(branchOnFalse=%d, condop=%s, ra=%s rb=%s targ=%p)",
            branchOnFalse, lirNames[condop], gpn(ra), gpn(rb), targ);
        return patch;
    }

    NIns* Assembler::asm_branch_ov(LOpcode op, NIns* target)
    {
        USE(op);
        NanoAssert(target != NULL);

        NIns* patch = asm_bxx(true, LIR_eqi, AT, ZERO, target);

        TAG("asm_branch_ov(op=%s, target=%p)", lirNames[op], target);
        return patch;
    }

    Branches Assembler::asm_branch(bool branchOnFalse, LIns *cond, NIns * const targ)
    {
        NanoAssert(cond->isCmp());
        LOpcode condop = cond->opcode();
        RegisterMask allow = isCmpDOpcode(condop) ? FpRegs : GpRegs;
        LIns *a = cond->oprnd1();
        LIns *b = cond->oprnd2();
        Register ra = findRegFor(a, allow);
        Register rb = (b==a) ? ra : findRegFor(b, allow & ~rmask(ra));

        return Branches(asm_bxx(branchOnFalse, condop, ra, rb, targ));
    }

    void Assembler::asm_j(NIns * const targ, bool bdelay)
    {
        if (targ == NULL) {
            NanoAssert(bdelay);
            (void) asm_bxx(false, LIR_eqi, ZERO, ZERO, targ);
        }
        else {
            NanoAssert(SEG(targ) == SEG(_nIns));
            if (bdelay) {
                underrunProtect(2*4);    
                NOP();
            }
            J(targ);
        }
        TAG("asm_j(targ=%p) bdelay=%d", targ);
    }

    void
    Assembler::asm_spill(Register rr, int d, bool quad)
    {
        USE(quad);
        NanoAssert(d);
        if (IsFpReg(rr)) {
            NanoAssert(quad);
            asm_ldst64(true, rr, d, FP);
        }
        else {
            NanoAssert(!quad);
            asm_ldst(OP_SW, rr, d, FP);
        }
        TAG("asm_spill(rr=%d, d=%d, quad=%d)", rr, d, quad);
    }

    void
    Assembler::asm_nongp_copy(Register dst, Register src)
    {
        NanoAssert ((rmask(dst) & FpRegs) && (rmask(src) & FpRegs));
        MOV_D(dst, src);
        TAG("asm_nongp_copy(dst=%d src=%d)", dst, src);
    }

    











    void
    Assembler::asm_arg(ArgType ty, LIns* arg, Register& r, Register& fr, int& stkd)
    {
        
        NanoAssert((stkd & 3) == 0);

        if (ty == ARGTYPE_D) {
            
            asm_arg_64(arg, r, fr, stkd);
        } else {
            NanoAssert(ty == ARGTYPE_I || ty == ARGTYPE_UI);
            if (stkd < 16) {
                asm_regarg(ty, arg, r);
                fr = Register(fr + 1);
                r = Register(r + 1);
            }
            else
                asm_stkarg(arg, stkd);
            
            
            fr = r;
            stkd += 4;
        }
    }

    void
    Assembler::asm_call(LIns* ins)
    {
        if (!ins->isop(LIR_callv)) {
            Register rr;
            LOpcode op = ins->opcode();

            switch (op) {
            case LIR_calli:
                rr = retRegs[0];
                break;
            case LIR_calld:
                NanoAssert(cpu_has_fpu);
                rr = FV0;
                break;
            default:
                BADOPCODE(op);
                return;
            }

            deprecated_prepResultReg(ins, rmask(rr));
        }

        
        
        evictScratchRegsExcept(0);

        const CallInfo* ci = ins->callInfo();
        ArgType argTypes[MAXARGS];
        uint32_t argc = ci->getArgTypes(argTypes);
        bool indirect = ci->isIndirect();

        

        underrunProtect(2*4);    
        NOP();
        JALR(T9);

        if (!indirect)
            
            
            asm_li(T9, ci->_address);
        else
            
            
            asm_regarg(ARGTYPE_P, ins->arg(--argc), T9);

        
        Register    r = A0, fr = FA0;
        int         stkd = 0;

        
        
        
        
        while(argc--)
            asm_arg(argTypes[argc], ins->arg(argc), r, fr, stkd);

        if (stkd > max_out_args)
            max_out_args = stkd;
        TAG("asm_call(ins=%p{%s})", ins, lirNames[ins->opcode()]);
    }

    Register
    Assembler::nRegisterAllocFromSet(RegisterMask set)
    {
        Register i;
        int n;

        
        if (set & 0xffffffff) {
            
            n = ffs(int(set));
            NanoAssert(n != 0);
            i = Register(n - 1);
        }
        else {
            
            NanoAssert(cpu_has_fpu);
            n = ffs(int(set >> 32));
            NanoAssert(n != 0);
            i = Register(32 + n - 1);
        }
        _allocator.free &= ~rmask(i);
        TAG("nRegisterAllocFromSet(set=%016llx) => %s", set, gpn(i));
        return i;
    }

    void
    Assembler::nRegisterResetAll(RegAlloc& regs)
    {
        regs.clear();
        regs.free = GpRegs;
        if (cpu_has_fpu)
            regs.free |= FpRegs;
    }

#define signextend16(s) ((int32_t(s)<<16)>>16)

    void
    Assembler::nPatchBranch(NIns* branch, NIns* target)
    {
        uint32_t op = (branch[0] >> 26) & 0x3f;
        uint32_t bdoffset = target-(branch+1);

        if (op == OP_BEQ || op == OP_BNE ||
            ((branch[0] & 0xfffe0000) == ((OP_COP1 << 26) | (COP1_BC << 21)))) {
            if (isS16(bdoffset)) {
                
                
                branch[0] = (branch[0]  & 0xffff0000) | (bdoffset & 0xffff);
            }
            else {
                
                NIns *tramp = branch + 1 + (signextend16(branch[0] & 0xffff));
                if (SEG(branch) == SEG(target)) {
                    *tramp = J_FORMAT(OP_J,JINDEX(target));
                }
                else {
                    
                    
                    
                    
                    
                    
                    
                    
                    branch[1] = U_FORMAT(OP_LUI,0,AT,hi(uint32_t(target)));
                    tramp[0] = U_FORMAT(OP_ADDIU,AT,AT,lo(uint32_t(target)));
                    tramp[1] = R_FORMAT(OP_SPECIAL,AT,0,0,0,SPECIAL_JR);
                }
            }
        }
        else if (op == OP_J) {
            NanoAssert (SEG(branch) == SEG(target));
            branch[0] = J_FORMAT(OP_J,JINDEX(target));
        }
        else
            TODO(unknown_patch);
        
    }

    void
    Assembler::nFragExit(LIns *guard)
    {
        SideExit *exit = guard->record()->exit;
        Fragment *frag = exit->target;
        bool destKnown = (frag && frag->fragEntry);

        

        
        if (destKnown) {
            
            
            underrunProtect(2 * 4);     
            MOVE(V0, ZERO);
            asm_j(frag->fragEntry, false);
        }
        else {
            
            
            if (!_epilogue)
                _epilogue = genEpilogue();
            GuardRecord *lr = guard->record();
            
            
            
            
            underrunProtect(2 * 4);     
            ADDIU(V0, V0, lo(int32_t(lr)));
            asm_j(_epilogue, false);
            LUI(V0, hi(int32_t(lr)));
            lr->jmp = _nIns;
        }

        
        verbose_only(
            if (_logc->lcbits & LC_FragProfile) {
                
                
                
                
                uint32_t profCount = uint32_t(&guard->record()->profCount);
                SW(AT, lo(profCount), FP);
                ADDIU(AT, AT, 1);
                LW(AT, lo(profCount), FP);
                LUI(FP, hi(profCount));
            }
        )

        
        MOVE(SP, FP);

        
        TAG("nFragExit(guard=%p{%s})", guard, lirNames[guard->opcode()]);
    }

    void
    Assembler::nInit()
    {
        nHints[LIR_calli]  = rmask(V0);
#if NJ_SOFTFLOAT_SUPPORTED
        nHints[LIR_hcalli] = rmask(V1);
#endif
        nHints[LIR_calld]  = rmask(FV0);
        nHints[LIR_paramp] = PREFER_SPECIAL;
    }

    void Assembler::nBeginAssembly()
    {
        max_out_args = 16;        
    }

    
    
    verbose_only(
    void Assembler::asm_inc_m32(uint32_t* )
    {
        
    }
    )

    void
    Assembler::nativePageReset(void)
    {
        _nSlot = 0;
        _nExitSlot = 0;
        TAG("nativePageReset()");
    }

    void
    Assembler::nativePageSetup(void)
    {
        NanoAssert(!_inExit);
        if (!_nIns)
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
        if (!_nExitIns)
            codeAlloc(exitStart, exitEnd, _nExitIns verbose_only(, exitBytes));

        
        

        if (!_nSlot)
            _nSlot = codeStart;
        if (!_nExitSlot)
            _nExitSlot = exitStart;

        TAG("nativePageSetup()");
    }


    NIns*
    Assembler::genPrologue(void)
    {
        








        uint32_t stackNeeded = max_out_args + STACK_GRANULARITY * _activation.stackSlotsNeeded();
        uint32_t amt = alignUp(stackNeeded, NJ_ALIGN_STACK);

        if (amt) {
            if (isS16(-amt))
                ADDIU(SP, SP, -amt);
            else {
                ADDU(SP, SP, AT);
                asm_li(AT, -amt);
            }
        }

        NIns *patchEntry = _nIns; 

        MOVE(FP, SP);
        SW(FP, FP_OFFSET, SP);
        SW(RA, RA_OFFSET, SP);        
        ADDIU(SP, SP, -FRAMESIZE);

        TAG("genPrologue()");

        return patchEntry;
    }

    NIns*
    Assembler::genEpilogue(void)
    {
        






        ADDIU(SP, SP, FRAMESIZE);
        JR(RA);
        LW(FP, FP_OFFSET, SP);
        LW(RA, RA_OFFSET, SP);
        MOVE(SP, FP);

        TAG("genEpilogue()");

        return _nIns;
    }

    RegisterMask
    Assembler::nHint(LIns* ins)
    {
        NanoAssert(ins->isop(LIR_paramp));
        RegisterMask prefer = 0;
        
        if (ins->paramKind() == 0)
            if (ins->paramArg() < 4)
                prefer = rmask(argRegs[ins->paramArg()]);
        return prefer;
    }

    void
    Assembler::underrunProtect(int bytes)
    {
        NanoAssertMsg(bytes<=LARGEST_UNDERRUN_PROT, "constant LARGEST_UNDERRUN_PROT is too small");
        NanoAssert(_nSlot != 0);
        uintptr_t top = uintptr_t(_nSlot);
        uintptr_t pc = uintptr_t(_nIns);
        if (pc - bytes < top) {
            verbose_only(verbose_outputf("        %p:", _nIns);)
            NIns* target = _nIns;
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));

            _nSlot = codeStart;

            
            
            asm_j(target, true);
        }
    }

    void
    Assembler::swapCodeChunks() {
        if (!_nExitIns)
            codeAlloc(exitStart, exitEnd, _nExitIns verbose_only(, exitBytes));
        if (!_nExitSlot)
            _nExitSlot = exitStart;
        SWAP(NIns*, _nIns, _nExitIns);
        SWAP(NIns*, _nSlot, _nExitSlot);
        SWAP(NIns*, codeStart, exitStart);
        SWAP(NIns*, codeEnd, exitEnd);
        verbose_only( SWAP(size_t, codeBytes, exitBytes); )
    }

    void
    Assembler::asm_insert_random_nop() {
        NanoAssert(0); 
    }

}

#endif 
