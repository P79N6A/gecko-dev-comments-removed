







































class RecursiveSlotMap : public SlotMap
{
  public:
    RecursiveSlotMap(TraceRecorder& rec)
      : SlotMap(rec)
    {
    }

    JS_REQUIRES_STACK void
    adjustTypes()
    {
    }
};

#if defined DEBUG
static JS_REQUIRES_STACK void
AssertDownFrameIsConsistent(JSContext* cx, VMSideExit* anchor, FrameInfo* fi)
{
    JS_ASSERT(anchor->recursive_down);
    JS_ASSERT(anchor->recursive_down->callerHeight == fi->callerHeight);

    unsigned downPostSlots = fi->callerHeight;
    JSTraceType* typeMap = fi->get_typemap();

    js_CaptureStackTypes(cx, 1, typeMap);
    const JSTraceType* m1 = anchor->recursive_down->get_typemap();
    for (unsigned i = 0; i < downPostSlots; i++) {
        if (m1[i] == typeMap[i])
            continue;
        if (typeMap[i] == TT_INT32 && m1[i] == TT_DOUBLE)
            continue;
        JS_NOT_REACHED("invalid RECURSIVE_MISMATCH exit");
    }
    JS_ASSERT(memcmp(anchor->recursive_down, fi, sizeof(FrameInfo)) == 0);
}
#endif

JS_REQUIRES_STACK VMSideExit*
TraceRecorder::downSnapshot(FrameInfo* downFrame)
{
    JS_ASSERT(!pendingSpecializedNative);

    
    unsigned downPostSlots = downFrame->callerHeight;
    unsigned ngslots = treeInfo->globalSlots->length();
    unsigned exitTypeMapLen = downPostSlots + 1 + ngslots;
    JSTraceType* exitTypeMap = (JSTraceType*)alloca(sizeof(JSTraceType) * exitTypeMapLen);
    JSTraceType* typeMap = downFrame->get_typemap();
    for (unsigned i = 0; i < downPostSlots; i++)
        exitTypeMap[i] = typeMap[i];
    exitTypeMap[downPostSlots] = determineSlotType(&stackval(-1));
    determineGlobalTypes(&exitTypeMap[downPostSlots + 1]);

    VMSideExit* exit = (VMSideExit*)
        traceMonitor->traceAlloc->alloc(sizeof(VMSideExit) + sizeof(JSTraceType) * exitTypeMapLen);

    memset(exit, 0, sizeof(VMSideExit));
    exit->from = fragment;
    exit->calldepth = 0;
    JS_ASSERT(unsigned(exit->calldepth) == getCallDepth());
    exit->numGlobalSlots = ngslots;
    exit->numStackSlots = downPostSlots + 1;
    exit->numStackSlotsBelowCurrentFrame = cx->fp->down->argv ?
        nativeStackOffset(&cx->fp->argv[-2]) / sizeof(double) : 0;
    exit->exitType = UNSTABLE_LOOP_EXIT;
    exit->block = cx->fp->down->blockChain;
    exit->pc = downFrame->pc + JSOP_CALL_LENGTH;
    exit->imacpc = NULL;
    exit->sp_adj = ((downPostSlots + 1) * sizeof(double)) - treeInfo->nativeStackBase;
    exit->rp_adj = exit->calldepth * sizeof(FrameInfo*);
    exit->nativeCalleeWord = 0;
    exit->lookupFlags = js_InferFlags(cx, 0);
    memcpy(exit->fullTypeMap(), exitTypeMap, sizeof(JSTraceType) * exitTypeMapLen);
#if defined JS_JIT_SPEW
    TreevisLogExit(cx, exit);
#endif
    return exit;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::upRecursion()
{
    JS_ASSERT((JSOp)*cx->fp->down->regs->pc == JSOP_CALL);
    JS_ASSERT(js_CodeSpec[js_GetOpcode(cx, cx->fp->down->script,
              cx->fp->down->regs->pc)].length == JSOP_CALL_LENGTH);

    JS_ASSERT(callDepth == 0);

    




    if (anchor && (anchor->exitType == RECURSIVE_EMPTY_RP_EXIT ||
        anchor->exitType == RECURSIVE_SLURP_MISMATCH_EXIT ||
        anchor->exitType == RECURSIVE_SLURP_FAIL_EXIT)) {
        return InjectStatus(slurpDownFrames(cx->fp->down->regs->pc));
    }

    jsbytecode* return_pc = cx->fp->down->regs->pc;
    jsbytecode* recursive_pc = return_pc + JSOP_CALL_LENGTH;

    












    unsigned totalSlots = NativeStackSlots(cx, 1);
    unsigned downPostSlots = totalSlots - NativeStackSlots(cx, 0);
    FrameInfo* fi = (FrameInfo*)alloca(sizeof(FrameInfo) + totalSlots * sizeof(JSTraceType));
    fi->block = cx->fp->blockChain;
    fi->pc = (jsbytecode*)return_pc;
    fi->imacpc = NULL;

    



    fi->spdist = cx->fp->down->regs->sp - cx->fp->down->slots;
    JS_ASSERT(cx->fp->argc == cx->fp->down->argc);
    fi->set_argc(cx->fp->argc, false);
    fi->callerHeight = downPostSlots;
    fi->callerArgc = cx->fp->down->argc;

    if (anchor && anchor->exitType == RECURSIVE_MISMATCH_EXIT) {
        




        #if defined DEBUG
        AssertDownFrameIsConsistent(cx, anchor, fi);
        #endif
        fi = anchor->recursive_down;
    } else if (recursive_pc != fragment->root->ip) {
        



        js_CaptureStackTypes(cx, 1, fi->get_typemap());
    } else {
        
        JS_ASSERT(treeInfo->nStackTypes == downPostSlots + 1);
        JSTraceType* typeMap = fi->get_typemap();
        for (unsigned i = 0; i < downPostSlots; i++)
            typeMap[i] = treeInfo->typeMap[i];
    }

    fi = traceMonitor->frameCache->memoize(fi);

    



    if (!anchor || anchor->exitType != RECURSIVE_MISMATCH_EXIT) {
        VMSideExit* exit = snapshot(RECURSIVE_EMPTY_RP_EXIT);

        
        guard(true,
              lir->ins2(LIR_pge, lirbuf->rp,
                        lir->ins2(LIR_piadd,
                                  lir->insLoad(LIR_ldp, lirbuf->state,
                                               offsetof(InterpState, sor)),
                                  INS_CONSTWORD(sizeof(FrameInfo*)))),
              exit);
    }

    debug_only_printf(LC_TMRecorder, "guardUpRecursive fragment->root=%p fi=%p\n", (void*)fragment->root, (void*)fi);

    
    VMSideExit* exit = snapshot(RECURSIVE_MISMATCH_EXIT);
    LIns* prev_rp = lir->insLoad(LIR_ldp, lirbuf->rp, -int32_t(sizeof(FrameInfo*)));
    guard(true, lir->ins2(LIR_peq, prev_rp, INS_CONSTPTR(fi)), exit);

    



    exit = downSnapshot(fi);

    
    rval_ins = get(&stackval(-1));
    if (isPromoteInt(rval_ins))
        rval_ins = demoteIns(rval_ins);

    

















    lir->insStorei(rval_ins, lirbuf->sp, -treeInfo->nativeStackBase);

    
    lirbuf->sp = lir->ins2(LIR_piadd, lirbuf->sp,
                           lir->insImmWord(-int(downPostSlots) * sizeof(double)));
    lir->insStorei(lirbuf->sp, lirbuf->state, offsetof(InterpState, sp));
    lirbuf->rp = lir->ins2(LIR_piadd, lirbuf->rp,
                           lir->insImmWord(-int(sizeof(FrameInfo*))));
    lir->insStorei(lirbuf->rp, lirbuf->state, offsetof(InterpState, rp));

    RecursiveSlotMap slotMap(*this);
    for (unsigned i = 0; i < downPostSlots; i++)
        slotMap.addSlot(exit->stackType(i));
    slotMap.addSlot(&stackval(-1));
    VisitGlobalSlots(slotMap, cx, *treeInfo->globalSlots);
    if (recursive_pc == (jsbytecode*)fragment->root->ip) {
        debug_only_print0(LC_TMTracer, "Compiling up-recursive loop...\n");
    } else {
        debug_only_print0(LC_TMTracer, "Compiling up-recursive branch...\n");
        exit->exitType = RECURSIVE_UNLINKED_EXIT;
        exit->recursive_pc = recursive_pc;
    }
    JS_ASSERT(treeInfo->recursion != Recursion_Disallowed);
    if (treeInfo->recursion != Recursion_Detected)
        treeInfo->recursion = Recursion_Unwinds;
    return closeLoop(slotMap, exit);
}

class SlurpInfo
{
public:
    unsigned curSlot;
    JSTraceType* typeMap;
    VMSideExit* exit;
    unsigned slurpFailSlot;
};

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::slurpDownFrames(jsbytecode* return_pc)
{
    
    if (cx->fp->argc != cx->fp->fun->nargs)
        RETURN_STOP_A("argc != nargs");

    LIns* argv_ins;
    unsigned frameDepth;
    unsigned downPostSlots;

    JSStackFrame* fp = cx->fp;
    LIns* fp_ins = addName(lir->insLoad(LIR_ldp, cx_ins, offsetof(JSContext, fp)), "fp");

    





    if (!anchor || anchor->exitType != RECURSIVE_SLURP_FAIL_EXIT) {
        fp_ins = addName(lir->insLoad(LIR_ldp, fp_ins, offsetof(JSStackFrame, down)), "downFp");
        fp = fp->down;

        argv_ins = addName(lir->insLoad(LIR_ldp, fp_ins, offsetof(JSStackFrame, argv)), "argv");

        
        if (!anchor || anchor->exitType != RECURSIVE_SLURP_MISMATCH_EXIT) {
            
            guard(false, lir->ins_peq0(fp_ins), RECURSIVE_LOOP_EXIT);

            
            guard(false, lir->ins_peq0(argv_ins), RECURSIVE_LOOP_EXIT);

            





            guard(true,
                  lir->ins2(LIR_peq,
                            addName(lir->insLoad(LIR_ldp,
                                                 fp_ins,
                                                 offsetof(JSStackFrame, script)),
                                    "script"),
                            INS_CONSTPTR(cx->fp->down->script)),
                  RECURSIVE_LOOP_EXIT);
        }

        
        guard(true,
              lir->ins2(LIR_peq,
                        lir->insLoad(LIR_ldp,
                                     addName(lir->insLoad(LIR_ldp, fp_ins, offsetof(JSStackFrame, regs)),
                                             "regs"),
                                     offsetof(JSFrameRegs, pc)),
                        INS_CONSTPTR(return_pc)),
              RECURSIVE_SLURP_MISMATCH_EXIT);

        
        guard(true,
              lir->ins2(LIR_eq,
                        addName(lir->insLoad(LIR_ld, fp_ins, offsetof(JSStackFrame, argc)),
                                "argc"),
                        INS_CONST(cx->fp->argc)),
              MISMATCH_EXIT);

        
        LIns* args[] = { lirbuf->state, cx_ins };
        guard(false, lir->ins_eq0(lir->insCall(&js_PopInterpFrame_ci, args)), MISMATCH_EXIT);

        
        downPostSlots = NativeStackSlots(cx, 1) - NativeStackSlots(cx, 0);
        frameDepth = 1;
    } else {
        
        argv_ins = addName(lir->insLoad(LIR_ldp, fp_ins, offsetof(JSStackFrame, argv)), "argv");

        
        downPostSlots = NativeStackSlots(cx, 0) - 1;
        frameDepth = 0;
    }

    





    unsigned numGlobalSlots = treeInfo->globalSlots->length();
    unsigned safeSlots = NativeStackSlots(cx, frameDepth) + 1 + numGlobalSlots;
    jsbytecode* recursive_pc = return_pc + JSOP_CALL_LENGTH;
    LIns* data = lir->insSkip(sizeof(VMSideExit) + sizeof(JSTraceType) * safeSlots);
    VMSideExit* exit = (VMSideExit*)data->payload();
    memset(exit, 0, sizeof(VMSideExit));
    exit->pc = (jsbytecode*)recursive_pc;
    exit->from = fragment;
    exit->exitType = RECURSIVE_SLURP_FAIL_EXIT;
    exit->numStackSlots = downPostSlots + 1;
    exit->numGlobalSlots = numGlobalSlots;
    exit->sp_adj = ((downPostSlots + 1) * sizeof(double)) - treeInfo->nativeStackBase;
    exit->recursive_pc = recursive_pc;

    



    JSTraceType* typeMap = exit->stackTypeMap();
    jsbytecode* oldpc = cx->fp->regs->pc;
    cx->fp->regs->pc = exit->pc;
    js_CaptureStackTypes(cx, frameDepth, typeMap);
    cx->fp->regs->pc = oldpc;
    typeMap[downPostSlots] = determineSlotType(&stackval(-1));
    if (typeMap[downPostSlots] == TT_INT32 &&
        oracle.isStackSlotUndemotable(cx, downPostSlots, recursive_pc)) {
        typeMap[downPostSlots] = TT_DOUBLE;
    }
    determineGlobalTypes(&typeMap[exit->numStackSlots]);
#if defined JS_JIT_SPEW
    TreevisLogExit(cx, exit);
#endif

    



    if (!anchor || anchor->exitType != RECURSIVE_SLURP_FAIL_EXIT) {
        JS_ASSERT(exit->sp_adj >= int(sizeof(double)));
        ptrdiff_t actRetOffset = exit->sp_adj - sizeof(double);
        LIns* rval = get(&stackval(-1));
        if (typeMap[downPostSlots] == TT_INT32)
            rval = demoteIns(rval);
        lir->insStorei(addName(rval, "rval"), lirbuf->sp, actRetOffset);
    }

    
    SlurpInfo info;
    info.curSlot = 0;
    info.exit = exit;
    info.typeMap = typeMap;
    info.slurpFailSlot = (anchor && anchor->exitType == RECURSIVE_SLURP_FAIL_EXIT) ?
                         anchor->slurpFailSlot : 0;

    
    slurpSlot(lir->insLoad(LIR_ldp, argv_ins, -2 * ptrdiff_t(sizeof(jsval))),
              &fp->argv[-2],
              &info);
    
    slurpSlot(lir->insLoad(LIR_ldp, argv_ins, -1 * ptrdiff_t(sizeof(jsval))),
              &fp->argv[-1],
              &info);
    
    for (unsigned i = 0; i < JS_MAX(fp->argc, fp->fun->nargs); i++)
        slurpSlot(lir->insLoad(LIR_ldp, argv_ins, i * sizeof(jsval)), &fp->argv[i], &info);
    
    slurpSlot(addName(lir->insLoad(LIR_ldp, fp_ins, offsetof(JSStackFrame, argsobj)), "argsobj"),
              &fp->argsobj,
              &info);
    
    LIns* slots_ins = addName(lir->insLoad(LIR_ldp, fp_ins, offsetof(JSStackFrame, slots)),
                              "slots");
    for (unsigned i = 0; i < fp->script->nfixed; i++)
        slurpSlot(lir->insLoad(LIR_ldp, slots_ins, i * sizeof(jsval)), &fp->slots[i], &info);
    
    unsigned nfixed = fp->script->nfixed;
    jsval* stack = StackBase(fp);
    LIns* stack_ins = addName(lir->ins2(LIR_piadd,
                                        slots_ins,
                                        INS_CONSTWORD(nfixed * sizeof(jsval))),
                              "stackBase");
    size_t limit = size_t(fp->regs->sp - StackBase(fp));
    if (anchor && anchor->exitType == RECURSIVE_SLURP_FAIL_EXIT)
        limit--;
    else
        limit -= fp->fun->nargs + 2;
    for (size_t i = 0; i < limit; i++)
        slurpSlot(lir->insLoad(LIR_ldp, stack_ins, i * sizeof(jsval)), &stack[i], &info);

    JS_ASSERT(info.curSlot == downPostSlots);

    
    exit = copy(exit);
    exit->exitType = UNSTABLE_LOOP_EXIT;
#if defined JS_JIT_SPEW
    TreevisLogExit(cx, exit);
#endif

    
    RecursiveSlotMap slotMap(*this);
    for (unsigned i = 0; i < downPostSlots; i++)
        slotMap.addSlot(typeMap[i]);
    slotMap.addSlot(&stackval(-1));
    VisitGlobalSlots(slotMap, cx, *treeInfo->globalSlots);
    debug_only_print0(LC_TMTracer, "Compiling up-recursive slurp...\n");
    exit = copy(exit);
    if (exit->recursive_pc == fragment->root->ip)
        exit->exitType = UNSTABLE_LOOP_EXIT;
    else
        exit->exitType = RECURSIVE_UNLINKED_EXIT;
    debug_only_printf(LC_TMTreeVis, "TREEVIS CHANGEEXIT EXIT=%p TYPE=%s\n", (void*)exit,
                      getExitName(exit->exitType));
    JS_ASSERT(treeInfo->recursion >= Recursion_Unwinds);
    return closeLoop(slotMap, exit);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::downRecursion()
{
    JSStackFrame* fp = cx->fp;
    if ((jsbytecode*)fragment->ip < fp->script->code ||
        (jsbytecode*)fragment->ip >= fp->script->code + fp->script->length) {
        RETURN_STOP_A("inner recursive call must compile first");
    }

    
    int slots = NativeStackSlots(cx, 1) - NativeStackSlots(cx, 0);
    JS_ASSERT(unsigned(slots) == NativeStackSlots(cx, 1) - fp->argc - 2 - fp->script->nfixed - 1);

    
    JS_ASSERT(treeInfo->maxNativeStackSlots >= treeInfo->nativeStackBase / sizeof(double));
    int guardSlots = slots + treeInfo->maxNativeStackSlots -
                     treeInfo->nativeStackBase / sizeof(double);
    LIns* sp_top = lir->ins2(LIR_piadd, lirbuf->sp, lir->insImmWord(guardSlots * sizeof(double)));
    guard(true, lir->ins2(LIR_plt, sp_top, eos_ins), OOM_EXIT);

    
    LIns* rp_top = lir->ins2(LIR_piadd, lirbuf->rp, lir->insImmWord(sizeof(FrameInfo*)));
    guard(true, lir->ins2(LIR_plt, rp_top, eor_ins), OOM_EXIT);

    
    lirbuf->sp = lir->ins2(LIR_piadd, lirbuf->sp, lir->insImmWord(slots * sizeof(double)));
    lir->insStorei(lirbuf->sp, lirbuf->state, offsetof(InterpState, sp));
    lirbuf->rp = lir->ins2(LIR_piadd, lirbuf->rp, lir->insImmWord(sizeof(FrameInfo*)));
    lir->insStorei(lirbuf->rp, lirbuf->state, offsetof(InterpState, rp));
    --callDepth;
    clearFrameSlotsFromCache();

    








    VMSideExit* exit;
    if ((jsbytecode*)fragment->root->ip == fp->script->code)
        exit = snapshot(UNSTABLE_LOOP_EXIT);
    else
        exit = snapshot(RECURSIVE_UNLINKED_EXIT);
    exit->recursive_pc = fp->script->code;
    debug_only_print0(LC_TMTracer, "Compiling down-recursive function call.\n");
    JS_ASSERT(treeInfo->recursion != Recursion_Disallowed);
    treeInfo->recursion = Recursion_Detected;
    return closeLoop(exit);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::slurpInt32Slot(LIns* val_ins, jsval* vp, VMSideExit* exit)
{
    guard(true,
          lir->ins2(LIR_or,
                    lir->ins2(LIR_peq,
                              lir->ins2(LIR_piand, val_ins, INS_CONSTWORD(JSVAL_TAGMASK)),
                              INS_CONSTWORD(JSVAL_DOUBLE)),
                    lir->ins2(LIR_peq,
                              lir->ins2(LIR_piand, val_ins, INS_CONSTWORD(1)),
                              INS_CONSTWORD(1))),
          exit);
    LIns* space = lir->insAlloc(sizeof(int32));
    LIns* args[] = { space, val_ins };
    LIns* result = lir->insCall(&js_TryUnboxInt32_ci, args);
    guard(false, lir->ins_eq0(result), exit);
    LIns* int32_ins = lir->insLoad(LIR_ld, space, 0);
    return int32_ins;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::slurpDoubleSlot(LIns* val_ins, jsval* vp, VMSideExit* exit)
{
    guard(true,
          lir->ins2(LIR_or,
                    lir->ins2(LIR_peq,
                              lir->ins2(LIR_piand, val_ins, INS_CONSTWORD(JSVAL_TAGMASK)),
                              INS_CONSTWORD(JSVAL_DOUBLE)),
                    lir->ins2(LIR_peq,
                              lir->ins2(LIR_piand, val_ins, INS_CONSTWORD(1)),
                              INS_CONSTWORD(1))),
          exit);
    LIns* args[] = { val_ins };
    LIns* dbl_ins = lir->insCall(&js_UnboxDouble_ci, args);
    return dbl_ins;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::slurpBoolSlot(LIns* val_ins, jsval* vp, VMSideExit* exit)
{
    guard(true,
          lir->ins2(LIR_eq,
                    lir->ins2(LIR_piand, val_ins, INS_CONSTWORD(JSVAL_TAGMASK)),
                    INS_CONSTWORD(JSVAL_SPECIAL)),
          exit);
    LIns* bool_ins = lir->ins2(LIR_pilsh, val_ins, INS_CONSTWORD(JSVAL_TAGBITS));
    bool_ins = p2i(bool_ins);
    return bool_ins;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::slurpStringSlot(LIns* val_ins, jsval* vp, VMSideExit* exit)
{
    guard(true,
          lir->ins2(LIR_eq,
                    lir->ins2(LIR_piand, val_ins, INS_CONSTWORD(JSVAL_TAGMASK)),
                    INS_CONSTWORD(JSVAL_STRING)),
          exit);
    LIns* str_ins = lir->ins2(LIR_piand, val_ins, INS_CONSTWORD(~JSVAL_TAGMASK));
    return str_ins;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::slurpNullSlot(LIns* val_ins, jsval* vp, VMSideExit* exit)
{
    guard(true, lir->ins_peq0(val_ins), exit);
    return val_ins;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::slurpObjectSlot(LIns* val_ins, jsval* vp, VMSideExit* exit)
{
    
    guard(false, lir->ins_peq0(val_ins), exit);

    
    guard(true,
          lir->ins_peq0(lir->ins2(LIR_piand, val_ins, INS_CONSTWORD(JSVAL_TAGMASK))),
          exit);

    
    guard(false,
          lir->ins2(LIR_peq,
                    lir->ins2(LIR_piand,
                              lir->insLoad(LIR_ldp, val_ins, offsetof(JSObject, classword)),
                              INS_CONSTWORD(~JSSLOT_CLASS_MASK_BITS)),
                    INS_CONSTPTR(&js_FunctionClass)),
          exit);
    return val_ins;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::slurpFunctionSlot(LIns* val_ins, jsval* vp, VMSideExit* exit)
{
    
    guard(false, lir->ins_peq0(val_ins), exit);

    
    guard(true,
          lir->ins_peq0(lir->ins2(LIR_piand, val_ins, INS_CONSTWORD(JSVAL_TAGMASK))),
          exit);

    
    guard(true,
          lir->ins2(LIR_peq,
                    lir->ins2(LIR_piand,
                              lir->insLoad(LIR_ldp, val_ins, offsetof(JSObject, classword)),
                              INS_CONSTWORD(~JSSLOT_CLASS_MASK_BITS)),
                    INS_CONSTPTR(&js_FunctionClass)),
          exit);
    return val_ins;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::slurpSlot(LIns* val_ins, jsval* vp, VMSideExit* exit)
{
    switch (exit->slurpType)
    {
    case TT_PSEUDOBOOLEAN:
        return slurpBoolSlot(val_ins, vp, exit);
    case TT_INT32:
        return slurpInt32Slot(val_ins, vp, exit);
    case TT_DOUBLE:
        return slurpDoubleSlot(val_ins, vp, exit);
    case TT_STRING:
        return slurpStringSlot(val_ins, vp, exit);
    case TT_NULL:
        return slurpNullSlot(val_ins, vp, exit);
    case TT_OBJECT:
        return slurpObjectSlot(val_ins, vp, exit);
    case TT_FUNCTION:
        return slurpFunctionSlot(val_ins, vp, exit);
    default:
        JS_NOT_REACHED("invalid type in typemap");
        return NULL;
    }
}

JS_REQUIRES_STACK void
TraceRecorder::slurpSlot(LIns* val_ins, jsval* vp, SlurpInfo* info)
{
    
    if (info->curSlot < info->slurpFailSlot) {
        info->curSlot++;
        return;
    }
    VMSideExit* exit = copy(info->exit);
    exit->slurpFailSlot = info->curSlot;
    exit->slurpType = info->typeMap[info->curSlot];

#if defined DEBUG
    
    JS_ASSERT_IF(anchor && anchor->exitType == RECURSIVE_SLURP_FAIL_EXIT &&
                 info->curSlot == info->slurpFailSlot,
                 anchor->slurpType != exit->slurpType);
#endif

    LIns* val = slurpSlot(val_ins, vp, exit);
    lir->insStorei(val,
                   lirbuf->sp,
                   -treeInfo->nativeStackBase + ptrdiff_t(info->curSlot) * sizeof(double));
    info->curSlot++;
}

