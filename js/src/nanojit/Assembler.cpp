






































#include "nanojit.h"

#ifdef FEATURE_NANOJIT

#ifdef VTUNE
#include "../core/CodegenLIR.h"
#endif

#ifdef _MSC_VER
    
    #pragma warning(disable:4310) // cast truncates constant value
#endif

namespace nanojit
{
    




    Assembler::Assembler(CodeAlloc& codeAlloc, Allocator& alloc, AvmCore* core, LogControl* logc)
        : codeList(NULL)
        , alloc(alloc)
        , _codeAlloc(codeAlloc)
        , _thisfrag(NULL)
        , _branchStateMap(alloc)
        , _patches(alloc)
        , _labels(alloc)
        , _epilogue(NULL)
        , _err(None)
    #if PEDANTIC
        , pedanticTop(NULL)
    #endif
    #ifdef VTUNE
        , cgen(NULL)
    #endif
        , config(core->config)
    {
        VMPI_memset(&_stats, 0, sizeof(_stats));
        nInit(core);
        (void)logc;
        verbose_only( _logc = logc; )
        verbose_only( _outputCache = 0; )
        verbose_only( outlineEOL[0] = '\0'; )
        verbose_only( outputAddr = false; )

        reset();
    }

    void Assembler::arReset()
    {
        _activation.lowwatermark = 0;
        _activation.tos = 0;

        for(uint32_t i=0; i<NJ_MAX_STACK_ENTRY; i++)
            _activation.entry[i] = 0;

        _branchStateMap.clear();
        _patches.clear();
        _labels.clear();
    }

    void Assembler::registerResetAll()
    {
        nRegisterResetAll(_allocator);

        
        NanoAssert(0 != _allocator.free);
        NanoAssert(0 == _allocator.countActive());
#ifdef NANOJIT_IA32
        debug_only(_fpuStkDepth = 0; )
#endif
    }

    Register Assembler::registerAlloc(RegisterMask allow)
    {
        RegAlloc &regs = _allocator;
        RegisterMask allowedAndFree = allow & regs.free;

        if (allowedAndFree)
        {
            
            
            RegisterMask preferredAndFree = allowedAndFree & SavedRegs;
            RegisterMask set = ( preferredAndFree ? preferredAndFree : allowedAndFree );
            Register r = nRegisterAllocFromSet(set);
            return r;
        }
        counter_increment(steals);

        
        
        LIns* vic = findVictim(allow);
        NanoAssert(vic);

        
        Register r = vic->getReg();
        regs.removeActive(r);
        vic->setReg(UnknownReg);

        asm_restore(vic, vic->resv(), r);
        return r;
    }

    



    bool Assembler::canRemat(LIns *i) {
        return i->isconst() || i->isconstq() || i->isop(LIR_alloc);
    }

    void Assembler::codeAlloc(NIns *&start, NIns *&end, NIns *&eip
                              verbose_only(, size_t &nBytes))
    {
        
        if (start)
            CodeAlloc::add(codeList, start, end);

        
        _codeAlloc.alloc(start, end);
        verbose_only( nBytes += (end - start) * sizeof(NIns); )
        NanoAssert(uintptr_t(end) - uintptr_t(start) >= (size_t)LARGEST_UNDERRUN_PROT);
        eip = end;

        #ifdef VTUNE
        if (_nIns && _nExitIns) {
            
            cgen->jitCodePosUpdate((uintptr_t)list->code);
            cgen->jitPushInfo(); 
        }
        #endif
    }

    void Assembler::reset()
    {
        _nIns = 0;
        _nExitIns = 0;
        codeStart = codeEnd = 0;
        exitStart = exitEnd = 0;
        _stats.pages = 0;
        codeList = 0;

        nativePageReset();
        registerResetAll();
        arReset();
    }

    #ifdef _DEBUG
    void Assembler::pageValidate()
    {
        if (error()) return;
        
        NanoAssertMsg(_inExit ? containsPtr(exitStart, exitEnd, _nIns) : containsPtr(codeStart, codeEnd, _nIns),
                     "Native instruction pointer overstep paging bounds; check overrideProtect for last instruction");
    }
    #endif

    #ifdef _DEBUG

    void Assembler::resourceConsistencyCheck()
    {
        NanoAssert(!error());

#ifdef NANOJIT_IA32
        NanoAssert((_allocator.active[FST0] && _fpuStkDepth == -1) ||
            (!_allocator.active[FST0] && _fpuStkDepth == 0));
#endif

        AR &ar = _activation;
        
        NanoAssert(ar.tos < NJ_MAX_STACK_ENTRY);
        LIns* ins = 0;
        RegAlloc* regs = &_allocator;
        for(uint32_t i = ar.lowwatermark; i < ar.tos; i++)
        {
            ins = ar.entry[i];
            if ( !ins )
                continue;
            Register r = ins->getReg();
            uint32_t arIndex = ins->getArIndex();
            if (arIndex != 0) {
                if (ins->isop(LIR_alloc)) {
                    int j=i+1;
                    for (int n = i + (ins->size()>>2); j < n; j++) {
                        NanoAssert(ar.entry[j]==ins);
                    }
                    NanoAssert(arIndex == (uint32_t)j-1);
                    i = j-1;
                }
                else if (ins->isQuad()) {
                    NanoAssert(ar.entry[i - stack_direction(1)]==ins);
                    i += 1; 
                }
                else {
                    NanoAssertMsg(arIndex == i, "Stack record index mismatch");
                }
            }
            NanoAssertMsg( !isKnownReg(r) || regs->isConsistent(r,ins), "Register record mismatch");
        }

        registerConsistencyCheck();
    }

    void Assembler::registerConsistencyCheck()
    {
        RegisterMask managed = _allocator.managed;
        for (Register r = FirstReg; r <= LastReg; r = nextreg(r)) {
            if (rmask(r) & managed) {
                
                
                if (_allocator.isFree(r)) {
                    NanoAssertMsgf(_allocator.getActive(r)==0,
                        "register %s is free but assigned to ins", gpn(r));
                } else {
                    
                    
                    LIns* ins = _allocator.getActive(r);
                    NanoAssert(ins);
                    NanoAssertMsg(r == ins->getReg(), "Register record mismatch");
                }
            } else {
                
                
                NanoAssert(!_allocator.isFree(r));
                NanoAssert(!_allocator.getActive(r));
            }
        }
    }
    #endif 

    void Assembler::findRegFor2(RegisterMask allow, LIns* ia, Reservation* &resva, LIns* ib, Reservation* &resvb)
    {
        if (ia == ib)
        {
            findRegFor(ia, allow);
            resva = resvb = ia->resvUsed();
        }
        else
        {
            resvb = ib->resv();
            bool rbDone = (resvb->used && isKnownReg(resvb->reg) && (allow & rmask(resvb->reg)));
            if (rbDone) {
                
                allow &= ~rmask(resvb->reg);
            }
            Register ra = findRegFor(ia, allow);
            resva = ia->resv();
            NanoAssert(error() || (resva->used && isKnownReg(ra)));
            if (!rbDone) {
                allow &= ~rmask(ra);
                findRegFor(ib, allow);
                resvb = ib->resvUsed();
            }
        }
    }

    
    void Assembler::findRegFor2b(RegisterMask allow, LIns* ia, Register &ra, LIns* ib, Register &rb)
    {
        Reservation *resva, *resvb;
        findRegFor2(allow, ia, resva, ib, resvb);
        ra = resva->reg;
        rb = resvb->reg;
    }

    Register Assembler::findSpecificRegFor(LIns* i, Register w)
    {
        return findRegFor(i, rmask(w));
    }

    Register Assembler::getBaseReg(LIns *i, int &d, RegisterMask allow)
    {
    #if !PEDANTIC
        if (i->isop(LIR_alloc)) {
            d += findMemFor(i);
            return FP;
        }
    #else
        (void) d;
    #endif
        return findRegFor(i, allow);
    }

    
    
    
    
    
    
    
    
    
    Register Assembler::findRegFor(LIns* ins, RegisterMask allow)
    {
        if (ins->isop(LIR_alloc)) {
            
            findMemFor(ins);
        }

        Register r;

        if (!ins->isUsed()) {
            
            ins->markAsUsed();
            RegisterMask prefer = hint(ins, allow);
            r = registerAlloc(prefer);
            ins->setReg(r);
            _allocator.addActive(r, ins);

        } else if (!ins->hasKnownReg()) {
            
            
            RegisterMask prefer = hint(ins, allow);
            r = registerAlloc(prefer);
            ins->setReg(r);
            _allocator.addActive(r, ins);

        } else if (rmask(r = ins->getReg()) & allow) {
            
            
            _allocator.useActive(r);

        } else {
            
            
            RegisterMask prefer = hint(ins, allow);
#ifdef NANOJIT_IA32
            if (((rmask(r)&XmmRegs) && !(allow&XmmRegs)) ||
                ((rmask(r)&x87Regs) && !(allow&x87Regs)))
            {
                
                
                evict(r, ins);
                r = registerAlloc(prefer);
                ins->setReg(r);
                _allocator.addActive(r, ins);
            } else
#elif defined(NANOJIT_PPC)
            if (((rmask(r)&GpRegs) && !(allow&GpRegs)) ||
                ((rmask(r)&FpRegs) && !(allow&FpRegs)))
            {
                evict(r, ins);
                r = registerAlloc(prefer);
                ins->setReg(r);
                _allocator.addActive(r, ins);
            } else
#endif
            {
                
                
                
                
                
                
                
                
                _allocator.retire(r);
                Register s = r;
                r = registerAlloc(prefer);
                ins->setReg(r);
                _allocator.addActive(r, ins);
                if ((rmask(s) & GpRegs) && (rmask(r) & GpRegs)) {
#ifdef NANOJIT_ARM
                    MOV(s, r);  
#else
                    MR(s, r);
#endif
                }
                else {
                    asm_nongp_copy(s, r);
                }
            }
        }
        return r;
    }

    
    
    
    
    
    Register Assembler::findSpecificRegForUnallocated(LIns* ins, Register r)
    {
        if (ins->isop(LIR_alloc)) {
            
            findMemFor(ins);
        }

        NanoAssert(ins->isUnusedOrHasUnknownReg());
        NanoAssert(_allocator.free & rmask(r));

        if (!ins->isUsed())
            ins->markAsUsed();
        ins->setReg(r);
        _allocator.removeFree(r);
        _allocator.addActive(r, ins);

        return r;
    }
 
    int Assembler::findMemFor(LIns *ins)
    {
        if (!ins->isUsed())
            ins->markAsUsed();
        if (!ins->getArIndex()) {
            ins->setArIndex(arReserve(ins));
            NanoAssert(ins->getArIndex() <= _activation.tos);
        }
        return disp(ins);
    }

    Register Assembler::prepResultReg(LIns *ins, RegisterMask allow)
    {
        const bool pop = ins->isUnusedOrHasUnknownReg();
        Register r = findRegFor(ins, allow);
        freeRsrcOf(ins, pop);
        return r;
    }

    void Assembler::asm_spilli(LInsp ins, bool pop)
    {
        int d = disp(ins);
        Register r = ins->getReg();
        verbose_only( if (d && (_logc->lcbits & LC_RegAlloc)) {
                         outputForEOL("  <= spill %s",
                                      _thisfrag->lirbuf->names->formatRef(ins)); } )
        asm_spill(r, d, pop, ins->isQuad());
    }

    
    
    
    
    void Assembler::freeRsrcOf(LIns *ins, bool pop)
    {
        Register r = ins->getReg();
        if (isKnownReg(r)) {
            asm_spilli(ins, pop);
            _allocator.retire(r);   
        }
        int arIndex = ins->getArIndex();
        if (arIndex) {
            NanoAssert(_activation.entry[arIndex] == ins);
            arFree(arIndex);        
        }
        ins->markAsClear();
    }

    
    void Assembler::evictIfActive(Register r)
    {
        if (LIns* vic = _allocator.getActive(r)) {
            evict(r, vic);
        }
    }

    
    
    
    
    
    
    
    
    
    
    
    void Assembler::evict(Register r, LIns* vic)
    {
        
        counter_increment(steals);

        
        NanoAssert(!_allocator.isFree(r));
        NanoAssert(vic == _allocator.getActive(r));
        NanoAssert(r == vic->getReg());

        
        _allocator.retire(r);
        vic->setReg(UnknownReg);

        
        asm_restore(vic, vic->resv(), r);
    }

    void Assembler::patch(GuardRecord *lr)
    {
        if (!lr->jmp) 
            return;
        Fragment *frag = lr->exit->target;
        NanoAssert(frag->fragEntry != 0);
        nPatchBranch((NIns*)lr->jmp, frag->fragEntry);
        CodeAlloc::flushICache(lr->jmp, LARGEST_BRANCH_PATCH);
        verbose_only(verbose_outputf("patching jump at %p to target %p\n",
            lr->jmp, frag->fragEntry);)
    }

    void Assembler::patch(SideExit *exit)
    {
        GuardRecord *rec = exit->guards;
        NanoAssert(rec);
        while (rec) {
            patch(rec);
            rec = rec->next;
        }
    }

#ifdef NANOJIT_IA32
    void Assembler::patch(SideExit* exit, SwitchInfo* si)
    {
        for (GuardRecord* lr = exit->guards; lr; lr = lr->next) {
            Fragment *frag = lr->exit->target;
            NanoAssert(frag->fragEntry != 0);
            si->table[si->index] = frag->fragEntry;
        }
    }
#endif

    NIns* Assembler::asm_exit(LInsp guard)
    {
        SideExit *exit = guard->record()->exit;
        NIns* at = 0;
        if (!_branchStateMap.get(exit))
        {
            at = asm_leave_trace(guard);
        }
        else
        {
            RegAlloc* captured = _branchStateMap.get(exit);
            intersectRegisterState(*captured);
            at = exit->target->fragEntry;
            NanoAssert(at != 0);
            _branchStateMap.remove(exit);
        }
        return at;
    }

    NIns* Assembler::asm_leave_trace(LInsp guard)
    {
        verbose_only( int32_t nativeSave = _stats.native );
        verbose_only( verbose_outputf("----------------------------------- ## END exit block %p", guard);)

        RegAlloc capture = _allocator;

        
        
        
        releaseRegisters();

        swapptrs();
        _inExit = true;

#ifdef NANOJIT_IA32
        debug_only( _sv_fpuStkDepth = _fpuStkDepth; _fpuStkDepth = 0; )
#endif

        nFragExit(guard);

        
        assignSavedRegs();
        assignParamRegs();

        intersectRegisterState(capture);

        
        
        

        
        NIns* jmpTarget = _nIns;     

        
        swapptrs();
        _inExit = false;

        
        verbose_only( verbose_outputf("%010lx:", (unsigned long)jmpTarget);)
        verbose_only( verbose_outputf("----------------------------------- ## BEGIN exit block (LIR_xt|LIR_xf)") );

#ifdef NANOJIT_IA32
        NanoAssertMsgf(_fpuStkDepth == _sv_fpuStkDepth, "LIR_xtf, _fpuStkDepth=%d, expect %d",_fpuStkDepth, _sv_fpuStkDepth);
        debug_only( _fpuStkDepth = _sv_fpuStkDepth; _sv_fpuStkDepth = 9999; )
#endif

        verbose_only(_stats.exitnative += (_stats.native-nativeSave));

        return jmpTarget;
    }

    void Assembler::beginAssembly(Fragment *frag)
    {
        verbose_only( codeBytes = 0; )
        verbose_only( exitBytes = 0; )

        reset();

        NanoAssert(codeList == 0);
        NanoAssert(codeStart == 0);
        NanoAssert(codeEnd == 0);
        NanoAssert(exitStart == 0);
        NanoAssert(exitEnd == 0);
        NanoAssert(_nIns == 0);
        NanoAssert(_nExitIns == 0);

        _thisfrag = frag;
        _activation.lowwatermark = 1;
        _activation.tos = _activation.lowwatermark;
        _inExit = false;

        counter_reset(native);
        counter_reset(exitnative);
        counter_reset(steals);
        counter_reset(spills);
        counter_reset(remats);

        setError(None);

        
        nativePageSetup();

        
        if (error()) return;

#ifdef PERFM
        _stats.pages = 0;
        _stats.codeStart = _nIns-1;
        _stats.codeExitStart = _nExitIns-1;
#endif 

        _epilogue = NULL;

        nBeginAssembly();
    }

    void Assembler::assemble(Fragment* frag, LirFilter* reader)
    {
        if (error()) return;
        _thisfrag = frag;

        
        verbose_only( NanoAssert(frag->nStaticExits == 0); )
        verbose_only( NanoAssert(frag->nCodeBytes == 0); )
        verbose_only( NanoAssert(frag->nExitBytes == 0); )
        verbose_only( NanoAssert(frag->profCount == 0); )
        verbose_only( if (_logc->lcbits & LC_FragProfile)
                          NanoAssert(frag->profFragID > 0);
                      else
                          NanoAssert(frag->profFragID == 0); )

        _inExit = false;

        gen(reader);

        if (!error()) {
            
            NInsMap::Iter iter(_patches);
            while (iter.next()) {
                NIns* where = iter.key();
                LIns* targ = iter.value();
                LabelState *label = _labels.get(targ);
                NIns* ntarg = label->addr;
                if (ntarg) {
                    nPatchBranch(where,ntarg);
                }
                else {
                    setError(UnknownBranch);
                    break;
                }
            }
        }
    }

    void Assembler::endAssembly(Fragment* frag)
    {
        
        
        if (error()) {
            
            _codeAlloc.freeAll(codeList);
            _codeAlloc.free(exitStart, exitEnd);
            _codeAlloc.free(codeStart, codeEnd);
            return;
        }

        NIns* fragEntry = genPrologue();
        verbose_only( outputAddr=true; )
        verbose_only( asm_output("[prologue]"); )

        
        debug_only(
            for (uint32_t i = _activation.lowwatermark; i < _activation.tos; i++) {
                NanoAssertMsgf(_activation.entry[i] == 0, "frame entry %d wasn't freed\n",-4*i);
            }
        )

        
#ifdef NANOJIT_ARM
        
        _codeAlloc.addRemainder(codeList, exitStart, exitEnd, _nExitSlot, _nExitIns);
        _codeAlloc.addRemainder(codeList, codeStart, codeEnd, _nSlot, _nIns);
        verbose_only( exitBytes -= (_nExitIns - _nExitSlot) * sizeof(NIns); )
        verbose_only( codeBytes -= (_nIns - _nSlot) * sizeof(NIns); )
#else
        
        _codeAlloc.addRemainder(codeList, exitStart, exitEnd, exitStart, _nExitIns);
        _codeAlloc.addRemainder(codeList, codeStart, codeEnd, codeStart, _nIns);
        verbose_only( exitBytes -= (_nExitIns - exitStart) * sizeof(NIns); )
        verbose_only( codeBytes -= (_nIns - codeStart) * sizeof(NIns); )
#endif

        
        
        CodeAlloc::flushICache(codeList);

        
        frag->fragEntry = fragEntry;
        frag->setCode(_nIns);
        PERFM_NVPROF("code", CodeAlloc::size(codeList));

#ifdef NANOJIT_IA32
        NanoAssertMsgf(_fpuStkDepth == 0,"_fpuStkDepth %d\n",_fpuStkDepth);
#endif

        debug_only( pageValidate(); )
        NanoAssert(_branchStateMap.isEmpty());
    }

    void Assembler::releaseRegisters()
    {
        for (Register r = FirstReg; r <= LastReg; r = nextreg(r))
        {
            LIns *ins = _allocator.getActive(r);
            if (ins) {
                
                _allocator.retire(r);
                NanoAssert(r == ins->getReg());
                ins->setReg(UnknownReg);

                if (!ins->getArIndex()) {
                    ins->markAsClear();
                }
            }
        }
    }

#ifdef PERFM
#define countlir_live() _nvprof("lir-live",1)
#define countlir_ret() _nvprof("lir-ret",1)
#define countlir_alloc() _nvprof("lir-alloc",1)
#define countlir_var() _nvprof("lir-var",1)
#define countlir_use() _nvprof("lir-use",1)
#define countlir_def() _nvprof("lir-def",1)
#define countlir_imm() _nvprof("lir-imm",1)
#define countlir_param() _nvprof("lir-param",1)
#define countlir_cmov() _nvprof("lir-cmov",1)
#define countlir_ld() _nvprof("lir-ld",1)
#define countlir_ldq() _nvprof("lir-ldq",1)
#define countlir_alu() _nvprof("lir-alu",1)
#define countlir_qjoin() _nvprof("lir-qjoin",1)
#define countlir_qlo() _nvprof("lir-qlo",1)
#define countlir_qhi() _nvprof("lir-qhi",1)
#define countlir_fpu() _nvprof("lir-fpu",1)
#define countlir_st() _nvprof("lir-st",1)
#define countlir_stq() _nvprof("lir-stq",1)
#define countlir_jmp() _nvprof("lir-jmp",1)
#define countlir_jcc() _nvprof("lir-jcc",1)
#define countlir_label() _nvprof("lir-label",1)
#define countlir_xcc() _nvprof("lir-xcc",1)
#define countlir_x() _nvprof("lir-x",1)
#define countlir_call() _nvprof("lir-call",1)
#else
#define countlir_live()
#define countlir_ret()
#define countlir_alloc()
#define countlir_var()
#define countlir_use()
#define countlir_def()
#define countlir_imm()
#define countlir_param()
#define countlir_cmov()
#define countlir_ld()
#define countlir_ldq()
#define countlir_alu()
#define countlir_qjoin()
#define countlir_qlo()
#define countlir_qhi()
#define countlir_fpu()
#define countlir_st()
#define countlir_stq()
#define countlir_jmp()
#define countlir_jcc()
#define countlir_label()
#define countlir_xcc()
#define countlir_x()
#define countlir_call()
#endif

    void Assembler::gen(LirFilter* reader)
    {
        NanoAssert(_thisfrag->nStaticExits == 0);

        
        NanoAssert(reader->pos()->isop(LIR_x) ||
                   reader->pos()->isop(LIR_ret) ||
                   reader->pos()->isop(LIR_fret) ||
                   reader->pos()->isop(LIR_xtbl) ||
                   reader->pos()->isop(LIR_flive) ||
                   reader->pos()->isop(LIR_live));

        InsList pending_lives(alloc);

        for (LInsp ins = reader->read(); !ins->isop(LIR_start) && !error();
                                         ins = reader->read())
        {
            







































            bool required = ins->isStmt() || ins->isUsed();
            if (!required)
                continue;

            LOpcode op = ins->opcode();
            switch(op)
            {
                default:
                    NanoAssertMsgf(false, "unsupported LIR instruction: %d (~0x40: %d)\n", op, op&~LIR64);
                    break;

                case LIR_regfence:
                    evictAllActiveRegs();
                    break;

                case LIR_flive:
                case LIR_live: {
                    countlir_live();
                    LInsp op1 = ins->oprnd1();
                    
                    
                    
                    
                    
                    
                    
                    if (op1->isop(LIR_alloc)) {
                        findMemFor(op1);
                    } else {
                        pending_lives.add(ins);
                    }
                    break;
                }

                case LIR_fret:
                case LIR_ret:  {
                    countlir_ret();
                    asm_ret(ins);
                    break;
                }

                
                
                case LIR_alloc: {
                    countlir_alloc();
                    NanoAssert(ins->getArIndex() != 0);
                    Register r = ins->getReg();
                    if (isKnownReg(r)) {
                        _allocator.retire(r);
                        ins->setReg(UnknownReg);
                        asm_restore(ins, ins->resv(), r);
                    }
                    freeRsrcOf(ins, 0);
                    break;
                }
                case LIR_int:
                {
                    countlir_imm();
                    asm_int(ins);
                    break;
                }
                case LIR_float:
                case LIR_quad:
                {
                    countlir_imm();
                    asm_quad(ins);
                    break;
                }
#if !defined NANOJIT_64BIT
                case LIR_callh:
                {
                    
                    prepResultReg(ins, rmask(retRegs[1]));
                    
                    findSpecificRegFor(ins->oprnd1(), retRegs[0]);
                    break;
                }
#endif
                case LIR_param:
                {
                    countlir_param();
                    asm_param(ins);
                    break;
                }
                case LIR_qlo:
                {
                    countlir_qlo();
                    asm_qlo(ins);
                    break;
                }
                case LIR_qhi:
                {
                    countlir_qhi();
                    asm_qhi(ins);
                    break;
                }
                case LIR_qcmov:
                case LIR_cmov:
                {
                    countlir_cmov();
                    asm_cmov(ins);
                    break;
                }
                case LIR_ld:
                case LIR_ldc:
                case LIR_ldcb:
                case LIR_ldcs:
                {
                    countlir_ld();
                    asm_ld(ins);
                    break;
                }
                case LIR_ldq:
                case LIR_ldqc:
                {
                    countlir_ldq();
                    asm_load64(ins);
                    break;
                }
                case LIR_neg:
                case LIR_not:
                {
                    countlir_alu();
                    asm_neg_not(ins);
                    break;
                }
                case LIR_qjoin:
                {
                    countlir_qjoin();
                    asm_qjoin(ins);
                    break;
                }

#if defined NANOJIT_64BIT
                case LIR_qiadd:
                case LIR_qiand:
                case LIR_qilsh:
                case LIR_qursh:
                case LIR_qirsh:
                case LIR_qior:
                case LIR_qaddp:
                case LIR_qxor:
                {
                    asm_qbinop(ins);
                    break;
                }
#endif

                case LIR_add:
                case LIR_iaddp:
                case LIR_sub:
                case LIR_mul:
                case LIR_and:
                case LIR_or:
                case LIR_xor:
                case LIR_lsh:
                case LIR_rsh:
                case LIR_ush:
                case LIR_div:
                case LIR_mod:
                {
                    countlir_alu();
                    asm_arith(ins);
                    break;
                }
                case LIR_fneg:
                {
                    countlir_fpu();
                    asm_fneg(ins);
                    break;
                }
                case LIR_fadd:
                case LIR_fsub:
                case LIR_fmul:
                case LIR_fdiv:
                {
                    countlir_fpu();
                    asm_fop(ins);
                    break;
                }
                case LIR_i2f:
                {
                    countlir_fpu();
                    asm_i2f(ins);
                    break;
                }
                case LIR_u2f:
                {
                    countlir_fpu();
                    asm_u2f(ins);
                    break;
                }
                case LIR_i2q:
                case LIR_u2q:
                {
                    countlir_alu();
                    asm_promote(ins);
                    break;
                }
                case LIR_sti:
                {
                    countlir_st();
                    asm_store32(ins->oprnd1(), ins->disp(), ins->oprnd2());
                    break;
                }
                case LIR_stqi:
                {
                    countlir_stq();
                    LIns* value = ins->oprnd1();
                    LIns* base = ins->oprnd2();
                    int dr = ins->disp();
                    if (value->isop(LIR_qjoin))
                    {
                        
                        asm_store32(value->oprnd1(), dr, base);
                        asm_store32(value->oprnd2(), dr+4, base);
                    }
                    else
                    {
                        asm_store64(value, dr, base);
                    }
                    break;
                }

                case LIR_j:
                {
                    countlir_jmp();
                    LInsp to = ins->getTarget();
                    LabelState *label = _labels.get(to);
                    
                    
                    
                    
                    releaseRegisters();
                    if (label && label->addr) {
                        
                        unionRegisterState(label->regs);
                        JMP(label->addr);
                    }
                    else {
                        
                        handleLoopCarriedExprs(pending_lives);
                        if (!label) {
                            
                            _labels.add(to, 0, _allocator);
                        }
                        else {
                            intersectRegisterState(label->regs);
                        }
                        JMP(0);
                        _patches.put(_nIns, to);
                    }
                    break;
                }

                case LIR_jt:
                case LIR_jf:
                {
                    countlir_jcc();
                    LInsp to = ins->getTarget();
                    LIns* cond = ins->oprnd1();
                    LabelState *label = _labels.get(to);
                    if (label && label->addr) {
                        
                        unionRegisterState(label->regs);
                        asm_branch(op == LIR_jf, cond, label->addr);
                    }
                    else {
                        
                        handleLoopCarriedExprs(pending_lives);
                        if (!label) {
                            
                            evictAllActiveRegs();
                            _labels.add(to, 0, _allocator);
                        }
                        else {
                            
                            intersectRegisterState(label->regs);
                        }
                        NIns *branch = asm_branch(op == LIR_jf, cond, 0);
                        _patches.put(branch,to);
                    }
                    break;
                }
                case LIR_label:
                {
                    countlir_label();
                    LabelState *label = _labels.get(ins);
                    
                    verbose_only( if (_logc->lcbits & LC_FragProfile) {
                        if (ins == _thisfrag->loopLabel)
                            asm_inc_m32(& _thisfrag->profCount);
                    })
                    if (!label) {
                        
                        _labels.add(ins, _nIns, _allocator);
                    }
                    else {
                        
                        NanoAssert(label->addr == 0);
                        
                        intersectRegisterState(label->regs);
                        label->addr = _nIns;
                    }
                    verbose_only( if (_logc->lcbits & LC_Assembly) { 
                        outputAddr=true; asm_output("[%s]", 
                        _thisfrag->lirbuf->names->formatRef(ins)); 
                    })
                    break;
                }
                case LIR_xbarrier: {
                    break;
                }
#ifdef NANOJIT_IA32
                case LIR_xtbl: {
                    NIns* exit = asm_exit(ins); 
                    asm_switch(ins, exit);
                    break;
                }
#else
                 case LIR_xtbl:
                    NanoAssertMsg(0, "Not supported for this architecture");
                    break;
#endif
                case LIR_xt:
                case LIR_xf:
                {
                    verbose_only( _thisfrag->nStaticExits++; )
                    countlir_xcc();
                    
                    NIns* exit = asm_exit(ins); 
                    LIns* cond = ins->oprnd1();
                    asm_branch(op == LIR_xf, cond, exit);
                    break;
                }
                case LIR_x:
                {
                    verbose_only( _thisfrag->nStaticExits++; )
                    countlir_x();
                    
                    NIns *exit = asm_exit(ins);
                    JMP( exit );
                    break;
                }

                case LIR_feq:
                case LIR_fle:
                case LIR_flt:
                case LIR_fgt:
                case LIR_fge:
                {
                    countlir_fpu();
                    asm_fcond(ins);
                    break;
                }
                case LIR_eq:
                case LIR_ov:
                case LIR_le:
                case LIR_lt:
                case LIR_gt:
                case LIR_ge:
                case LIR_ult:
                case LIR_ule:
                case LIR_ugt:
                case LIR_uge:
#ifdef NANOJIT_64BIT
                case LIR_qeq:
                case LIR_qle:
                case LIR_qlt:
                case LIR_qgt:
                case LIR_qge:
                case LIR_qult:
                case LIR_qule:
                case LIR_qugt:
                case LIR_quge:
#endif
                {
                    countlir_alu();
                    asm_cond(ins);
                    break;
                }

                case LIR_fcall:
            #ifdef NANOJIT_64BIT
                case LIR_qcall:
            #endif
                case LIR_icall:
                {
                    countlir_call();
                    Register rr = UnknownReg;
                    if (ARM_VFP && op == LIR_fcall)
                    {
                        
                        rr = asm_prep_fcall(getresv(ins), ins);
                    }
                    else
                    {
                        rr = retRegs[0];
                        prepResultReg(ins, rmask(rr));
                    }

                    
                    

                    evictScratchRegs();

                    asm_call(ins);
                    break;
                }

                #ifdef VTUNE
                case LIR_file:
                {
                    
                    
                    uintptr_t currentFile = ins->oprnd1()->imm32();
                    cgen->jitFilenameUpdate(currentFile);
                    break;
                }
                case LIR_line:
                {
                    
                    
                    
                    uint32_t currentLine = (uint32_t) ins->oprnd1()->imm32();
                    cgen->jitLineNumUpdate(currentLine);
                    cgen->jitAddRecord((uintptr_t)_nIns, 0, currentLine, true);
                    break;
                }
                #endif 
            }

#ifdef NJ_VERBOSE
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (_logc->lcbits & LC_Assembly) {
                LirNameMap* names = _thisfrag->lirbuf->names;
                outputf("    %s", names->formatIns(ins));
                if (ins->isGuard() && ins->oprnd1()) {
                    
                    
                    
                    
                    
                    
                    
                    outputf("    %s       # codegen'd with the %s",
                            names->formatIns(ins->oprnd1()), lirNames[op]);

                } else if (ins->isop(LIR_cmov) || ins->isop(LIR_qcmov)) {
                    
                    outputf("    %s       # codegen'd with the %s",
                            names->formatIns(ins->oprnd1()), lirNames[op]);

                } else if (ins->isop(LIR_mod)) {
                    
                    outputf("    %s       # codegen'd with the mod",
                            names->formatIns(ins->oprnd1()));
                }
            }
#endif

            if (error())
                return;

        #ifdef VTUNE
            cgen->jitCodePosUpdate((uintptr_t)_nIns);
        #endif

            
            debug_only( pageValidate(); )
            debug_only( resourceConsistencyCheck();  )
        }
    }

    




    void Assembler::emitJumpTable(SwitchInfo* si, NIns* target)
    {
        si->table = (NIns **) alloc.alloc(si->count * sizeof(NIns*));
        for (uint32_t i = 0; i < si->count; ++i)
            si->table[i] = target;
    }

    void Assembler::assignSavedRegs()
    {
        
        releaseRegisters();
        LirBuffer *b = _thisfrag->lirbuf;
        for (int i=0, n = NumSavedRegs; i < n; i++) {
            LIns *p = b->savedRegs[i];
            if (p)
                findSpecificRegForUnallocated(p, savedRegs[p->paramArg()]);
        }
    }

    void Assembler::reserveSavedRegs()
    {
        LirBuffer *b = _thisfrag->lirbuf;
        for (int i=0, n = NumSavedRegs; i < n; i++) {
            LIns *p = b->savedRegs[i];
            if (p)
                findMemFor(p);
        }
    }

    
    void Assembler::assignParamRegs()
    {
        LInsp state = _thisfrag->lirbuf->state;
        if (state)
            findSpecificRegForUnallocated(state, argRegs[state->paramArg()]);
        LInsp param1 = _thisfrag->lirbuf->param1;
        if (param1)
            findSpecificRegForUnallocated(param1, argRegs[param1->paramArg()]);
    }

    void Assembler::handleLoopCarriedExprs(InsList& pending_lives)
    {
        
        reserveSavedRegs();
        for (Seq<LIns*> *p = pending_lives.get(); p != NULL; p = p->tail) {
            LIns *i = p->head;
            NanoAssert(i->isop(LIR_live) || i->isop(LIR_flive));
            LIns *op1 = i->oprnd1();
            
            
            
            findMemFor(op1);
            if (! (op1->isconst() || op1->isconstf() || op1->isconstq()))
                findRegFor(op1, i->isop(LIR_flive) ? FpRegs : GpRegs);
        }

        
        
        pending_lives.clear();
    }

    void Assembler::arFree(uint32_t idx)
    {
        verbose_only( printActivationState(" >FP"); )

        AR &ar = _activation;
        LIns *i = ar.entry[idx];
        NanoAssert(i != 0);
        do {
            ar.entry[idx] = 0;
            idx--;
        } while (ar.entry[idx] == i);
    }

#ifdef NJ_VERBOSE
    void Assembler::printActivationState(const char* what)
    {
        if (!(_logc->lcbits & LC_Activation))
            return;

        char* s = &outline[0];
        VMPI_memset(s, ' ', 45);  s[45] = '\0';
        s += VMPI_strlen(s);
        VMPI_sprintf(s, "%s", what);
        s += VMPI_strlen(s);

        int32_t max = _activation.tos < NJ_MAX_STACK_ENTRY ? _activation.tos : NJ_MAX_STACK_ENTRY;
        for(int32_t i = _activation.lowwatermark; i < max; i++) {
            LIns *ins = _activation.entry[i];
            if (ins) {
                const char* n = _thisfrag->lirbuf->names->formatRef(ins);
                if (ins->isop(LIR_alloc)) {
                    int32_t count = ins->size()>>2;
                    VMPI_sprintf(s," %d-%d(%s)", 4*i, 4*(i+count-1), n);
                    count += i-1;
                    while (i < count) {
                        NanoAssert(_activation.entry[i] == ins);
                        i++;
                    }
                }
                else if (ins->isQuad()) {
                    VMPI_sprintf(s," %d+(%s)", 4*i, n);
                    NanoAssert(_activation.entry[i+1] == ins);
                    i++;
                }
                else {
                    VMPI_sprintf(s," %d(%s)", 4*i, n);
                }
            }
            s += VMPI_strlen(s);
        }
        output(&outline[0]);
    }
#endif

    bool canfit(int32_t size, int32_t loc, AR &ar) {
        for (int i=0; i < size; i++) {
            if (ar.entry[loc+stack_direction(i)])
                return false;
        }
        return true;
    }

    uint32_t Assembler::arReserve(LIns* l)
    {
        int32_t size = l->isop(LIR_alloc) ? (l->size()>>2) : l->isQuad() ? 2 : 1;
        AR &ar = _activation;
        const int32_t tos = ar.tos;
        int32_t start = ar.lowwatermark;
        int32_t i = 0;
        NanoAssert(start>0);
        verbose_only( printActivationState(" <FP"); )

        if (size == 1) {
            
            for (i=start; i < NJ_MAX_STACK_ENTRY; i++) {
                if (ar.entry[i] == 0) {
                    
                    ar.entry[i] = l;
                    break;
                }
            }
        }
        else if (size == 2) {
            if ( (start&1)==1 ) start++;  
            for (i=start; i < NJ_MAX_STACK_ENTRY; i+=2) {
                if ( (ar.entry[i+stack_direction(1)] == 0) && (i==tos || (ar.entry[i] == 0)) ) {
                    
                    NanoAssert(ar.entry[i] == 0);
                    NanoAssert(ar.entry[i+stack_direction(1)] == 0);
                    ar.entry[i] = l;
                    ar.entry[i+stack_direction(1)] = l;
                    break;
                }
            }
        }
        else {
            
            if (start < size) start = size;
            if ((start&1)==1) start++;
            for (i=start; i < NJ_MAX_STACK_ENTRY; i+=2) {
                if (canfit(size, i, ar)) {
                    
                    for (int32_t j=0; j < size; j++) {
                        NanoAssert(ar.entry[i+stack_direction(j)] == 0);
                        ar.entry[i+stack_direction(j)] = l;
                    }
                    break;
                }
            }
        }
        if (i >= (int32_t)ar.tos) {
            ar.tos = i+1;
        }
        if (tos+size >= NJ_MAX_STACK_ENTRY) {
            setError(StackFull);
        }
        return i;
    }

    


    void Assembler::evictScratchRegs()
    {
        

        
        

        Register tosave[LastReg-FirstReg+1];
        int len=0;
        RegAlloc *regs = &_allocator;
        for (Register r = FirstReg; r <= LastReg; r = nextreg(r)) {
            if (rmask(r) & GpRegs) {
                LIns *i = regs->getActive(r);
                if (i) {
                    if (canRemat(i)) {
                        evict(r, i);
                    }
                    else {
                        int32_t pri = regs->getPriority(r);
                        
                        int j = len++;
                        while (j > 0 && pri > regs->getPriority(tosave[j/2])) {
                            tosave[j] = tosave[j/2];
                            j /= 2;
                        }
                        NanoAssert(size_t(j) < sizeof(tosave)/sizeof(tosave[0]));
                        tosave[j] = r;
                    }
                }
            }
        }

        
        

        RegisterMask allow = SavedRegs;
        while (allow && len > 0) {
            
            Register hi = tosave[0];
            if (!(rmask(hi) & SavedRegs)) {
                LIns *i = regs->getActive(hi);
                Register r = findRegFor(i, allow);
                allow &= ~rmask(r);
            }
            else {
                
                allow &= ~rmask(hi);
            }

            
            if (allow && --len > 0) {
                Register last = tosave[len];
                int j = 0;
                while (j+1 < len) {
                    int child = j+1;
                    if (j+2 < len && regs->getPriority(tosave[j+2]) > regs->getPriority(tosave[j+1]))
                        child++;
                    if (regs->getPriority(last) > regs->getPriority(tosave[child]))
                        break;
                    tosave[j] = tosave[child];
                    j = child;
                }
                tosave[j] = last;
            }
        }

        
        evictSomeActiveRegs(~SavedRegs);
    }

    void Assembler::evictAllActiveRegs()
    {
        
        
        for (Register r = FirstReg; r <= LastReg; r = nextreg(r)) {
            evictIfActive(r);
        }
    }

    void Assembler::evictSomeActiveRegs(RegisterMask regs)
    {
        
        
        for (Register r = FirstReg; r <= LastReg; r = nextreg(r)) {
            if ((rmask(r) & regs)) {
                evictIfActive(r);
            }
        }
    }

    






    void Assembler::intersectRegisterState(RegAlloc& saved)
    {
        
        RegisterMask skip = 0;
        verbose_only(bool shouldMention=false; )
        for (Register r=FirstReg; r <= LastReg; r = nextreg(r))
        {
            LIns * curins = _allocator.getActive(r);
            LIns * savedins = saved.getActive(r);
            if (curins == savedins)
            {
                
                skip |= rmask(r);
            }
            else
            {
                if (curins) {
                    
                    verbose_only( shouldMention=true; )
                    evict(r, curins);
                }

                #ifdef NANOJIT_IA32
                if (savedins && (rmask(r) & x87Regs)) {
                    verbose_only( shouldMention=true; )
                    FSTP(r);
                }
                #endif
            }
        }
        assignSaved(saved, skip);
        verbose_only(
            if (shouldMention)
                verbose_outputf("## merging registers (intersect) "
                                "with existing edge");
        )
    }

    







    void Assembler::unionRegisterState(RegAlloc& saved)
    {
        
        verbose_only(bool shouldMention=false; )
        RegisterMask skip = 0;
        for (Register r=FirstReg; r <= LastReg; r = nextreg(r))
        {
            LIns * curins = _allocator.getActive(r);
            LIns * savedins = saved.getActive(r);
            if (curins == savedins)
            {
                
                skip |= rmask(r);
            }
            else
            {
                if (curins && savedins) {
                    
                    verbose_only( shouldMention=true; )
                    evict(r, curins);
                }

                #ifdef NANOJIT_IA32
                if (rmask(r) & x87Regs) {
                    if (savedins) {
                        FSTP(r);
                    }
                    else {
                        
                        
                        evictIfActive(r);
                    }
                    verbose_only( shouldMention=true; )
                }
                #endif
            }
        }
        assignSaved(saved, skip);
        verbose_only( if (shouldMention) verbose_outputf("                                              merging registers (union) with existing edge");  )
    }

    void Assembler::assignSaved(RegAlloc &saved, RegisterMask skip)
    {
        
        for (Register r=FirstReg; r <= LastReg; r = nextreg(r))
        {
            LIns *i = saved.getActive(r);
            if (i && !(skip&rmask(r)))
                findSpecificRegFor(i, r);
        }
    }

    
    
    LIns* Assembler::findVictim(RegisterMask allow)
    {
        NanoAssert(allow != 0);
        LIns *i, *a=0;
        int allow_pri = 0x7fffffff;
        for (Register r=FirstReg; r <= LastReg; r = nextreg(r))
        {
            if ((allow & rmask(r)) && (i = _allocator.getActive(r)) != 0)
            {
                int pri = canRemat(i) ? 0 : _allocator.getPriority(r);
                if (!a || pri < allow_pri) {
                    a = i;
                    allow_pri = pri;
                }
            }
        }
        NanoAssert(a != 0);
        return a;
    }

#ifdef NJ_VERBOSE
    
    
    
    char Assembler::outline[8192];
    char Assembler::outlineEOL[512];

    void Assembler::outputForEOL(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        outlineEOL[0] = '\0';
        vsprintf(outlineEOL, format, args);
    }

    void Assembler::outputf(const char* format, ...)
    {
        va_list     args;
        va_start(args, format);
        outline[0] = '\0';

        
        
        uint32_t outline_len = vsprintf(outline, format, args);

        
        
        
        VMPI_strncat(outline, outlineEOL, sizeof(outline)-(outline_len+1));
        outlineEOL[0] = '\0';

        output(outline);
    }

    void Assembler::output(const char* s)
    {
        if (_outputCache)
        {
            char* str = new (alloc) char[VMPI_strlen(s)+1];
            VMPI_strcpy(str, s);
            _outputCache->insert(str);
        }
        else
        {
            _logc->printf("%s\n", s);
        }
    }

    void Assembler::output_asm(const char* s)
    {
        if (!(_logc->lcbits & LC_Assembly))
            return;

        
        
        
        VMPI_strncat(outline, outlineEOL, sizeof(outline)-(strlen(outline)+1));
        outlineEOL[0] = '\0';

        output(s);
    }

    char* Assembler::outputAlign(char *s, int col)
    {
        int len = (int)VMPI_strlen(s);
        int add = ((col-len)>0) ? col-len : 1;
        VMPI_memset(&s[len], ' ', add);
        s[col] = '\0';
        return &s[col];
    }
#endif 

    uint32_t CallInfo::_count_args(uint32_t mask) const
    {
        uint32_t argc = 0;
        uint32_t argt = _argtypes;
        for (uint32_t i = 0; i < MAXARGS; ++i) {
            argt >>= ARGSIZE_SHIFT;
            if (!argt)
                break;
            argc += (argt & mask) != 0;
        }
        return argc;
    }

    uint32_t CallInfo::get_sizes(ArgSize* sizes) const
    {
        uint32_t argt = _argtypes;
        uint32_t argc = 0;
        for (uint32_t i = 0; i < MAXARGS; i++) {
            argt >>= ARGSIZE_SHIFT;
            ArgSize a = ArgSize(argt & ARGSIZE_MASK_ANY);
            if (a != ARGSIZE_NONE) {
                sizes[argc++] = a;
            } else {
                break;
            }
        }
        return argc;
    }

    void LabelStateMap::add(LIns *label, NIns *addr, RegAlloc &regs) {
        LabelState *st = new (alloc) LabelState(addr, regs);
        labels.put(label, st);
    }

    LabelState* LabelStateMap::get(LIns *label) {
        return labels.get(label);
    }
}
#endif 
