






































#include "nanojit.h"

#if defined FEATURE_NANOJIT && defined NANOJIT_PPC

namespace nanojit
{
    const Register Assembler::retRegs[] = { R3, R4 }; 
    const Register Assembler::argRegs[] = { R3, R4, R5, R6, R7, R8, R9, R10 };

    const Register Assembler::savedRegs[] = {
    #if !defined NANOJIT_64BIT
        R13,
    #endif
        R14, R15, R16, R17, R18, R19, R20, R21, R22,
        R23, R24, R25, R26, R27, R28, R29, R30
    };

    const char *regNames[] = {
        "r0",  "sp",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
        "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
        "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
        "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
        "f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15",
        "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
        "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
    };

    const char *bitNames[] = { "lt", "gt", "eq", "so" };

    #define TODO(x) do{ avmplus::AvmLog(#x); NanoAssertMsgf(false, "%s", #x); } while(0)

    

















    const int linkage_size = 6*sizeof(void*);
    const int lr_offset = 2*sizeof(void*); 
    const int cr_offset = 1*sizeof(void*); 

    NIns* Assembler::genPrologue() {
        
        
        

        
        uint32_t stackNeeded = max_param_size + linkage_size +
            _activation.highwatermark * 4;
        uint32_t aligned = alignUp(stackNeeded, NJ_ALIGN_STACK);

        UNLESS_PEDANTIC( if (isS16(aligned)) {
            STPU(SP, -aligned, SP); 
        } else ) {
            STPUX(SP, SP, R0);
            asm_li(R0, -aligned);
        }

        NIns *patchEntry = _nIns;
        MR(FP,SP);                
        STP(FP, cr_offset, SP); 
        STP(R0, lr_offset, SP); 
        MFLR(R0);

        return patchEntry;
    }

    NIns* Assembler::genEpilogue() {
        max_param_size = 0;
        BLR();
        MTLR(R0);
        LP(R0, lr_offset, SP);
        LP(FP, cr_offset, SP); 
        MR(SP,FP);
        return _nIns;
    }

    void Assembler::asm_qjoin(LIns *ins) {
        int d = findMemFor(ins);
        NanoAssert(d && isS16(d));
        LIns* lo = ins->oprnd1();
        LIns* hi = ins->oprnd2();

        Register r = findRegFor(hi, GpRegs);
        STW(r, d+4, FP);

        
        r = findRegFor(lo, GpRegs);
        STW(r, d, FP);
        freeRsrcOf(ins, false); 
    }

    void Assembler::asm_ld(LIns *ins) {
        LIns* base = ins->oprnd1();
        LIns* disp = ins->oprnd2();
        Register rr = prepResultReg(ins, GpRegs);
        int d = disp->constval();
        Register ra = getBaseReg(base, d, GpRegs);

        #if !PEDANTIC
        if (isS16(d)) {
            if (ins->isop(LIR_ldcb)) {
                LBZ(rr, d, ra);
            } else {
                LWZ(rr, d, ra);
            }
            return;
        }
        #endif

        
        underrunProtect(12);
        LWZX(rr, ra, R0); 
        asm_li(R0,d);
    }

    void Assembler::asm_store32(LIns *value, int32_t dr, LIns *base) {
        Register rs = findRegFor(value, GpRegs);
        Register ra = value == base ? rs : getBaseReg(base, dr, GpRegs & ~rmask(rs));

    #if !PEDANTIC
        if (isS16(dr)) {
            STW(rs, dr, ra);
            return;
        }
    #endif

        
        STWX(rs, ra, R0);
        asm_li(R0, dr);
    }

    void Assembler::asm_load64(LIns *ins) {
        LIns* base = ins->oprnd1();
    #ifdef NANOJIT_64BIT
        Reservation *resv = getresv(ins);
        Register rr;
        if (resv && (rr = resv->reg) != UnknownReg && (rmask(rr) & FpRegs)) {
            
            freeRsrcOf(ins, false);
        } else {
            
            
            rr = prepResultReg(ins, GpRegs);
        }
    #else
        Register rr = prepResultReg(ins, FpRegs);
    #endif

        int dr = ins->oprnd2()->constval();
        Register ra = getBaseReg(base, dr, GpRegs);

    #ifdef NANOJIT_64BIT
        if (rmask(rr) & GpRegs) {
            #if !PEDANTIC
                if (isS16(dr)) {
                    LD(rr, dr, ra);
                    return;
                }
            #endif
            
            LDX(rr, ra, R0);
            asm_li(R0, dr);
            return;
        }
    #endif

        
    #if !PEDANTIC
        if (isS16(dr)) {
            LFD(rr, dr, ra);
            return;
        }
    #endif

        
        LFDX(rr, ra, R0);
        asm_li(R0, dr);
    }

    void Assembler::asm_li(Register r, int32_t imm) {
    #if !PEDANTIC
        if (isS16(imm)) {
            LI(r, imm);
            return;
        }
        if ((imm & 0xffff) == 0) {
            imm = uint32_t(imm) >> 16;
            LIS(r, imm);
            return;
        }
    #endif
        asm_li32(r, imm);
    }

    void Assembler::asm_li32(Register r, int32_t imm) {
        
        
        ORI(r, r, imm);
        LIS(r, imm>>16);  
    }

    void Assembler::asm_li64(Register r, uint64_t imm) {
        underrunProtect(5*sizeof(NIns)); 
        ORI(r,r,uint16_t(imm));        
        ORIS(r,r,uint16_t(imm>>16));   
        SLDI(r,r,32);                  
        asm_li32(r, int32_t(imm>>32)); 
    }

    void Assembler::asm_store64(LIns *value, int32_t dr, LIns *base) {
        NanoAssert(value->isQuad());
        Register ra = getBaseReg(base, dr, GpRegs);

    #if !PEDANTIC && !defined NANOJIT_64BIT
        if (value->isop(LIR_quad) && isS16(dr) && isS16(dr+4)) {
            
            uint64_t q = value->constvalq();
            STW(R0, dr, ra);   
            asm_li(R0, int32_t(q>>32)); 
            STW(R0, dr+4, ra); 
            asm_li(R0, int32_t(q));     
            return;
        }
        if (value->isop(LIR_qjoin) && isS16(dr) && isS16(dr+4)) {
            
            RegisterMask allow = GpRegs & ~rmask(ra);
            LIns *lo = value->oprnd1();
            Register rlo = findRegFor(lo, allow);
            LIns *hi = value->oprnd2();
            Register rhi = hi == lo ? rlo : findRegFor(hi, allow & ~rmask(rlo));
            STW(rhi, dr, ra); 
            STW(rlo, dr+4, ra); 
            return;
        }
    #endif 

        
    #if !defined NANOJIT_64BIT
        
        Register rs = findRegFor(value, FpRegs);
    #else
        
        Reservation *resv = getresv(value);
        Register rs;
        if (!resv || (rs = resv->reg) == UnknownReg) {
            rs = findRegFor(value, GpRegs & ~rmask(ra));
        }

        if (rmask(rs) & GpRegs) {
        #if !PEDANTIC
            if (isS16(dr)) {
                
                STD(rs, dr, ra);
                return;
            }
        #endif
            
            STDX(rs, ra, R0);
            asm_li(R0, dr);
            return;
        }
    #endif 

    #if !PEDANTIC
        if (isS16(dr)) {
            
            STFD(rs, dr, ra);
            return;
        }
    #endif

        
        STFDX(rs, ra, R0);
        asm_li(R0, dr);
    }

    void Assembler::asm_cond(LIns *ins) {
        LOpcode op = ins->opcode();
        LIns *a = ins->oprnd1();
        LIns *b = ins->oprnd2();
        ConditionRegister cr = CR7;
        Register r = prepResultReg(ins, GpRegs);
        switch (op) {
        case LIR_eq: case LIR_feq:
        case LIR_qeq:
            EXTRWI(r, r, 1, 4*cr+COND_eq); 
            MFCR(r);
            break;
        case LIR_lt: case LIR_ult:
        case LIR_flt: case LIR_fle:
        case LIR_qlt: case LIR_qult:
            EXTRWI(r, r, 1, 4*cr+COND_lt); 
            MFCR(r);
            break;
        case LIR_gt: case LIR_ugt:
        case LIR_fgt: case LIR_fge:
        case LIR_qgt: case LIR_qugt:
            EXTRWI(r, r, 1, 4*cr+COND_gt); 
            MFCR(r);
            break;
        case LIR_le: case LIR_ule:
        case LIR_qle: case LIR_qule:
            EXTRWI(r, r, 1, 4*cr+COND_eq); 
            MFCR(r);
            CROR(CR7, eq, lt, eq);
            break;
        case LIR_ge: case LIR_uge:
        case LIR_qge: case LIR_quge:
            EXTRWI(r, r, 1, 4*cr+COND_eq); 
            MFCR(r);
            CROR(CR7, eq, gt, eq);
            break;
        default:
            debug_only(outputf("%s",lirNames[ins->opcode()]);)
            TODO(asm_cond);
            break;
        }
        asm_cmp(op, a, b, cr);
    }

    void Assembler::asm_fcond(LIns *ins) {
        asm_cond(ins);
    }

    
    #define isS14(i) ((int32_t(bd<<18)>>18) == (i))

    NIns* Assembler::asm_branch(bool onfalse, LIns *cond, NIns * const targ) {
        LOpcode condop = cond->opcode();
        NanoAssert(cond->isCond());

        
        NIns *patch;
    #if !PEDANTIC
        ptrdiff_t bd = targ - (_nIns-1);
        if (targ && isS24(bd))
            patch = asm_branch_near(onfalse, cond, targ);
        else
    #endif
            patch = asm_branch_far(onfalse, cond, targ);
        asm_cmp(condop, cond->oprnd1(), cond->oprnd2(), CR7);
        return patch;
    }

    NIns* Assembler::asm_branch_near(bool onfalse, LIns *cond, NIns * const targ) {
        NanoAssert(targ != 0);
        underrunProtect(4);
        ptrdiff_t bd = targ - (_nIns-1);
        NIns *patch = 0;
        if (!isS14(bd)) {
            underrunProtect(8);
            bd = targ - (_nIns-1);
            if (isS24(bd)) {
                
                
                
                verbose_only(verbose_outputf("%p:", _nIns);)
                NIns *skip = _nIns;
                B(bd);
                patch = _nIns; 
                onfalse = !onfalse;
                bd = skip - (_nIns-1);
                NanoAssert(isS14(bd));
                verbose_only(verbose_outputf("branch24");)
            }
            else {
                
                return asm_branch_far(onfalse, cond, targ);
            }
        }
        ConditionRegister cr = CR7;
        switch (cond->opcode()) {
        case LIR_eq:
        case LIR_feq:
        case LIR_qeq:
            if (onfalse) BNE(cr,bd); else BEQ(cr,bd);
            break;
        case LIR_lt: case LIR_ult:
        case LIR_flt: case LIR_fle:
        case LIR_qlt: case LIR_qult:
            if (onfalse) BNL(cr,bd); else BLT(cr,bd);
            break;
        case LIR_le: case LIR_ule:
        case LIR_qle: case LIR_qule:
            if (onfalse) BGT(cr,bd); else BLE(cr,bd);
            break;
        case LIR_gt: case LIR_ugt:
        case LIR_fgt: case LIR_fge:
        case LIR_qgt: case LIR_qugt:
            if (onfalse) BNG(cr,bd); else BGT(cr,bd);
            break;
        case LIR_ge: case LIR_uge:
        case LIR_qge: case LIR_quge:
            if (onfalse) BLT(cr,bd); else BGE(cr,bd);
            break;
        default:
            debug_only(outputf("%s",lirNames[cond->opcode()]);)
            TODO(unknown_cond);
        }
        if (!patch)
            patch = _nIns;
        return patch;
    }

    
    NIns *Assembler::asm_branch_far(bool onfalse, LIns *cond, NIns * const targ) {
        LOpcode condop = cond->opcode();
        ConditionRegister cr = CR7;
        underrunProtect(16);
        switch (condop) {
        case LIR_eq:
        case LIR_feq:
        case LIR_qeq:
            if (onfalse) BNECTR(cr); else BEQCTR(cr);
            break;
        case LIR_lt: case LIR_ult:
        case LIR_qlt: case LIR_qult:
        case LIR_flt: case LIR_fle:
            if (onfalse) BNLCTR(cr); else BLTCTR(cr);
            break;
        case LIR_le: case LIR_ule:
        case LIR_qle: case LIR_qule:
            if (onfalse) BGTCTR(cr); else BLECTR(cr);
            break;
        case LIR_gt: case LIR_ugt:
        case LIR_qgt: case LIR_qugt:
        case LIR_fgt: case LIR_fge:
            if (onfalse) BNGCTR(cr); else BGTCTR(cr);
            break;
        case LIR_ge: case LIR_uge:
        case LIR_qge: case LIR_quge:
            if (onfalse) BLTCTR(cr); else BGECTR(cr);
            break;
        default:
            debug_only(outputf("%s",lirNames[condop]);)
            TODO(unknown_cond);
        }

    #if !defined NANOJIT_64BIT
        MTCTR(R0);
        asm_li32(R0, (int)targ);
    #else
        MTCTR(R0);
        if (!targ || !isU32(uintptr_t(targ))) {
            asm_li64(R0, uint64_t(targ));
        } else {
            asm_li32(R0, uint32_t(uintptr_t(targ)));
        }
    #endif
        return _nIns;
    }

    void Assembler::asm_cmp(LOpcode condop, LIns *a, LIns *b, ConditionRegister cr) {
        RegisterMask allow = condop >= LIR_feq && condop <= LIR_fge ? FpRegs : GpRegs;
        Register ra = findRegFor(a, allow);

    #if !PEDANTIC
        if (b->isconst()) {
            int32_t d = b->constval();
            if (isS16(d)) {
                if (condop >= LIR_eq && condop <= LIR_ge) {
                    CMPWI(cr, ra, d);
                    return;
                }
                if (condop >= LIR_qeq && condop <= LIR_qge) {
                    CMPDI(cr, ra, d);
                    TODO(cmpdi);
                    return;
                }
            }
            if (isU16(d)) {
                if ((condop == LIR_eq || condop >= LIR_ult && condop <= LIR_uge)) {
                    CMPLWI(cr, ra, d);
                    return;
                }
                if ((condop == LIR_qeq || condop >= LIR_qult && condop <= LIR_quge)) {
                    CMPLDI(cr, ra, d);
                    TODO(cmpldi);
                    return;
                }
            }
        }
    #endif

        
        Register rb = b==a ? ra : findRegFor(b, allow & ~rmask(ra));
        if (condop >= LIR_eq && condop <= LIR_ge) {
            CMPW(cr, ra, rb);
        } else if (condop >= LIR_ult && condop <= LIR_uge) {
            CMPLW(cr, ra, rb);
        } else if (condop >= LIR_qeq && condop <= LIR_qge) {
            CMPD(cr, ra, rb);
        }
        else if (condop >= LIR_qult && condop <= LIR_quge) {
            CMPLD(cr, ra, rb);
        }
        else if (condop >= LIR_feq && condop <= LIR_fge) {
            
            
            
            if (condop == LIR_fle)
                CROR(cr, lt, lt, eq); 
            else if (condop == LIR_fge)
                CROR(cr, gt, gt, eq); 
            FCMPU(cr, ra, rb);
        }
        else {
            TODO(asm_cmp);
        }
    }

    void Assembler::asm_ret(LIns *ins) {
        UNLESS_PEDANTIC( if (_nIns != _epilogue) ) {
            br(_epilogue, 0);
        }
        assignSavedParams();
        LIns *value = ins->oprnd1();
        Register r = ins->isop(LIR_ret) ? R3 : F1;
        findSpecificRegFor(value, r);
    }

    void Assembler::asm_nongp_copy(Register r, Register s) {
        
        NanoAssert((rmask(r) & FpRegs) && (rmask(s) & FpRegs));
        FMR(r, s);
    }

    void Assembler::asm_restore(LIns *i, Reservation *resv, Register r) {
        int d;
        if (i->isop(LIR_alloc)) {
            d = disp(resv);
            ADDI(r, FP, d);
        }
        else if (i->isconst()) {
            if (!resv->arIndex) {
                reserveFree(i);
            }
            asm_li(r, i->constval());
        }
        else {
            d = findMemFor(i);
            if (IsFpReg(r)) {
                NanoAssert(i->isQuad());
                LFD(r, d, FP);
            } else if (i->isQuad()) {
                LD(r, d, FP);
            } else {
                LWZ(r, d, FP);
            }
            verbose_only(
                if (_verbose)
                    outputf("        restore %s",_thisfrag->lirbuf->names->formatRef(i));
            )
        }
    }

    Register Assembler::asm_prep_fcall(Reservation*, LIns *ins) {
        return prepResultReg(ins, rmask(F1));
    }

    void Assembler::asm_int(LIns *ins) {
        Register rr = prepResultReg(ins, GpRegs);
        asm_li(rr, ins->constval());
    }

    void Assembler::asm_short(LIns *ins) {
        int32_t val = ins->imm16();
        Register rr = prepResultReg(ins, GpRegs);
        LI(rr, val);
    }

    void Assembler::asm_fneg(LIns *ins) {
        Register rr = prepResultReg(ins, FpRegs);
        Register ra = findRegFor(ins->oprnd1(), FpRegs);
        FNEG(rr,ra);
    }

    void Assembler::asm_param(LIns *ins) {
        uint32_t a = ins->imm8();
        uint32_t kind = ins->imm8b();
        if (kind == 0) {
            
            
            if (a < 8) {
                
                prepResultReg(ins, rmask(argRegs[a]));
            } else {
                
                
                TODO(asm_param_stk);
            }
        }
        else {
            
            prepResultReg(ins, rmask(savedRegs[a]));
        }
    }

    void Assembler::asm_call(LIns *ins) {
        const CallInfo* call = ins->callInfo();
        ArgSize sizes[MAXARGS];
        uint32_t argc = call->get_sizes(sizes);

        bool indirect;
        if (!(indirect = call->isIndirect())) {
            verbose_only(if (_verbose)
                outputf("        %p:", _nIns);
            )
            br((NIns*)call->_address, 1);
        } else {
            
            
            
            underrunProtect(8); 
            BCTRL();
            MTCTR(R11);
            asm_regarg(ARGSIZE_P, ins->arg(--argc), R11);
        }

        int param_size = 0;

        Register r = R3;
        Register fr = F1;
        for(uint32_t i = 0; i < argc; i++) {
            uint32_t j = argc - i - 1;
            ArgSize sz = sizes[j];
            LInsp arg = ins->arg(j);
            if (sz & ARGSIZE_MASK_INT) {
                
                if (r <= R10) {
                    asm_regarg(sz, arg, r);
                    r = nextreg(r);
                    param_size += sizeof(void*);
                } else {
                    
                    TODO(stack_int32);
                }
            } else if (sz == ARGSIZE_F) {
                
                if (fr <= F13) {
                    asm_regarg(sz, arg, fr);
                    fr = nextreg(fr);
                #ifdef NANOJIT_64BIT
                    r = nextreg(r);
                #else
                    r = nextreg(nextreg(r)); 
                #endif
                    param_size += sizeof(double);
                } else {
                    
                    TODO(stack_double);
                }
            } else {
                TODO(ARGSIZE_UNK);
            }
        }
        if (param_size > max_param_size)
            max_param_size = param_size;
    }

    void Assembler::asm_regarg(ArgSize sz, LInsp p, Register r)
    {
        NanoAssert(r != UnknownReg);
        if (sz & ARGSIZE_MASK_INT)
        {
        #ifdef NANOJIT_64BIT
            if (sz == ARGSIZE_I) {
                
                EXTSW(r, r);
            } else if (sz == ARGSIZE_U) {
                
                CLRLDI(r, r, 32);
            }
        #endif
            
            if (p->isconst()) {
                asm_li(r, p->constval());
            } else {
                Reservation* rA = getresv(p);
                if (rA) {
                    if (rA->reg == UnknownReg) {
                        
                        int d = findMemFor(p);
                        if (p->isop(LIR_alloc)) {
                            NanoAssert(isS16(d));
                            ADDI(r, FP, d);
                        } else if (p->isQuad()) {
                            LD(r, d, FP);
                        } else {
                            LWZ(r, d, FP);
                        }
                    } else {
                        
                        MR(r, rA->reg);
                    }
                }
                else {
                    
                    
                    findSpecificRegFor(p, r);
                }
            }
        }
        else if (sz == ARGSIZE_F) {
            Reservation* rA = getresv(p);
            if (rA) {
                if (rA->reg == UnknownReg || !IsFpReg(rA->reg)) {
                    
                    int d = findMemFor(p);
                    LFD(r, d, FP);
                } else {
                    
                    NanoAssert(IsFpReg(r) && IsFpReg(rA->reg));
                    FMR(r, rA->reg);
                }
            }
            else {
                
                
                findSpecificRegFor(p, r);
            }
        }
        else {
            TODO(ARGSIZE_UNK);
        }
    }

    void Assembler::asm_spill(Register rr, int d, bool , bool quad) {
        (void)quad;
        if (d) {
            if (IsFpReg(rr)) {
                NanoAssert(quad);
                STFD(rr, d, FP);
            }
        #ifdef NANOJIT_64BIT
            else if (quad) {
                STD(rr, d, FP);
            }
        #endif
            else {
                NanoAssert(!quad);
                STW(rr, d, FP);
            }
        }
    }

    void Assembler::asm_arith(LIns *ins) {
        LOpcode op = ins->opcode();
        LInsp lhs = ins->oprnd1();
        LInsp rhs = ins->oprnd2();
        RegisterMask allow = GpRegs;
        Register rr = prepResultReg(ins, allow);
        Register ra = findRegFor(lhs, GpRegs);

        if (rhs->isconst()) {
            int32_t rhsc = rhs->constval();
            if (isS16(rhsc)) {
                
                switch (op) {
                case LIR_add:
                case LIR_iaddp:
                IF_64BIT(case LIR_qiadd:)
                IF_64BIT(case LIR_qaddp:)
                    ADDI(rr, ra, rhsc);
                    return;
                case LIR_sub:
                    SUBI(rr, ra, rhsc);
                    return;
                case LIR_mul:
                    MULLI(rr, ra, rhsc);
                    return;
                }
            }
            if (isU16(rhsc)) {
                
                switch (op) {
                IF_64BIT(case LIR_qior:)
                case LIR_or:
                    ORI(rr, ra, rhsc);
                    return;
                IF_64BIT(case LIR_qiand:)
                case LIR_and:
                    ANDI(rr, ra, rhsc);
                    return;
                IF_64BIT(case LIR_qxor:)
                case LIR_xor:
                    XORI(rr, ra, rhsc);
                    return;
                }
            }

            
            switch (op) {
            case LIR_lsh:
                SLWI(rr, ra, rhsc&31);
                return;
            case LIR_ush:
                SRWI(rr, ra, rhsc&31);
                return;
            case LIR_rsh:
                SRAWI(rr, ra, rhsc&31);
                return;
            }
        }

        
        Register rb = rhs==lhs ? ra : findRegFor(rhs, GpRegs&~rmask(ra));
        switch (op) {
            IF_64BIT(case LIR_qiadd:)
            IF_64BIT(case LIR_qaddp:)
            case LIR_add:
            case LIR_iaddp:
                ADD(rr, ra, rb);
                break;
            IF_64BIT(case LIR_qiand:)
            case LIR_and:
                AND(rr, ra, rb);
                break;
            IF_64BIT(case LIR_qior:)
            case LIR_or:
                OR(rr, ra, rb);
                break;
            IF_64BIT(case LIR_qxor:)
            case LIR_xor:
                XOR(rr, ra, rb);
                break;
            case LIR_sub:  SUBF(rr, rb, ra);    break;
            case LIR_lsh:  SLW(rr, ra, R0);        ANDI(R0, rb, 31);    break;
            case LIR_rsh:  SRAW(rr, ra, R0);    ANDI(R0, rb, 31);    break;
            case LIR_ush:  SRW(rr, ra, R0);        ANDI(R0, rb, 31);    break;
            case LIR_mul:  MULLW(rr, ra, rb);    break;
        #ifdef NANOJIT_64BIT
            case LIR_qilsh:
                SLD(rr, ra, R0);
                ANDI(R0, rb, 63);
                break;
            case LIR_qursh:
                SRD(rr, ra, R0);
                ANDI(R0, rb, 63);
                break;
            case LIR_qirsh:
                SRAD(rr, ra, R0);
                ANDI(R0, rb, 63);
                TODO(qirsh);
                break;
        #endif
            default:
                debug_only(outputf("%s",lirNames[op]);)
                TODO(asm_arith);
        }
    }

    void Assembler::asm_fop(LIns *ins) {
        LOpcode op = ins->opcode();
        LInsp lhs = ins->oprnd1();
        LInsp rhs = ins->oprnd2();
        RegisterMask allow = FpRegs;
        Register rr = prepResultReg(ins, allow);
        Reservation *rA, *rB;
        findRegFor2(allow, lhs, rA, rhs, rB);
        Register ra = rA->reg;
        Register rb = rB->reg;
        switch (op) {
            case LIR_fadd: FADD(rr, ra, rb); break;
            case LIR_fsub: FSUB(rr, ra, rb); break;
            case LIR_fmul: FMUL(rr, ra, rb); break;
            case LIR_fdiv: FDIV(rr, ra, rb); break;
            default:
                debug_only(outputf("%s",lirNames[op]);)
                TODO(asm_fop);
        }
    }

    void Assembler::asm_i2f(LIns *ins) {
        Register r = prepResultReg(ins, FpRegs);
        Register v = findRegFor(ins->oprnd1(), GpRegs);
        const int d = 16; 

    #if defined NANOJIT_64BIT && !PEDANTIC
        FCFID(r, r);    
        LFD(r, d, SP);    
        STD(v, d, SP);    
        EXTSW(v, v);    
    #else
        FSUB(r, r, F0);
        LFD(r, d, SP); 
        STW(R0, d+4, SP);
        XORIS(R0, v, 0x8000);
        LFD(F0, d, SP);
        STW(R0, d+4, SP);
        LIS(R0, 0x8000);
        STW(R0, d, SP);
        LIS(R0, 0x4330);
    #endif
    }

    void Assembler::asm_u2f(LIns *ins) {
        Register r = prepResultReg(ins, FpRegs);
        Register v = findRegFor(ins->oprnd1(), GpRegs);
        const int d = 16;

    #if defined NANOJIT_64BIT && !PEDANTIC
        FCFID(r, r);    
        LFD(r, d, SP);    
        STD(v, d, SP);    
        CLRLDI(v, v, 32); 
    #else
        FSUB(r, r, F0);
        LFD(F0, d, SP);
        STW(R0, d+4, SP);
        LI(R0, 0);
        LFD(r, d, SP);
        STW(v, d+4, SP);
        STW(R0, d, SP);
        LIS(R0, 0x4330);
    #endif
    }

    void Assembler::asm_promote(LIns *ins) {
        LOpcode op = ins->opcode();
        Register r = prepResultReg(ins, GpRegs);
        Register v = findRegFor(ins->oprnd1(), GpRegs);
        switch (op) {
        default:
            debug_only(outputf("%s",lirNames[op]));
            TODO(asm_promote);
        case LIR_u2q:
            CLRLDI(r, v, 32); 
            break;
        case LIR_i2q:
            EXTSW(r, v);
            break;
        }
    }

    void Assembler::asm_quad(LIns *ins) {
    #ifdef NANOJIT_64BIT
        Reservation *resv = getresv(ins);
        Register r;
        if (resv && (r = resv->reg) != UnknownReg && (rmask(r) & FpRegs)) {
            
            freeRsrcOf(ins, false);
        } else {
            
            
            r = prepResultReg(ins, GpRegs);
        }
    #else
        Register r = prepResultReg(ins, FpRegs);
    #endif

        if (rmask(r) & FpRegs) {
            union {
                double d;
                struct {
                    int32_t hi, lo;
                } w;
            };
            d = ins->constvalf();
            LFD(r, 12, SP);
            STW(R0, 12, SP);
            asm_li(R0, w.hi);
            STW(R0, 16, SP);
            asm_li(R0, w.lo);
        }
        else {
            int64_t q = ins->constvalq();
            if (isS32(q)) {
                asm_li(r, int32_t(q));
                return;
            }
            RLDIMI(r,R0,32,0); 
            asm_li(R0, int32_t(q>>32)); 
            asm_li(r, int32_t(q)); 
        }
    }

    void Assembler::br(NIns* addr, int link) {
        
        if (!addr) {
            br_far(addr,link);
            return;
        }

        
        underrunProtect(4);       
        ptrdiff_t offset = addr - (_nIns-1); 

        #if !PEDANTIC
        if (isS24(offset)) {
            Bx(offset, 0, link); 
            return;
        }
        ptrdiff_t absaddr = addr - (NIns*)0; 
        if (isS24(absaddr)) {
            Bx(absaddr, 1, link); 
            return;
        }
        #endif 

        br_far(addr,link);
    }

    void Assembler::br_far(NIns* addr, int link) {
        
        
        
        
        
    #ifdef NANOJIT_64BIT
        if (addr==0 || !isU32(uintptr_t(addr))) {
            
            underrunProtect(28); 
            BCTR(link);
            MTCTR(R2);
            asm_li64(R2, uintptr_t(addr)); 
            return;
        }
    #endif
        underrunProtect(16);
        BCTR(link);
        MTCTR(R2);
        asm_li32(R2, uint32_t(uintptr_t(addr))); 
    }

    void Assembler::underrunProtect(int bytes) {
        NanoAssertMsg(bytes<=LARGEST_UNDERRUN_PROT, "constant LARGEST_UNDERRUN_PROT is too small");
        int instr = (bytes + sizeof(NIns) - 1) / sizeof(NIns);
        NIns *top = _inExit ? this->exitStart : this->codeStart;
        NIns *pc = _nIns;

    #if PEDANTIC
        
        
        
        

        NanoAssert(pedanticTop >= top);
        if (pc - instr < pedanticTop) {
            
        #ifdef NANOJIT_64BIT
            const int br_size = 7;
        #else
            const int br_size = 4;
        #endif
            if (pc - instr - br_size < top) {
                
                verbose_only(if (_verbose) outputf("newpage %p:", pc);)
                codeAlloc();
            }
            
            
            pedanticTop = _nIns - br_size;
            br(pc, 0);
            pedanticTop = _nIns - instr;
        }
    #else
        if (pc - instr < top) {
            verbose_only(if (_verbose) outputf("newpage %p:", pc);)
            codeAlloc();
            
            
            br(pc, 0);
        }
    #endif
    }

    void Assembler::asm_cmov(LIns *ins) {
        NanoAssert(ins->isop(LIR_cmov) || ins->isop(LIR_qcmov));
        LIns* cond = ins->oprnd1();
        NanoAssert(cond->isCmp());
        LIns* iftrue = ins->oprnd2();
        LIns* iffalse = ins->oprnd3();
        NanoAssert(iftrue->isQuad() == iffalse->isQuad());
        
        Register rr = prepResultReg(ins, GpRegs);
        findSpecificRegFor(iftrue, rr);
        Register rf = findRegFor(iffalse, GpRegs & ~rmask(rr));
        NIns *after = _nIns;
        verbose_only(if (_verbose) outputf("%p:",after);)
        MR(rr, rf);
        asm_branch(false, cond, after);
    }

    RegisterMask Assembler::hint(LIns *i, RegisterMask allow) {
        LOpcode op = i->opcode();
        RegisterMask prefer = ~0LL;
        if (op == LIR_icall || op == LIR_qcall)
            prefer = rmask(R3);
        else if (op == LIR_fcall)
            prefer = rmask(F1);
        else if (op == LIR_iparam) {
            if (i->imm8() < 8) {
                prefer = rmask(argRegs[i->imm8()]);
            }
        }
        
        if (_allocator.free & allow & prefer)
            allow &= prefer;
        return allow;
    }

    void Assembler::asm_neg_not(LIns *ins) {
        Register rr = prepResultReg(ins, GpRegs);
        Register ra = findRegFor(ins->oprnd1(), GpRegs);
        if (ins->isop(LIR_neg)) {
            NEG(rr, ra);
        } else {
            NOT(rr, ra);
        }
    }

    void Assembler::asm_qlo(LIns *ins) {
        Register rr = prepResultReg(ins, GpRegs);
        int d = findMemFor(ins->oprnd1());
        LWZ(rr, d+4, FP);
    }

    void Assembler::asm_qhi(LIns *ins) {
        Register rr = prepResultReg(ins, GpRegs);
        int d = findMemFor(ins->oprnd1());
        LWZ(rr, d, FP);
        TODO(asm_qhi);
    }

    void Assembler::nInit(AvmCore*) {
    }

    void Assembler::nativePageSetup() {
        if (!_nIns) {
            codeAlloc();
            IF_PEDANTIC( pedanticTop = _nIns; )
        }
        if (!_nExitIns) {
            codeAlloc(true);
        }
    }

    void Assembler::nativePageReset()
    {}

    void Assembler::nPatchBranch(NIns *branch, NIns *target) {
        
        ptrdiff_t bd = target - branch;
        if (branch[0] == PPC_b) {
            
            
            
            NanoAssert(isS24(bd));
            branch[0] |= (bd & 0xffffff) << 2;
        }
        else if ((branch[0] & PPC_bc) == PPC_bc) {
            
            
            
            NanoAssert(isS14(bd));
            NanoAssert(((branch[0] & 0x3fff)<<2) == 0);
            branch[0] |= (bd & 0x3fff) << 2;
            TODO(patch_bc);
        }
    #ifdef NANOJIT_64BIT
        
        else if ((branch[0] & ~(31<<21)) == PPC_addis) {
            
            Register rd = Register((branch[0] >> 21) & 31);
            NanoAssert(branch[1] == PPC_ori  | GPR(rd)<<21 | GPR(rd)<<16);
            NanoAssert(branch[3] == PPC_oris | GPR(rd)<<21 | GPR(rd)<<16);
            NanoAssert(branch[4] == PPC_ori  | GPR(rd)<<21 | GPR(rd)<<16);
            uint64_t imm = uintptr_t(target);
            uint32_t lo = uint32_t(imm);
            uint32_t hi = uint32_t(imm>>32);
            branch[0] = PPC_addis | GPR(rd)<<21 |               uint16_t(hi>>16);
            branch[1] = PPC_ori   | GPR(rd)<<21 | GPR(rd)<<16 | uint16_t(hi);
            branch[3] = PPC_oris  | GPR(rd)<<21 | GPR(rd)<<16 | uint16_t(lo>>16);
            branch[4] = PPC_ori   | GPR(rd)<<21 | GPR(rd)<<16 | uint16_t(lo);
        }
    #else 
        
        else if ((branch[0] & ~(31<<21)) == PPC_addis) {
            
            
            Register rd = Register((branch[0] >> 21) & 31);
            NanoAssert(branch[1] == PPC_ori | GPR(rd)<<21 | GPR(rd)<<16);
            uint32_t imm = uint32_t(target);
            branch[0] = PPC_addis | GPR(rd)<<21 | uint16_t(imm >> 16); 
            branch[1] = PPC_ori | GPR(rd)<<21 | GPR(rd)<<16 | uint16_t(imm); 
        }
    #endif 
        else {
            TODO(unknown_patch);
        }
    }

    static int cntzlw(int set) {
        
        
        register Register i;
        #ifdef __GNUC__
        asm ("cntlzw %0,%1" : "=r" (i) : "r" (set));
        #else 
        # error("unsupported compiler")
        #endif 
        return 31-i;
    }

    Register Assembler::nRegisterAllocFromSet(RegisterMask set) {
        Register i;
        
        if (set & 0xffffffff) {
            i = Register(cntzlw(int(set))); 
        } else {
            i = Register(32+cntzlw(int(set>>32))); 
        }
        _allocator.free &= ~rmask(i);
        return i;
    }

    void Assembler::nRegisterResetAll(RegAlloc &regs) {
        regs.clear();
        regs.used = 0;
        regs.free = SavedRegs | 0x1ff8  | 0x3ffe00000000LL ;
        debug_only(regs.managed = regs.free);
    }

#ifdef NANOJIT_64BIT
    void Assembler::asm_qbinop(LIns *ins) {
        LOpcode op = ins->opcode();
        switch (op) {
        case LIR_qaddp:
        case LIR_qior:
        case LIR_qiand:
        case LIR_qursh:
        case LIR_qirsh:
        case LIR_qilsh:
        case LIR_qxor:
            asm_arith(ins);
            break;
        default:
            debug_only(outputf("%s",lirNames[op]));
            TODO(asm_qbinop);
        }
    }
#endif 

    void Assembler::asm_loop(LIns*, NInsList&) {
        TODO(asm_loop);
    }

    void Assembler::nFragExit(LIns*) {
        TODO(nFragExit);
    }

    NIns* Assembler::asm_adjustBranch(NIns*, NIns*) {
        TODO(asm_adjustBranch);
        return 0;
    }

} 

#endif 
