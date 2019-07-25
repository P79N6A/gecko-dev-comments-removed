









































#if JS_THREADED_INTERP
  interrupt:
#else 
  case -1:
    JS_ASSERT(switchMask == -1);
#endif 
    {
        bool moreInterrupts = false;
        JSInterruptHook hook = cx->debugHooks->interruptHook;
        if (hook) {
#ifdef JS_TRACER
            if (TRACE_RECORDER(cx))
                AbortRecording(cx, "interrupt hook");
#endif
            Value rval;
            switch (hook(cx, script, regs.pc, Jsvalify(&rval),
                         cx->debugHooks->interruptHookData)) {
              case JSTRAP_ERROR:
                goto error;
              case JSTRAP_CONTINUE:
                break;
              case JSTRAP_RETURN:
                fp->rval = rval;
                interpReturnOK = JS_TRUE;
                goto forced_return;
              case JSTRAP_THROW:
                cx->throwing = JS_TRUE;
                cx->exception = rval;
                goto error;
              default:;
            }
            moreInterrupts = true;
        }

#ifdef JS_TRACER
        if (TraceRecorder* tr = TRACE_RECORDER(cx)) {
            AbortableRecordingStatus status = tr->monitorRecording(op);
            JS_ASSERT_IF(cx->throwing, status == ARECORD_ERROR);
            switch (status) {
              case ARECORD_CONTINUE:
                moreInterrupts = true;
                break;
              case ARECORD_IMACRO:
                atoms = COMMON_ATOMS_START(&rt->atomState);
                op = JSOp(*regs.pc);
                DO_OP();    
                break;
              case ARECORD_ERROR:
                
                goto error;
              case ARECORD_ABORTED:
              case ARECORD_COMPLETED:
                break;
              case ARECORD_STOP:
                
              default:
                JS_NOT_REACHED("Bad recording status");
            }
        }
#endif 

#if JS_THREADED_INTERP
#ifdef MOZ_TRACEVIS
        if (!moreInterrupts)
            ExitTraceVisState(cx, R_ABORT);
#endif
        jumpTable = moreInterrupts ? interruptJumpTable : normalJumpTable;
        JS_EXTENSION_(goto *normalJumpTable[op]);
#else
        switchMask = moreInterrupts ? -1 : 0;
        switchOp = intN(op);
        goto do_switch;
#endif
    }


ADD_EMPTY_CASE(JSOP_NOP)
ADD_EMPTY_CASE(JSOP_CONDSWITCH)
ADD_EMPTY_CASE(JSOP_TRY)
ADD_EMPTY_CASE(JSOP_TRACE)
#if JS_HAS_XML_SUPPORT
ADD_EMPTY_CASE(JSOP_STARTXML)
ADD_EMPTY_CASE(JSOP_STARTXMLEXPR)
#endif
END_EMPTY_CASES


BEGIN_CASE(JSOP_LINENO)
END_CASE(JSOP_LINENO)

BEGIN_CASE(JSOP_PUSH)
    PUSH_UNDEFINED();
END_CASE(JSOP_PUSH)

BEGIN_CASE(JSOP_POP)
    regs.sp--;
END_CASE(JSOP_POP)

BEGIN_CASE(JSOP_POPN)
{
    regs.sp -= GET_UINT16(regs.pc);
#ifdef DEBUG
    JS_ASSERT(fp->base() <= regs.sp);
    JSObject *obj = fp->blockChain;
    JS_ASSERT_IF(obj,
                 OBJ_BLOCK_DEPTH(cx, obj) + OBJ_BLOCK_COUNT(cx, obj)
                 <= (size_t) (regs.sp - fp->base()));
    for (obj = fp->scopeChain; obj; obj = obj->getParent()) {
        Class *clasp = obj->getClass();
        if (clasp != &js_BlockClass && clasp != &js_WithClass)
            continue;
        if (obj->getPrivate() != js_FloatingFrameIfGenerator(cx, fp))
            break;
        JS_ASSERT(fp->base() + OBJ_BLOCK_DEPTH(cx, obj)
                             + ((clasp == &js_BlockClass)
                                ? OBJ_BLOCK_COUNT(cx, obj)
                                : 1)
                  <= regs.sp);
    }
#endif
}
END_CASE(JSOP_POPN)

BEGIN_CASE(JSOP_SETRVAL)
BEGIN_CASE(JSOP_POPV)
    ASSERT_NOT_THROWING(cx);
    POP_COPY_TO(fp->rval);
END_CASE(JSOP_POPV)

BEGIN_CASE(JSOP_ENTERWITH)
    if (!js_EnterWith(cx, -1))
        goto error;

    








    regs.sp[-1].setNonFunObj(*fp->scopeChain);
END_CASE(JSOP_ENTERWITH)

BEGIN_CASE(JSOP_LEAVEWITH)
    JS_ASSERT(&regs.sp[-1].asNonFunObj() == fp->scopeChain);
    regs.sp--;
    js_LeaveWith(cx);
END_CASE(JSOP_LEAVEWITH)

BEGIN_CASE(JSOP_RETURN)
    POP_COPY_TO(fp->rval);
    

BEGIN_CASE(JSOP_RETRVAL)    
BEGIN_CASE(JSOP_STOP)
{
    



    ASSERT_NOT_THROWING(cx);
    CHECK_BRANCH();

    if (fp->imacpc) {
        



        JS_ASSERT(op == JSOP_STOP);
        JS_ASSERT((uintN)(regs.sp - fp->slots()) <= script->nslots);
        regs.pc = fp->imacpc + js_CodeSpec[*fp->imacpc].length;
        fp->imacpc = NULL;
        atoms = script->atomMap.vector;
        op = JSOp(*regs.pc);
        DO_OP();
    }

    JS_ASSERT(regs.sp == fp->base());
    if ((fp->flags & JSFRAME_CONSTRUCTING) && fp->rval.isPrimitive())
        fp->rval = fp->thisv;

    interpReturnOK = true;
    if (inlineCallCount)
  inline_return:
    {
        JS_ASSERT(!fp->blockChain);
        JS_ASSERT(!js_IsActiveWithOrBlock(cx, fp->scopeChain, 0));

        if (JS_LIKELY(script->staticLevel < JS_DISPLAY_SIZE))
            cx->display[script->staticLevel] = fp->displaySave;

        void *hookData = fp->hookData;
        if (JS_UNLIKELY(hookData != NULL)) {
            if (JSInterpreterHook hook = cx->debugHooks->callHook) {
                hook(cx, fp, JS_FALSE, &interpReturnOK, hookData);
                CHECK_INTERRUPT_HANDLER();
            }
        }

        





        fp->putActivationObjects(cx);

        DTrace::exitJSFun(cx, fp, fp->fun, fp->rval);

        
        if (JS_LIKELY(cx->version == currentVersion)) {
            currentVersion = fp->callerVersion;
            if (currentVersion != cx->version)
                js_SetVersion(cx, currentVersion);
        }

        



        if (fp->flags & JSFRAME_CONSTRUCTING) {
            if (fp->rval.isPrimitive())
                fp->rval = fp->thisv;
            JS_RUNTIME_METER(cx->runtime, constructs);
        }

        JSStackFrame *down = fp->down;
        bool recursive = fp->script == down->script;

        
        cx->stack().popInlineFrame(cx, fp, down);

        
        regs.sp[-1] = fp->rval;

        
        fp = cx->fp;
        script = fp->script;
        atoms = FrameAtomBase(cx, fp);

        
        inlineCallCount--;
        if (JS_LIKELY(interpReturnOK)) {
            JS_ASSERT(js_CodeSpec[js_GetOpcode(cx, script, regs.pc)].length
                      == JSOP_CALL_LENGTH);
            TRACE_0(LeaveFrame);
            if (!TRACE_RECORDER(cx) && recursive) {
                if (*(regs.pc + JSOP_CALL_LENGTH) == JSOP_TRACE) {
                    regs.pc += JSOP_CALL_LENGTH;
                    MONITOR_BRANCH(Record_LeaveFrame);
                    op = (JSOp)*regs.pc;
                    DO_OP();
                }
            }
            if (*(regs.pc + JSOP_CALL_LENGTH) == JSOP_TRACE ||
                *(regs.pc + JSOP_CALL_LENGTH) == JSOP_NOP) {
                JS_STATIC_ASSERT(JSOP_TRACE_LENGTH == JSOP_NOP_LENGTH);
                regs.pc += JSOP_CALL_LENGTH;
                len = JSOP_TRACE_LENGTH;
            } else {
                len = JSOP_CALL_LENGTH;
            }
            DO_NEXT_OP(len);
        }
        goto error;
    }
    interpReturnOK = true;
    goto exit;
}

BEGIN_CASE(JSOP_DEFAULT)
    regs.sp--;
    
BEGIN_CASE(JSOP_GOTO)
{
    len = GET_JUMP_OFFSET(regs.pc);
    BRANCH(len);
}
END_CASE(JSOP_GOTO)

BEGIN_CASE(JSOP_IFEQ)
{
    bool cond;
    Value *_;
    POP_BOOLEAN(cx, _, cond);
    if (cond == false) {
        len = GET_JUMP_OFFSET(regs.pc);
        BRANCH(len);
    }
}
END_CASE(JSOP_IFEQ)

BEGIN_CASE(JSOP_IFNE)
{
    bool cond;
    Value *_;
    POP_BOOLEAN(cx, _, cond);
    if (cond != false) {
        len = GET_JUMP_OFFSET(regs.pc);
        BRANCH(len);
    }
}
END_CASE(JSOP_IFNE)

BEGIN_CASE(JSOP_OR)
{
    bool cond;
    Value *vp;
    POP_BOOLEAN(cx, vp, cond);
    if (cond == true) {
        len = GET_JUMP_OFFSET(regs.pc);
        PUSH_COPY(*vp);
        DO_NEXT_OP(len);
    }
}
END_CASE(JSOP_OR)

BEGIN_CASE(JSOP_AND)
{
    bool cond;
    Value *vp;
    POP_BOOLEAN(cx, vp, cond);
    if (cond == false) {
        len = GET_JUMP_OFFSET(regs.pc);
        PUSH_COPY(*vp);
        DO_NEXT_OP(len);
    }
}
END_CASE(JSOP_AND)

BEGIN_CASE(JSOP_DEFAULTX)
    regs.sp--;
    
BEGIN_CASE(JSOP_GOTOX)
{
    len = GET_JUMPX_OFFSET(regs.pc);
    BRANCH(len);
}
END_CASE(JSOP_GOTOX);

BEGIN_CASE(JSOP_IFEQX)
{
    bool cond;
    Value *_;
    POP_BOOLEAN(cx, _, cond);
    if (cond == false) {
        len = GET_JUMPX_OFFSET(regs.pc);
        BRANCH(len);
    }
}
END_CASE(JSOP_IFEQX)

BEGIN_CASE(JSOP_IFNEX)
{
    bool cond;
    Value *_;
    POP_BOOLEAN(cx, _, cond);
    if (cond != false) {
        len = GET_JUMPX_OFFSET(regs.pc);
        BRANCH(len);
    }
}
END_CASE(JSOP_IFNEX)

BEGIN_CASE(JSOP_ORX)
{
    bool cond;
    Value *vp;
    POP_BOOLEAN(cx, vp, cond);
    if (cond == true) {
        len = GET_JUMPX_OFFSET(regs.pc);
        PUSH_COPY(*vp);
        DO_NEXT_OP(len);
    }
}
END_CASE(JSOP_ORX)

BEGIN_CASE(JSOP_ANDX)
{
    bool cond;
    Value *vp;
    POP_BOOLEAN(cx, vp, cond);
    if (cond == JS_FALSE) {
        len = GET_JUMPX_OFFSET(regs.pc);
        PUSH_COPY(*vp);
        DO_NEXT_OP(len);
    }
}
END_CASE(JSOP_ANDX)







#define FETCH_ELEMENT_ID(obj, n, id)                                          \
    JS_BEGIN_MACRO                                                            \
        const Value &idval_ = regs.sp[n];                                     \
        int32_t i;                                                            \
        if (ValueFitsInInt32(idval_, &i) && INT32_FITS_IN_JSID(i)) {          \
            id = INT_TO_JSID(i);                                              \
        } else {                                                              \
            if (!js_InternNonIntElementId(cx, obj, idval_, &id, &regs.sp[n])) \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO

#define TRY_BRANCH_AFTER_COND(cond,spdec)                                     \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(js_CodeSpec[op].length == 1);                               \
        uintN diff_ = (uintN) regs.pc[1] - (uintN) JSOP_IFEQ;                 \
        if (diff_ <= 1) {                                                     \
            regs.sp -= spdec;                                                 \
            if (cond == (diff_ != 0)) {                                       \
                ++regs.pc;                                                    \
                len = GET_JUMP_OFFSET(regs.pc);                         \
                BRANCH(len);                                                  \
            }                                                                 \
            len = 1 + JSOP_IFEQ_LENGTH;                                 \
            DO_NEXT_OP(len);                                                  \
        }                                                                     \
    JS_END_MACRO

BEGIN_CASE(JSOP_IN)
{
    const Value &rref = regs.sp[-1];
    if (!rref.isObject()) {
        js_ReportValueError(cx, JSMSG_IN_NOT_OBJECT, -1, rref, NULL);
        goto error;
    }
    JSObject *obj = &rref.asObject();
    jsid id;
    FETCH_ELEMENT_ID(obj, -2, id);
    JSObject *obj2;
    JSProperty *prop;
    if (!obj->lookupProperty(cx, id, &obj2, &prop))
        goto error;
    bool cond = prop != NULL;
    if (prop)
        obj2->dropProperty(cx, prop);
    TRY_BRANCH_AFTER_COND(cond, 2);
    regs.sp--;
    regs.sp[-1].setBoolean(cond);
}
END_CASE(JSOP_IN)

BEGIN_CASE(JSOP_ITER)
{
    JS_ASSERT(regs.sp > fp->base());
    uintN flags = regs.pc[1];
    if (!js_ValueToIterator(cx, flags, &regs.sp[-1]))
        goto error;
    CHECK_INTERRUPT_HANDLER();
    JS_ASSERT(!regs.sp[-1].isPrimitive());
}
END_CASE(JSOP_ITER)

BEGIN_CASE(JSOP_MOREITER)
{
    JS_ASSERT(regs.sp - 1 >= fp->base());
    JS_ASSERT(regs.sp[-1].isObject());
    PUSH_NULL();
    bool cond;
    if (!IteratorMore(cx, &regs.sp[-2].asObject(), &cond, &regs.sp[-1]))
        goto error;
    CHECK_INTERRUPT_HANDLER();
    TRY_BRANCH_AFTER_COND(cond, 1);
    JS_ASSERT(regs.pc[1] == JSOP_IFNEX);
    regs.sp[-1].setBoolean(cond);
}
END_CASE(JSOP_MOREITER)

BEGIN_CASE(JSOP_ENDITER)
{
    JS_ASSERT(regs.sp - 1 >= fp->base());
    bool ok = !!js_CloseIterator(cx, regs.sp[-1]);
    regs.sp--;
    if (!ok)
        goto error;
}
END_CASE(JSOP_ENDITER)

BEGIN_CASE(JSOP_FORARG)
{
    JS_ASSERT(regs.sp - 1 >= fp->base());
    uintN slot = GET_ARGNO(regs.pc);
    JS_ASSERT(slot < fp->fun->nargs);
    JS_ASSERT(regs.sp[-1].isObject());
    if (!IteratorNext(cx, &regs.sp[-1].asObject(), &fp->argv[slot]))
        goto error;
}
END_CASE(JSOP_FORARG)

BEGIN_CASE(JSOP_FORLOCAL)
{
    JS_ASSERT(regs.sp - 1 >= fp->base());
    uintN slot = GET_SLOTNO(regs.pc);
    JS_ASSERT(slot < fp->script->nslots);
    JS_ASSERT(regs.sp[-1].isObject());
    if (!IteratorNext(cx, &regs.sp[-1].asObject(), &fp->slots()[slot]))
        goto error;
}
END_CASE(JSOP_FORLOCAL)

BEGIN_CASE(JSOP_FORNAME)
{
    JS_ASSERT(regs.sp - 1 >= fp->base());
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    jsid id = ATOM_TO_JSID(atom);
    JSObject *obj, *obj2;
    JSProperty *prop;
    if (!js_FindProperty(cx, id, &obj, &obj2, &prop))
        goto error;
    if (prop)
        obj2->dropProperty(cx, prop);
    {
        AutoValueRooter tvr(cx);
        JS_ASSERT(regs.sp[-1].isObject());
        if (!IteratorNext(cx, &regs.sp[-1].asObject(), tvr.addr()))
            goto error;
        if (!obj->setProperty(cx, id, tvr.addr()))
            goto error;
    }
}
END_CASE(JSOP_FORNAME)

BEGIN_CASE(JSOP_FORPROP)
{
    JS_ASSERT(regs.sp - 2 >= fp->base());
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    jsid id = ATOM_TO_JSID(atom);
    JSObject *obj;
    FETCH_OBJECT(cx, -1, obj);
    {
        AutoValueRooter tvr(cx);
        JS_ASSERT(regs.sp[-2].isObject());
        if (!IteratorNext(cx, &regs.sp[-2].asObject(), tvr.addr()))
            goto error;
        if (!obj->setProperty(cx, id, tvr.addr()))
            goto error;
    }
    regs.sp--;
}
END_CASE(JSOP_FORPROP)

BEGIN_CASE(JSOP_FORELEM)
    





    JS_ASSERT(regs.sp - 1 >= fp->base());
    JS_ASSERT(regs.sp[-1].isObject());
    PUSH_NULL();
    if (!IteratorNext(cx, &regs.sp[-2].asObject(), &regs.sp[-1]))
        goto error;
END_CASE(JSOP_FORELEM)

BEGIN_CASE(JSOP_DUP)
{
    JS_ASSERT(regs.sp > fp->base());
    const Value &rref = regs.sp[-1];
    PUSH_COPY(rref);
}
END_CASE(JSOP_DUP)

BEGIN_CASE(JSOP_DUP2)
{
    JS_ASSERT(regs.sp - 2 >= fp->base());
    const Value &lref = regs.sp[-2];
    const Value &rref = regs.sp[-1];
    PUSH_COPY(lref);
    PUSH_COPY(rref);
}
END_CASE(JSOP_DUP2)

BEGIN_CASE(JSOP_SWAP)
{
    JS_ASSERT(regs.sp - 2 >= fp->base());
    Value &lref = regs.sp[-2];
    Value &rref = regs.sp[-1];
    lref.swap(rref);
}
END_CASE(JSOP_SWAP)

BEGIN_CASE(JSOP_PICK)
{
    jsint i = regs.pc[1];
    JS_ASSERT(regs.sp - (i+1) >= fp->base());
    Value lval;
    lval = regs.sp[-(i+1)];
    memmove(regs.sp - (i+1), regs.sp - i, sizeof(Value)*i);
    regs.sp[-1] = lval;
}
END_CASE(JSOP_PICK)

#define NATIVE_GET(cx,obj,pobj,sprop,getHow,vp)                               \
    JS_BEGIN_MACRO                                                            \
        if (sprop->hasDefaultGetter()) {                                      \
            /* Fast path for Object instance properties. */                   \
            JS_ASSERT((sprop)->slot != SPROP_INVALID_SLOT ||                  \
                      !sprop->hasDefaultSetter());                            \
            if (((sprop)->slot != SPROP_INVALID_SLOT))                        \
                *(vp) = (pobj)->lockedGetSlot((sprop)->slot);                 \
            else                                                              \
                (vp)->setUndefined();                                         \
        } else {                                                              \
            if (!js_NativeGet(cx, obj, pobj, sprop, getHow, vp))              \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO

#define NATIVE_SET(cx,obj,sprop,entry,vp)                                     \
    JS_BEGIN_MACRO                                                            \
        TRACE_2(SetPropHit, entry, sprop);                                    \
        if (sprop->hasDefaultSetter() &&                                      \
            (sprop)->slot != SPROP_INVALID_SLOT &&                            \
            !(obj)->scope()->brandedOrHasMethodBarrier()) {                   \
            /* Fast path for, e.g., plain Object instance properties. */      \
            (obj)->lockedSetSlot((sprop)->slot, *vp);                         \
        } else {                                                              \
            if (!js_NativeSet(cx, obj, sprop, false, vp))                     \
                goto error;                                                   \
        }                                                                     \
    JS_END_MACRO












#define SKIP_POP_AFTER_SET(oplen,spdec)                                       \
            if (regs.pc[oplen] == JSOP_POP) {                                 \
                regs.sp -= spdec;                                             \
                regs.pc += oplen + JSOP_POP_LENGTH;                           \
                op = (JSOp) *regs.pc;                                         \
                DO_OP();                                                      \
            }

#define END_SET_CASE(OP)                                                      \
            SKIP_POP_AFTER_SET(OP##_LENGTH, 1);                               \
          END_CASE(OP)

#define END_SET_CASE_STORE_RVAL(OP,spdec)                                     \
            SKIP_POP_AFTER_SET(OP##_LENGTH, spdec);                           \
            {                                                                 \
                Value *newsp = regs.sp - ((spdec) - 1);                       \
                newsp[-1] = regs.sp[-1];                                      \
                regs.sp = newsp;                                              \
            }                                                                 \
          END_CASE(OP)

BEGIN_CASE(JSOP_SETCONST)
{
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    JSObject *obj = fp->varobj(cx);
    const Value &ref = regs.sp[-1];
    if (!obj->defineProperty(cx, ATOM_TO_JSID(atom), ref,
                             PropertyStub, PropertyStub,
                             JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY)) {
        goto error;
    }
}
END_SET_CASE(JSOP_SETCONST);

#if JS_HAS_DESTRUCTURING
BEGIN_CASE(JSOP_ENUMCONSTELEM)
{
    const Value &ref = regs.sp[-3];
    JSObject *obj;
    FETCH_OBJECT(cx, -2, obj);
    jsid id;
    FETCH_ELEMENT_ID(obj, -1, id);
    if (!obj->defineProperty(cx, id, ref,
                             PropertyStub, PropertyStub,
                             JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY)) {
        goto error;
    }
    regs.sp -= 3;
}
END_CASE(JSOP_ENUMCONSTELEM)
#endif

BEGIN_CASE(JSOP_BINDNAME)
{
    JSObject *obj;
    do {
        















        obj = fp->scopeChain;
        if (!obj->getParent())
            break;

        PropertyCacheEntry *entry;
        JSObject *obj2;
        JSAtom *atom;
        JS_PROPERTY_CACHE(cx).test(cx, regs.pc, obj, obj2, entry, atom);
        if (!atom) {
            ASSERT_VALID_PROPERTY_CACHE_HIT(0, obj, obj2, entry);
            break;
        }

        jsid id = ATOM_TO_JSID(atom);
        obj = js_FindIdentifierBase(cx, fp->scopeChain, id);
        if (!obj)
            goto error;
    } while (0);
    PUSH_NONFUNOBJ(*obj);
}
END_CASE(JSOP_BINDNAME)

BEGIN_CASE(JSOP_IMACOP)
    JS_ASSERT(JS_UPTRDIFF(fp->imacpc, script->code) < script->length);
    op = JSOp(*fp->imacpc);
    DO_OP();

#define BITWISE_OP(OP)                                                        \
    JS_BEGIN_MACRO                                                            \
        int32_t i, j;                                                         \
        if (!ValueToECMAInt32(cx, regs.sp[-2], &i))                           \
            goto error;                                                       \
        if (!ValueToECMAInt32(cx, regs.sp[-1], &j))                           \
            goto error;                                                       \
        i = i OP j;                                                           \
        regs.sp--;                                                            \
        regs.sp[-1].setInt32(i);                                              \
    JS_END_MACRO

BEGIN_CASE(JSOP_BITOR)
    BITWISE_OP(|);
END_CASE(JSOP_BITOR)

BEGIN_CASE(JSOP_BITXOR)
    BITWISE_OP(^);
END_CASE(JSOP_BITXOR)

BEGIN_CASE(JSOP_BITAND)
    BITWISE_OP(&);
END_CASE(JSOP_BITAND)

#undef BITWISE_OP






#if JS_HAS_XML_SUPPORT
#define XML_EQUALITY_OP(OP)                                                   \
    if ((lmask == JSVAL_MASK32_NONFUNOBJ && lref.asObject().isXML()) ||       \
        (rmask == JSVAL_MASK32_NONFUNOBJ && rref.asObject().isXML())) {       \
        if (!js_TestXMLEquality(cx, lref, rref, &cond))                       \
            goto error;                                                       \
        cond = cond OP JS_TRUE;                                               \
    } else

#define EXTENDED_EQUALITY_OP(OP)                                              \
    if (((clasp = l->getClass())->flags & JSCLASS_IS_EXTENDED) &&             \
        ((ExtendedClass *)clasp)->equality) {                                 \
        if (!((ExtendedClass *)clasp)->equality(cx, l, &rref, &cond))         \
            goto error;                                                       \
        cond = cond OP JS_TRUE;                                               \
    } else
#else
#define XML_EQUALITY_OP(OP)
#define EXTENDED_EQUALITY_OP(OP)
#endif

#define EQUALITY_OP(OP, IFNAN)                                                \
    JS_BEGIN_MACRO                                                            \
        /* Depends on the value representation. */                            \
        Class *clasp;                                                         \
        JSBool cond;                                                          \
        Value &rref = regs.sp[-1];                                            \
        Value &lref = regs.sp[-2];                                            \
        uint32 rmask = rref.data.s.mask32;                                    \
        uint32 lmask = lref.data.s.mask32;                                    \
        XML_EQUALITY_OP(OP)                                                   \
        if (lmask == rmask ||                                                 \
            (Value::isObjectMask(lmask) && Value::isObjectMask(rmask))) {     \
            if (lmask == JSVAL_MASK32_STRING) {                               \
                JSString *l = lref.asString(), *r = rref.asString();          \
                cond = js_EqualStrings(l, r) OP JSVAL_TRUE;                   \
            } else if (Value::isObjectMask(lmask)) {                          \
                JSObject *l = &lref.asObject(), *r = &rref.asObject();        \
                EXTENDED_EQUALITY_OP(OP)                                      \
                cond = l OP r;                                                \
            } else if (JS_UNLIKELY(Value::isDoubleMask(lmask))) {             \
                double l = lref.asDouble(), r = rref.asDouble();              \
                cond = JSDOUBLE_COMPARE(l, OP, r, IFNAN);                     \
            } else {                                                          \
                cond = lref.data.s.payload.u32 OP rref.data.s.payload.u32;    \
            }                                                                 \
        } else if (Value::isDoubleMask(lmask) && Value::isDoubleMask(rmask)) { \
            double l = lref.asDouble(), r = rref.asDouble();                  \
            cond = JSDOUBLE_COMPARE(l, OP, r, IFNAN);                         \
        } else {                                                              \
            if (Value::isNullOrUndefinedMask(lmask)) {                        \
                cond = Value::isNullOrUndefinedMask(rmask) OP true;           \
            } else if (Value::isNullOrUndefinedMask(rmask)) {                 \
                cond = true OP false;                                         \
            } else {                                                          \
                if (Value::isObjectMask(lmask)) {                             \
                    JSObject &obj = lref.asObject();                          \
                    if (!obj.defaultValue(cx, JSTYPE_VOID, &lref))            \
                        goto error;                                           \
                    lmask = lref.data.s.mask32;                               \
                }                                                             \
                if (Value::isObjectMask(rmask)) {                             \
                    JSObject &obj = rref.asObject();                          \
                    if (!obj.defaultValue(cx, JSTYPE_VOID, &rref))            \
                        goto error;                                           \
                    rmask = rref.data.s.mask32;                               \
                }                                                             \
                if (lmask == JSVAL_MASK32_STRING && rmask == JSVAL_MASK32_STRING) { \
                    JSString *l = lref.asString(), *r = rref.asString();      \
                    cond = js_EqualStrings(l, r) OP JS_TRUE;                  \
                } else {                                                      \
                    double l, r;                                              \
                    if (!ValueToNumber(cx, lref, &l) ||                       \
                        !ValueToNumber(cx, rref, &r)) {                       \
                        goto error;                                           \
                    }                                                         \
                    cond = JSDOUBLE_COMPARE(l, OP, r, IFNAN);                 \
                }                                                             \
            }                                                                 \
        }                                                                     \
        TRY_BRANCH_AFTER_COND(cond, 2);                                       \
        regs.sp--;                                                            \
        regs.sp[-1].setBoolean(cond);                                         \
    JS_END_MACRO

BEGIN_CASE(JSOP_EQ)
    EQUALITY_OP(==, false);
END_CASE(JSOP_EQ)

BEGIN_CASE(JSOP_NE)
    EQUALITY_OP(!=, true);
END_CASE(JSOP_NE)

#undef EQUALITY_OP
#undef XML_EQUALITY_OP
#undef EXTENDED_EQUALITY_OP

#define STRICT_EQUALITY_OP(OP, COND)                                          \
    JS_BEGIN_MACRO                                                            \
        const Value &rref = regs.sp[-1];                                      \
        const Value &lref = regs.sp[-2];                                      \
        COND = StrictlyEqual(cx, lref, rref) OP true;                         \
        regs.sp--;                                                            \
    JS_END_MACRO

BEGIN_CASE(JSOP_STRICTEQ)
{
    bool cond;
    STRICT_EQUALITY_OP(==, cond);
    regs.sp[-1].setBoolean(cond);
}
END_CASE(JSOP_STRICTEQ)

BEGIN_CASE(JSOP_STRICTNE)
{
    bool cond;
    STRICT_EQUALITY_OP(!=, cond);
    regs.sp[-1].setBoolean(cond);
}
END_CASE(JSOP_STRICTNE)

BEGIN_CASE(JSOP_CASE)
{
    bool cond;
    STRICT_EQUALITY_OP(==, cond);
    if (cond) {
        regs.sp--;
        len = GET_JUMP_OFFSET(regs.pc);
        BRANCH(len);
    }
}
END_CASE(JSOP_CASE)

BEGIN_CASE(JSOP_CASEX)
{
    bool cond;
    STRICT_EQUALITY_OP(==, cond);
    if (cond) {
        regs.sp--;
        len = GET_JUMPX_OFFSET(regs.pc);
        BRANCH(len);
    }
}
END_CASE(JSOP_CASEX)

#undef STRICT_EQUALITY_OP

#define RELATIONAL_OP(OP)                                                     \
    JS_BEGIN_MACRO                                                            \
        /* Depends on the value representation */                             \
        Value &rref = regs.sp[-1];                                            \
        Value &lref = regs.sp[-2];                                            \
        uint32 rmask = rref.data.s.mask32;                                    \
        uint32 lmask = lref.data.s.mask32;                                    \
        uint32 maskand = lmask & rmask;                                       \
        bool cond;                                                            \
        /* Optimize for two int-tagged operands (typical loop control). */    \
        if (maskand == JSVAL_MASK32_INT32) {                                  \
            cond = lref.asInt32() OP rref.asInt32();                          \
        } else {                                                              \
            if (Value::isObjectMask(lmask | rmask)) {                         \
                if (Value::isObjectMask(lmask)) {                             \
                    JSObject &obj = lref.asObject();                          \
                    if (!obj.defaultValue(cx, JSTYPE_NUMBER, &lref))          \
                        goto error;                                           \
                    lmask = lref.data.s.mask32;                               \
                }                                                             \
                if (Value::isObjectMask(rmask)) {                             \
                    JSObject &obj = rref.asObject();                          \
                    if (!obj.defaultValue(cx, JSTYPE_NUMBER, &rref))          \
                        goto error;                                           \
                    rmask = rref.data.s.mask32;                               \
                }                                                             \
                maskand = lmask & rmask;                                      \
            }                                                                 \
            if (maskand == JSVAL_MASK32_STRING) {                             \
                JSString *l = lref.asString(), *r = rref.asString();          \
                cond = js_CompareStrings(l, r) OP 0;                          \
            } else {                                                          \
                double l, r;                                                  \
                if (!ValueToNumber(cx, lref, &l) ||                           \
                    !ValueToNumber(cx, rref, &r)) {                           \
                    goto error;                                               \
                }                                                             \
                cond = JSDOUBLE_COMPARE(l, OP, r, false);                     \
            }                                                                 \
        }                                                                     \
        TRY_BRANCH_AFTER_COND(cond, 2);                                       \
        regs.sp--;                                                            \
        regs.sp[-1].setBoolean(cond);                                         \
    JS_END_MACRO

BEGIN_CASE(JSOP_LT)
    RELATIONAL_OP(<);
END_CASE(JSOP_LT)

BEGIN_CASE(JSOP_LE)
    RELATIONAL_OP(<=);
END_CASE(JSOP_LE)

BEGIN_CASE(JSOP_GT)
    RELATIONAL_OP(>);
END_CASE(JSOP_GT)

BEGIN_CASE(JSOP_GE)
    RELATIONAL_OP(>=);
END_CASE(JSOP_GE)

#undef RELATIONAL_OP

#define SIGNED_SHIFT_OP(OP)                                                   \
    JS_BEGIN_MACRO                                                            \
        int32_t i, j;                                                         \
        if (!ValueToECMAInt32(cx, regs.sp[-2], &i))                           \
            goto error;                                                       \
        if (!ValueToECMAInt32(cx, regs.sp[-1], &j))                           \
            goto error;                                                       \
        i = i OP (j & 31);                                                    \
        regs.sp--;                                                            \
        regs.sp[-1].setInt32(i);                                              \
    JS_END_MACRO

BEGIN_CASE(JSOP_LSH)
    SIGNED_SHIFT_OP(<<);
END_CASE(JSOP_LSH)

BEGIN_CASE(JSOP_RSH)
    SIGNED_SHIFT_OP(>>);
END_CASE(JSOP_RSH)

#undef SIGNED_SHIFT_OP

BEGIN_CASE(JSOP_URSH)
{
    uint32_t u;
    if (!ValueToECMAUint32(cx, regs.sp[-2], &u))
        goto error;
    int32_t j;
    if (!ValueToECMAInt32(cx, regs.sp[-1], &j))
        goto error;

    u >>= (j & 31);

    regs.sp--;
    Uint32ToValue(u, &regs.sp[-1]);
}
END_CASE(JSOP_URSH)

BEGIN_CASE(JSOP_ADD)
{
    
    Value &rref = regs.sp[-1];
    Value &lref = regs.sp[-2];
    uint32 rmask = rref.data.s.mask32;
    uint32 lmask = lref.data.s.mask32;

    if ((lmask & rmask) == JSVAL_MASK32_INT32) {
        int32_t l = lref.asInt32(), r = rref.asInt32();
        int32_t sum = l + r;
        regs.sp--;
        if (JS_UNLIKELY(bool((l ^ sum) & (r ^ sum) & 0x80000000)))
            regs.sp[-1].setDouble(double(l) + double(r));
        else
            regs.sp[-1].setInt32(sum);
    } else
#if JS_HAS_XML_SUPPORT
    if (lmask == JSVAL_MASK32_NONFUNOBJ && lref.asObject().isXML() &&
        rmask == JSVAL_MASK32_NONFUNOBJ && rref.asObject().isXML()) {
        Value rval;
        if (!js_ConcatenateXML(cx, &lref.asObject(), &rref.asObject(), &rval))
            goto error;
        regs.sp--;
        regs.sp[-1] = rval;
    } else
#endif
    {
        if (Value::isObjectMask(lmask)) {
            if (!lref.asObject().defaultValue(cx, JSTYPE_VOID, &lref))
                goto error;
            lmask = lref.data.s.mask32;
        }
        if (Value::isObjectMask(rmask)) {
            if (!rref.asObject().defaultValue(cx, JSTYPE_VOID, &rref))
                goto error;
            rmask = rref.data.s.mask32;
        }
        if (lmask == JSVAL_MASK32_STRING || rmask == JSVAL_MASK32_STRING) {
            JSString *str1, *str2;
            if (lmask == rmask) {
                str1 = lref.asString();
                str2 = rref.asString();
            } else if (lmask == JSVAL_MASK32_STRING) {
                str1 = lref.asString();
                str2 = js_ValueToString(cx, rref);
                if (!str2)
                    goto error;
            } else {
                str2 = rref.asString();
                str1 = js_ValueToString(cx, lref);
                if (!str1)
                    goto error;
            }
            JSString *str = js_ConcatStrings(cx, str1, str2);
            if (!str)
                goto error;
            regs.sp--;
            regs.sp[-1].setString(str);
        } else {
            double l, r;
            if (!ValueToNumber(cx, lref, &l) || !ValueToNumber(cx, rref, &r))
                goto error;
            l += r;
            regs.sp--;
            regs.sp[-1].setNumber(l);
        }
    }
}
END_CASE(JSOP_ADD)

BEGIN_CASE(JSOP_OBJTOSTR)
{
    const Value &ref = regs.sp[-1];
    if (ref.isObject()) {
        JSString *str = js_ValueToString(cx, ref);
        if (!str)
            goto error;
        regs.sp[-1].setString(str);
    }
}
END_CASE(JSOP_OBJTOSTR)

BEGIN_CASE(JSOP_CONCATN)
{
    JSCharBuffer buf(cx);
    uintN argc = GET_ARGC(regs.pc);
    for (Value *vp = regs.sp - argc; vp < regs.sp; vp++) {
        JS_ASSERT(vp->isPrimitive());
        if (!js_ValueToCharBuffer(cx, *vp, buf))
            goto error;
    }
    JSString *str = js_NewStringFromCharBuffer(cx, buf);
    if (!str)
        goto error;
    regs.sp -= argc - 1;
    regs.sp[-1].setString(str);
}
END_CASE(JSOP_CONCATN)

#define BINARY_OP(OP)                                                         \
    JS_BEGIN_MACRO                                                            \
        double d1, d2;                                                        \
        if (!ValueToNumber(cx, regs.sp[-2], &d1) ||                           \
            !ValueToNumber(cx, regs.sp[-1], &d2)) {                           \
            goto error;                                                       \
        }                                                                     \
        double d = d1 OP d2;                                                  \
        regs.sp--;                                                            \
        regs.sp[-1].setNumber(d);                                             \
    JS_END_MACRO

BEGIN_CASE(JSOP_SUB)
    BINARY_OP(-);
END_CASE(JSOP_SUB)

BEGIN_CASE(JSOP_MUL)
    BINARY_OP(*);
END_CASE(JSOP_MUL)

#undef BINARY_OP

BEGIN_CASE(JSOP_DIV)
{
    double d1, d2;
    if (!ValueToNumber(cx, regs.sp[-2], &d1) ||
        !ValueToNumber(cx, regs.sp[-1], &d2)) {
        goto error;
    }
    regs.sp--;
    if (d2 == 0) {
        const Value *vp;
#ifdef XP_WIN
        
        if (JSDOUBLE_IS_NaN(d2))
            vp = &rt->NaNValue;
        else
#endif
        if (d1 == 0 || JSDOUBLE_IS_NaN(d1))
            vp = &rt->NaNValue;
        else if (JSDOUBLE_IS_NEG(d1) != JSDOUBLE_IS_NEG(d2))
            vp = &rt->negativeInfinityValue;
        else
            vp = &rt->positiveInfinityValue;
        regs.sp[-1] = *vp;
    } else {
        d1 /= d2;
        regs.sp[-1].setNumber(d1);
    }
}
END_CASE(JSOP_DIV)

BEGIN_CASE(JSOP_MOD)
{
    Value &lref = regs.sp[-2];
    Value &rref = regs.sp[-1];
    int32_t l, r;
    if (lref.isInt32() && rref.isInt32() &&
        (l = lref.asInt32()) >= 0 && (r = rref.asInt32()) > 0) {
        int32_t mod = l % r;
        regs.sp--;
        regs.sp[-1].setInt32(mod);
    } else {
        double d1, d2;
        if (!ValueToNumber(cx, regs.sp[-2], &d1) ||
            !ValueToNumber(cx, regs.sp[-1], &d2)) {
            goto error;
        }
        regs.sp--;
        if (d2 == 0) {
            regs.sp[-1].setDouble(js_NaN);
        } else {
            d1 = js_fmod(d1, d2);
            regs.sp[-1].setDouble(d1);
        }
    }
}
END_CASE(JSOP_MOD)

BEGIN_CASE(JSOP_NOT)
{
    Value *_;
    bool cond;
    POP_BOOLEAN(cx, _, cond);
    PUSH_BOOLEAN(!cond);
}
END_CASE(JSOP_NOT)

BEGIN_CASE(JSOP_BITNOT)
{
    int32_t i;
    if (!ValueToECMAInt32(cx, regs.sp[-1], &i))
        goto error;
    i = ~i;
    regs.sp[-1].setInt32(i);
}
END_CASE(JSOP_BITNOT)

BEGIN_CASE(JSOP_NEG)
{
    




    const Value &ref = regs.sp[-1];
    int32_t i;
    if (ref.isInt32() && (i = ref.asInt32()) != 0 && i != INT32_MIN) {
        i = -i;
        regs.sp[-1].setInt32(i);
    } else {
        double d;
        if (!ValueToNumber(cx, regs.sp[-1], &d))
            goto error;
        d = -d;
        regs.sp[-1].setDouble(d);
    }
}
END_CASE(JSOP_NEG)

BEGIN_CASE(JSOP_POS)
    if (!ValueToNumber(cx, &regs.sp[-1]))
        goto error;
END_CASE(JSOP_POS)

BEGIN_CASE(JSOP_DELNAME)
{
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    jsid id = ATOM_TO_JSID(atom);
    JSObject *obj, *obj2;
    JSProperty *prop;
    if (!js_FindProperty(cx, id, &obj, &obj2, &prop))
        goto error;

    
    PUSH_BOOLEAN(true);
    if (prop) {
        obj2->dropProperty(cx, prop);
        if (!obj->deleteProperty(cx, id, &regs.sp[-1]))
            goto error;
    }
}
END_CASE(JSOP_DELNAME)

BEGIN_CASE(JSOP_DELPROP)
{
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    jsid id = ATOM_TO_JSID(atom);

    JSObject *obj;
    FETCH_OBJECT(cx, -1, obj);

    Value rval;
    if (!obj->deleteProperty(cx, id, &rval))
        goto error;

    regs.sp[-1] = rval;
}
END_CASE(JSOP_DELPROP)

BEGIN_CASE(JSOP_DELELEM)
{
    
    JSObject *obj;
    FETCH_OBJECT(cx, -2, obj);

    
    jsid id;
    FETCH_ELEMENT_ID(obj, -1, id);

    
    if (!obj->deleteProperty(cx, id, &regs.sp[-2]))
        goto error;

    regs.sp--;
}
END_CASE(JSOP_DELELEM)

BEGIN_CASE(JSOP_TYPEOFEXPR)
BEGIN_CASE(JSOP_TYPEOF)
{
    const Value &ref = regs.sp[-1];
    JSType type = JS_TypeOfValue(cx, Jsvalify(ref));
    JSAtom *atom = rt->atomState.typeAtoms[type];
    regs.sp[-1].setString(ATOM_TO_STRING(atom));
}
END_CASE(JSOP_TYPEOF)

BEGIN_CASE(JSOP_VOID)
    regs.sp[-1].setUndefined();
END_CASE(JSOP_VOID)

{
    JSObject *obj;
    JSAtom *atom;
    jsid id;
    jsint i;

BEGIN_CASE(JSOP_INCELEM)
BEGIN_CASE(JSOP_DECELEM)
BEGIN_CASE(JSOP_ELEMINC)
BEGIN_CASE(JSOP_ELEMDEC)

    



    id = 0;
    i = -2;
    goto fetch_incop_obj;

BEGIN_CASE(JSOP_INCPROP)
BEGIN_CASE(JSOP_DECPROP)
BEGIN_CASE(JSOP_PROPINC)
BEGIN_CASE(JSOP_PROPDEC)
    LOAD_ATOM(0, atom);
    id = ATOM_TO_JSID(atom);
    i = -1;

  fetch_incop_obj:
    FETCH_OBJECT(cx, i, obj);
    if (id == 0)
        FETCH_ELEMENT_ID(obj, -1, id);
    goto do_incop;

BEGIN_CASE(JSOP_INCNAME)
BEGIN_CASE(JSOP_DECNAME)
BEGIN_CASE(JSOP_NAMEINC)
BEGIN_CASE(JSOP_NAMEDEC)
{
    obj = fp->scopeChain;

    JSObject *obj2;
    PropertyCacheEntry *entry;
    JS_PROPERTY_CACHE(cx).test(cx, regs.pc, obj, obj2, entry, atom);
    if (!atom) {
        ASSERT_VALID_PROPERTY_CACHE_HIT(0, obj, obj2, entry);
        if (obj == obj2 && entry->vword.isSlot()) {
            uint32 slot = entry->vword.toSlot();
            JS_ASSERT(slot < obj->scope()->freeslot);
            Value &rref = obj->getSlotRef(slot);
            int32_t tmp;
            if (JS_LIKELY(rref.isInt32() && CanIncDecWithoutOverflow(tmp = rref.asInt32()))) {
                int32_t inc = tmp + ((js_CodeSpec[op].format & JOF_INC) ? 1 : -1);
                if (!(js_CodeSpec[op].format & JOF_POST))
                    tmp = inc;
                rref.asInt32Ref() = inc;
                PUSH_INT32(tmp);
                len = JSOP_INCNAME_LENGTH;
                DO_NEXT_OP(len);
            }
        }
        LOAD_ATOM(0, atom);
    }

    id = ATOM_TO_JSID(atom);
    JSProperty *prop;
    if (!js_FindPropertyHelper(cx, id, true, &obj, &obj2, &prop))
        goto error;
    if (!prop) {
        atomNotDefined = atom;
        goto atom_not_defined;
    }
    obj2->dropProperty(cx, prop);
}

do_incop:
{
    



    PUSH_NULL();
    if (!obj->getProperty(cx, id, &regs.sp[-1]))
        goto error;

    const JSCodeSpec *cs = &js_CodeSpec[op];
    JS_ASSERT(cs->ndefs == 1);
    JS_ASSERT((cs->format & JOF_TMPSLOT_MASK) == JOF_TMPSLOT2);
    Value &ref = regs.sp[-1];
    int32_t tmp;
    if (JS_LIKELY(ref.isInt32() && CanIncDecWithoutOverflow(tmp = ref.asInt32()))) {
        int incr = (cs->format & JOF_INC) ? 1 : -1;
        if (cs->format & JOF_POST)
            ref.asInt32Ref() = tmp + incr;
        else
            ref.asInt32Ref() = tmp += incr;
        fp->flags |= JSFRAME_ASSIGNING;
        JSBool ok = obj->setProperty(cx, id, &ref);
        fp->flags &= ~JSFRAME_ASSIGNING;
        if (!ok)
            goto error;

        



        ref.setInt32(tmp);
    } else {
        
        PUSH_NULL();
        if (!js_DoIncDec(cx, cs, &regs.sp[-2], &regs.sp[-1]))
            goto error;
        fp->flags |= JSFRAME_ASSIGNING;
        JSBool ok = obj->setProperty(cx, id, &regs.sp[-1]);
        fp->flags &= ~JSFRAME_ASSIGNING;
        if (!ok)
            goto error;
        regs.sp--;
    }

    if (cs->nuses == 0) {
        
    } else {
        regs.sp[-1 - cs->nuses] = regs.sp[-1];
        regs.sp -= cs->nuses;
    }
    len = cs->length;
    DO_NEXT_OP(len);
}
}

{
    int incr, incr2;
    Value *vp;

    
BEGIN_CASE(JSOP_DECARG)
    incr = -1; incr2 = -1; goto do_arg_incop;
BEGIN_CASE(JSOP_ARGDEC)
    incr = -1; incr2 =  0; goto do_arg_incop;
BEGIN_CASE(JSOP_INCARG)
    incr =  1; incr2 =  1; goto do_arg_incop;
BEGIN_CASE(JSOP_ARGINC)
    incr =  1; incr2 =  0;

  do_arg_incop:
    
    uint32 slot;
    slot = GET_ARGNO(regs.pc);
    JS_ASSERT(slot < fp->fun->nargs);
    METER_SLOT_OP(op, slot);
    vp = fp->argv + slot;
    goto do_int_fast_incop;

BEGIN_CASE(JSOP_DECLOCAL)
    incr = -1; incr2 = -1; goto do_local_incop;
BEGIN_CASE(JSOP_LOCALDEC)
    incr = -1; incr2 =  0; goto do_local_incop;
BEGIN_CASE(JSOP_INCLOCAL)
    incr =  1; incr2 =  1; goto do_local_incop;
BEGIN_CASE(JSOP_LOCALINC)
    incr =  1; incr2 =  0;

  




  do_local_incop:
    slot = GET_SLOTNO(regs.pc);
    JS_ASSERT(slot < fp->script->nslots);
    vp = fp->slots() + slot;
    METER_SLOT_OP(op, slot);
    vp = fp->slots() + slot;

  do_int_fast_incop:
    int32_t tmp;
    if (JS_LIKELY(vp->isInt32() && CanIncDecWithoutOverflow(tmp = vp->asInt32()))) {
        vp->asInt32Ref() = tmp + incr;
        JS_ASSERT(JSOP_INCARG_LENGTH == js_CodeSpec[op].length);
        SKIP_POP_AFTER_SET(JSOP_INCARG_LENGTH, 0);
        PUSH_INT32(tmp + incr2);
    } else {
        PUSH_COPY(*vp);
        if (!js_DoIncDec(cx, &js_CodeSpec[op], &regs.sp[-1], vp))
            goto error;
    }
    len = JSOP_INCARG_LENGTH;
    JS_ASSERT(len == js_CodeSpec[op].length);
    DO_NEXT_OP(len);
}


#define FAST_GLOBAL_INCREMENT_OP(SLOWOP,INCR,INCR2)                           \
    op2 = SLOWOP;                                                             \
    incr = INCR;                                                              \
    incr2 = INCR2;                                                            \
    goto do_global_incop

{
    JSOp op2;
    int incr, incr2;

BEGIN_CASE(JSOP_DECGVAR)
    FAST_GLOBAL_INCREMENT_OP(JSOP_DECNAME, -1, -1);
BEGIN_CASE(JSOP_GVARDEC)
    FAST_GLOBAL_INCREMENT_OP(JSOP_NAMEDEC, -1,  0);
BEGIN_CASE(JSOP_INCGVAR)
    FAST_GLOBAL_INCREMENT_OP(JSOP_INCNAME,  1,  1);
BEGIN_CASE(JSOP_GVARINC)
    FAST_GLOBAL_INCREMENT_OP(JSOP_NAMEINC,  1,  0);

#undef FAST_GLOBAL_INCREMENT_OP

  do_global_incop:
    JS_ASSERT((js_CodeSpec[op].format & JOF_TMPSLOT_MASK) ==
              JOF_TMPSLOT2);
    uint32 slot = GET_SLOTNO(regs.pc);
    JS_ASSERT(slot < GlobalVarCount(fp));
    METER_SLOT_OP(op, slot);
    const Value &lref = fp->slots()[slot];
    if (lref.isNull()) {
        op = op2;
        DO_OP();
    }
    slot = (uint32)lref.asInt32();
    JS_ASSERT(fp->varobj(cx) == cx->activeCallStack()->getInitialVarObj());
    JSObject *varobj = cx->activeCallStack()->getInitialVarObj();

    



    Value &rref = varobj->getSlotRef(slot);
    int32_t tmp;
    if (JS_LIKELY(rref.isInt32() && CanIncDecWithoutOverflow(tmp = rref.asInt32()))) {
        PUSH_INT32(tmp + incr2);
        rref.asInt32Ref() = tmp + incr;
    } else {
        PUSH_COPY(rref);
        if (!js_DoIncDec(cx, &js_CodeSpec[op], &regs.sp[-1], &rref))
            goto error;
    }
    len = JSOP_INCGVAR_LENGTH;  
    JS_ASSERT(len == js_CodeSpec[op].length);
    DO_NEXT_OP(len);
}

BEGIN_CASE(JSOP_THIS)
    if (!fp->getThisObject(cx))
        goto error;
    PUSH_COPY(fp->thisv);
END_CASE(JSOP_THIS)

BEGIN_CASE(JSOP_UNBRANDTHIS)
{
    JSObject *obj = fp->getThisObject(cx);
    if (!obj)
        goto error;
    if (!obj->unbrand(cx))
        goto error;
}
END_CASE(JSOP_UNBRANDTHIS)

{
    JSObject *obj;
    Value *vp;
    jsint i;

BEGIN_CASE(JSOP_GETTHISPROP)
    obj = fp->getThisObject(cx);
    if (!obj)
        goto error;
    i = 0;
    PUSH_NULL();
    goto do_getprop_with_obj;

BEGIN_CASE(JSOP_GETARGPROP)
{
    i = ARGNO_LEN;
    uint32 slot = GET_ARGNO(regs.pc);
    JS_ASSERT(slot < fp->fun->nargs);
    PUSH_COPY(fp->argv[slot]);
    goto do_getprop_body;
}

BEGIN_CASE(JSOP_GETLOCALPROP)
{
    i = SLOTNO_LEN;
    uint32 slot = GET_SLOTNO(regs.pc);
    JS_ASSERT(slot < script->nslots);
    PUSH_COPY(fp->slots()[slot]);
    goto do_getprop_body;
}

BEGIN_CASE(JSOP_GETPROP)
BEGIN_CASE(JSOP_GETXPROP)
    i = 0;

  do_getprop_body:
    vp = &regs.sp[-1];

  do_getprop_with_lval:
    VALUE_TO_OBJECT(cx, vp, obj);

  do_getprop_with_obj:
    {
        Value rval;
        do {
            




            JSObject *aobj = js_GetProtoIfDenseArray(obj);

            PropertyCacheEntry *entry;
            JSObject *obj2;
            JSAtom *atom;
            JS_PROPERTY_CACHE(cx).test(cx, regs.pc, aobj, obj2, entry, atom);
            if (!atom) {
                ASSERT_VALID_PROPERTY_CACHE_HIT(i, aobj, obj2, entry);
                if (entry->vword.isFunObj()) {
                    rval.setFunObj(entry->vword.toFunObj());
                } else if (entry->vword.isSlot()) {
                    uint32 slot = entry->vword.toSlot();
                    JS_ASSERT(slot < obj2->scope()->freeslot);
                    rval = obj2->lockedGetSlot(slot);
                } else {
                    JS_ASSERT(entry->vword.isSprop());
                    JSScopeProperty *sprop = entry->vword.toSprop();
                    NATIVE_GET(cx, obj, obj2, sprop,
                               fp->imacpc ? JSGET_NO_METHOD_BARRIER : JSGET_METHOD_BARRIER,
                               &rval);
                }
                break;
            }

            jsid id = ATOM_TO_JSID(atom);
            if (JS_LIKELY(aobj->map->ops->getProperty == js_GetProperty)
                ? !js_GetPropertyHelper(cx, obj, id,
                                        fp->imacpc
                                        ? JSGET_CACHE_RESULT | JSGET_NO_METHOD_BARRIER
                                        : JSGET_CACHE_RESULT | JSGET_METHOD_BARRIER,
                                        &rval)
                : !obj->getProperty(cx, id, &rval)) {
                goto error;
            }
        } while (0);

        regs.sp[-1] = rval;
        JS_ASSERT(JSOP_GETPROP_LENGTH + i == js_CodeSpec[op].length);
        len = JSOP_GETPROP_LENGTH + i;
    }
END_VARLEN_CASE

BEGIN_CASE(JSOP_LENGTH)
    vp = &regs.sp[-1];
    if (vp->isString()) {
        vp->setInt32(vp->asString()->length());
    } else if (vp->isObject()) {
        JSObject *obj = &vp->asObject();
        if (obj->isArray()) {
            jsuint length = obj->getArrayLength();
            regs.sp[-1].setDouble(length);
        } else if (obj->isArguments() && !obj->isArgsLengthOverridden()) {
            uint32 length = obj->getArgsLength();
            JS_ASSERT(length < INT32_MAX);
            regs.sp[-1].setInt32(int32_t(length));
        } else {
            i = -2;
            goto do_getprop_with_lval;
        }
    } else {
        i = -2;
        goto do_getprop_with_lval;
    }
END_CASE(JSOP_LENGTH)

}

BEGIN_CASE(JSOP_CALLPROP)
{
    Value lval;
    lval = regs.sp[-1];

    Value objv;
    if (lval.isObject()) {
        objv = lval;
    } else {
        JSProtoKey protoKey;
        if (lval.isString()) {
            protoKey = JSProto_String;
        } else if (lval.isNumber()) {
            protoKey = JSProto_Number;
        } else if (lval.isBoolean()) {
            protoKey = JSProto_Boolean;
        } else {
            JS_ASSERT(lval.isNull() || lval.isUndefined());
            js_ReportIsNullOrUndefined(cx, -1, lval, NULL);
            goto error;
        }
        JSObject *pobj;
        if (!js_GetClassPrototype(cx, NULL, protoKey, &pobj))
            goto error;
        objv.setNonFunObj(*pobj);
    }

    JSObject *aobj = js_GetProtoIfDenseArray(&objv.asObject());
    Value rval;

    PropertyCacheEntry *entry;
    JSObject *obj2;
    JSAtom *atom;
    JS_PROPERTY_CACHE(cx).test(cx, regs.pc, aobj, obj2, entry, atom);
    if (!atom) {
        ASSERT_VALID_PROPERTY_CACHE_HIT(0, aobj, obj2, entry);
        if (entry->vword.isFunObj()) {
            regs.sp[-1].setFunObj(entry->vword.toFunObj());
            PUSH_COPY(lval);
            goto end_callprop;
        } else if (entry->vword.isSlot()) {
            uint32 slot = entry->vword.toSlot();
            JS_ASSERT(slot < obj2->scope()->freeslot);
            rval = obj2->lockedGetSlot(slot);
        } else {
            JS_ASSERT(entry->vword.isSprop());
            JSScopeProperty *sprop = entry->vword.toSprop();
            NATIVE_GET(cx, &objv.asObject(), obj2, sprop, JSGET_NO_METHOD_BARRIER, &rval);
        }
        regs.sp[-1] = rval;
        PUSH_COPY(lval);
        goto end_callprop;
    }

    



    jsid id;
    id = ATOM_TO_JSID(atom);

    PUSH_NULL();
    if (lval.isObject()) {
        if (!js_GetMethod(cx, &objv.asObject(), id,
                          JS_LIKELY(aobj->map->ops->getProperty == js_GetProperty)
                          ? JSGET_CACHE_RESULT | JSGET_NO_METHOD_BARRIER
                          : JSGET_NO_METHOD_BARRIER,
                          &rval)) {
            goto error;
        }
        regs.sp[-1] = objv;
        regs.sp[-2] = rval;
    } else {
        JS_ASSERT(objv.asObject().map->ops->getProperty == js_GetProperty);
        if (!js_GetPropertyHelper(cx, &objv.asObject(), id,
                                  JSGET_CACHE_RESULT | JSGET_NO_METHOD_BARRIER,
                                  &rval)) {
            goto error;
        }
        regs.sp[-1] = lval;
        regs.sp[-2] = rval;
    }

  end_callprop:
    
    if (lval.isPrimitive()) {
        
        if (!rval.isFunObj() ||
            !PrimitiveValue::test(GET_FUNCTION_PRIVATE(cx, &rval.asFunObj()),
                                  lval)) {
            if (!js_PrimitiveToObject(cx, &regs.sp[-1]))
                goto error;
        }
    }
#if JS_HAS_NO_SUCH_METHOD
    if (JS_UNLIKELY(rval.isUndefined())) {
        LOAD_ATOM(0, atom);
        regs.sp[-2].setString(ATOM_TO_STRING(atom));
        if (!js_OnUnknownMethod(cx, regs.sp - 2))
            goto error;
    }
#endif
}
END_CASE(JSOP_CALLPROP)

BEGIN_CASE(JSOP_UNBRAND)
    JS_ASSERT(regs.sp - fp->slots() >= 1);
    if (!regs.sp[-1].asObject().unbrand(cx))
        goto error;
END_CASE(JSOP_UNBRAND)

BEGIN_CASE(JSOP_SETNAME)
BEGIN_CASE(JSOP_SETPROP)
BEGIN_CASE(JSOP_SETMETHOD)
{
    Value &rref = regs.sp[-1];
    JS_ASSERT_IF(op == JSOP_SETMETHOD, rref.isFunObj());
    Value &lref = regs.sp[-2];
    JS_ASSERT_IF(op == JSOP_SETNAME, lref.isObject());
    JSObject *obj;
    VALUE_TO_OBJECT(cx, &lref, obj);

    do {
        PropertyCache *cache = &JS_PROPERTY_CACHE(cx);

        


















        PropertyCacheEntry *entry;
        JSObject *obj2;
        JSAtom *atom;
        if (cache->testForSet(cx, regs.pc, obj, &entry, &obj2, &atom)) {
            








            JS_ASSERT(entry->vword.isSprop());
            JSScopeProperty *sprop = entry->vword.toSprop();
            JS_ASSERT_IF(sprop->isDataDescriptor(), sprop->writable());
            JS_ASSERT_IF(sprop->hasSlot(), entry->vcapTag() == 0);

            JSScope *scope = obj->scope();
            JS_ASSERT(!scope->sealed());

            





            bool checkForAdd;
            if (!sprop->hasSlot()) {
                if (entry->vcapTag() == 0 ||
                    ((obj2 = obj->getProto()) &&
                     obj2->isNative() &&
                     obj2->shape() == entry->vshape())) {
                    goto fast_set_propcache_hit;
                }

                
                checkForAdd = false;
            } else if (!scope->isSharedEmpty()) {
                if (sprop == scope->lastProperty() || scope->hasProperty(sprop)) {
                  fast_set_propcache_hit:
                    PCMETER(cache->pchits++);
                    PCMETER(cache->setpchits++);
                    NATIVE_SET(cx, obj, sprop, entry, &rref);
                    break;
                }
                checkForAdd = sprop->hasSlot() && sprop->parent == scope->lastProperty();
            } else {
                




                JS_ASSERT(CX_OWNS_OBJECT_TITLE(cx, obj));
                scope = js_GetMutableScope(cx, obj);
                JS_ASSERT(CX_OWNS_OBJECT_TITLE(cx, obj));
                if (!scope)
                    goto error;
                checkForAdd = !sprop->parent;
            }

            uint32 slot;
            if (checkForAdd &&
                entry->vshape() == rt->protoHazardShape &&
                sprop->hasDefaultSetter() &&
                (slot = sprop->slot) == scope->freeslot) {
                









                PCMETER(cache->pchits++);
                PCMETER(cache->addpchits++);

                




                if (slot < obj->numSlots() &&
                    !obj->getClass()->reserveSlots) {
                    ++scope->freeslot;
                } else {
                    if (!js_AllocSlot(cx, obj, &slot))
                        goto error;
                }

                









                if (slot != sprop->slot || scope->table) {
                    JSScopeProperty *sprop2 =
                        scope->putProperty(cx, sprop->id,
                                           sprop->getter(), sprop->setter(),
                                           slot, sprop->attributes(),
                                           sprop->getFlags(), sprop->shortid);
                    if (!sprop2) {
                        js_FreeSlot(cx, obj, slot);
                        goto error;
                    }
                    sprop = sprop2;
                } else {
                    scope->extend(cx, sprop);
                }

                





                TRACE_2(SetPropHit, entry, sprop);
                obj->lockedSetSlot(slot, rref);

                




                js_PurgeScopeChain(cx, obj, sprop->id);
                break;
            }
            PCMETER(cache->setpcmisses++);
            atom = NULL;
        } else if (!atom) {
            



            ASSERT_VALID_PROPERTY_CACHE_HIT(0, obj, obj2, entry);
            JSScopeProperty *sprop = NULL;
            if (obj == obj2) {
                sprop = entry->vword.toSprop();
                JS_ASSERT(sprop->writable());
                JS_ASSERT(!obj2->scope()->sealed());
                NATIVE_SET(cx, obj, sprop, entry, &rref);
            }
            if (sprop)
                break;
        }

        if (!atom)
            LOAD_ATOM(0, atom);
        jsid id = ATOM_TO_JSID(atom);
        if (entry && JS_LIKELY(obj->map->ops->setProperty == js_SetProperty)) {
            uintN defineHow;
            if (op == JSOP_SETMETHOD)
                defineHow = JSDNP_CACHE_RESULT | JSDNP_SET_METHOD;
            else if (op == JSOP_SETNAME)
                defineHow = JSDNP_CACHE_RESULT | JSDNP_UNQUALIFIED;
            else
                defineHow = JSDNP_CACHE_RESULT;
            if (!js_SetPropertyHelper(cx, obj, id, defineHow, &rref))
                goto error;
        } else {
            if (!obj->setProperty(cx, id, &rref))
                goto error;
            ABORT_RECORDING(cx, "Non-native set");
        }
    } while (0);
}
END_SET_CASE_STORE_RVAL(JSOP_SETPROP, 2);

BEGIN_CASE(JSOP_GETELEM)
{
    Value &lref = regs.sp[-2];
    Value &rref = regs.sp[-1];
    if (lref.isString() && rref.isInt32()) {
        JSString *str = lref.asString();
        int32_t i = rref.asInt32();
        if (size_t(i) < str->length()) {
            str = JSString::getUnitString(cx, str, size_t(i));
            if (!str)
                goto error;
            regs.sp--;
            regs.sp[-1].setString(str);
            len = JSOP_GETELEM_LENGTH;
            DO_NEXT_OP(len);
        }
    }

    JSObject *obj;
    VALUE_TO_OBJECT(cx, &lref, obj);

    const Value *copyFrom;
    Value rval;
    jsid id;
    if (rref.isInt32()) {
        if (obj->isDenseArray()) {
            jsuint idx = jsuint(rref.asInt32());

            if (idx < obj->getArrayLength() &&
                idx < obj->getDenseArrayCapacity()) {
                copyFrom = obj->addressOfDenseArrayElement(idx);
                if (!copyFrom->isMagic())
                    goto end_getelem;

                
                copyFrom = &regs.sp[-1];
            }
        } else if (obj->isArguments()
#ifdef JS_TRACER
                   && !GetArgsPrivateNative(obj)
#endif
                  ) {
            uint32 arg = uint32(rref.asInt32());

            if (arg < obj->getArgsLength()) {
                JSStackFrame *afp = (JSStackFrame *) obj->getPrivate();
                if (afp) {
                    copyFrom = &afp->argv[arg];
                    goto end_getelem;
                }

                copyFrom = obj->addressOfArgsElement(arg);
                if (!copyFrom->isMagic())
                    goto end_getelem;
                copyFrom = &regs.sp[-1];
            }
        }
        int32_t i32 = rref.asInt32();
        if (INT32_FITS_IN_JSID(i32))
            id = INT_TO_JSID(i32);
        else
            goto intern_big_int;
    } else {
      intern_big_int:
        if (!js_InternNonIntElementId(cx, obj, rref, &id))
            goto error;
    }

    if (!obj->getProperty(cx, id, &rval))
        goto error;
    copyFrom = &rval;

  end_getelem:
    regs.sp--;
    regs.sp[-1] = *copyFrom;
}
END_CASE(JSOP_GETELEM)

BEGIN_CASE(JSOP_CALLELEM)
{
    

    
    JSObject *obj;
    FETCH_OBJECT(cx, -2, obj);

    
    uint32 objmask = regs.sp[-2].data.s.mask32;

    
    jsid id;
    FETCH_ELEMENT_ID(obj, -1, id);

    
    if (!js_GetMethod(cx, obj, id, JSGET_NO_METHOD_BARRIER, &regs.sp[-2]))
        goto error;

#if JS_HAS_NO_SUCH_METHOD
    if (JS_UNLIKELY(regs.sp[-2].isUndefined())) {
        regs.sp[-2] = regs.sp[-1];
        regs.sp[-1].setObject(*obj);
        if (!js_OnUnknownMethod(cx, regs.sp - 2))
            goto error;
    } else
#endif
    {
        regs.sp[-1].data.s.mask32 = objmask;
        regs.sp[-1].data.s.payload.obj = obj;
    }
}
END_CASE(JSOP_CALLELEM)

BEGIN_CASE(JSOP_SETELEM)
{
    JSObject *obj;
    FETCH_OBJECT(cx, -3, obj);
    jsid id;
    FETCH_ELEMENT_ID(obj, -2, id);
    do {
        if (obj->isDenseArray() && JSID_IS_INT(id)) {
            jsuint length = obj->getDenseArrayCapacity();
            jsint i = JSID_TO_INT(id);
            if ((jsuint)i < length) {
                if (obj->getDenseArrayElement(i).isMagic(JS_ARRAY_HOLE)) {
                    if (js_PrototypeHasIndexedProperties(cx, obj))
                        break;
                    if ((jsuint)i >= obj->getArrayLength())
                        obj->setDenseArrayLength(i + 1);
                    obj->incDenseArrayCountBy(1);
                }
                obj->setDenseArrayElement(i, regs.sp[-1]);
                goto end_setelem;
            }
        }
    } while (0);
    if (!obj->setProperty(cx, id, &regs.sp[-1]))
        goto error;
  end_setelem:;
}
END_SET_CASE_STORE_RVAL(JSOP_SETELEM, 3)

BEGIN_CASE(JSOP_ENUMELEM)
{
    
    JSObject *obj;
    FETCH_OBJECT(cx, -2, obj);
    jsid id;
    FETCH_ELEMENT_ID(obj, -1, id);
    if (!obj->setProperty(cx, id, &regs.sp[-3]))
        goto error;
    regs.sp -= 3;
}
END_CASE(JSOP_ENUMELEM)

{
    JSFunction *fun;
    JSObject *obj;
    uintN flags;
    uintN argc;
    Value *vp;

BEGIN_CASE(JSOP_NEW)
{
    
    argc = GET_ARGC(regs.pc);
    vp = regs.sp - (2 + argc);
    JS_ASSERT(vp >= fp->base());

    



    if (vp[0].isFunObj()) {
        obj = &vp[0].asFunObj();
        fun = GET_FUNCTION_PRIVATE(cx, obj);
        if (fun->isInterpreted()) {
            
            if (!obj->getProperty(cx,
                                  ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                                  &vp[1])) {
                goto error;
            }
            JSObject *proto = vp[1].isObject() ? &vp[1].asObject() : NULL;
            JSObject *obj2 = NewObject(cx, &js_ObjectClass, proto, obj->getParent());
            if (!obj2)
                goto error;

            if (fun->u.i.script->isEmpty()) {
                vp[0].setNonFunObj(*obj2);
                regs.sp = vp + 1;
                goto end_new;
            }

            vp[1].setNonFunObj(*obj2);
            flags = JSFRAME_CONSTRUCTING;
            goto inline_call;
        }
    }

    if (!InvokeConstructor(cx, InvokeArgsGuard(vp, argc), JS_FALSE))
        goto error;
    regs.sp = vp + 1;
    CHECK_INTERRUPT_HANDLER();
    TRACE_0(NativeCallComplete);

  end_new:;
}
END_CASE(JSOP_NEW)

BEGIN_CASE(JSOP_CALL)
BEGIN_CASE(JSOP_EVAL)
BEGIN_CASE(JSOP_APPLY)
{
    argc = GET_ARGC(regs.pc);
    vp = regs.sp - (argc + 2);

    if (vp->isFunObj()) {
        obj = &vp->asFunObj();
        fun = GET_FUNCTION_PRIVATE(cx, obj);

        
        flags = 0;
        if (FUN_INTERPRETED(fun))
      inline_call:
        {
            JSScript *newscript = fun->u.i.script;
            if (JS_UNLIKELY(newscript->isEmpty())) {
                vp->setUndefined();
                regs.sp = vp + 1;
                goto end_call;
            }

            
            if (JS_UNLIKELY(inlineCallCount >= JS_MAX_INLINE_CALL_COUNT)) {
                js_ReportOverRecursed(cx);
                goto error;
            }

            



            StackSpace &stack = cx->stack();
            uintN nfixed = newscript->nslots;
            uintN funargs = fun->nargs;
            JSStackFrame *newfp;
            if (argc < funargs) {
                uintN missing = funargs - argc;
                newfp = stack.getInlineFrame(cx, regs.sp, missing, nfixed);
                if (!newfp)
                    goto error;
                SetValueRangeToUndefined(regs.sp, missing);
            } else {
                newfp = stack.getInlineFrame(cx, regs.sp, 0, nfixed);
                if (!newfp)
                    goto error;
            }

            
            newfp->callobj = NULL;
            newfp->argsobj = NULL;
            newfp->script = newscript;
            newfp->fun = fun;
            newfp->argc = argc;
            newfp->argv = vp + 2;
            newfp->rval.setUndefined();
            newfp->annotation = NULL;
            newfp->scopeChain = obj->getParent();
            newfp->flags = flags;
            newfp->blockChain = NULL;
            if (JS_LIKELY(newscript->staticLevel < JS_DISPLAY_SIZE)) {
                JSStackFrame **disp = &cx->display[newscript->staticLevel];
                newfp->displaySave = *disp;
                *disp = newfp;
            }
            JS_ASSERT(!JSFUN_BOUND_METHOD_TEST(fun->flags));
            newfp->thisv = vp[1];
            newfp->imacpc = NULL;

            
            Value *newsp = newfp->base();
            SetValueRangeToUndefined(newfp->slots(), newsp);

            
            if (fun->isHeavyweight() && !js_GetCallObject(cx, newfp))
                goto error;

            
            newfp->callerVersion = (JSVersion) cx->version;
            if (JS_LIKELY(cx->version == currentVersion)) {
                currentVersion = (JSVersion) newscript->version;
                if (JS_UNLIKELY(currentVersion != cx->version))
                    js_SetVersion(cx, currentVersion);
            }

            
            stack.pushInlineFrame(cx, fp, regs.pc, newfp);

            
            regs.pc = newscript->code;
            regs.sp = newsp;

            
            JS_ASSERT(newfp == cx->fp);
            fp = newfp;
            script = newscript;
            atoms = script->atomMap.vector;

            
            if (JSInterpreterHook hook = cx->debugHooks->callHook) {
                fp->hookData = hook(cx, fp, JS_TRUE, 0,
                                    cx->debugHooks->callHookData);
                CHECK_INTERRUPT_HANDLER();
            } else {
                fp->hookData = NULL;
            }

            inlineCallCount++;
            JS_RUNTIME_METER(rt, inlineCalls);

            DTrace::enterJSFun(cx, fp, fun, fp->down, fp->argc, fp->argv);

#ifdef JS_TRACER
            if (TraceRecorder *tr = TRACE_RECORDER(cx)) {
                AbortableRecordingStatus status = tr->record_EnterFrame(inlineCallCount);
                RESTORE_INTERP_VARS();
                if (StatusAbortsRecorderIfActive(status)) {
                    if (TRACE_RECORDER(cx)) {
                        JS_ASSERT(TRACE_RECORDER(cx) == tr);
                        AbortRecording(cx, "record_EnterFrame failed");
                    }
                    if (status == ARECORD_ERROR)
                        goto error;
                }
            } else if (fp->script == fp->down->script &&
                       *fp->down->savedPC == JSOP_CALL &&
                       *regs.pc == JSOP_TRACE) {
                MONITOR_BRANCH(Record_EnterFrame);
            }
#endif

            
            op = (JSOp) *regs.pc;
            DO_OP();
        }

        if (fun->flags & JSFUN_FAST_NATIVE) {
            DTrace::enterJSFun(cx, NULL, fun, fp, argc, vp + 2, vp);

            JS_ASSERT(fun->u.n.extra == 0);
            JS_ASSERT(vp[1].isObjectOrNull() || PrimitiveValue::test(fun, vp[1]));
            JSBool ok = ((FastNative) fun->u.n.native)(cx, argc, vp);
            DTrace::exitJSFun(cx, NULL, fun, *vp, vp);
            regs.sp = vp + 1;
            if (!ok)
                goto error;
            TRACE_0(NativeCallComplete);
            goto end_call;
        }
    }

    JSBool ok;
    ok = Invoke(cx, InvokeArgsGuard(vp, argc), 0);
    regs.sp = vp + 1;
    CHECK_INTERRUPT_HANDLER();
    if (!ok)
        goto error;
    JS_RUNTIME_METER(rt, nonInlineCalls);
    TRACE_0(NativeCallComplete);

  end_call:;
}
END_CASE(JSOP_CALL)
}

BEGIN_CASE(JSOP_SETCALL)
{
    uintN argc = GET_ARGC(regs.pc);
    Value *vp = regs.sp - argc - 2;
    JSBool ok = Invoke(cx, InvokeArgsGuard(vp, argc), 0);
    if (ok)
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_LEFTSIDE_OF_ASS);
    goto error;
}
END_CASE(JSOP_SETCALL)

BEGIN_CASE(JSOP_NAME)
BEGIN_CASE(JSOP_CALLNAME)
{
    JSObject *obj = fp->scopeChain;

    JSScopeProperty *sprop;
    Value rval;

    PropertyCacheEntry *entry;
    JSObject *obj2;
    JSAtom *atom;
    JS_PROPERTY_CACHE(cx).test(cx, regs.pc, obj, obj2, entry, atom);
    if (!atom) {
        ASSERT_VALID_PROPERTY_CACHE_HIT(0, obj, obj2, entry);
        if (entry->vword.isFunObj()) {
            PUSH_FUNOBJ(entry->vword.toFunObj());
            goto do_push_obj_if_call;
        }

        if (entry->vword.isSlot()) {
            uintN slot = entry->vword.toSlot();
            JS_ASSERT(slot < obj2->scope()->freeslot);
            PUSH_COPY(obj2->lockedGetSlot(slot));
            goto do_push_obj_if_call;
        }

        JS_ASSERT(entry->vword.isSprop());
        sprop = entry->vword.toSprop();
        goto do_native_get;
    }

    jsid id;
    id = ATOM_TO_JSID(atom);
    JSProperty *prop;
    if (!js_FindPropertyHelper(cx, id, true, &obj, &obj2, &prop))
        goto error;
    if (!prop) {
        
        JSOp op2 = js_GetOpcode(cx, script, regs.pc + JSOP_NAME_LENGTH);
        if (op2 == JSOP_TYPEOF) {
            PUSH_UNDEFINED();
            len = JSOP_NAME_LENGTH;
            DO_NEXT_OP(len);
        }
        atomNotDefined = atom;
        goto atom_not_defined;
    }

    
    if (!obj->isNative() || !obj2->isNative()) {
        obj2->dropProperty(cx, prop);
        if (!obj->getProperty(cx, id, &rval))
            goto error;
    } else {
        sprop = (JSScopeProperty *)prop;
  do_native_get:
        NATIVE_GET(cx, obj, obj2, sprop, JSGET_METHOD_BARRIER, &rval);
        obj2->dropProperty(cx, (JSProperty *) sprop);
    }

    PUSH_COPY(rval);

  do_push_obj_if_call:
    
    if (op == JSOP_CALLNAME)
        PUSH_NONFUNOBJ(*obj);
}
END_CASE(JSOP_NAME)

BEGIN_CASE(JSOP_UINT16)
    PUSH_INT32((int32_t) GET_UINT16(regs.pc));
END_CASE(JSOP_UINT16)

BEGIN_CASE(JSOP_UINT24)
    PUSH_INT32((int32_t) GET_UINT24(regs.pc));
END_CASE(JSOP_UINT24)

BEGIN_CASE(JSOP_INT8)
    PUSH_INT32(GET_INT8(regs.pc));
END_CASE(JSOP_INT8)

BEGIN_CASE(JSOP_INT32)
    PUSH_INT32(GET_INT32(regs.pc));
END_CASE(JSOP_INT32)

BEGIN_CASE(JSOP_INDEXBASE)
    



    atoms += GET_INDEXBASE(regs.pc);
END_CASE(JSOP_INDEXBASE)

BEGIN_CASE(JSOP_INDEXBASE1)
BEGIN_CASE(JSOP_INDEXBASE2)
BEGIN_CASE(JSOP_INDEXBASE3)
    atoms += (op - JSOP_INDEXBASE1 + 1) << 16;
END_CASE(JSOP_INDEXBASE3)

BEGIN_CASE(JSOP_RESETBASE0)
BEGIN_CASE(JSOP_RESETBASE)
    atoms = script->atomMap.vector;
END_CASE(JSOP_RESETBASE)

BEGIN_CASE(JSOP_DOUBLE)
{
    JS_ASSERT(!fp->imacpc);
    JS_ASSERT(size_t(atoms - script->atomMap.vector) < script->atomMap.length);
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    PUSH_DOUBLE(*ATOM_TO_DOUBLE(atom));
}
END_CASE(JSOP_DOUBLE)

BEGIN_CASE(JSOP_STRING)
{
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    PUSH_STRING(ATOM_TO_STRING(atom));
}
END_CASE(JSOP_STRING)

BEGIN_CASE(JSOP_OBJECT)
{
    JSObject *obj;
    LOAD_OBJECT(0, obj);
    
    PUSH_NONFUNOBJ(*obj);
}
END_CASE(JSOP_OBJECT)

BEGIN_CASE(JSOP_REGEXP)
{
    







    jsatomid index = GET_FULL_INDEX(0);
    JSObject *proto;
    if (!js_GetClassPrototype(cx, fp->scopeChain, JSProto_RegExp, &proto))
        goto error;
    JS_ASSERT(proto);
    JSObject *obj = js_CloneRegExpObject(cx, script->getRegExp(index), proto);
    if (!obj)
        goto error;
    PUSH_NONFUNOBJ(*obj);
}
END_CASE(JSOP_REGEXP)

BEGIN_CASE(JSOP_ZERO)
    PUSH_INT32(0);
END_CASE(JSOP_ZERO)

BEGIN_CASE(JSOP_ONE)
    PUSH_INT32(1);
END_CASE(JSOP_ONE)

BEGIN_CASE(JSOP_NULL)
    PUSH_NULL();
END_CASE(JSOP_NULL)

BEGIN_CASE(JSOP_FALSE)
    PUSH_BOOLEAN(false);
END_CASE(JSOP_FALSE)

BEGIN_CASE(JSOP_TRUE)
    PUSH_BOOLEAN(true);
END_CASE(JSOP_TRUE)

{
BEGIN_CASE(JSOP_TABLESWITCH)
{
    jsbytecode *pc2 = regs.pc;
    len = GET_JUMP_OFFSET(pc2);

    




    const Value &rref = *--regs.sp;
    int32_t i;
    if (rref.isInt32()) {
        i = rref.asInt32();
    } else if (rref.isDouble() && rref.asDouble() == 0) {
        
        i = 0;
    } else {
        DO_NEXT_OP(len);
    }

    pc2 += JUMP_OFFSET_LEN;
    jsint low = GET_JUMP_OFFSET(pc2);
    pc2 += JUMP_OFFSET_LEN;
    jsint high = GET_JUMP_OFFSET(pc2);

    i -= low;
    if ((jsuint)i < (jsuint)(high - low + 1)) {
        pc2 += JUMP_OFFSET_LEN + JUMP_OFFSET_LEN * i;
        jsint off = (jsint) GET_JUMP_OFFSET(pc2);
        if (off)
            len = off;
    }
}
END_VARLEN_CASE
}

{
BEGIN_CASE(JSOP_TABLESWITCHX)
{
    jsbytecode *pc2 = regs.pc;
    len = GET_JUMPX_OFFSET(pc2);

    




    const Value &rref = *--regs.sp;
    int32_t i;
    if (rref.isInt32()) {
        i = rref.asInt32();
    } else if (rref.isDouble() && rref.asDouble() == 0) {
        
        i = 0;
    } else {
        DO_NEXT_OP(len);
    }

    pc2 += JUMPX_OFFSET_LEN;
    jsint low = GET_JUMP_OFFSET(pc2);
    pc2 += JUMP_OFFSET_LEN;
    jsint high = GET_JUMP_OFFSET(pc2);

    i -= low;
    if ((jsuint)i < (jsuint)(high - low + 1)) {
        pc2 += JUMP_OFFSET_LEN + JUMPX_OFFSET_LEN * i;
        jsint off = (jsint) GET_JUMPX_OFFSET(pc2);
        if (off)
            len = off;
    }
}
END_VARLEN_CASE
}

{
BEGIN_CASE(JSOP_LOOKUPSWITCHX)
{
    jsint off;
    off = JUMPX_OFFSET_LEN;
    goto do_lookup_switch;

BEGIN_CASE(JSOP_LOOKUPSWITCH)
    off = JUMP_OFFSET_LEN;

  do_lookup_switch:
    



    JS_ASSERT(!fp->imacpc);
    JS_ASSERT(atoms == script->atomMap.vector);
    jsbytecode *pc2 = regs.pc;

    Value lval;
    lval = regs.sp[-1];
    regs.sp--;

    if (!lval.isPrimitive())
        goto end_lookup_switch;

    pc2 += off;
    jsint npairs;
    npairs = (jsint) GET_UINT16(pc2);
    pc2 += UINT16_LEN;
    JS_ASSERT(npairs);  

    bool match;
#define SEARCH_PAIRS(MATCH_CODE)                                              \
    for (;;) {                                                                \
        JS_ASSERT(GET_INDEX(pc2) < script->atomMap.length);                   \
        JSAtom *atom = atoms[GET_INDEX(pc2)];                                 \
        jsboxedword rval = ATOM_KEY(atom);                                    \
        MATCH_CODE                                                            \
        pc2 += INDEX_LEN;                                                     \
        if (match)                                                            \
            break;                                                            \
        pc2 += off;                                                           \
        if (--npairs == 0) {                                                  \
            pc2 = regs.pc;                                                    \
            break;                                                            \
        }                                                                     \
    }

    if (lval.isString()) {
        JSString *str = lval.asString();
        JSString *str2;
        SEARCH_PAIRS(
            match = (JSBOXEDWORD_IS_STRING(rval) &&
                     ((str2 = JSBOXEDWORD_TO_STRING(rval)) == str ||
                      js_EqualStrings(str2, str)));
        )
    } else if (lval.isNumber()) {
        double dbl = lval.asNumber();
        SEARCH_PAIRS(
            match = (JSBOXEDWORD_IS_INT(rval) && dbl == (double)JSBOXEDWORD_TO_INT(rval)) ||
                    (JSBOXEDWORD_IS_DOUBLE(rval) && dbl == *JSBOXEDWORD_TO_DOUBLE(rval));
        )
    } else if (lval.isUndefined() || lval.isBoolean()) {
        jsint s = lval.isUndefined() ? 2 : lval.asBoolean();
        SEARCH_PAIRS(
            match = JSBOXEDWORD_IS_SPECIAL(rval) && JSBOXEDWORD_TO_SPECIAL(rval) == s;
        )
    } else {
        JS_ASSERT(lval.isNull());
        SEARCH_PAIRS(
            match = JSBOXEDWORD_IS_NULL(rval);
        )
    }
#undef SEARCH_PAIRS

  end_lookup_switch:
    len = (op == JSOP_LOOKUPSWITCH)
          ? GET_JUMP_OFFSET(pc2)
          : GET_JUMPX_OFFSET(pc2);
}
END_VARLEN_CASE
}

BEGIN_CASE(JSOP_TRAP)
{
    Value rval;
    JSTrapStatus status = JS_HandleTrap(cx, script, regs.pc, Jsvalify(&rval));
    switch (status) {
      case JSTRAP_ERROR:
        goto error;
      case JSTRAP_RETURN:
        fp->rval = rval;
        interpReturnOK = JS_TRUE;
        goto forced_return;
      case JSTRAP_THROW:
        cx->throwing = JS_TRUE;
        cx->exception = rval;
        goto error;
      default:
        break;
    }
    JS_ASSERT(status == JSTRAP_CONTINUE);
    CHECK_INTERRUPT_HANDLER();
    JS_ASSERT(rval.isInt32());
    op = (JSOp) rval.asInt32();
    JS_ASSERT((uintN)op < (uintN)JSOP_LIMIT);
    DO_OP();
}

BEGIN_CASE(JSOP_ARGUMENTS)
{
    Value rval;
    if (!js_GetArgsValue(cx, fp, &rval))
        goto error;
    PUSH_COPY(rval);
}
END_CASE(JSOP_ARGUMENTS)

BEGIN_CASE(JSOP_ARGSUB)
{
    jsid id = INT_TO_JSID(GET_ARGNO(regs.pc));
    Value rval;
    if (!js_GetArgsProperty(cx, fp, id, &rval))
        goto error;
    PUSH_COPY(rval);
}
END_CASE(JSOP_ARGSUB)

BEGIN_CASE(JSOP_ARGCNT)
{
    jsid id = ATOM_TO_JSID(rt->atomState.lengthAtom);
    Value rval;
    if (!js_GetArgsProperty(cx, fp, id, &rval))
        goto error;
    PUSH_COPY(rval);
}
END_CASE(JSOP_ARGCNT)

BEGIN_CASE(JSOP_GETARG)
BEGIN_CASE(JSOP_CALLARG)
{
    uint32 slot = GET_ARGNO(regs.pc);
    JS_ASSERT(slot < fp->fun->nargs);
    METER_SLOT_OP(op, slot);
    PUSH_COPY(fp->argv[slot]);
    if (op == JSOP_CALLARG)
        PUSH_NULL();
}
END_CASE(JSOP_GETARG)

BEGIN_CASE(JSOP_SETARG)
{
    uint32 slot = GET_ARGNO(regs.pc);
    JS_ASSERT(slot < fp->fun->nargs);
    METER_SLOT_OP(op, slot);
    fp->argv[slot] = regs.sp[-1];
}
END_SET_CASE(JSOP_SETARG)

BEGIN_CASE(JSOP_GETLOCAL)
{
    uint32 slot = GET_SLOTNO(regs.pc);
    JS_ASSERT(slot < script->nslots);
    PUSH_COPY(fp->slots()[slot]);
}
END_CASE(JSOP_GETLOCAL)

BEGIN_CASE(JSOP_CALLLOCAL)
{
    uint32 slot = GET_SLOTNO(regs.pc);
    JS_ASSERT(slot < script->nslots);
    PUSH_COPY(fp->slots()[slot]);
    PUSH_NULL();
}
END_CASE(JSOP_CALLLOCAL)

BEGIN_CASE(JSOP_SETLOCAL)
{
    uint32 slot = GET_SLOTNO(regs.pc);
    JS_ASSERT(slot < script->nslots);
    fp->slots()[slot] = regs.sp[-1];
}
END_SET_CASE(JSOP_SETLOCAL)

BEGIN_CASE(JSOP_GETUPVAR)
BEGIN_CASE(JSOP_CALLUPVAR)
{
    JSUpvarArray *uva = script->upvars();

    uintN index = GET_UINT16(regs.pc);
    JS_ASSERT(index < uva->length);

    const Value &rval = js_GetUpvar(cx, script->staticLevel, uva->vector[index]);
    PUSH_COPY(rval);

    if (op == JSOP_CALLUPVAR)
        PUSH_NULL();
}
END_CASE(JSOP_GETUPVAR)

BEGIN_CASE(JSOP_GETUPVAR_DBG)
BEGIN_CASE(JSOP_CALLUPVAR_DBG)
{
    JSFunction *fun = fp->fun;
    JS_ASSERT(FUN_KIND(fun) == JSFUN_INTERPRETED);
    JS_ASSERT(fun->u.i.wrapper);

    
    JSObject *obj, *obj2;
    JSProperty *prop;
    jsid id;
    JSAtom *atom;
    {
        void *mark = JS_ARENA_MARK(&cx->tempPool);
        jsuword *names = js_GetLocalNameArray(cx, fun, &cx->tempPool);
        if (!names)
            goto error;

        uintN index = fun->countArgsAndVars() + GET_UINT16(regs.pc);
        atom = JS_LOCAL_NAME_TO_ATOM(names[index]);
        id = ATOM_TO_JSID(atom);

        JSBool ok = js_FindProperty(cx, id, &obj, &obj2, &prop);
        JS_ARENA_RELEASE(&cx->tempPool, mark);
        if (!ok)
            goto error;
    }

    if (!prop) {
        atomNotDefined = atom;
        goto atom_not_defined;
    }

    
    obj2->dropProperty(cx, prop);
    Value *vp = regs.sp;
    PUSH_NULL();
    if (!obj->getProperty(cx, id, vp))
        goto error;

    if (op == JSOP_CALLUPVAR_DBG)
        PUSH_NULL();
}
END_CASE(JSOP_GETUPVAR_DBG)

BEGIN_CASE(JSOP_GETDSLOT)
BEGIN_CASE(JSOP_CALLDSLOT)
{
    JS_ASSERT(fp->argv);
    JSObject *obj = &fp->argv[-2].asObject();
    JS_ASSERT(obj);
    JS_ASSERT(obj->dslots);

    uintN index = GET_UINT16(regs.pc);
    JS_ASSERT(JS_INITIAL_NSLOTS + index < obj->dslots[-1].asPrivateUint32());
    JS_ASSERT_IF(obj->scope()->object == obj,
                 JS_INITIAL_NSLOTS + index < obj->scope()->freeslot);

    PUSH_COPY(obj->dslots[index]);
    if (op == JSOP_CALLDSLOT)
        PUSH_NULL();
}
END_CASE(JSOP_GETDSLOT)

BEGIN_CASE(JSOP_GETGVAR)
BEGIN_CASE(JSOP_CALLGVAR)
{
    uint32 slot = GET_SLOTNO(regs.pc);
    JS_ASSERT(slot < GlobalVarCount(fp));
    METER_SLOT_OP(op, slot);
    const Value &lval = fp->slots()[slot];
    if (lval.isNull()) {
        op = (op == JSOP_GETGVAR) ? JSOP_NAME : JSOP_CALLNAME;
        DO_OP();
    }
    JS_ASSERT(fp->varobj(cx) == cx->activeCallStack()->getInitialVarObj());
    JSObject *varobj = cx->activeCallStack()->getInitialVarObj();

    



    slot = (uint32)lval.asInt32();
    const Value &rref = varobj->lockedGetSlot(slot);
    PUSH_COPY(rref);
    if (op == JSOP_CALLGVAR)
        PUSH_NULL();
}
END_CASE(JSOP_GETGVAR)

BEGIN_CASE(JSOP_SETGVAR)
{
    uint32 slot = GET_SLOTNO(regs.pc);
    JS_ASSERT(slot < GlobalVarCount(fp));
    METER_SLOT_OP(op, slot);
    const Value &rref = regs.sp[-1];
    JS_ASSERT(fp->varobj(cx) == cx->activeCallStack()->getInitialVarObj());
    JSObject *obj = cx->activeCallStack()->getInitialVarObj();
    const Value &lref = fp->slots()[slot];
    if (lref.isNull()) {
        




#ifdef JS_TRACER
        if (TRACE_RECORDER(cx))
            AbortRecording(cx, "SETGVAR with NULL slot");
#endif
        JSAtom *atom;
        LOAD_ATOM(0, atom);
        jsid id = ATOM_TO_JSID(atom);
        Value rval;
        rval = rref;
        if (!obj->setProperty(cx, id, &rval))
            goto error;
    } else {
        uint32 slot = (uint32)lref.asInt32();
        JS_LOCK_OBJ(cx, obj);
        JSScope *scope = obj->scope();
        if (!scope->methodWriteBarrier(cx, slot, rref)) {
            JS_UNLOCK_SCOPE(cx, scope);
            goto error;
        }
        obj->lockedSetSlot(slot, rref);
        JS_UNLOCK_SCOPE(cx, scope);
    }
}
END_SET_CASE(JSOP_SETGVAR)

BEGIN_CASE(JSOP_DEFCONST)
BEGIN_CASE(JSOP_DEFVAR)
{
    uint32 index = GET_INDEX(regs.pc);
    JSAtom *atom = atoms[index];

    



    index += atoms - script->atomMap.vector;
    JSObject *obj = fp->varobj(cx);
    JS_ASSERT(obj->map->ops->defineProperty == js_DefineProperty);
    uintN attrs = JSPROP_ENUMERATE;
    if (!(fp->flags & JSFRAME_EVAL))
        attrs |= JSPROP_PERMANENT;
    if (op == JSOP_DEFCONST)
        attrs |= JSPROP_READONLY;

    
    jsid id = ATOM_TO_JSID(atom);
    JSProperty *prop = NULL;
    JSObject *obj2;
    if (op == JSOP_DEFVAR) {
        



        if (!obj->lookupProperty(cx, id, &obj2, &prop))
            goto error;
    } else {
        if (!CheckRedeclaration(cx, obj, id, attrs, &obj2, &prop))
            goto error;
    }

    
    if (!prop) {
        if (!js_DefineNativeProperty(cx, obj, id, Value(UndefinedTag()), PropertyStub, PropertyStub,
                                     attrs, 0, 0, &prop)) {
            goto error;
        }
        JS_ASSERT(prop);
        obj2 = obj;
    }

    





    if (!fp->fun &&
        index < GlobalVarCount(fp) &&
        obj2 == obj &&
        obj->isNative()) {
        JSScopeProperty *sprop = (JSScopeProperty *) prop;
        if (!sprop->configurable() &&
            SPROP_HAS_VALID_SLOT(sprop, obj->scope()) &&
            sprop->hasDefaultGetterOrIsMethod() &&
            sprop->hasDefaultSetter()) {
            





            fp->slots()[index].setInt32(sprop->slot);
        }
    }

    obj2->dropProperty(cx, prop);
}
END_CASE(JSOP_DEFVAR)

BEGIN_CASE(JSOP_DEFFUN)
{
    PropertyOp getter, setter;
    bool doSet;
    JSObject *pobj;
    JSProperty *prop;
    uint32 old;

    





    JSFunction *fun;
    LOAD_FUNCTION(0);
    JSObject *obj = FUN_OBJECT(fun);

    JSObject *obj2;
    if (FUN_NULL_CLOSURE(fun)) {
        




        obj2 = fp->scopeChain;
    } else {
        JS_ASSERT(!FUN_FLAT_CLOSURE(fun));

        



        if (!fp->blockChain) {
            obj2 = fp->scopeChain;
        } else {
            obj2 = js_GetScopeChain(cx, fp);
            if (!obj2)
                goto error;
        }
    }

    








    if (obj->getParent() != obj2) {
        obj = CloneFunctionObject(cx, fun, obj2);
        if (!obj)
            goto error;
    }

    




    MUST_FLOW_THROUGH("restore_scope");
    fp->scopeChain = obj;

    Value rval;
    rval.setFunObj(*obj);

    



    uintN attrs = (fp->flags & JSFRAME_EVAL)
                  ? JSPROP_ENUMERATE
                  : JSPROP_ENUMERATE | JSPROP_PERMANENT;

    




    getter = setter = PropertyStub;
    uintN flags = JSFUN_GSFLAG2ATTR(fun->flags);
    if (flags) {
        
        JS_ASSERT(flags == JSPROP_GETTER || flags == JSPROP_SETTER);
        attrs |= flags | JSPROP_SHARED;
        rval.setUndefined();
        if (flags == JSPROP_GETTER)
            getter = CastAsPropertyOp(obj);
        else
            setter = CastAsPropertyOp(obj);
    }

    




    JSObject *parent = fp->varobj(cx);
    JS_ASSERT(parent);

    




    jsid id = ATOM_TO_JSID(fun->atom);
    prop = NULL;
    JSBool ok = CheckRedeclaration(cx, parent, id, attrs, &pobj, &prop);
    if (!ok)
        goto restore_scope;

    










    doSet = (attrs == JSPROP_ENUMERATE);
    JS_ASSERT_IF(doSet, fp->flags & JSFRAME_EVAL);
    if (prop) {
        if (parent == pobj &&
            parent->getClass() == &js_CallClass &&
            (old = ((JSScopeProperty *) prop)->attributes(),
             !(old & (JSPROP_GETTER|JSPROP_SETTER)) &&
             (old & (JSPROP_ENUMERATE|JSPROP_PERMANENT)) == attrs)) {
            



            JS_ASSERT(!(attrs & ~(JSPROP_ENUMERATE|JSPROP_PERMANENT)));
            JS_ASSERT(!(old & JSPROP_READONLY));
            doSet = JS_TRUE;
        }
        pobj->dropProperty(cx, prop);
    }
    ok = doSet
         ? parent->setProperty(cx, id, &rval)
         : parent->defineProperty(cx, id, rval, getter, setter, attrs);

  restore_scope:
    
    fp->scopeChain = obj2;
    if (!ok)
        goto error;
}
END_CASE(JSOP_DEFFUN)

BEGIN_CASE(JSOP_DEFFUN_FC)
BEGIN_CASE(JSOP_DEFFUN_DBGFC)
{
    JSFunction *fun;
    LOAD_FUNCTION(0);

    JSObject *obj = (op == JSOP_DEFFUN_FC)
                    ? js_NewFlatClosure(cx, fun)
                    : js_NewDebuggableFlatClosure(cx, fun);
    if (!obj)
        goto error;

    Value rval;
    rval.setFunObj(*obj);

    uintN attrs = (fp->flags & JSFRAME_EVAL)
                  ? JSPROP_ENUMERATE
                  : JSPROP_ENUMERATE | JSPROP_PERMANENT;

    uintN flags = JSFUN_GSFLAG2ATTR(fun->flags);
    if (flags) {
        attrs |= flags | JSPROP_SHARED;
        rval.setUndefined();
    }

    JSObject *parent = fp->varobj(cx);
    JS_ASSERT(parent);

    jsid id = ATOM_TO_JSID(fun->atom);
    JSBool ok = CheckRedeclaration(cx, parent, id, attrs, NULL, NULL);
    if (ok) {
        if (attrs == JSPROP_ENUMERATE) {
            JS_ASSERT(fp->flags & JSFRAME_EVAL);
            ok = parent->setProperty(cx, id, &rval);
        } else {
            JS_ASSERT(attrs & JSPROP_PERMANENT);

            ok = parent->defineProperty(cx, id, rval,
                                        (flags & JSPROP_GETTER)
                                        ? CastAsPropertyOp(obj)
                                        : PropertyStub,
                                        (flags & JSPROP_SETTER)
                                        ? CastAsPropertyOp(obj)
                                        : PropertyStub,
                                        attrs);
        }
    }

    if (!ok)
        goto error;
}
END_CASE(JSOP_DEFFUN_FC)

BEGIN_CASE(JSOP_DEFLOCALFUN)
{
    






    JSFunction *fun;
    LOAD_FUNCTION(SLOTNO_LEN);
    JS_ASSERT(fun->isInterpreted());
    JS_ASSERT(!FUN_FLAT_CLOSURE(fun));
    JSObject *obj = FUN_OBJECT(fun);

    if (FUN_NULL_CLOSURE(fun)) {
        obj = CloneFunctionObject(cx, fun, fp->scopeChain);
        if (!obj)
            goto error;
    } else {
        JSObject *parent = js_GetScopeChain(cx, fp);
        if (!parent)
            goto error;

        if (obj->getParent() != parent) {
#ifdef JS_TRACER
            if (TRACE_RECORDER(cx))
                AbortRecording(cx, "DEFLOCALFUN for closure");
#endif
            obj = CloneFunctionObject(cx, fun, parent);
            if (!obj)
                goto error;
        }
    }

    uint32 slot = GET_SLOTNO(regs.pc);
    TRACE_2(DefLocalFunSetSlot, slot, obj);

    fp->slots()[slot].setFunObj(*obj);
}
END_CASE(JSOP_DEFLOCALFUN)

BEGIN_CASE(JSOP_DEFLOCALFUN_FC)
{
    JSFunction *fun;
    LOAD_FUNCTION(SLOTNO_LEN);

    JSObject *obj = js_NewFlatClosure(cx, fun);
    if (!obj)
        goto error;

    uint32 slot = GET_SLOTNO(regs.pc);
    TRACE_2(DefLocalFunSetSlot, slot, obj);

    fp->slots()[slot].setFunObj(*obj);
}
END_CASE(JSOP_DEFLOCALFUN_FC)

BEGIN_CASE(JSOP_DEFLOCALFUN_DBGFC)
{
    JSFunction *fun;
    LOAD_FUNCTION(SLOTNO_LEN);

    JSObject *obj = js_NewDebuggableFlatClosure(cx, fun);
    if (!obj)
        goto error;

    uint32 slot = GET_SLOTNO(regs.pc);
    fp->slots()[slot].setFunObj(*obj);
}
END_CASE(JSOP_DEFLOCALFUN_DBGFC)

BEGIN_CASE(JSOP_LAMBDA)
{
    
    JSFunction *fun;
    LOAD_FUNCTION(0);
    JSObject *obj = FUN_OBJECT(fun);

    
    do {
        JSObject *parent;
        if (FUN_NULL_CLOSURE(fun)) {
            parent = fp->scopeChain;

            if (obj->getParent() == parent) {
                op = JSOp(regs.pc[JSOP_LAMBDA_LENGTH]);

                



                if (op == JSOP_SETMETHOD) {
#ifdef DEBUG
                    JSOp op2 = JSOp(regs.pc[JSOP_LAMBDA_LENGTH + JSOP_SETMETHOD_LENGTH]);
                    JS_ASSERT(op2 == JSOP_POP || op2 == JSOP_POPV);
#endif

                    const Value &lref = regs.sp[-1];
                    if (lref.isObject() &&
                        lref.asObject().getClass() == &js_ObjectClass) {
                        break;
                    }
                } else if (op == JSOP_INITMETHOD) {
#ifdef DEBUG
                    const Value &lref = regs.sp[-1];
                    JS_ASSERT(lref.isObject());
                    JSObject *obj2 = &lref.asObject();
                    JS_ASSERT(obj2->getClass() == &js_ObjectClass);
                    JS_ASSERT(obj2->scope()->object == obj2);
#endif
                    break;
                }
            }
        } else {
            parent = js_GetScopeChain(cx, fp);
            if (!parent)
                goto error;
        }

        obj = CloneFunctionObject(cx, fun, parent);
        if (!obj)
            goto error;
    } while (0);

    PUSH_FUNOBJ(*obj);
}
END_CASE(JSOP_LAMBDA)

BEGIN_CASE(JSOP_LAMBDA_FC)
{
    JSFunction *fun;
    LOAD_FUNCTION(0);

    JSObject *obj = js_NewFlatClosure(cx, fun);
    if (!obj)
        goto error;

    PUSH_FUNOBJ(*obj);
}
END_CASE(JSOP_LAMBDA_FC)

BEGIN_CASE(JSOP_LAMBDA_DBGFC)
{
    JSFunction *fun;
    LOAD_FUNCTION(0);

    JSObject *obj = js_NewDebuggableFlatClosure(cx, fun);
    if (!obj)
        goto error;

    PUSH_FUNOBJ(*obj);
}
END_CASE(JSOP_LAMBDA_DBGFC)

BEGIN_CASE(JSOP_CALLEE)
    PUSH_COPY(fp->argv[-2]);
END_CASE(JSOP_CALLEE)

BEGIN_CASE(JSOP_GETTER)
BEGIN_CASE(JSOP_SETTER)
{
  do_getter_setter:
    JSOp op2 = (JSOp) *++regs.pc;
    jsid id;
    Value rval;
    jsint i;
    JSObject *obj;
    switch (op2) {
      case JSOP_INDEXBASE:
        atoms += GET_INDEXBASE(regs.pc);
        regs.pc += JSOP_INDEXBASE_LENGTH - 1;
        goto do_getter_setter;
      case JSOP_INDEXBASE1:
      case JSOP_INDEXBASE2:
      case JSOP_INDEXBASE3:
        atoms += (op2 - JSOP_INDEXBASE1 + 1) << 16;
        goto do_getter_setter;

      case JSOP_SETNAME:
      case JSOP_SETPROP:
      {
        JSAtom *atom;
        LOAD_ATOM(0, atom);
        id = ATOM_TO_JSID(atom);
        rval = regs.sp[-1];
        i = -1;
        goto gs_pop_lval;
      }
      case JSOP_SETELEM:
        rval = regs.sp[-1];
        id = 0;
        i = -2;
      gs_pop_lval:
        FETCH_OBJECT(cx, i - 1, obj);
        break;

      case JSOP_INITPROP:
      {
        JS_ASSERT(regs.sp - fp->base() >= 2);
        rval = regs.sp[-1];
        i = -1;
        JSAtom *atom;
        LOAD_ATOM(0, atom);
        id = ATOM_TO_JSID(atom);
        goto gs_get_lval;
      }
      default:
        JS_ASSERT(op2 == JSOP_INITELEM);

        JS_ASSERT(regs.sp - fp->base() >= 3);
        rval = regs.sp[-1];
        id = 0;
        i = -2;
      gs_get_lval:
      {
        const Value &lref = regs.sp[i-1];
        JS_ASSERT(lref.isObject());
        obj = &lref.asObject();
        break;
      }
    }

    
    if (id == 0)
        FETCH_ELEMENT_ID(obj, i, id);

    if (!js_IsCallable(rval)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_GETTER_OR_SETTER,
                             (op == JSOP_GETTER)
                             ? js_getter_str
                             : js_setter_str);
        goto error;
    }

    



    Value rtmp;
    uintN attrs;
    if (!obj->checkAccess(cx, id, JSACC_WATCH, &rtmp, &attrs))
        goto error;

    PropertyOp getter, setter;
    if (op == JSOP_GETTER) {
        getter = CastAsPropertyOp(&rval.asObject());
        setter = PropertyStub;
        attrs = JSPROP_GETTER;
    } else {
        getter = PropertyStub;
        setter = CastAsPropertyOp(&rval.asObject());
        attrs = JSPROP_SETTER;
    }
    attrs |= JSPROP_ENUMERATE | JSPROP_SHARED;

    
    if (!CheckRedeclaration(cx, obj, id, attrs, NULL, NULL))
        goto error;

    if (!obj->defineProperty(cx, id, Value(UndefinedTag()), getter, setter, attrs))
        goto error;

    regs.sp += i;
    if (js_CodeSpec[op2].ndefs > js_CodeSpec[op2].nuses) {
        JS_ASSERT(js_CodeSpec[op2].ndefs == js_CodeSpec[op2].nuses + 1);
        regs.sp[-1] = rval;
    }
    len = js_CodeSpec[op2].length;
    DO_NEXT_OP(len);
}

BEGIN_CASE(JSOP_HOLE)
    PUSH_HOLE();
END_CASE(JSOP_HOLE)

BEGIN_CASE(JSOP_NEWARRAY)
{
    len = GET_UINT16(regs.pc);
    cx->assertValidStackDepth(len);
    JSObject *obj = js_NewArrayObject(cx, len, regs.sp - len, JS_TRUE);
    if (!obj)
        goto error;
    regs.sp -= len - 1;
    regs.sp[-1].setNonFunObj(*obj);
}
END_CASE(JSOP_NEWARRAY)

BEGIN_CASE(JSOP_NEWINIT)
{
    jsint i = GET_INT8(regs.pc);
    JS_ASSERT(i == JSProto_Array || i == JSProto_Object);
    JSObject *obj;
    if (i == JSProto_Array) {
        obj = js_NewArrayObject(cx, 0, NULL);
        if (!obj)
            goto error;
    } else {
        obj = NewObject(cx, &js_ObjectClass, NULL, NULL);
        if (!obj)
            goto error;

        if (regs.pc[JSOP_NEWINIT_LENGTH] != JSOP_ENDINIT) {
            JS_LOCK_OBJ(cx, obj);
            JSScope *scope = js_GetMutableScope(cx, obj);
            if (!scope) {
                JS_UNLOCK_OBJ(cx, obj);
                goto error;
            }

            





            JS_UNLOCK_SCOPE(cx, scope);
        }
    }

    PUSH_NONFUNOBJ(*obj);
    CHECK_INTERRUPT_HANDLER();
}
END_CASE(JSOP_NEWINIT)

BEGIN_CASE(JSOP_ENDINIT)
{
    
    JS_ASSERT(regs.sp - fp->base() >= 1);
    const Value &lref = regs.sp[-1];
    JS_ASSERT(lref.isObject());
    cx->weakRoots.finalizableNewborns[FINALIZE_OBJECT] = &lref.asObject();
}
END_CASE(JSOP_ENDINIT)

BEGIN_CASE(JSOP_INITPROP)
BEGIN_CASE(JSOP_INITMETHOD)
{
    
    JS_ASSERT(regs.sp - fp->base() >= 2);
    Value rval;
    rval = regs.sp[-1];

    
    JSObject *obj = &regs.sp[-2].asObject();
    JS_ASSERT(obj->isNative());
    JS_ASSERT(!obj->getClass()->reserveSlots);

    JSScope *scope = obj->scope();

    










    PropertyCacheEntry *entry;
    JSScopeProperty *sprop;
    if (CX_OWNS_OBJECT_TITLE(cx, obj) &&
        JS_PROPERTY_CACHE(cx).testForInit(rt, regs.pc, obj, scope, &sprop, &entry) &&
        sprop->hasDefaultSetter() &&
        sprop->parent == scope->lastProperty())
    {
        
        uint32 slot = sprop->slot;
        JS_ASSERT(slot == scope->freeslot);
        if (slot < obj->numSlots()) {
            ++scope->freeslot;
        } else {
            if (!js_AllocSlot(cx, obj, &slot))
                goto error;
            JS_ASSERT(slot == sprop->slot);
        }

        JS_ASSERT(!scope->lastProperty() ||
                  scope->shape == scope->lastProperty()->shape);
        if (scope->table) {
            JSScopeProperty *sprop2 =
                scope->addProperty(cx, sprop->id, sprop->getter(), sprop->setter(), slot,
                                   sprop->attributes(), sprop->getFlags(), sprop->shortid);
            if (!sprop2) {
                js_FreeSlot(cx, obj, slot);
                goto error;
            }
            JS_ASSERT(sprop2 == sprop);
        } else {
            JS_ASSERT(!scope->isSharedEmpty());
            scope->extend(cx, sprop);
        }

        




        TRACE_2(SetPropHit, entry, sprop);
        obj->lockedSetSlot(slot, rval);
    } else {
        PCMETER(JS_PROPERTY_CACHE(cx).inipcmisses++);

        
        JSAtom *atom;
        LOAD_ATOM(0, atom);
        jsid id = ATOM_TO_JSID(atom);

        
        if (!CheckRedeclaration(cx, obj, id, JSPROP_INITIALIZER, NULL, NULL))
            goto error;

        uintN defineHow = (op == JSOP_INITMETHOD)
                          ? JSDNP_CACHE_RESULT | JSDNP_SET_METHOD
                          : JSDNP_CACHE_RESULT;
        if (!(JS_UNLIKELY(atom == cx->runtime->atomState.protoAtom)
              ? js_SetPropertyHelper(cx, obj, id, defineHow, &rval)
              : js_DefineNativeProperty(cx, obj, id, rval, NULL, NULL,
                                        JSPROP_ENUMERATE, 0, 0, NULL,
                                        defineHow))) {
            goto error;
        }
    }

    
    regs.sp--;
}
END_CASE(JSOP_INITPROP);

BEGIN_CASE(JSOP_INITELEM)
{
    
    JS_ASSERT(regs.sp - fp->base() >= 3);
    const Value &rref = regs.sp[-1];

    
    const Value &lref = regs.sp[-3];
    JS_ASSERT(lref.isObject());
    JSObject *obj = &lref.asObject();

    
    jsid id;
    FETCH_ELEMENT_ID(obj, -2, id);

    



    if (!CheckRedeclaration(cx, obj, id, JSPROP_INITIALIZER, NULL, NULL))
        goto error;

    




    if (rref.isMagic(JS_ARRAY_HOLE)) {
        JS_ASSERT(obj->isArray());
        JS_ASSERT(JSID_IS_INT(id));
        JS_ASSERT(jsuint(JSID_TO_INT(id)) < JS_ARGS_LENGTH_MAX);
        if (js_GetOpcode(cx, script, regs.pc + JSOP_INITELEM_LENGTH) == JSOP_ENDINIT &&
            !js_SetLengthProperty(cx, obj, (jsuint) (JSID_TO_INT(id) + 1))) {
            goto error;
        }
    } else {
        if (!obj->defineProperty(cx, id, rref, NULL, NULL, JSPROP_ENUMERATE))
            goto error;
    }
    regs.sp -= 2;
}
END_CASE(JSOP_INITELEM)

#if JS_HAS_SHARP_VARS

BEGIN_CASE(JSOP_DEFSHARP)
{
    uint32 slot = GET_UINT16(regs.pc);
    JS_ASSERT(slot + 1 < fp->script->nfixed);
    const Value &lref = fp->slots()[slot];
    JSObject *obj;
    if (lref.isObject()) {
        obj = &lref.asObject();
    } else {
        JS_ASSERT(lref.isUndefined());
        obj = js_NewArrayObject(cx, 0, NULL);
        if (!obj)
            goto error;
        fp->slots()[slot].setNonFunObj(*obj);
    }
    jsint i = (jsint) GET_UINT16(regs.pc + UINT16_LEN);
    jsid id = INT_TO_JSID(i);
    const Value &rref = regs.sp[-1];
    if (rref.isPrimitive()) {
        char numBuf[12];
        JS_snprintf(numBuf, sizeof numBuf, "%u", (unsigned) i);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_SHARP_DEF, numBuf);
        goto error;
    }
    if (!obj->defineProperty(cx, id, rref, NULL, NULL, JSPROP_ENUMERATE))
        goto error;
}
END_CASE(JSOP_DEFSHARP)

BEGIN_CASE(JSOP_USESHARP)
{
    uint32 slot = GET_UINT16(regs.pc);
    JS_ASSERT(slot + 1 < fp->script->nfixed);
    const Value &lref = fp->slots()[slot];
    jsint i = (jsint) GET_UINT16(regs.pc + UINT16_LEN);
    Value rval;
    if (lref.isUndefined()) {
        rval.setUndefined();
    } else {
        JSObject *obj = &fp->slots()[slot].asObject();
        jsid id = INT_TO_JSID(i);
        if (!obj->getProperty(cx, id, &rval))
            goto error;
    }
    if (!rval.isObjectOrNull()) {
        char numBuf[12];

        JS_snprintf(numBuf, sizeof numBuf, "%u", (unsigned) i);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_SHARP_USE, numBuf);
        goto error;
    }
    PUSH_COPY(rval);
}
END_CASE(JSOP_USESHARP)

BEGIN_CASE(JSOP_SHARPINIT)
{
    uint32 slot = GET_UINT16(regs.pc);
    JS_ASSERT(slot + 1 < fp->script->nfixed);
    Value *vp = &fp->slots()[slot];
    Value rval;
    rval = vp[1];

    





    if (regs.pc[JSOP_SHARPINIT_LENGTH] != JSOP_ENDINIT) {
        rval.setInt32(rval.isUndefined() ? 1 : rval.asInt32() + 1);
    } else {
        JS_ASSERT(rval.isInt32());
        rval.asInt32Ref() -= 1;
        if (rval.asInt32() == 0)
            vp[0].setUndefined();
    }
    vp[1] = rval;
}
END_CASE(JSOP_SHARPINIT)

#endif 

{
BEGIN_CASE(JSOP_GOSUB)
    PUSH_BOOLEAN(false);
    jsint i = (regs.pc - script->main) + JSOP_GOSUB_LENGTH;
    PUSH_INT32(i);
    len = GET_JUMP_OFFSET(regs.pc);
END_VARLEN_CASE
}

{
BEGIN_CASE(JSOP_GOSUBX)
    PUSH_BOOLEAN(false);
    jsint i = (regs.pc - script->main) + JSOP_GOSUBX_LENGTH;
    len = GET_JUMPX_OFFSET(regs.pc);
    PUSH_INT32(i);
END_VARLEN_CASE
}

{
BEGIN_CASE(JSOP_RETSUB)
    
    Value rval, lval;
    POP_COPY_TO(rval);
    POP_COPY_TO(lval);
    JS_ASSERT(lval.isBoolean());
    if (lval.isBoolean()) {
        





        cx->throwing = JS_TRUE;
        cx->exception = rval;
        goto error;
    }
    JS_ASSERT(rval.isInt32());
    len = rval.asInt32();
    regs.pc = script->main;
END_VARLEN_CASE
}

BEGIN_CASE(JSOP_EXCEPTION)
    JS_ASSERT(cx->throwing);
    PUSH_COPY(cx->exception);
    cx->throwing = JS_FALSE;
    CHECK_BRANCH();
END_CASE(JSOP_EXCEPTION)

BEGIN_CASE(JSOP_FINALLY)
    CHECK_BRANCH();
END_CASE(JSOP_FINALLY)

BEGIN_CASE(JSOP_THROWING)
    JS_ASSERT(!cx->throwing);
    cx->throwing = JS_TRUE;
    POP_COPY_TO(cx->exception);
END_CASE(JSOP_THROWING)

BEGIN_CASE(JSOP_THROW)
    JS_ASSERT(!cx->throwing);
    CHECK_BRANCH();
    cx->throwing = JS_TRUE;
    POP_COPY_TO(cx->exception);
    
    goto error;

BEGIN_CASE(JSOP_SETLOCALPOP)
{
    



    JS_ASSERT((size_t) (regs.sp - fp->base()) >= 2);
    uint32 slot = GET_UINT16(regs.pc);
    JS_ASSERT(slot + 1 < script->nslots);
    POP_COPY_TO(fp->slots()[slot]);
}
END_CASE(JSOP_SETLOCALPOP)

BEGIN_CASE(JSOP_IFPRIMTOP)
    



    JS_ASSERT(regs.sp > fp->base());
    if (regs.sp[-1].isPrimitive()) {
        len = GET_JUMP_OFFSET(regs.pc);
        BRANCH(len);
    }
END_CASE(JSOP_IFPRIMTOP)

BEGIN_CASE(JSOP_PRIMTOP)
    JS_ASSERT(regs.sp > fp->base());
    if (regs.sp[-1].isObject()) {
        jsint i = GET_INT8(regs.pc);
        js_ReportValueError2(cx, JSMSG_CANT_CONVERT_TO, -2, regs.sp[-2], NULL,
                             (i == JSTYPE_VOID) ? "primitive type" : JS_TYPE_STR(i));
        goto error;
    }
END_CASE(JSOP_PRIMTOP)

BEGIN_CASE(JSOP_OBJTOP)
    if (regs.sp[-1].isPrimitive()) {
        js_ReportValueError(cx, GET_UINT16(regs.pc), -1, regs.sp[-1], NULL);
        goto error;
    }
END_CASE(JSOP_OBJTOP)

BEGIN_CASE(JSOP_INSTANCEOF)
{
    const Value &rref = regs.sp[-1];
    JSObject *obj;
    if (rref.isPrimitive() ||
        !(obj = &rref.asObject())->map->ops->hasInstance) {
        js_ReportValueError(cx, JSMSG_BAD_INSTANCEOF_RHS,
                            -1, rref, NULL);
        goto error;
    }
    const Value &lref = regs.sp[-2];
    JSBool cond = JS_FALSE;
    if (!obj->map->ops->hasInstance(cx, obj, &lref, &cond))
        goto error;
    regs.sp--;
    regs.sp[-1].setBoolean(cond);
}
END_CASE(JSOP_INSTANCEOF)

#if JS_HAS_DEBUGGER_KEYWORD
BEGIN_CASE(JSOP_DEBUGGER)
{
    JSDebuggerHandler handler = cx->debugHooks->debuggerHandler;
    if (handler) {
        Value rval;
        switch (handler(cx, script, regs.pc, Jsvalify(&rval), cx->debugHooks->debuggerHandlerData)) {
        case JSTRAP_ERROR:
            goto error;
        case JSTRAP_CONTINUE:
            break;
        case JSTRAP_RETURN:
            fp->rval = rval;
            interpReturnOK = JS_TRUE;
            goto forced_return;
        case JSTRAP_THROW:
            cx->throwing = JS_TRUE;
            cx->exception = rval;
            goto error;
        default:;
        }
        CHECK_INTERRUPT_HANDLER();
    }
}
END_CASE(JSOP_DEBUGGER)
#endif 

#if JS_HAS_XML_SUPPORT
BEGIN_CASE(JSOP_DEFXMLNS)
{
    Value rval;
    POP_COPY_TO(rval);
    if (!js_SetDefaultXMLNamespace(cx, rval))
        goto error;
}
END_CASE(JSOP_DEFXMLNS)

BEGIN_CASE(JSOP_ANYNAME)
{
    jsid id;
    if (!js_GetAnyName(cx, &id))
        goto error;
    PUSH_COPY(IdToValue(id));
}
END_CASE(JSOP_ANYNAME)

BEGIN_CASE(JSOP_QNAMEPART)
{
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    PUSH_STRING(ATOM_TO_STRING(atom));
}
END_CASE(JSOP_QNAMEPART)

BEGIN_CASE(JSOP_QNAMECONST)
{
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    Value rval = StringTag(ATOM_TO_STRING(atom));
    Value lval;
    lval = regs.sp[-1];
    JSObject *obj = js_ConstructXMLQNameObject(cx, lval, rval);
    if (!obj)
        goto error;
    regs.sp[-1].setObject(*obj);
}
END_CASE(JSOP_QNAMECONST)

BEGIN_CASE(JSOP_QNAME)
{
    Value rval, lval;
    rval = regs.sp[-1];
    lval = regs.sp[-2];
    JSObject *obj = js_ConstructXMLQNameObject(cx, lval, rval);
    if (!obj)
        goto error;
    regs.sp--;
    regs.sp[-1].setObject(*obj);
}
END_CASE(JSOP_QNAME)

BEGIN_CASE(JSOP_TOATTRNAME)
{
    Value rval;
    rval = regs.sp[-1];
    if (!js_ToAttributeName(cx, &rval))
        goto error;
    regs.sp[-1] = rval;
}
END_CASE(JSOP_TOATTRNAME)

BEGIN_CASE(JSOP_TOATTRVAL)
{
    Value rval;
    rval = regs.sp[-1];
    JS_ASSERT(rval.isString());
    JSString *str = js_EscapeAttributeValue(cx, rval.asString(), JS_FALSE);
    if (!str)
        goto error;
    regs.sp[-1].setString(str);
}
END_CASE(JSOP_TOATTRVAL)

BEGIN_CASE(JSOP_ADDATTRNAME)
BEGIN_CASE(JSOP_ADDATTRVAL)
{
    Value rval, lval;
    rval = regs.sp[-1];
    lval = regs.sp[-2];
    JSString *str = lval.asString();
    JSString *str2 = rval.asString();
    str = js_AddAttributePart(cx, op == JSOP_ADDATTRNAME, str, str2);
    if (!str)
        goto error;
    regs.sp--;
    regs.sp[-1].setString(str);
}
END_CASE(JSOP_ADDATTRNAME)

BEGIN_CASE(JSOP_BINDXMLNAME)
{
    Value lval;
    lval = regs.sp[-1];
    JSObject *obj;
    jsid id;
    if (!js_FindXMLProperty(cx, lval, &obj, &id))
        goto error;
    regs.sp[-1].setObjectOrNull(obj);
    PUSH_COPY(IdToValue(id));
}
END_CASE(JSOP_BINDXMLNAME)

BEGIN_CASE(JSOP_SETXMLNAME)
{
    JSObject *obj = &regs.sp[-3].asObject();
    Value rval;
    rval = regs.sp[-1];
    jsid id;
    FETCH_ELEMENT_ID(obj, -2, id);
    if (!obj->setProperty(cx, id, &rval))
        goto error;
    rval = regs.sp[-1];
    regs.sp -= 2;
    regs.sp[-1] = rval;
}
END_CASE(JSOP_SETXMLNAME)

BEGIN_CASE(JSOP_CALLXMLNAME)
BEGIN_CASE(JSOP_XMLNAME)
{
    Value lval;
    lval = regs.sp[-1];
    JSObject *obj;
    jsid id;
    if (!js_FindXMLProperty(cx, lval, &obj, &id))
        goto error;
    Value rval;
    if (!obj->getProperty(cx, id, &rval))
        goto error;
    regs.sp[-1] = rval;
    if (op == JSOP_CALLXMLNAME)
        PUSH_OBJECT(*obj);
}
END_CASE(JSOP_XMLNAME)

BEGIN_CASE(JSOP_DESCENDANTS)
BEGIN_CASE(JSOP_DELDESC)
{
    JSObject *obj;
    FETCH_OBJECT(cx, -2, obj);
    jsval rval = Jsvalify(regs.sp[-1]);
    if (!js_GetXMLDescendants(cx, obj, rval, &rval))
        goto error;

    if (op == JSOP_DELDESC) {
        regs.sp[-1] = Valueify(rval);   
        if (!js_DeleteXMLListElements(cx, JSVAL_TO_OBJECT(rval)))
            goto error;
        rval = JSVAL_TRUE;                  
    }

    regs.sp--;
    regs.sp[-1] = Valueify(rval);
}
END_CASE(JSOP_DESCENDANTS)

{
BEGIN_CASE(JSOP_FILTER)
    




    PUSH_HOLE();
    len = GET_JUMP_OFFSET(regs.pc);
    JS_ASSERT(len > 0);
END_VARLEN_CASE
}

BEGIN_CASE(JSOP_ENDFILTER)
{
    bool cond = !regs.sp[-1].isMagic();
    if (cond) {
        
        js_LeaveWith(cx);
    }
    if (!js_StepXMLListFilter(cx, cond))
        goto error;
    if (!regs.sp[-1].isNull()) {
        



        JS_ASSERT(IsXML(regs.sp[-1]));
        if (!js_EnterWith(cx, -2))
            goto error;
        regs.sp--;
        len = GET_JUMP_OFFSET(regs.pc);
        JS_ASSERT(len < 0);
        BRANCH(len);
    }
    regs.sp--;
}
END_CASE(JSOP_ENDFILTER);

BEGIN_CASE(JSOP_TOXML)
{
    Value rval;
    rval = regs.sp[-1];
    JSObject *obj = js_ValueToXMLObject(cx, rval);
    if (!obj)
        goto error;
    regs.sp[-1].setObject(*obj);
}
END_CASE(JSOP_TOXML)

BEGIN_CASE(JSOP_TOXMLLIST)
{
    Value rval;
    rval = regs.sp[-1];
    JSObject *obj = js_ValueToXMLListObject(cx, rval);
    if (!obj)
        goto error;
    regs.sp[-1].setObject(*obj);
}
END_CASE(JSOP_TOXMLLIST)

BEGIN_CASE(JSOP_XMLTAGEXPR)
{
    Value rval;
    rval = regs.sp[-1];
    JSString *str = js_ValueToString(cx, rval);
    if (!str)
        goto error;
    regs.sp[-1].setString(str);
}
END_CASE(JSOP_XMLTAGEXPR)

BEGIN_CASE(JSOP_XMLELTEXPR)
{
    Value rval;
    rval = regs.sp[-1];
    JSString *str;
    if (IsXML(rval)) {
        str = js_ValueToXMLString(cx, rval);
    } else {
        str = js_ValueToString(cx, rval);
        if (str)
            str = js_EscapeElementValue(cx, str);
    }
    if (!str)
        goto error;
    regs.sp[-1].setString(str);
}
END_CASE(JSOP_XMLELTEXPR)

BEGIN_CASE(JSOP_XMLOBJECT)
{
    JSObject *obj;
    LOAD_OBJECT(0, obj);
    obj = js_CloneXMLObject(cx, obj);
    if (!obj)
        goto error;
    PUSH_OBJECT(*obj);
}
END_CASE(JSOP_XMLOBJECT)

BEGIN_CASE(JSOP_XMLCDATA)
{
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    JSString *str = ATOM_TO_STRING(atom);
    JSObject *obj = js_NewXMLSpecialObject(cx, JSXML_CLASS_TEXT, NULL, str);
    if (!obj)
        goto error;
    PUSH_OBJECT(*obj);
}
END_CASE(JSOP_XMLCDATA)

BEGIN_CASE(JSOP_XMLCOMMENT)
{
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    JSString *str = ATOM_TO_STRING(atom);
    JSObject *obj = js_NewXMLSpecialObject(cx, JSXML_CLASS_COMMENT, NULL, str);
    if (!obj)
        goto error;
    PUSH_OBJECT(*obj);
}
END_CASE(JSOP_XMLCOMMENT)

BEGIN_CASE(JSOP_XMLPI)
{
    JSAtom *atom;
    LOAD_ATOM(0, atom);
    JSString *str = ATOM_TO_STRING(atom);
    Value rval;
    rval = regs.sp[-1];
    JSString *str2 = rval.asString();
    JSObject *obj = js_NewXMLSpecialObject(cx, JSXML_CLASS_PROCESSING_INSTRUCTION, str, str2);
    if (!obj)
        goto error;
    regs.sp[-1].setObject(*obj);
}
END_CASE(JSOP_XMLPI)

BEGIN_CASE(JSOP_GETFUNNS)
{
    Value rval;
    if (!js_GetFunctionNamespace(cx, &rval))
        goto error;
    PUSH_COPY(rval);
}
END_CASE(JSOP_GETFUNNS)
#endif 

BEGIN_CASE(JSOP_ENTERBLOCK)
{
    JSObject *obj;
    LOAD_OBJECT(0, obj);
    JS_ASSERT(!OBJ_IS_CLONED_BLOCK(obj));
    JS_ASSERT(fp->base() + OBJ_BLOCK_DEPTH(cx, obj) == regs.sp);
    Value *vp = regs.sp + OBJ_BLOCK_COUNT(cx, obj);
    JS_ASSERT(regs.sp < vp);
    JS_ASSERT(vp <= fp->slots() + script->nslots);
    SetValueRangeToUndefined(regs.sp, vp);
    regs.sp = vp;

#ifdef DEBUG
    JS_ASSERT(fp->blockChain == obj->getParent());

    






    JSObject *obj2 = fp->scopeChain;
    Class *clasp;
    while ((clasp = obj2->getClass()) == &js_WithClass)
        obj2 = obj2->getParent();
    if (clasp == &js_BlockClass &&
        obj2->getPrivate() == js_FloatingFrameIfGenerator(cx, fp)) {
        JSObject *youngestProto = obj2->getProto();
        JS_ASSERT(!OBJ_IS_CLONED_BLOCK(youngestProto));
        JSObject *parent = obj;
        while ((parent = parent->getParent()) != youngestProto)
            JS_ASSERT(parent);
    }
#endif

    fp->blockChain = obj;
}
END_CASE(JSOP_ENTERBLOCK)

BEGIN_CASE(JSOP_LEAVEBLOCKEXPR)
BEGIN_CASE(JSOP_LEAVEBLOCK)
{
#ifdef DEBUG
    JS_ASSERT(fp->blockChain->getClass() == &js_BlockClass);
    uintN blockDepth = OBJ_BLOCK_DEPTH(cx, fp->blockChain);

    JS_ASSERT(blockDepth <= StackDepth(script));
#endif
    




    JSObject *obj = fp->scopeChain;
    if (obj->getProto() == fp->blockChain) {
        JS_ASSERT(obj->getClass() == &js_BlockClass);
        if (!js_PutBlockObject(cx, JS_TRUE))
            goto error;
    }

    
    fp->blockChain = fp->blockChain->getParent();

    
    Value *vp = NULL;  
    if (op == JSOP_LEAVEBLOCKEXPR)
        vp = &regs.sp[-1];
    regs.sp -= GET_UINT16(regs.pc);
    if (op == JSOP_LEAVEBLOCKEXPR) {
        JS_ASSERT(fp->base() + blockDepth == regs.sp - 1);
        regs.sp[-1] = *vp;
    } else {
        JS_ASSERT(fp->base() + blockDepth == regs.sp);
    }
}
END_CASE(JSOP_LEAVEBLOCK)

#if JS_HAS_GENERATORS
BEGIN_CASE(JSOP_GENERATOR)
{
    ASSERT_NOT_THROWING(cx);
    regs.pc += JSOP_GENERATOR_LENGTH;
    JSObject *obj = js_NewGenerator(cx);
    if (!obj)
        goto error;
    JS_ASSERT(!fp->callobj && !fp->argsobj);
    fp->rval.setNonFunObj(*obj);
    interpReturnOK = true;
    if (inlineCallCount != 0)
        goto inline_return;
    goto exit;
}

BEGIN_CASE(JSOP_YIELD)
    ASSERT_NOT_THROWING(cx);
    if (cx->generatorFor(fp)->state == JSGEN_CLOSING) {
        js_ReportValueError(cx, JSMSG_BAD_GENERATOR_YIELD,
                            JSDVG_SEARCH_STACK, fp->argv[-2], NULL);
        goto error;
    }
    fp->rval = regs.sp[-1];
    fp->flags |= JSFRAME_YIELDING;
    regs.pc += JSOP_YIELD_LENGTH;
    interpReturnOK = JS_TRUE;
    goto exit;

BEGIN_CASE(JSOP_ARRAYPUSH)
{
    uint32 slot = GET_UINT16(regs.pc);
    JS_ASSERT(script->nfixed <= slot);
    JS_ASSERT(slot < script->nslots);
    JSObject *obj = &fp->slots()[slot].asObject();
    if (!js_ArrayCompPush(cx, obj, regs.sp[-1]))
        goto error;
    regs.sp--;
}
END_CASE(JSOP_ARRAYPUSH)
#endif 

#if JS_THREADED_INTERP
  L_JSOP_BACKPATCH:
  L_JSOP_BACKPATCH_POP:

# if !JS_HAS_GENERATORS
  L_JSOP_GENERATOR:
  L_JSOP_YIELD:
  L_JSOP_ARRAYPUSH:
# endif

# if !JS_HAS_SHARP_VARS
  L_JSOP_DEFSHARP:
  L_JSOP_USESHARP:
  L_JSOP_SHARPINIT:
# endif

# if !JS_HAS_DESTRUCTURING
  L_JSOP_ENUMCONSTELEM:
# endif

# if !JS_HAS_XML_SUPPORT
  L_JSOP_CALLXMLNAME:
  L_JSOP_STARTXMLEXPR:
  L_JSOP_STARTXML:
  L_JSOP_DELDESC:
  L_JSOP_GETFUNNS:
  L_JSOP_XMLPI:
  L_JSOP_XMLCOMMENT:
  L_JSOP_XMLCDATA:
  L_JSOP_XMLOBJECT:
  L_JSOP_XMLELTEXPR:
  L_JSOP_XMLTAGEXPR:
  L_JSOP_TOXMLLIST:
  L_JSOP_TOXML:
  L_JSOP_ENDFILTER:
  L_JSOP_FILTER:
  L_JSOP_DESCENDANTS:
  L_JSOP_XMLNAME:
  L_JSOP_SETXMLNAME:
  L_JSOP_BINDXMLNAME:
  L_JSOP_ADDATTRVAL:
  L_JSOP_ADDATTRNAME:
  L_JSOP_TOATTRVAL:
  L_JSOP_TOATTRNAME:
  L_JSOP_QNAME:
  L_JSOP_QNAMECONST:
  L_JSOP_QNAMEPART:
  L_JSOP_ANYNAME:
  L_JSOP_DEFXMLNS:
# endif

  L_JSOP_UNUSED218:

#endif 
