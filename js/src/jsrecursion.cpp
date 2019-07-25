







































#include "jsapi.h"

class RecursiveSlotMap : public SlotMap
{
  protected:
    unsigned downPostSlots;
    LIns *rval_ins;

  public:
    RecursiveSlotMap(TraceRecorder& rec, unsigned downPostSlots, LIns* rval_ins)
      : SlotMap(rec), downPostSlots(downPostSlots), rval_ins(rval_ins)
    {
    }

    JS_REQUIRES_STACK void
    adjustTypes()
    {
        
        if (slots[downPostSlots].lastCheck == TypeCheck_Demote)
            rval_ins = mRecorder.lir->ins1(LIR_i2d, rval_ins);
        
        for (unsigned i = downPostSlots + 1; i < slots.length(); i++)
            adjustType(slots[i]);
    }

    JS_REQUIRES_STACK void
    adjustTail(TypeConsensus consensus)
    {
        




        ptrdiff_t retOffset = downPostSlots * sizeof(double) -
                              mRecorder.tree->nativeStackBase;
        mRecorder.lir->insStore(mRecorder.addName(rval_ins, "rval_ins"),
                                 mRecorder.lirbuf->sp, retOffset, ACCSET_STACK);
    }
};

class UpRecursiveSlotMap : public RecursiveSlotMap
{
  public:
    UpRecursiveSlotMap(TraceRecorder& rec, unsigned downPostSlots, LIns* rval_ins)
      : RecursiveSlotMap(rec, downPostSlots, rval_ins)
    {
    }

    JS_REQUIRES_STACK void
    adjustTail(TypeConsensus consensus)
    {
        LirBuffer* lirbuf = mRecorder.lirbuf;
        LirWriter* lir = mRecorder.lir;

        


















        lir->insStore(rval_ins, lirbuf->sp, -mRecorder.tree->nativeStackBase, ACCSET_STACK);

        lirbuf->sp = lir->ins2(LIR_addp, lirbuf->sp,
                               lir->insImmWord(-int(downPostSlots) * sizeof(double)));
        lir->insStore(lirbuf->sp, lirbuf->state, offsetof(TracerState, sp), ACCSET_OTHER);
        lirbuf->rp = lir->ins2(LIR_addp, lirbuf->rp,
                               lir->insImmWord(-int(sizeof(FrameInfo*))));
        lir->insStore(lirbuf->rp, lirbuf->state, offsetof(TracerState, rp), ACCSET_OTHER);
    }
};

#if defined DEBUG
JS_REQUIRES_STACK void
TraceRecorder::assertDownFrameIsConsistent(VMSideExit* anchor, FrameInfo* fi)
{
    JS_ASSERT(anchor->recursive_down);
    JS_ASSERT(anchor->recursive_down->callerHeight == fi->callerHeight);

    unsigned downPostSlots = fi->callerHeight;
    JSValueType* typeMap = fi->get_typemap();

    captureStackTypes(1, typeMap);
    const JSValueType* m1 = anchor->recursive_down->get_typemap();
    for (unsigned i = 0; i < downPostSlots; i++) {
        if (m1[i] == typeMap[i])
            continue;
        if ((typeMap[i] == JSVAL_TYPE_INT32 && m1[i] == JSVAL_TYPE_DOUBLE) ||
            (typeMap[i] == JSVAL_TYPE_DOUBLE && m1[i] == JSVAL_TYPE_INT32)) {
            continue;
        }
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
    unsigned ngslots = tree->globalSlots->length();
    unsigned exitTypeMapLen = downPostSlots + 1 + ngslots;
    JSValueType* exitTypeMap = (JSValueType*)alloca(sizeof(JSValueType) * exitTypeMapLen);
    JSValueType* typeMap = downFrame->get_typemap();


    
    for (unsigned i = 0; i < downPostSlots; i++)
        exitTypeMap[i] = typeMap[i];

    
    JS_ASSERT_IF(*cx->regs->pc != JSOP_RETURN, *cx->regs->pc == JSOP_STOP);
    if (*cx->regs->pc == JSOP_RETURN)
        exitTypeMap[downPostSlots] = determineSlotType(&stackval(-1));
    else
        exitTypeMap[downPostSlots] = JSVAL_TYPE_UNDEFINED;

    
    determineGlobalTypes(&exitTypeMap[downPostSlots + 1]);

    VMSideExit* exit = (VMSideExit*)
        traceMonitor->traceAlloc->alloc(sizeof(VMSideExit) + sizeof(JSValueType) * exitTypeMapLen);

    PodZero(exit);
    exit->from = fragment;
    exit->calldepth = 0;
    JS_ASSERT(unsigned(exit->calldepth) == callDepth);
    exit->numGlobalSlots = ngslots;
    exit->numStackSlots = downPostSlots + 1;
    exit->numStackSlotsBelowCurrentFrame = cx->fp->down->argv ?
        nativeStackOffset(&cx->fp->argv[-2]) / sizeof(double) : 0;
    exit->exitType = UNSTABLE_LOOP_EXIT;
    exit->block = cx->fp->down->maybeBlockChain();
    exit->pc = downFrame->pc + JSOP_CALL_LENGTH;
    exit->imacpc = NULL;
    exit->sp_adj = ((downPostSlots + 1) * sizeof(double)) - tree->nativeStackBase;
    exit->rp_adj = exit->calldepth * sizeof(FrameInfo*);
    exit->nativeCalleeWord = 0;
    exit->lookupFlags = js_InferFlags(cx, 0);
    memcpy(exit->fullTypeMap(), exitTypeMap, sizeof(JSValueType) * exitTypeMapLen);
#if defined JS_JIT_SPEW
    TreevisLogExit(cx, exit);
#endif
    return exit;
}

static JS_REQUIRES_STACK Value *
DownFrameSP(JSContext *cx)
{
    FrameRegsIter i(cx);
    ++i;
    JS_ASSERT(i.fp() == cx->fp->down);
    return i.sp();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::upRecursion()
{
    JS_ASSERT((JSOp)*cx->fp->down->savedPC == JSOP_CALL);
    JS_ASSERT(js_CodeSpec[js_GetOpcode(cx, cx->fp->down->script,
              cx->fp->down->savedPC)].length == JSOP_CALL_LENGTH);

    JS_ASSERT(callDepth == 0);

    




    if (anchor && (anchor->exitType == RECURSIVE_EMPTY_RP_EXIT ||
        anchor->exitType == RECURSIVE_SLURP_MISMATCH_EXIT ||
        anchor->exitType == RECURSIVE_SLURP_FAIL_EXIT)) {
        return slurpDownFrames(cx->fp->down->savedPC);
    }

    jsbytecode* return_pc = cx->fp->down->savedPC;
    jsbytecode* recursive_pc = return_pc + JSOP_CALL_LENGTH;

    












    unsigned totalSlots = NativeStackSlots(cx, 1);
    unsigned downPostSlots = totalSlots - NativeStackSlots(cx, 0);
    FrameInfo* fi = (FrameInfo*)alloca(sizeof(FrameInfo) + totalSlots * sizeof(JSValueType));
    fi->block = NULL;
    fi->pc = (jsbytecode*)return_pc;
    fi->imacpc = NULL;

    



    fi->spdist = DownFrameSP(cx) - cx->fp->down->slots();
    JS_ASSERT(cx->fp->argc == cx->fp->down->argc);
    fi->set_argc(uint16(cx->fp->argc), false);
    fi->callerHeight = downPostSlots;
    fi->callerArgc = cx->fp->down->argc;

    if (anchor && anchor->exitType == RECURSIVE_MISMATCH_EXIT) {
        




#if defined DEBUG
        assertDownFrameIsConsistent(anchor, fi);
#endif
        fi = anchor->recursive_down;
    } else if (recursive_pc != fragment->root->ip) {
        



        captureStackTypes(1, fi->get_typemap());
    } else {
        
        JS_ASSERT(tree->nStackTypes == downPostSlots + 1);
        JSValueType* typeMap = fi->get_typemap();
        for (unsigned i = 0; i < downPostSlots; i++)
            typeMap[i] = tree->typeMap[i];
    }

    fi = traceMonitor->frameCache->memoize(fi);

    



    if (!anchor || anchor->exitType != RECURSIVE_MISMATCH_EXIT) {
        VMSideExit* exit = snapshot(RECURSIVE_EMPTY_RP_EXIT);

        
        guard(true,
              lir->ins2(LIR_gep, lirbuf->rp,
                        lir->ins2(LIR_addp,
                                  lir->insLoad(LIR_ldp, lirbuf->state,
                                               offsetof(TracerState, sor), ACCSET_OTHER),
                                  INS_CONSTWORD(sizeof(FrameInfo*)))),
              exit);
    }

    debug_only_printf(LC_TMRecorder, "guardUpRecursive fragment->root=%p fi=%p\n", (void*)fragment->root, (void*)fi);

    
    VMSideExit* exit = snapshot(RECURSIVE_MISMATCH_EXIT);
    LIns* prev_rp = lir->insLoad(LIR_ldp, lirbuf->rp, -int32_t(sizeof(FrameInfo*)), ACCSET_RSTACK);
    guard(true, lir->ins2(LIR_eqp, prev_rp, INS_CONSTPTR(fi)), exit);

    



    exit = downSnapshot(fi);

    LIns* rval_ins;
    if (*cx->regs->pc == JSOP_RETURN) {
        JS_ASSERT(!anchor || anchor->exitType != RECURSIVE_SLURP_FAIL_EXIT);
        rval_ins = get(&stackval(-1));
        JS_ASSERT(rval_ins);
    } else {
        rval_ins = INS_UNDEFINED();
    }

    JSValueType returnType = exit->stackTypeMap()[downPostSlots];
    if (returnType == JSVAL_TYPE_INT32) {
        JS_ASSERT(*cx->regs->pc == JSOP_RETURN);
        JS_ASSERT(determineSlotType(&stackval(-1)) == JSVAL_TYPE_INT32);
        JS_ASSERT(isPromoteInt(rval_ins));
        rval_ins = demote(lir, rval_ins);
    }

    UpRecursiveSlotMap slotMap(*this, downPostSlots, rval_ins);
    for (unsigned i = 0; i < downPostSlots; i++)
        slotMap.addSlot(exit->stackType(i));
    if (*cx->regs->pc == JSOP_RETURN)
        slotMap.addSlot(&stackval(-1));
    else
        slotMap.addSlot(JSVAL_TYPE_UNDEFINED);
    VisitGlobalSlots(slotMap, cx, *tree->globalSlots);
    if (recursive_pc == (jsbytecode*)fragment->root->ip) {
        debug_only_print0(LC_TMTracer, "Compiling up-recursive loop...\n");
    } else {
        debug_only_print0(LC_TMTracer, "Compiling up-recursive branch...\n");
        exit->exitType = RECURSIVE_UNLINKED_EXIT;
        exit->recursive_pc = recursive_pc;
    }
    JS_ASSERT(tree->recursion != Recursion_Disallowed);
    if (tree->recursion != Recursion_Detected)
        tree->recursion = Recursion_Unwinds;
    return closeLoop(slotMap, exit);
}

class SlurpInfo
{
public:
    unsigned curSlot;
    JSValueType* typeMap;
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

    FrameRegsIter i(cx);
    LIns* fp_ins =
        addName(lir->insLoad(LIR_ldp, cx_ins, offsetof(JSContext, fp), ACCSET_OTHER), "fp");

    





    if (!anchor || anchor->exitType != RECURSIVE_SLURP_FAIL_EXIT) {
        fp_ins = addName(lir->insLoad(LIR_ldp, fp_ins, offsetof(JSStackFrame, down), ACCSET_OTHER),
                         "downFp");
        ++i;

        argv_ins = addName(lir->insLoad(LIR_ldp, fp_ins, offsetof(JSStackFrame, argv), ACCSET_OTHER),
                           "argv");

        
        if (!anchor || anchor->exitType != RECURSIVE_SLURP_MISMATCH_EXIT) {
            
            guard(false, lir->insEqP_0(fp_ins), RECURSIVE_LOOP_EXIT);

            
            guard(false, lir->insEqP_0(argv_ins), RECURSIVE_LOOP_EXIT);

            





            guard(true,
                  lir->ins2(LIR_eqp,
                            addName(lir->insLoad(LIR_ldp, fp_ins,
                                                 offsetof(JSStackFrame, script), ACCSET_OTHER),
                                    "script"),
                            INS_CONSTPTR(cx->fp->down->script)),
                  RECURSIVE_LOOP_EXIT);
        }

        
        guard(true,
              lir->ins2(LIR_eqp,
                        addName(lir->insLoad(LIR_ldp, fp_ins, offsetof(JSStackFrame, savedPC),  
                                             ACCSET_OTHER),
                                "savedPC"),
                        INS_CONSTPTR(return_pc)),
              RECURSIVE_SLURP_MISMATCH_EXIT);

        
        guard(true,
              lir->ins2(LIR_eqi,
                        addName(lir->insLoad(LIR_ldi, fp_ins, offsetof(JSStackFrame, argc),
                                             ACCSET_OTHER),
                                "argc"),
                        INS_CONST(cx->fp->argc)),
              MISMATCH_EXIT);

        
        LIns* args[] = { lirbuf->state, cx_ins };
        guard(false, lir->insEqI_0(lir->insCall(&js_PopInterpFrame_ci, args)), MISMATCH_EXIT);

        
        downPostSlots = NativeStackSlots(cx, 1) - NativeStackSlots(cx, 0);
        frameDepth = 1;
    } else {
        
        argv_ins = addName(lir->insLoad(LIR_ldp, fp_ins, offsetof(JSStackFrame, argv), ACCSET_OTHER),
                           "argv");

        
        downPostSlots = NativeStackSlots(cx, 0) - 1;
        frameDepth = 0;
    }

    





    unsigned numGlobalSlots = tree->globalSlots->length();
    unsigned safeSlots = NativeStackSlots(cx, frameDepth) + 1 + numGlobalSlots;
    jsbytecode* recursive_pc = return_pc + JSOP_CALL_LENGTH;
    VMSideExit* exit = (VMSideExit*)
        traceMonitor->traceAlloc->alloc(sizeof(VMSideExit) + sizeof(JSValueType) * safeSlots);
    PodZero(exit);
    exit->pc = (jsbytecode*)recursive_pc;
    exit->from = fragment;
    exit->exitType = RECURSIVE_SLURP_FAIL_EXIT;
    exit->numStackSlots = downPostSlots + 1;
    exit->numGlobalSlots = numGlobalSlots;
    exit->sp_adj = ((downPostSlots + 1) * sizeof(double)) - tree->nativeStackBase;
    exit->recursive_pc = recursive_pc;

    



    JSValueType* typeMap = exit->stackTypeMap();
    jsbytecode* oldpc = cx->regs->pc;
    cx->regs->pc = exit->pc;
    captureStackTypes(frameDepth, typeMap);
    cx->regs->pc = oldpc;
    if (!anchor || anchor->exitType != RECURSIVE_SLURP_FAIL_EXIT) {
        JS_ASSERT_IF(*cx->regs->pc != JSOP_RETURN, *cx->regs->pc == JSOP_STOP);
        if (*cx->regs->pc == JSOP_RETURN)
            typeMap[downPostSlots] = determineSlotType(&stackval(-1));
        else
            typeMap[downPostSlots] = JSVAL_TYPE_UNDEFINED;
    } else {
        typeMap[downPostSlots] = anchor->stackTypeMap()[anchor->numStackSlots - 1];
    }
    determineGlobalTypes(&typeMap[exit->numStackSlots]);
#if defined JS_JIT_SPEW
    TreevisLogExit(cx, exit);
#endif

    








    LIns* rval_ins;
    intptr_t offset = exit->sp_adj - sizeof(double);
    JSValueType returnType = exit->stackTypeMap()[downPostSlots];

    if (!anchor || anchor->exitType != RECURSIVE_SLURP_FAIL_EXIT) {
        



        JSOp op = JSOp(*cx->regs->pc);
        JS_ASSERT(op == JSOP_RETURN || op == JSOP_STOP);

        if (op == JSOP_RETURN) {
            rval_ins = get(&stackval(-1));
            if (returnType == JSVAL_TYPE_INT32) {
                JS_ASSERT(determineSlotType(&stackval(-1)) == JSVAL_TYPE_INT32);
                JS_ASSERT(isPromoteInt(rval_ins));
                rval_ins = demote(lir, rval_ins);
            }
        } else {
            rval_ins = INS_UNDEFINED();
        }

        



        lir->insStore(rval_ins, lirbuf->sp, offset, ACCSET_STACK);
    } else {
        switch (returnType)
        {
          case JSVAL_TYPE_BOOLEAN:
          case JSVAL_TYPE_UNDEFINED:
          case JSVAL_TYPE_INT32:
            rval_ins = lir->insLoad(LIR_ldi, lirbuf->sp, offset, ACCSET_STACK);
            break;
          case JSVAL_TYPE_DOUBLE:
            rval_ins = lir->insLoad(LIR_ldd, lirbuf->sp, offset, ACCSET_STACK);
            break;
          case JSVAL_TYPE_FUNOBJ:
          case JSVAL_TYPE_NONFUNOBJ:
          case JSVAL_TYPE_STRING:
          case JSVAL_TYPE_NULL:
            rval_ins = lir->insLoad(LIR_ldp, lirbuf->sp, offset, ACCSET_STACK);
            break;
          default:
            JS_NOT_REACHED("unknown type");
            RETURN_STOP_A("unknown type"); 
        }
    }

    
    SlurpInfo info;
    info.curSlot = 0;
    info.exit = exit;
    info.typeMap = typeMap;
    info.slurpFailSlot = (anchor && anchor->exitType == RECURSIVE_SLURP_FAIL_EXIT) ?
                         anchor->slurpFailSlot : 0;

    JSStackFrame *const fp = i.fp();

    
    slurpSlot(argv_ins, -2 * ptrdiff_t(sizeof(Value)), &fp->argv[-2], &info);
    
    slurpSlot(argv_ins, -1 * ptrdiff_t(sizeof(Value)), &fp->argv[-1], &info);
    
    for (unsigned i = 0; i < JS_MAX(fp->argc, fp->fun->nargs); i++)
        slurpSlot(argv_ins, i * sizeof(Value), &fp->argv[i], &info);
    
    slurpFrameObjPtrSlot(fp_ins, JSStackFrame::offsetArgsObj(), fp->addressArgsObj(), &info);
    
    slurpFrameObjPtrSlot(fp_ins, JSStackFrame::offsetScopeChain(), fp->addressScopeChain(), &info);
    
    LIns* slots_ins = addName(lir->ins2(LIR_addp, fp_ins, INS_CONSTWORD(sizeof(JSStackFrame))),
                              "slots");
    for (unsigned i = 0; i < fp->script->nfixed; i++)
        slurpSlot(slots_ins, i * sizeof(Value), &fp->slots()[i], &info);
    
    unsigned nfixed = fp->script->nfixed;
    Value* stack = fp->base();
    LIns* stack_ins = addName(lir->ins2(LIR_addp,
                                        slots_ins,
                                        INS_CONSTWORD(nfixed * sizeof(Value))),
                              "stackBase");

    size_t limit = size_t(i.sp() - fp->base());
    if (anchor && anchor->exitType == RECURSIVE_SLURP_FAIL_EXIT)
        limit--;
    else
        limit -= fp->fun->nargs + 2;
    for (size_t i = 0; i < limit; i++)
        slurpSlot(stack_ins, i * sizeof(Value), &stack[i], &info);

    JS_ASSERT(info.curSlot == downPostSlots);

    
    exit = copy(exit);
    exit->exitType = UNSTABLE_LOOP_EXIT;
#if defined JS_JIT_SPEW
    TreevisLogExit(cx, exit);
#endif

    RecursiveSlotMap slotMap(*this, downPostSlots, rval_ins);
    for (unsigned i = 0; i < downPostSlots; i++)
        slotMap.addSlot(typeMap[i]);
    if (*cx->regs->pc == JSOP_RETURN)
        slotMap.addSlot(&stackval(-1), typeMap[downPostSlots]);
    else
        slotMap.addSlot(JSVAL_TYPE_UNDEFINED);
    VisitGlobalSlots(slotMap, cx, *tree->globalSlots);
    debug_only_print0(LC_TMTracer, "Compiling up-recursive slurp...\n");
    exit = copy(exit);
    if (exit->recursive_pc == fragment->root->ip)
        exit->exitType = UNSTABLE_LOOP_EXIT;
    else
        exit->exitType = RECURSIVE_UNLINKED_EXIT;
    debug_only_printf(LC_TMTreeVis, "TREEVIS CHANGEEXIT EXIT=%p TYPE=%s\n", (void*)exit,
                      getExitName(exit->exitType));
    JS_ASSERT(tree->recursion >= Recursion_Unwinds);
    return closeLoop(slotMap, exit);
}

class ImportFrameSlotsVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
public:
    ImportFrameSlotsVisitor(TraceRecorder &recorder) : mRecorder(recorder)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, JSStackFrame* fp) {
        
        for (size_t i = 0; i < count; ++i)
            mRecorder.get(vp++);
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject **p, JSStackFrame* fp) {
        
        return visitStackSlots((Value *)p, 1, fp);
    }
};

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::downRecursion()
{
    JSStackFrame* fp = cx->fp;
    if ((jsbytecode*)fragment->ip < fp->script->code ||
        (jsbytecode*)fragment->ip >= fp->script->code + fp->script->length) {
        RETURN_STOP_A("inner recursive call must compile first");
    }

    
    int slots = NativeStackSlots(cx, 1) - NativeStackSlots(cx, 0);
    JS_ASSERT(unsigned(slots) == NativeStackSlots(cx, 1) - fp->argc - 2 - fp->script->nfixed - 2);

    
    JS_ASSERT(tree->maxNativeStackSlots >= tree->nativeStackBase / sizeof(double));
    int guardSlots = slots + tree->maxNativeStackSlots -
                     tree->nativeStackBase / sizeof(double);
    LIns* sp_top = lir->ins2(LIR_addp, lirbuf->sp, lir->insImmWord(guardSlots * sizeof(double)));
    guard(true, lir->ins2(LIR_ltp, sp_top, eos_ins), OOM_EXIT);

    
    LIns* rp_top = lir->ins2(LIR_addp, lirbuf->rp,
                             lir->insImmWord((tree->maxCallDepth + 1) * sizeof(FrameInfo*)));
    guard(true, lir->ins2(LIR_ltp, rp_top, eor_ins), OOM_EXIT);

    






    ImportFrameSlotsVisitor visitor(*this);
    VisitStackSlots(visitor, cx, callDepth);

    
    lirbuf->sp = lir->ins2(LIR_addp, lirbuf->sp, lir->insImmWord(slots * sizeof(double)));
    lir->insStore(lirbuf->sp, lirbuf->state, offsetof(TracerState, sp), ACCSET_OTHER);
    lirbuf->rp = lir->ins2(LIR_addp, lirbuf->rp, lir->insImmWord(sizeof(FrameInfo*)));
    lir->insStore(lirbuf->rp, lirbuf->state, offsetof(TracerState, rp), ACCSET_OTHER);
    --callDepth;
    clearCurrentFrameSlotsFromTracker(nativeFrameTracker);

    








    VMSideExit* exit;
    if ((jsbytecode*)fragment->root->ip == fp->script->code)
        exit = snapshot(UNSTABLE_LOOP_EXIT);
    else
        exit = snapshot(RECURSIVE_UNLINKED_EXIT);
    exit->recursive_pc = fp->script->code;
    debug_only_print0(LC_TMTracer, "Compiling down-recursive function call.\n");
    JS_ASSERT(tree->recursion != Recursion_Disallowed);
    tree->recursion = Recursion_Detected;
    return closeLoop(exit);
}

#if JS_BITS_PER_WORD == 32
JS_REQUIRES_STACK inline LIns*
TraceRecorder::slurpDoubleSlot(LIns* addr_ins, ptrdiff_t offset, VMSideExit* exit)
{
    LIns* tag_ins = lir->insLoad(LIR_ldi, addr_ins, offset + sTagOffset, ACCSET_OTHER);
    return unbox_number_as_double(addr_ins, offset, tag_ins, exit, ACCSET_OTHER);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::slurpObjectSlot(LIns* addr_ins, ptrdiff_t offset, JSValueType type, VMSideExit* exit)
{
    LIns* tag_ins = lir->insLoad(LIR_ldi, addr_ins, offset + sTagOffset, ACCSET_OTHER);
    return unbox_object(addr_ins, offset, tag_ins, type, exit, ACCSET_OTHER);
}

JS_REQUIRES_STACK inline LIns*
TraceRecorder::slurpNonDoubleObjectSlot(LIns* addr_ins, ptrdiff_t offset, JSValueType type, VMSideExit* exit)
{
    LIns* tag_ins = lir->insLoad(LIR_ldi, addr_ins, offset + sTagOffset, ACCSET_OTHER);
    return unbox_non_double_object(addr_ins, offset, tag_ins, type, exit, ACCSET_OTHER);
}
#elif JS_BITS_PER_WORD == 64
JS_REQUIRES_STACK inline LIns*
TraceRecorder::slurpDoubleSlot(LIns* addr_ins, ptrdiff_t offset, VMSideExit* exit)
{
    LIns* v_ins = lir->insLoad(LIR_ldq, addr_ins, offset, ACCSET_OTHER);
    return unbox_number_as_double(v_ins, exit);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::slurpObjectSlot(LIns* addr_ins, ptrdiff_t offset, JSValueType type, VMSideExit* exit)
{
    LIns* v_ins = lir->insLoad(LIR_ldq, addr_ins, offset, ACCSET_OTHER);
    return unbox_object(v_ins, type, exit);
}

JS_REQUIRES_STACK inline LIns*
TraceRecorder::slurpNonDoubleObjectSlot(LIns* addr_ins, ptrdiff_t offset, JSValueType type, VMSideExit* exit)
{
    LIns* v_ins = lir->insLoad(LIR_ldq, addr_ins, offset, ACCSET_OTHER);
    return unbox_non_double_object(v_ins, type, exit);
}
#endif

JS_REQUIRES_STACK inline LIns*
TraceRecorder::slurpSlot(LIns* addr_ins, ptrdiff_t offset, Value* vp, VMSideExit* exit)
{
    if (exit->slurpType == JSVAL_TYPE_DOUBLE)
        return slurpDoubleSlot(addr_ins, offset, exit);
    if (exit->slurpType == JSVAL_TYPE_FUNOBJ || exit->slurpType == JSVAL_TYPE_NONFUNOBJ)
        return slurpObjectSlot(addr_ins, offset, exit->slurpType, exit);
    JSValueType type = exit->slurpType;
    return slurpNonDoubleObjectSlot(addr_ins, offset, type, exit);
}

JS_REQUIRES_STACK void
TraceRecorder::slurpSlot(LIns* addr_ins, ptrdiff_t offset, Value* vp, SlurpInfo* info)
{
    
    if (info->curSlot < info->slurpFailSlot) {
        info->curSlot++;
        return;
    }
    VMSideExit* exit = copy(info->exit);
    exit->slurpFailSlot = info->curSlot;
    exit->slurpType = info->typeMap[info->curSlot];

    
    JS_ASSERT_IF(anchor && anchor->exitType == RECURSIVE_SLURP_FAIL_EXIT &&
                 info->curSlot == info->slurpFailSlot,
                 anchor->slurpType != exit->slurpType);

    LIns* val = slurpSlot(addr_ins, offset, vp, exit);
    lir->insStore(val,
                  lirbuf->sp,
                  -tree->nativeStackBase + ptrdiff_t(info->curSlot) * sizeof(double),
                  ACCSET_STACK);
    info->curSlot++;
}

JS_REQUIRES_STACK void
TraceRecorder::slurpFrameObjPtrSlot(LIns* addr_ins, ptrdiff_t offset, JSObject** p, SlurpInfo* info)
{
    
    if (info->curSlot < info->slurpFailSlot) {
        info->curSlot++;
        return;
    }
    VMSideExit* exit = copy(info->exit);
    exit->slurpFailSlot = info->curSlot;
    exit->slurpType = info->typeMap[info->curSlot];

    
    JS_ASSERT_IF(anchor && anchor->exitType == RECURSIVE_SLURP_FAIL_EXIT &&
                 info->curSlot == info->slurpFailSlot,
                 anchor->slurpType != exit->slurpType);

    LIns *val;
    LIns *ptr_val = lir->insLoad(LIR_ldp, addr_ins, offset, ACCSET_OTHER);
    LIns *ptr_is_null_ins = lir->insEqP_0(ptr_val);
    if (exit->slurpType == JSVAL_TYPE_NULL) {
        guard(true, ptr_is_null_ins, exit);
        val = INS_NULL();
    } else {
        JS_ASSERT(exit->slurpType == JSVAL_TYPE_NONFUNOBJ);
        guard(false, ptr_is_null_ins, exit);
        val = ptr_val;
    }

    lir->insStore(val,
                  lirbuf->sp,
                  -tree->nativeStackBase + ptrdiff_t(info->curSlot) * sizeof(double),
                  ACCSET_STACK);
    info->curSlot++;
}
