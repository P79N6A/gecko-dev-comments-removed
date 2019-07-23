






































#include "nanojit.h"

#ifdef FEATURE_NANOJIT

#ifdef AVMPLUS_PORTING_API
#include "portapi_nanojit.h"
#endif

namespace nanojit
{
    int UseSoftfloat = 0;

#ifdef NJ_VERBOSE
    class VerboseBlockReader: public LirFilter
    {
        Assembler *assm;
        LirNameMap *names;
        InsList block;
        bool flushnext;
    public:
        VerboseBlockReader(LirFilter *in, Assembler *a, LirNameMap *n)
            : LirFilter(in), assm(a), names(n), block(a->_gc), flushnext(false)
        {}

        void flush() {
            flushnext = false;
            if (!block.isEmpty()) {
                for (int j=0,n=block.size(); j < n; j++) {
                    LIns *i = block[j];
                    assm->outputf("    %s", names->formatIns(i));
                }
                block.clear();
            }
        }

        void flush_add(LInsp i) {
            flush();
            block.add(i);
        }

        LInsp read() {
            LInsp i = in->read();
            if (i->isop(LIR_start)) {
                flush();
                return i;
            }
            if (i->isGuard()) {
                flush_add(i);
                if (i->oprnd1())
                    block.add(i->oprnd1());
            }
            else if (i->isRet() || i->isBranch()) {
                flush_add(i);
            }
            else {
                if (flushnext)
                    flush();
                block.add(i);
                if (i->isop(LIR_label))
                    flushnext = true;
            }
            return i;
        }
    };

    




    class ReverseLister : public LirFilter
    {
        avmplus::GC* _gc;
        LirNameMap*  _names;
        const char*  _title;
        StringList*  _strs;
        LogControl*  _logc;
    public:
        ReverseLister(LirFilter* in, avmplus::GC* gc,
                      LirNameMap* names, LogControl* logc, const char* title)
            : LirFilter(in)
        {
            _gc    = gc;
            _names = names;
            _title = title;
            _strs  = new StringList(gc);
            _logc  = logc;
        }

        ~ReverseLister()
        {
            _logc->printf("\n");
            _logc->printf("=== BEGIN %s ===\n", _title);
            int i, j;
            const char* prefix = "  ";
            for (j = 0, i = _strs->size()-1; i >= 0; i--, j++) {
                char* str = _strs->get(i);
                _logc->printf("%s%02d: %s\n", prefix, j, str);
                _gc->Free(str);
            }
            delete _strs;
            _logc->printf("=== END %s ===\n", _title);
            _logc->printf("\n");
        }

        LInsp read()
        {
            LInsp i = in->read();
            const char* str = _names->formatIns(i);
            char* cpy = (char*)_gc->Alloc(strlen(str) + 1,  0);
            strcpy(cpy, str);
            _strs->add(cpy);
            return i;
        }
    };
#endif

    




    Assembler::Assembler(CodeAlloc* codeAlloc, AvmCore *core, LogControl* logc)
        : hasLoop(0)
        , codeList(0)
        , core(core)
        , _codeAlloc(codeAlloc)
        , config(core->config)
    {
        nInit(core);
        verbose_only( _logc = logc; )
        verbose_only( _outputCache = 0; )
        verbose_only( outlineEOL[0] = '\0'; )

        reset();
    }

    void Assembler::arReset()
    {
        _activation.highwatermark = 0;
        _activation.lowwatermark = 0;
        _activation.tos = 0;

        for(uint32_t i=0; i<NJ_MAX_STACK_ENTRY; i++)
            _activation.entry[i] = 0;
    }

     void Assembler::registerResetAll()
    {
        nRegisterResetAll(_allocator);

        
        debug_only(_allocator.count = _allocator.countFree(); )
        debug_only(_allocator.checkCount(); )
        debug_only(_fpuStkDepth = 0; )
    }

    Register Assembler::registerAlloc(RegisterMask allow)
    {
        RegAlloc &regs = _allocator;

        RegisterMask prefer = SavedRegs & allow;
        RegisterMask free = regs.free & allow;

        RegisterMask set = prefer;
        if (set == 0) set = allow;

        if (free)
        {
            
            set &= free;

            
            
            if (!set)
            {
                
                set = free;
            }
            NanoAssert((set & allow) != 0);
            Register r = nRegisterAllocFromSet(set);
            regs.used |= rmask(r);
            return r;
        }
        counter_increment(steals);

        
        
        LIns* vic = findVictim(regs, allow);
        NanoAssert(vic != NULL);

        Reservation* resv = getresv(vic);
        NanoAssert(resv);

        
        Register r = resv->reg;
        regs.removeActive(r);
        resv->reg = UnknownReg;

        asm_restore(vic, resv, r);
        return r;
    }

    



    bool Assembler::canRemat(LIns *i) {
        return i->isconst() || i->isconstq() || i->isop(LIR_ialloc);
    }

    void Assembler::codeAlloc(NIns *&start, NIns *&end, NIns *&eip)
    {
        
        if (start)
            CodeAlloc::add(codeList, start, end);

        
        _codeAlloc->alloc(start, end);
        NanoAssert(uintptr_t(end) - uintptr_t(start) >= (size_t)LARGEST_UNDERRUN_PROT);
        eip = end;
    }

    void Assembler::reset()
    {
        
        _nIns = 0;
        _nExitIns = 0;
        _startingIns = 0;
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

#endif


    #ifdef _DEBUG

    void Assembler::resourceConsistencyCheck()
    {
        if (error()) return;

#ifdef NANOJIT_IA32
        NanoAssert((_allocator.active[FST0] && _fpuStkDepth == -1) ||
            (!_allocator.active[FST0] && _fpuStkDepth == 0));
#endif

        AR &ar = _activation;
        
        NanoAssert(ar.highwatermark < NJ_MAX_STACK_ENTRY);
        LIns* ins = 0;
        RegAlloc* regs = &_allocator;
        for(uint32_t i = ar.lowwatermark; i < ar.tos; i++)
        {
            ins = ar.entry[i];
            if ( !ins )
                continue;
            Reservation *r = getresv(ins);
            NanoAssert(r != 0);
            if (r->arIndex) {
                if (ins->isop(LIR_ialloc)) {
                    int j=i+1;
                    for (int n = i + (ins->size()>>2); j < n; j++) {
                        NanoAssert(ar.entry[j]==ins);
                    }
                    NanoAssert(r->arIndex == (uint32_t)j-1);
                    i = j-1;
                }
                else if (ins->isQuad()) {
                    NanoAssert(ar.entry[i - stack_direction(1)]==ins);
                    i += 1; 
                }
                else {
                    NanoAssertMsg(r->arIndex == i, "Stack record index mismatch");
                }
            }
            NanoAssertMsg( r->reg==UnknownReg || regs->isConsistent(r->reg,ins), "Register record mismatch");
        }

        registerConsistencyCheck();
    }

    void Assembler::registerConsistencyCheck()
    {
        
        RegAlloc *regs = &_allocator;
        uint32_t managed = regs->managed;
        Register r = FirstReg;
        while(managed)
        {
            if (managed&1)
            {
                if (regs->isFree(r))
                {
                    NanoAssert(regs->getActive(r)==0);
                }
                else
                {
                    LIns* ins = regs->getActive(r);
                    
                    Reservation *v = getresv(ins);
                    NanoAssert(v != 0);
                    NanoAssertMsg( regs->getActive(v->reg)==ins, "Register record mismatch");
                }
            }

            
            r = nextreg(r);
            managed >>= 1;
        }
    }
    #endif

    void Assembler::findRegFor2(RegisterMask allow, LIns* ia, Reservation* &resva, LIns* ib, Reservation* &resvb)
    {
        if (ia == ib)
        {
            findRegFor(ia, allow);
            resva = resvb = getresv(ia);
        }
        else
        {
            Register rb = UnknownReg;
            resvb = getresv(ib);
            if (resvb && (rb = resvb->reg) != UnknownReg) {
                if (allow & rmask(rb)) {
                    
                    allow &= ~rmask(rb);
                } else {
                    
                    rb = UnknownReg;
                }
            }
            Register ra = findRegFor(ia, allow);
            resva = getresv(ia);
            NanoAssert(error() || (resva != 0 && ra != UnknownReg));
            if (rb == UnknownReg)
            {
                allow &= ~rmask(ra);
                findRegFor(ib, allow);
                resvb = getresv(ib);
            }
        }
    }

    Register Assembler::findSpecificRegFor(LIns* i, Register w)
    {
        return findRegFor(i, rmask(w));
    }

    Register Assembler::getBaseReg(LIns *i, int &d, RegisterMask allow)
    {
        if (i->isop(LIR_ialloc)) {
            d += findMemFor(i);
            return FP;
        } else {
            return findRegFor(i, allow);
        }
    }

    Register Assembler::findRegFor(LIns* i, RegisterMask allow)
    {
        if (i->isop(LIR_ialloc)) {
            
            findMemFor(i);
        }

        Reservation* resv = getresv(i);
        Register r;

        
        
        
        if (resv && (r=resv->reg) != UnknownReg && (rmask(r) & allow)) {
            _allocator.useActive(r);
            return r;
        }

        
        RegisterMask prefer = hint(i, allow);

        
        if (!resv) {
            (resv = i->resv())->init();
        }

        r = resv->reg;

#ifdef AVMPLUS_IA32
        if (r != UnknownReg &&
            (((rmask(r)&XmmRegs) && !(allow&XmmRegs)) ||
                 ((rmask(r)&x87Regs) && !(allow&x87Regs))))
        {
            
            
            evict(r);
            r = UnknownReg;
        }
#endif

        if (r == UnknownReg)
        {
            r = resv->reg = registerAlloc(prefer);
            _allocator.addActive(r, i);
            return r;
        }
        else
        {
            
            
            
            resv->reg = UnknownReg;
            _allocator.retire(r);
            Register s = resv->reg = registerAlloc(prefer);
            _allocator.addActive(s, i);
            if ((rmask(r) & GpRegs) && (rmask(s) & GpRegs)) {
#ifdef NANOJIT_ARM
                MOV(r, s);
#else
                MR(r, s);
#endif
            }
            else {
                asm_nongp_copy(r, s);
            }
            return s;
        }
    }

    int Assembler::findMemFor(LIns *i)
    {
        Reservation* resv = getresv(i);
        if (!resv)
            (resv = i->resv())->init();
        if (!resv->arIndex) {
            resv->arIndex = arReserve(i);
            NanoAssert(resv->arIndex <= _activation.highwatermark);
        }
        return disp(resv);
    }

    Register Assembler::prepResultReg(LIns *i, RegisterMask allow)
    {
        Reservation* resv = getresv(i);
        const bool pop = !resv || resv->reg == UnknownReg;
        Register rr = findRegFor(i, allow);
        freeRsrcOf(i, pop);
        return rr;
    }

    void Assembler::asm_spilli(LInsp i, Reservation *resv, bool pop)
    {
        int d = disp(resv);
        Register rr = resv->reg;
        bool quad = i->opcode() == LIR_iparam || i->isQuad();
        verbose_only( if (d && (_logc->lcbits & LC_RegAlloc)) {
                         outputForEOL("  <= spill %s",
                                      _thisfrag->lirbuf->names->formatRef(i)); } )
        asm_spill(rr, d, pop, quad);
    }

    
    
    
    
    void Assembler::freeRsrcOf(LIns *i, bool pop)
    {
        Reservation* resv = getresv(i);
        int index = resv->arIndex;
        Register rr = resv->reg;

        if (rr != UnknownReg)
        {
            asm_spilli(i, resv, pop);
            _allocator.retire(rr);    
        }
        if (index) {
            NanoAssert(_activation.entry[index] == i);
            arFree(index);            
        }
        i->resv()->clear();
    }

    void Assembler::evict(Register r)
    {
        registerAlloc(rmask(r));
        _allocator.addFree(r);
    }

    void Assembler::patch(GuardRecord *lr)
    {
        if (!lr->jmp) 
            return;
        Fragment *frag = lr->exit->target;
        NanoAssert(frag->fragEntry != 0);
        NIns* was = nPatchBranch((NIns*)lr->jmp, frag->fragEntry);
        NanoAssert(frag->fragEntry != was);
        verbose_only(verbose_outputf("patching jump at %p to target %p (was %p)\n",
            lr->jmp, frag->fragEntry, was);)
        (void)was;
    }

    void Assembler::patch(SideExit *exit)
    {
        GuardRecord *rec = exit->guards;
        AvmAssert(rec);
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
        if (!_branchStateMap->get(exit))
        {
            at = asm_leave_trace(guard);
        }
        else
        {
            RegAlloc* captured = _branchStateMap->get(exit);
            intersectRegisterState(*captured);
            at = exit->target->fragEntry;
            NanoAssert(at != 0);
            _branchStateMap->remove(exit);
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

        
        debug_only( _sv_fpuStkDepth = _fpuStkDepth; _fpuStkDepth = 0; )

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

    void Assembler::beginAssembly(Fragment *frag, RegAllocMap* branchStateMap)
    {
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
        _activation.highwatermark = _activation.tos;

        counter_reset(native);
        counter_reset(exitnative);
        counter_reset(steals);
        counter_reset(spills);
        counter_reset(remats);

        setError(None);

        
        nativePageSetup();

        
        underrunProtect(LARGEST_UNDERRUN_PROT);  
        recordStartingInstructionPointer();

    #ifdef AVMPLUS_PORTING_API
        _endJit2Addr = _nExitIns;
    #endif

        
        if (error()) return;

#ifdef PERFM
        _stats.pages = 0;
        _stats.codeStart = _nIns-1;
        _stats.codeExitStart = _nExitIns-1;
        
#endif 

        _epilogue = genEpilogue();
        _branchStateMap = branchStateMap;

        verbose_only( outputAddr=true; )
        verbose_only( asm_output("[epilogue]"); )
    }

    void Assembler::assemble(Fragment* frag,  NInsList& loopJumps)
    {
        if (error()) return;
        _thisfrag = frag;

        
        verbose_only(
        ReverseLister *pp_init = NULL;
        ReverseLister *pp_after_sf1 = NULL;
        ReverseLister *pp_after_sf2 = NULL;
        )

        
        avmplus::GC *gc = core->gc;
        LirReader bufreader(frag->lastIns);

        
        LirFilter* prev = &bufreader;

        
        

        
        verbose_only( if (_logc->lcbits & LC_ReadLIR) {
        pp_init = new ReverseLister(prev, gc, frag->lirbuf->names, _logc,
                                    "Initial LIR");
        prev = pp_init;
        })

        
        StackFilter storefilter1(prev, gc, frag->lirbuf, frag->lirbuf->sp);
        prev = &storefilter1;

        verbose_only( if (_logc->lcbits & LC_AfterSF_SP) {
        pp_after_sf1 = new ReverseLister(prev, gc, frag->lirbuf->names, _logc,
                                         "After Storefilter(sp)");
        prev = pp_after_sf1;
        })

        
        StackFilter storefilter2(prev, gc, frag->lirbuf, frag->lirbuf->rp);
        prev = &storefilter2;

        verbose_only( if (_logc->lcbits & LC_AfterSF_RP) {
        pp_after_sf2 = new ReverseLister(prev, gc, frag->lirbuf->names, _logc,
                                         "After StoreFilter(rp) (final LIR)");
        prev = pp_after_sf2;
        })

        
        verbose_only(
        VerboseBlockReader vbr(prev, this, frag->lirbuf->names);
        if (_logc->lcbits & LC_Assembly)
            prev = &vbr;
        )

        verbose_only(_thisfrag->compileNbr++; )
        _inExit = false;

        LabelStateMap labels(_gc);
        NInsMap patches(_gc);
        gen(prev, loopJumps, labels, patches);
        frag->loopEntry = _nIns;
        
        

        if (!error()) {
            
            while (!patches.isEmpty())
            {
                NIns* where = patches.lastKey();
                LInsp targ = patches.removeLast();
                LabelState *label = labels.get(targ);
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
        else {
            
            resetInstructionPointer();
        }

        
        
        
        verbose_only(
        if (pp_init)       delete pp_init;
        if (pp_after_sf1)  delete pp_after_sf1;
        if (pp_after_sf2)  delete pp_after_sf2;
        )
    }

    void Assembler::endAssembly(Fragment* frag, NInsList& loopJumps)
    {
        
        
        if (error())
            return;

        NIns* SOT = 0;
        if (frag->isRoot()) {
            SOT = frag->loopEntry;
            verbose_only( verbose_outputf("%010lx:", (unsigned long)_nIns); )
        } else {
            SOT = frag->root->fragEntry;
        }
        AvmAssert(SOT);
        while(!loopJumps.isEmpty())
        {
            NIns* loopJump = (NIns*)loopJumps.removeLast();
            verbose_only( verbose_outputf("## patching branch at %010lx to %010lx",
                                          loopJump, SOT); )
            nPatchBranch(loopJump, SOT);
        }

        NIns* fragEntry = 0;

        if (!error())
        {
            fragEntry = genPrologue();
            verbose_only( outputAddr=true; )
            verbose_only( asm_output("[prologue]"); )
        }

        
        if (!error())
        {
            
            debug_only(
                for(uint32_t i=_activation.lowwatermark;i<_activation.highwatermark; i++) {
                    NanoAssertMsgf(_activation.entry[i] == 0, "frame entry %d wasn't freed",-4*i);
                }
            )

            
#ifdef NANOJIT_ARM
            _codeAlloc->addRemainder(codeList, exitStart, exitEnd, _nExitSlot, _nExitIns);
            _codeAlloc->addRemainder(codeList, codeStart, codeEnd, _nSlot, _nIns);
#else
            _codeAlloc->addRemainder(codeList, exitStart, exitEnd, exitStart, _nExitIns);
            _codeAlloc->addRemainder(codeList, codeStart, codeEnd, codeStart, _nIns);
#endif

            debug_only( pageValidate(); )

            
            
            _codeAlloc->flushICache(codeList);

            
            frag->fragEntry = fragEntry;
            frag->setCode(_nIns);
        }
        else
        {
            
            _codeAlloc->freeAll(codeList);
            _codeAlloc->free(exitStart, exitEnd);
            _codeAlloc->free(codeStart, codeEnd);
        }

        NanoAssertMsgf(error() || _fpuStkDepth == 0,"_fpuStkDepth %d",_fpuStkDepth);

        debug_only( pageValidate(); )
        reset();
        NanoAssert( !_branchStateMap || _branchStateMap->isEmpty());
        _branchStateMap = 0;
    }

    void Assembler::copyRegisters(RegAlloc* copyTo)
    {
        *copyTo = _allocator;
    }

    void Assembler::releaseRegisters()
    {
        for (Register r = FirstReg; r <= LastReg; r = nextreg(r))
        {
            LIns *i = _allocator.getActive(r);
            if (i)
            {
                
                Reservation* resv = getresv(i);
                NanoAssert(resv != 0);
                _allocator.retire(r);
                if (r == resv->reg)
                    resv->reg = UnknownReg;

                if (!resv->arIndex && resv->reg == UnknownReg)
                {
                    i->resv()->clear();
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
#define countlir_loop() _nvprof("lir-loop",1)
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
#define countlir_loop()
#define countlir_call()
#endif

    void Assembler::gen(LirFilter* reader,  NInsList& loopJumps, LabelStateMap& labels,
                        NInsMap& patches)
    {
        
        NanoAssert(reader->pos()->isop(LIR_x) ||
                   reader->pos()->isop(LIR_loop) ||
                   reader->pos()->isop(LIR_ret) ||
                   reader->pos()->isop(LIR_xtbl));

        InsList pending_lives(_gc);

        for (LInsp ins = reader->read(); !ins->isop(LIR_start) && !error();
                                         ins = reader->read())
        {
            



































            bool required = ins->isStmt() || ins->resv()->used;
            if (!required)
                continue;
 
            LOpcode op = ins->opcode();
            switch(op)
            {
                default:
                    NanoAssertMsgf(false, "unsupported LIR instruction: %d (~0x40: %d)", op, op&~LIR64);
                    break;

                case LIR_live: {
                    countlir_live();
                    pending_lives.add(ins->oprnd1());
                    break;
                }

                case LIR_ret:  {
                    countlir_ret();
                    if (_nIns != _epilogue) {
                        JMP(_epilogue);
                    }
                    assignSavedRegs();
#ifdef NANOJIT_ARM
                    
                    
                    findSpecificRegFor(ins->oprnd1(), R2);
#else
                    findSpecificRegFor(ins->oprnd1(), retRegs[0]);
#endif
                    break;
                }

                case LIR_fret: {
                    countlir_ret();
                    if (_nIns != _epilogue) {
                        JMP(_epilogue);
                    }
                    assignSavedRegs();
#ifdef NANOJIT_IA32
                    findSpecificRegFor(ins->oprnd1(), FST0);
#else
                    NanoAssert(false);
#endif
                    fpu_pop();
                    break;
                }

                
                
                case LIR_ialloc: {
                    countlir_alloc();
                    Reservation *resv = getresv(ins);
                    NanoAssert(resv->arIndex != 0);
                    Register r = resv->reg;
                    if (r != UnknownReg) {
                        _allocator.retire(r);
                        resv->reg = UnknownReg;
                        asm_restore(ins, resv, r);
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
                case LIR_iparam:
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
                case LIR_qior:
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
                    LabelState *label = labels.get(to);
                    
                    
                    
                    
                    releaseRegisters();
                    if (label && label->addr) {
                        
                        unionRegisterState(label->regs);
                        JMP(label->addr);
                    }
                    else {
                        
                        hasLoop = true;
                        handleLoopCarriedExprs(pending_lives);
                        if (!label) {
                            
                            labels.add(to, 0, _allocator);
                        }
                        else {
                            intersectRegisterState(label->regs);
                        }
                        JMP(0);
                        patches.put(_nIns, to);
                    }
                    break;
                }

                case LIR_jt:
                case LIR_jf:
                {
                    countlir_jcc();
                    LInsp to = ins->getTarget();
                    LIns* cond = ins->oprnd1();
                    LabelState *label = labels.get(to);
                    if (label && label->addr) {
                        
                        unionRegisterState(label->regs);
                        asm_branch(op == LIR_jf, cond, label->addr);
                    }
                    else {
                        
                        hasLoop = true;
                        handleLoopCarriedExprs(pending_lives);
                        if (!label) {
                            
                            evictRegs(~_allocator.free);
                            labels.add(to, 0, _allocator);
                        }
                        else {
                            
                            intersectRegisterState(label->regs);
                        }
                        NIns *branch = asm_branch(op == LIR_jf, cond, 0);
                        patches.put(branch,to);
                    }
                    break;
                }
                case LIR_label:
                {
                    countlir_label();
                    LabelState *label = labels.get(ins);
                    if (!label) {
                        
                        labels.add(ins, _nIns, _allocator);
                    }
                    else {
                        
                        hasLoop = true;
                        NanoAssert(label->addr == 0 && label->regs.isValid());
                        
                        intersectRegisterState(label->regs);
                        label->addr = _nIns;
                    }
                    verbose_only( if (_logc->lcbits & LC_Assembly) { outputAddr=true; asm_output("[%s]", _thisfrag->lirbuf->names->formatRef(ins)); } )
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
                    countlir_xcc();
                    
                    NIns* exit = asm_exit(ins); 
                    LIns* cond = ins->oprnd1();
                    asm_branch(op == LIR_xf, cond, exit);
                    break;
                }
                case LIR_x:
                {
                    countlir_x();
                    verbose_only( if (_logc->lcbits & LC_Assembly)
                                      asm_output("FIXME-whats-this?\n"); )
                    
                    NIns *exit = asm_exit(ins);
                    JMP( exit );
                    break;
                }
                case LIR_loop:
                {
                    countlir_loop();
                    asm_loop(ins, loopJumps);
                    assignSavedRegs();
                    assignParamRegs();
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
                {
                    countlir_alu();
                    asm_cond(ins);
                    break;
                }

                case LIR_fcall:
#if defined NANOJIT_64BIT
                case LIR_callh:
#endif
                case LIR_call:
                {
                    countlir_call();
                    Register rr = UnknownReg;
                    if ((op&LIR64))
                    {
                        
                        Reservation* rR = getresv(ins);
                        rr = asm_prep_fcall(rR, ins);
                    }
                    else
                    {
                        rr = retRegs[0];
                        prepResultReg(ins, rmask(rr));
                    }

                    
                    

                    evictScratchRegs();

                    asm_call(ins);
                }
            }

            if (error())
                return;

            
            debug_only( pageValidate(); )
            debug_only( resourceConsistencyCheck();  )
        }
    }

    




    void Assembler::emitJumpTable(SwitchInfo* si, NIns* target)
    {
        underrunProtect(si->count * sizeof(NIns*) + 20);
        _nIns = reinterpret_cast<NIns*>(uintptr_t(_nIns) & ~(sizeof(NIns*) - 1));
        for (uint32_t i = 0; i < si->count; ++i) {
            _nIns = (NIns*) (((uint8*) _nIns) - sizeof(NIns*));
            *(NIns**) _nIns = target;
        }
        si->table = (NIns**) _nIns;
    }

    void Assembler::assignSavedRegs()
    {
        
        releaseRegisters();
        LirBuffer *b = _thisfrag->lirbuf;
        for (int i=0, n = NumSavedRegs; i < n; i++) {
            LIns *p = b->savedRegs[i];
            if (p)
                findSpecificRegFor(p, savedRegs[p->paramArg()]);
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
            findSpecificRegFor(state, argRegs[state->paramArg()]);
        LInsp param1 = _thisfrag->lirbuf->param1;
        if (param1)
            findSpecificRegFor(param1, argRegs[param1->paramArg()]);
    }

    void Assembler::handleLoopCarriedExprs(InsList& pending_lives)
    {
        
        reserveSavedRegs();
        for (int i=0, n=pending_lives.size(); i < n; i++) {
            findMemFor(pending_lives[i]);
        }
        




        pending_lives.clear();
    }

    void Assembler::arFree(uint32_t idx)
    {
        AR &ar = _activation;
        LIns *i = ar.entry[idx];
        do {
            ar.entry[idx] = 0;
            idx--;
        } while (ar.entry[idx] == i);
    }

#ifdef NJ_VERBOSE
    void Assembler::printActivationState()
    {
        bool verbose_activation = false;
        if (!verbose_activation)
            return;

#ifdef NANOJIT_ARM
        
        verbose_only(
            if (_logc->lcbits & LC_Assembly) {
                char* s = &outline[0];
                memset(s, ' ', 51);  s[51] = '\0';
                s += strlen(s);
                sprintf(s, " SP ");
                s += strlen(s);
                for(uint32_t i=_activation.lowwatermark; i<_activation.tos;i++) {
                    LInsp ins = _activation.entry[i];
                    if (ins && ins !=_activation.entry[i+1]) {
                        sprintf(s, "%d(%s) ", 4*i, _thisfrag->lirbuf->names->formatRef(ins));
                        s += strlen(s);
                    }
                }
                output(&outline[0]);
            }
        )
#else
        verbose_only(
            char* s = &outline[0];
            if (_logc->lcbits & LC_Assembly) {
                memset(s, ' ', 51);  s[51] = '\0';
                s += strlen(s);
                sprintf(s, " ebp ");
                s += strlen(s);

                for(uint32_t i=_activation.lowwatermark; i<_activation.tos;i++) {
                    LInsp ins = _activation.entry[i];
                    if (ins) {
                        sprintf(s, "%d(%s) ", -4*i,_thisfrag->lirbuf->names->formatRef(ins));
                        s += strlen(s);
                    }
                }
                output(&outline[0]);
            }
        )
#endif
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
        
        int32_t size = l->isop(LIR_ialloc) ? (l->size()>>2) : l->isQuad() ? 2 : sizeof(intptr_t)>>2;
        AR &ar = _activation;
        const int32_t tos = ar.tos;
        int32_t start = ar.lowwatermark;
        int32_t i = 0;
        NanoAssert(start>0);

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
            ar.tos = ar.highwatermark = i+1;
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
                        evict(r);
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

        
        evictRegs(~SavedRegs);
    }

    void Assembler::evictRegs(RegisterMask regs)
    {
        
        
        for (Register r = FirstReg; r <= LastReg; r = nextreg(r)) {
            if ((rmask(r) & regs) && _allocator.getActive(r)) {
                evict(r);
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
                    evict(r);
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
                    evict(r);
                }

                #ifdef NANOJIT_IA32
                if (rmask(r) & x87Regs) {
                    if (savedins) {
                        FSTP(r);
                    }
                    else {
                        
                        
                        evict(r);
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
        debug_only(saved.used = 0);  
    }

    
    
    LIns* Assembler::findVictim(RegAlloc &regs, RegisterMask allow)
    {
        NanoAssert(allow != 0);
        LIns *i, *a=0;
        int allow_pri = 0x7fffffff;
        for (Register r=FirstReg; r <= LastReg; r = nextreg(r))
        {
            if ((allow & rmask(r)) && (i = regs.getActive(r)) != 0)
            {
                int pri = canRemat(i) ? 0 : regs.getPriority(r);
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

            
            
            
            strncat(outline, outlineEOL, sizeof(outline)-(outline_len+1));
            outlineEOL[0] = '\0';

            output(outline);
        }

        void Assembler::output(const char* s)
        {
            if (_outputCache)
            {
                char* str = (char*)_gc->Alloc(strlen(s)+1);
                strcpy(str, s);
                _outputCache->add(str);
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

            
            
            
            strncat(outline, outlineEOL, sizeof(outline)-(strlen(outline)+1));
            outlineEOL[0] = '\0';

            output(s);
        }

        char* Assembler::outputAlign(char *s, int col)
        {
            int len = strlen(s);
            int add = ((col-len)>0) ? col-len : 1;
            memset(&s[len], ' ', add);
            s[col] = '\0';
            return &s[col];
        }
    #endif 

#if defined(FEATURE_NANOJIT) || defined(NJ_VERBOSE)
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
        LabelState *st = NJ_NEW(gc, LabelState)(addr, regs);
        labels.put(label, st);
    }

    LabelStateMap::~LabelStateMap() {
        LabelState *st;

        while (!labels.isEmpty()) {
            st = labels.removeLast();
            delete st;
        }
    }

    LabelState* LabelStateMap::get(LIns *label) {
        return labels.get(label);
    }
}
#endif 
