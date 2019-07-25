






































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

    

















    const int min_param_area_size = 8*sizeof(void*); 
    const int linkage_size = 6*sizeof(void*);
    const int lr_offset = 2*sizeof(void*); 
    const int cr_offset = 1*sizeof(void*); 

    NIns* Assembler::genPrologue() {
        
        
        

        
        
        
        
        uint32_t param_area = (max_param_size > min_param_area_size) ? max_param_size : min_param_area_size;
        
        uint32_t stackNeeded = param_area + linkage_size + _activation.stackSlotsNeeded() * 4;
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
        BLR();
        MTLR(R0);
        LP(R0, lr_offset, SP);
        LP(FP, cr_offset, SP); 
        MR(SP,FP);
        return _nIns;
    }

    void Assembler::asm_load32(LIns *ins) {
        LIns* base = ins->oprnd1();
        int d = ins->disp();
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        Register ra = getBaseReg(base, d, GpRegs);

        switch(ins->opcode()) {
            case LIR_lduc2ui:
                if (isS16(d)) {
                    LBZ(rr, d, ra);
                } else {
                    LBZX(rr, ra, R0); 
                    asm_li(R0,d);
                }
                return;
            case LIR_ldus2ui:
                
                if (isS16(d)) {
                    LHZ(rr, d, ra);
                } else {
                    LHZX(rr, ra, R0); 
                    asm_li(R0,d);
                }
                return;
            case LIR_ldi:
                
                if (isS16(d)) {
                    LWZ(rr, d, ra);
                } else {
                    LWZX(rr, ra, R0); 
                    asm_li(R0,d);
                }
                return;
            case LIR_ldc2i:
            case LIR_lds2i:
                NanoAssertMsg(0, "NJ_EXPANDED_LOADSTORE_SUPPORTED not yet supported for this architecture");
                return;
            default:
                NanoAssertMsg(0, "asm_load32 should never receive this LIR opcode");
                return;
        }
    }

    void Assembler::asm_store32(LOpcode op, LIns *value, int32_t dr, LIns *base) {

        switch (op) {
            case LIR_sti:
            case LIR_sti2c:
                
                break;
            case LIR_sti2s:
                NanoAssertMsg(0, "NJ_EXPANDED_LOADSTORE_SUPPORTED not yet supported for this architecture");
                return;
            default:
                NanoAssertMsg(0, "asm_store32 should never receive this LIR opcode");
                return;
        }

        Register rs = findRegFor(value, GpRegs);
        Register ra = value == base ? rs : getBaseReg(base, dr, GpRegs & ~rmask(rs));

    #if !PEDANTIC
        if (isS16(dr)) {
            switch (op) {
            case LIR_sti:
                STW(rs, dr, ra);
                break;
            case LIR_sti2c:
                STB(rs, dr, ra);
                break;
            }
            return;
        }
    #endif

        
        switch (op) {
        case LIR_sti:
            STWX(rs, ra, R0);
            break;
        case LIR_sti2c:
            STBX(rs, ra, R0);
            break;
        }
        asm_li(R0, dr);
    }

    void Assembler::asm_load64(LIns *ins) {

        switch (ins->opcode()) {
            case LIR_ldd:
            CASE64(LIR_ldq:)
                
                break;
            case LIR_ldf2d:
                NanoAssertMsg(0, "NJ_EXPANDED_LOADSTORE_SUPPORTED not yet supported for this architecture");
                return;
            default:
                NanoAssertMsg(0, "asm_load64 should never receive this LIR opcode");
                return;
        }

        LIns* base = ins->oprnd1();
    #ifdef NANOJIT_64BIT
        Register rr = ins->deprecated_getReg();
        if (deprecated_isKnownReg(rr) && (rmask(rr) & FpRegs)) {
            
            deprecated_freeRsrcOf(ins);
        } else {
            
            
            rr = deprecated_prepResultReg(ins, GpRegs);
        }
    #else
        Register rr = deprecated_prepResultReg(ins, FpRegs);
    #endif

        int dr = ins->disp();
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

    void Assembler::asm_store64(LOpcode op, LIns *value, int32_t dr, LIns *base) {
        NanoAssert(value->isQorD());

        switch (op) {
            case LIR_std:
            CASE64(LIR_stq:)
                
                break;
            case LIR_std2f:
                NanoAssertMsg(0, "NJ_EXPANDED_LOADSTORE_SUPPORTED not yet supported for this architecture");
                return;
            default:
                NanoAssertMsg(0, "asm_store64 should never receive this LIR opcode");
                return;
        }

        Register ra = getBaseReg(base, dr, GpRegs);

        
    #if !defined NANOJIT_64BIT
        
        Register rs = findRegFor(value, FpRegs);
    #else
        
        Register rs = ( !value->isInReg()
                      ? findRegFor(value, GpRegs & ~rmask(ra))
                      : value->deprecated_getReg() );

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
        Register r = deprecated_prepResultReg(ins, GpRegs);
        switch (op) {
        case LIR_eqi: case LIR_eqd:
        CASE64(LIR_eqq:)
            EXTRWI(r, r, 1, 4*cr+COND_eq); 
            MFCR(r);
            break;
        case LIR_lti: case LIR_ltui:
        case LIR_ltd: case LIR_led:
        CASE64(LIR_ltq:) CASE64(LIR_ltuq:)
            EXTRWI(r, r, 1, 4*cr+COND_lt); 
            MFCR(r);
            break;
        case LIR_gti: case LIR_gtui:
        case LIR_gtd: case LIR_ged:
        CASE64(LIR_gtq:) CASE64(LIR_gtuq:)
            EXTRWI(r, r, 1, 4*cr+COND_gt); 
            MFCR(r);
            break;
        case LIR_lei: case LIR_leui:
        CASE64(LIR_leq:) CASE64(LIR_leuq:)
            EXTRWI(r, r, 1, 4*cr+COND_eq); 
            MFCR(r);
            CROR(CR7, eq, lt, eq);
            break;
        case LIR_gei: case LIR_geui:
        CASE64(LIR_geq:) CASE64(LIR_geuq:)
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

    void Assembler::asm_condd(LIns *ins) {
        asm_cond(ins);
    }

    
    
    static inline bool isS14(ptrdiff_t d) {
        const int shift = sizeof(ptrdiff_t) * 8 - 14; 
        return ((d << shift) >> shift) == d;
    }

    Branches Assembler::asm_branch(bool onfalse, LIns *cond, NIns * const targ) {
        LOpcode condop = cond->opcode();
        NanoAssert(cond->isCmp());

        
        NIns *patch;
    #if !PEDANTIC
        ptrdiff_t bd = targ - (_nIns-1);
        if (targ && isS24(bd))
            patch = asm_branch_near(onfalse, cond, targ);
        else
    #endif
            patch = asm_branch_far(onfalse, cond, targ);
        asm_cmp(condop, cond->oprnd1(), cond->oprnd2(), CR7);
        return Branches(patch);
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
        case LIR_eqi:
        case LIR_eqd:
        CASE64(LIR_eqq:)
            if (onfalse) BNE(cr,bd); else BEQ(cr,bd);
            break;
        case LIR_lti: case LIR_ltui:
        case LIR_ltd: case LIR_led:
        CASE64(LIR_ltq:) CASE64(LIR_ltuq:)
            if (onfalse) BNL(cr,bd); else BLT(cr,bd);
            break;
        case LIR_lei: case LIR_leui:
        CASE64(LIR_leq:) CASE64(LIR_leuq:)
            if (onfalse) BGT(cr,bd); else BLE(cr,bd);
            break;
        case LIR_gti: case LIR_gtui:
        case LIR_gtd: case LIR_ged:
        CASE64(LIR_gtq:) CASE64(LIR_gtuq:)
            if (onfalse) BNG(cr,bd); else BGT(cr,bd);
            break;
        case LIR_gei: case LIR_geui:
        CASE64(LIR_geq:) CASE64(LIR_geuq:)
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
        case LIR_eqi:
        case LIR_eqd:
        CASE64(LIR_eqq:)
            if (onfalse) BNECTR(cr); else BEQCTR(cr);
            break;
        case LIR_lti: case LIR_ltui:
        CASE64(LIR_ltq:) CASE64(LIR_ltuq:)
        case LIR_ltd: case LIR_led:
            if (onfalse) BNLCTR(cr); else BLTCTR(cr);
            break;
        case LIR_lei: case LIR_leui:
        CASE64(LIR_leq:) CASE64(LIR_leuq:)
            if (onfalse) BGTCTR(cr); else BLECTR(cr);
            break;
        case LIR_gti: case LIR_gtui:
        CASE64(LIR_gtq:) CASE64(LIR_gtuq:)
        case LIR_gtd: case LIR_ged:
            if (onfalse) BNGCTR(cr); else BGTCTR(cr);
            break;
        case LIR_gei: case LIR_geui:
        CASE64(LIR_geq:) CASE64(LIR_geuq:)
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

    NIns* Assembler::asm_branch_ov(LOpcode, NIns*) {
        TODO(asm_branch_ov);
        return _nIns;
    }

    void Assembler::asm_cmp(LOpcode condop, LIns *a, LIns *b, ConditionRegister cr) {
        RegisterMask allow = isCmpDOpcode(condop) ? FpRegs : GpRegs;
        Register ra = findRegFor(a, allow);

    #if !PEDANTIC
        if (b->isImmI()) {
            int32_t d = b->immI();
            if (isS16(d)) {
                if (isCmpSIOpcode(condop)) {
                    CMPWI(cr, ra, d);
                    return;
                }
    #if defined NANOJIT_64BIT
                if (isCmpSQOpcode(condop)) {
                    CMPDI(cr, ra, d);
                    TODO(cmpdi);
                    return;
                }
    #endif
            }
            if (isU16(d)) {
                if (isCmpUIOpcode(condop)) {
                    CMPLWI(cr, ra, d);
                    return;
                }
    #if defined NANOJIT_64BIT
                if (isCmpUQOpcode(condop)) {
                    CMPLDI(cr, ra, d);
                    TODO(cmpldi);
                    return;
                }
    #endif
            }
        }
    #endif

        
        Register rb = b==a ? ra : findRegFor(b, allow & ~rmask(ra));
        if (isCmpSIOpcode(condop)) {
            CMPW(cr, ra, rb);
        }
        else if (isCmpUIOpcode(condop)) {
            CMPLW(cr, ra, rb);
        }
    #if defined NANOJIT_64BIT
        else if (isCmpSQOpcode(condop)) {
            CMPD(cr, ra, rb);
        }
        else if (isCmpUQOpcode(condop)) {
            CMPLD(cr, ra, rb);
        }
    #endif
        else if (isCmpDOpcode(condop)) {
            
            
            
            if (condop == LIR_led)
                CROR(cr, lt, lt, eq); 
            else if (condop == LIR_ged)
                CROR(cr, gt, gt, eq); 
            FCMPU(cr, ra, rb);
        }
        else {
            TODO(asm_cmp);
        }
    }

    void Assembler::asm_ret(LIns *ins) {
        genEpilogue();
        releaseRegisters();
        assignSavedRegs();
        LIns *value = ins->oprnd1();
        Register r = ins->isop(LIR_retd) ? F1 : R3;
        findSpecificRegFor(value, r);
    }

    void Assembler::asm_nongp_copy(Register r, Register s) {
        
        NanoAssert((rmask(r) & FpRegs) && (rmask(s) & FpRegs));
        FMR(r, s);
    }

    bool Assembler::canRemat(LIns* ins)
    {
        return ins->isImmI() || ins->isop(LIR_allocp);
    }

    void Assembler::asm_restore(LIns *i, Register r) {
        int d;
        if (i->isop(LIR_allocp)) {
            d = deprecated_disp(i);
            ADDI(r, FP, d);
        }
        else if (i->isImmI()) {
            asm_li(r, i->immI());
        }
        else {
            d = findMemFor(i);
            if (IsFpReg(r)) {
                NanoAssert(i->isQorD());
                LFD(r, d, FP);
            } else if (i->isQorD()) {
                NanoAssert(IsGpReg(r));
                LD(r, d, FP);
            } else {
                NanoAssert(i->isI());
                NanoAssert(IsGpReg(r));
                LWZ(r, d, FP);
            }
        }
    }

    void Assembler::asm_immi(LIns *ins) {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        asm_li(rr, ins->immI());
    }

    void Assembler::asm_fneg(LIns *ins) {
        Register rr = deprecated_prepResultReg(ins, FpRegs);
        Register ra = findRegFor(ins->oprnd1(), FpRegs);
        FNEG(rr,ra);
    }

    void Assembler::asm_param(LIns *ins) {
        uint32_t a = ins->paramArg();
        uint32_t kind = ins->paramKind();
        if (kind == 0) {
            
            
            if (a < 8) {
                
                deprecated_prepResultReg(ins, rmask(argRegs[a]));
            } else {
                
                
                TODO(asm_param_stk);
            }
        }
        else {
            
            deprecated_prepResultReg(ins, rmask(savedRegs[a]));
        }
    }

    void Assembler::asm_call(LIns *ins) {
        if (!ins->isop(LIR_callv)) {
            Register retReg = ( ins->isop(LIR_calld) ? F1 : retRegs[0] );
            deprecated_prepResultReg(ins, rmask(retReg));
        }

        
        
        evictScratchRegsExcept(0);

        const CallInfo* call = ins->callInfo();
        ArgType argTypes[MAXARGS];
        uint32_t argc = call->getArgTypes(argTypes);

        bool indirect;
        if (!(indirect = call->isIndirect())) {
            verbose_only(if (_logc->lcbits & LC_Native)
                outputf("        %p:", _nIns);
            )
            br((NIns*)call->_address, 1);
        } else {
            
            
            
            underrunProtect(8); 
            BCTRL();
            MTCTR(R11);
            asm_regarg(ARGTYPE_P, ins->arg(--argc), R11);
        }

        int param_size = 0;

        Register r = R3;
        Register fr = F1;
        for(uint32_t i = 0; i < argc; i++) {
            uint32_t j = argc - i - 1;
            ArgType ty = argTypes[j];
            LIns* arg = ins->arg(j);
            NanoAssert(ty != ARGTYPE_V);
            if (ty != ARGTYPE_D) {
                
                if (r <= R10) {
                    asm_regarg(ty, arg, r);
                    r = r + 1;
                    param_size += sizeof(void*);
                } else {
                    
                    TODO(stack_int32);
                }
            } else {
                
                if (fr <= F13) {
                    asm_regarg(ty, arg, fr);
                    fr = fr + 1;
                #ifdef NANOJIT_64BIT
                    r = r + 1;
                #else
                    r = r + 2; 
                #endif
                    param_size += sizeof(double);
                } else {
                    
                    TODO(stack_double);
                }
            }
        }
        if (param_size > max_param_size)
            max_param_size = param_size;
    }

    void Assembler::asm_regarg(ArgType ty, LIns* p, Register r)
    {
        NanoAssert(r != deprecated_UnknownReg);
        NanoAssert(ty != ARGTYPE_V);
        if (ty != ARGTYPE_D)
        {
        #ifdef NANOJIT_64BIT
            if (ty == ARGTYPE_I) {
                
                EXTSW(r, r);
            } else if (ty == ARGTYPE_UI) {
                
                CLRLDI(r, r, 32);
            }
        #endif
            
            if (p->isImmI()) {
                asm_li(r, p->immI());
            } else {
                if (p->isExtant()) {
                    if (!p->deprecated_hasKnownReg()) {
                        
                        int d = findMemFor(p);
                        if (p->isop(LIR_allocp)) {
                            NanoAssert(isS16(d));
                            ADDI(r, FP, d);
                        } else if (p->isQorD()) {
                            LD(r, d, FP);
                        } else {
                            LWZ(r, d, FP);
                        }
                    } else {
                        
                        MR(r, p->deprecated_getReg());
                    }
                }
                else {
                    
                    
                    findSpecificRegFor(p, r);
                }
            }
        }
        else {
            if (p->isExtant()) {
                Register rp = p->deprecated_getReg();
                if (!deprecated_isKnownReg(rp) || !IsFpReg(rp)) {
                    
                    int d = findMemFor(p);
                    LFD(r, d, FP);
                } else {
                    
                    NanoAssert(IsFpReg(r) && IsFpReg(rp));
                    FMR(r, rp);
                }
            }
            else {
                
                
                findSpecificRegFor(p, r);
            }
        }
    }

    void Assembler::asm_spill(Register rr, int d, bool quad) {
        (void)quad;
        NanoAssert(d);
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

    void Assembler::asm_arith(LIns *ins) {
        LOpcode op = ins->opcode();
        LIns* lhs = ins->oprnd1();
        LIns* rhs = ins->oprnd2();
        RegisterMask allow = GpRegs;
        Register rr = deprecated_prepResultReg(ins, allow);
        Register ra = findRegFor(lhs, GpRegs);

        if (rhs->isImmI()) {
            int32_t rhsc = rhs->immI();
            if (isS16(rhsc)) {
                
                switch (op) {
                case LIR_addi:
                CASE64(LIR_addq:)
                    ADDI(rr, ra, rhsc);
                    return;
                case LIR_subi:
                    SUBI(rr, ra, rhsc);
                    return;
                case LIR_muli:
                    MULLI(rr, ra, rhsc);
                    return;
                }
            }
            if (isU16(rhsc)) {
                
                switch (op) {
                CASE64(LIR_orq:)
                case LIR_ori:
                    ORI(rr, ra, rhsc);
                    return;
                CASE64(LIR_andq:)
                case LIR_andi:
                    ANDI(rr, ra, rhsc);
                    return;
                CASE64(LIR_xorq:)
                case LIR_xori:
                    XORI(rr, ra, rhsc);
                    return;
                }
            }

            
            switch (op) {
            case LIR_lshi:
                SLWI(rr, ra, rhsc&31);
                return;
            case LIR_rshui:
                SRWI(rr, ra, rhsc&31);
                return;
            case LIR_rshi:
                SRAWI(rr, ra, rhsc&31);
                return;
            }
        }

        
        Register rb = rhs==lhs ? ra : findRegFor(rhs, GpRegs&~rmask(ra));
        switch (op) {
            CASE64(LIR_addq:)
            case LIR_addi:
                ADD(rr, ra, rb);
                break;
            CASE64(LIR_andq:)
            case LIR_andi:
                AND(rr, ra, rb);
                break;
            CASE64(LIR_orq:)
            case LIR_ori:
                OR(rr, ra, rb);
                break;
            CASE64(LIR_xorq:)
            case LIR_xori:
                XOR(rr, ra, rb);
                break;
            case LIR_subi:  SUBF(rr, rb, ra);    break;
            case LIR_lshi:  SLW(rr, ra, R0);     ANDI(R0, rb, 31);   break;
            case LIR_rshi:  SRAW(rr, ra, R0);    ANDI(R0, rb, 31);   break;
            case LIR_rshui: SRW(rr, ra, R0);     ANDI(R0, rb, 31);   break;
            case LIR_muli:  MULLW(rr, ra, rb);   break;
        #ifdef NANOJIT_64BIT
            case LIR_lshq:
                SLD(rr, ra, R0);
                ANDI(R0, rb, 63);
                break;
            case LIR_rshuq:
                SRD(rr, ra, R0);
                ANDI(R0, rb, 63);
                break;
            case LIR_rshq:
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
        LIns* lhs = ins->oprnd1();
        LIns* rhs = ins->oprnd2();
        RegisterMask allow = FpRegs;
        Register rr = deprecated_prepResultReg(ins, allow);
        Register ra, rb;
        findRegFor2(allow, lhs, ra, allow, rhs, rb);
        switch (op) {
            case LIR_addd: FADD(rr, ra, rb); break;
            case LIR_subd: FSUB(rr, ra, rb); break;
            case LIR_muld: FMUL(rr, ra, rb); break;
            case LIR_divd: FDIV(rr, ra, rb); break;
            default:
                debug_only(outputf("%s",lirNames[op]);)
                TODO(asm_fop);
        }
    }

    void Assembler::asm_i2d(LIns *ins) {
        Register r = deprecated_prepResultReg(ins, FpRegs);
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

    void Assembler::asm_ui2d(LIns *ins) {
        Register r = deprecated_prepResultReg(ins, FpRegs);
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

    void Assembler::asm_d2i(LIns*) {
        NanoAssertMsg(0, "NJ_F2I_SUPPORTED not yet supported for this architecture");
    }

    #if defined NANOJIT_64BIT
    
    void Assembler::asm_q2i(LIns *ins) {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        int d = findMemFor(ins->oprnd1());
        LWZ(rr, d+4, FP);
    }

    void Assembler::asm_ui2uq(LIns *ins) {
        LOpcode op = ins->opcode();
        Register r = deprecated_prepResultReg(ins, GpRegs);
        Register v = findRegFor(ins->oprnd1(), GpRegs);
        switch (op) {
        default:
            debug_only(outputf("%s",lirNames[op]));
            TODO(asm_ui2uq);
        case LIR_ui2uq:
            CLRLDI(r, v, 32); 
            break;
        case LIR_i2q:
            EXTSW(r, v);
            break;
        }
    }

    void Assembler::asm_dasq(LIns*) {
        TODO(asm_dasq);
    }

    void Assembler::asm_qasd(LIns*) {
        TODO(asm_qasd);
    }

    #endif

#ifdef NANOJIT_64BIT
    void Assembler::asm_immq(LIns *ins) {
        Register r = ins->deprecated_getReg();
        if (deprecated_isKnownReg(r) && (rmask(r) & FpRegs)) {
            
            deprecated_freeRsrcOf(ins);
        } else {
            
            
            r = deprecated_prepResultReg(ins, GpRegs);
        }

        if (rmask(r) & FpRegs) {
            union {
                double d;
                struct {
                    int32_t hi, lo; 
                } w;
            };
            d = ins->immD();
            LFD(r, 8, SP);
            STW(R0, 12, SP);
            asm_li(R0, w.lo);
            STW(R0, 8, SP);
            asm_li(R0, w.hi);
        }
        else {
            int64_t q = ins->immQ();
            if (isS32(q)) {
                asm_li(r, int32_t(q));
                return;
            }
            RLDIMI(r,R0,32,0); 
            asm_li(R0, int32_t(q>>32)); 
            asm_li(r, int32_t(q)); 
        }
    }
#endif

    void Assembler::asm_immd(LIns *ins) {
    #ifdef NANOJIT_64BIT
        Register r = ins->deprecated_getReg();
        if (deprecated_isKnownReg(r) && (rmask(r) & FpRegs)) {
            
            deprecated_freeRsrcOf(ins);
        } else {
            
            
            r = deprecated_prepResultReg(ins, GpRegs);
        }
    #else
        Register r = deprecated_prepResultReg(ins, FpRegs);
    #endif

        if (rmask(r) & FpRegs) {
            union {
                double d;
                struct {
                    int32_t hi, lo; 
                } w;
            };
            d = ins->immD();
            LFD(r, 8, SP);
            STW(R0, 12, SP);
            asm_li(R0, w.lo);
            STW(R0, 8, SP);
            asm_li(R0, w.hi);
        }
        else {
            int64_t q = ins->immDasQ();
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
        NIns *pc = _nIns;
        NIns *top = codeStart;  

    #if PEDANTIC
        
        
        
        

        NanoAssert(pedanticTop >= top);
        if (pc - instr < pedanticTop) {
            
        #ifdef NANOJIT_64BIT
            const int br_size = 7;
        #else
            const int br_size = 4;
        #endif
            if (pc - instr - br_size < top) {
                
                verbose_only(if (_logc->lcbits & LC_Native) outputf("newpage %p:", pc);)
                codeAlloc();
            }
            
            
            pedanticTop = _nIns - br_size;
            br(pc, 0);
            pedanticTop = _nIns - instr;
        }
    #else
        if (pc - instr < top) {
            verbose_only(if (_logc->lcbits & LC_Native) outputf("newpage %p:", pc);)
            
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
            
            
            br(pc, 0);
        }
    #endif
    }

    void Assembler::asm_cmov(LIns* ins)
    {
        LIns* condval = ins->oprnd1();
        LIns* iftrue  = ins->oprnd2();
        LIns* iffalse = ins->oprnd3();

    #ifdef NANOJIT_64BIT
        NanoAssert((ins->opcode() == LIR_cmovi  && iftrue->isI() && iffalse->isI()) ||
                   (ins->opcode() == LIR_cmovq  && iftrue->isQ() && iffalse->isQ()));
    #else
        NanoAssert((ins->opcode() == LIR_cmovi  && iftrue->isI() && iffalse->isI()));
    #endif

        Register rr = prepareResultReg(ins, GpRegs);
        Register rf = findRegFor(iffalse, GpRegs & ~rmask(rr));

        
        Register rt = iftrue->isInReg() ? iftrue->getReg() : rr;

        underrunProtect(16); 
        NIns *after = _nIns;
        verbose_only(if (_logc->lcbits & LC_Native) outputf("%p:",after);)
        MR(rr,rf);

        NanoAssert(isS24(after - (_nIns-1)));
        asm_branch_near(false, condval, after);

        if (rr != rt)
            MR(rr, rt);

        freeResourcesOf(ins);
        if (!iftrue->isInReg()) {
            NanoAssert(rt == rr);
            findSpecificRegForUnallocated(iftrue, rr);
        }

        asm_cmp(condval->opcode(), condval->oprnd1(), condval->oprnd2(), CR7);
    }

    RegisterMask Assembler::nHint(LIns* ins) {
        NanoAssert(ins->isop(LIR_paramp));
        RegisterMask prefer = 0;
        if (ins->paramKind() == 0)
            if (ins->paramArg() < 8)
                prefer = rmask(argRegs[ins->paramArg()]);
        return prefer;
    }

    void Assembler::asm_neg_not(LIns *ins) {
        Register rr = deprecated_prepResultReg(ins, GpRegs);
        Register ra = findRegFor(ins->oprnd1(), GpRegs);
        if (ins->isop(LIR_negi)) {
            NEG(rr, ra);
        } else {
            NOT(rr, ra);
        }
    }

    void Assembler::nInit() {
        nHints[LIR_calli]  = rmask(R3);
    #ifdef NANOJIT_64BIT
        nHints[LIR_callq]  = rmask(R3);
    #endif
        nHints[LIR_calld]  = rmask(F1);
        nHints[LIR_paramp] = PREFER_SPECIAL;
    }

    void Assembler::nBeginAssembly() {
        max_param_size = 0;
    }

    void Assembler::nativePageSetup() {
        NanoAssert(!_inExit);
        if (!_nIns) {
            codeAlloc(codeStart, codeEnd, _nIns verbose_only(, codeBytes));
            IF_PEDANTIC( pedanticTop = _nIns; )
        }
    }

    void Assembler::nativePageReset()
    {}

    
    
    verbose_only(
    void Assembler::asm_inc_m32(uint32_t* )
    {
    }
    )

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
            
            Register rd = { (branch[0] >> 21) & 31 };
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
            
            
            Register rd = { (branch[0] >> 21) & 31 };
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
        
        
        register uint32_t i;
        #ifdef __GNUC__
        asm ("cntlzw %0,%1" : "=r" (i) : "r" (set));
        #else 
        # error("unsupported compiler")
        #endif 
        return 31-i;
    }

    Register Assembler::nRegisterAllocFromSet(RegisterMask set) {
        uint32_t i;
        
        if (set & 0xffffffff) {
            i = cntzlw(int(set)); 
        } else {
            i = 32 + cntzlw(int(set>>32)); 
        }
        Register r = { i };
        _allocator.free &= ~rmask(r);
        return r;
    }

    void Assembler::nRegisterResetAll(RegAlloc &regs) {
        regs.clear();
        regs.free = SavedRegs | 0x1ff8  | 0x3ffe00000000LL ;
    }

#ifdef NANOJIT_64BIT
    void Assembler::asm_qbinop(LIns *ins) {
        LOpcode op = ins->opcode();
        switch (op) {
        case LIR_orq:
        case LIR_andq:
        case LIR_rshuq:
        case LIR_rshq:
        case LIR_lshq:
        case LIR_xorq:
        case LIR_addq:
            asm_arith(ins);
            break;
        default:
            debug_only(outputf("%s",lirNames[op]));
            TODO(asm_qbinop);
        }
    }
#endif 

    void Assembler::nFragExit(LIns*) {
        TODO(nFragExit);
    }

    void Assembler::asm_jtbl(LIns* ins, NIns** native_table)
    {
        
        
        Register indexreg = findRegFor(ins->oprnd1(), GpRegs);
#ifdef NANOJIT_64BIT
        underrunProtect(9*4);
        BCTR(0);                                
        MTCTR(R2);                              
        LDX(R2, R2, R0);                        
        SLDI(R0, indexreg, 3);                  
        asm_li64(R2, uint64_t(native_table));   
#else 
        underrunProtect(6*4);
        BCTR(0);                                
        MTCTR(R2);                              
        LWZX(R2, R2, R0);                       
        SLWI(R0, indexreg, 2);                  
        asm_li(R2, int32_t(native_table));      
#endif 
    }

    void Assembler::swapCodeChunks() {
        if (!_nExitIns) {
            codeAlloc(exitStart, exitEnd, _nExitIns verbose_only(, exitBytes));
        }
        SWAP(NIns*, _nIns, _nExitIns);
        SWAP(NIns*, codeStart, exitStart);
        SWAP(NIns*, codeEnd, exitEnd);
        verbose_only( SWAP(size_t, codeBytes, exitBytes); )
    }

    void Assembler::asm_insert_random_nop() {
        NanoAssert(0); 
    }

} 

#endif 
