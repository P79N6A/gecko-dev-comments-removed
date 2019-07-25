





































#include "PolyIC.h"
#include "StubCalls.h"
#include "CodeGenIncludes.h"
#include "StubCalls-inl.h"
#include "BaseCompiler.h"
#include "assembler/assembler/LinkBuffer.h"
#include "TypedArrayIC.h"
#include "jsscope.h"
#include "jsnum.h"
#include "jstypedarray.h"
#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"
#include "jspropertycache.h"
#include "jspropertycacheinlines.h"
#include "jsinterpinlines.h"
#include "jsautooplen.h"

#include "vm/CallObject-inl.h"

#if defined JS_POLYIC

using namespace js;
using namespace js::mjit;
using namespace js::mjit::ic;

typedef JSC::FunctionPtr FunctionPtr;
typedef JSC::MacroAssembler::RegisterID RegisterID;
typedef JSC::MacroAssembler::Jump Jump;
typedef JSC::MacroAssembler::Imm32 Imm32;


static const uint32 INLINE_PATH_LENGTH = 64;




class PICLinker : public LinkerHelper
{
    ic::BasePolyIC &ic;

  public:
    PICLinker(Assembler &masm, ic::BasePolyIC &ic)
      : LinkerHelper(masm, JSC::METHOD_CODE), ic(ic)
    { }

    bool init(JSContext *cx) {
        JSC::ExecutablePool *pool = LinkerHelper::init(cx);
        if (!pool)
            return false;
        if (!ic.addPool(cx, pool)) {
            pool->release();
            js_ReportOutOfMemory(cx);
            return false;
        }
        return true;
    }
};

class PICStubCompiler : public BaseCompiler
{
  protected:
    const char *type;
    VMFrame &f;
    JSScript *script;
    ic::PICInfo &pic;
    void *stub;
    uint32 gcNumber;

  public:
    bool canCallHook;

    PICStubCompiler(const char *type, VMFrame &f, JSScript *script, ic::PICInfo &pic, void *stub)
      : BaseCompiler(f.cx), type(type), f(f), script(script), pic(pic), stub(stub),
        gcNumber(f.cx->runtime->gcNumber), canCallHook(pic.canCallHook)
    { }

    bool isCallOp() const {
        if (pic.kind == ic::PICInfo::CALL)
            return true;
        return !!(js_CodeSpec[pic.op].format & JOF_CALLOP);
    }

    LookupStatus error() {
        



        return Lookup_Error;
    }

    LookupStatus error(JSContext *cx) {
        return error();
    }

    LookupStatus disable(const char *reason) {
        return disable(f.cx, reason);
    }

    LookupStatus disable(JSContext *cx, const char *reason) {
        return pic.disable(cx, reason, stub);
    }

    bool hadGC() {
        return gcNumber != f.cx->runtime->gcNumber;
    }

  protected:
    void spew(const char *event, const char *op) {
#ifdef JS_METHODJIT_SPEW
        JaegerSpew(JSpew_PICs, "%s %s: %s (%s: %d)\n",
                   type, event, op, script->filename, CurrentLine(cx));
#endif
    }
};

class SetPropCompiler : public PICStubCompiler
{
    JSObject *obj;
    JSAtom *atom;
    int lastStubSecondShapeGuard;

  public:
    SetPropCompiler(VMFrame &f, JSScript *script, JSObject *obj, ic::PICInfo &pic, JSAtom *atom,
                    VoidStubPIC stub)
      : PICStubCompiler("setprop", f, script, pic, JS_FUNC_TO_DATA_PTR(void *, stub)),
        obj(obj), atom(atom), lastStubSecondShapeGuard(pic.secondShapeGuard)
    { }

    static void reset(Repatcher &repatcher, ic::PICInfo &pic)
    {
        SetPropLabels &labels = pic.setPropLabels();
        repatcher.repatchLEAToLoadPtr(labels.getDslotsLoad(pic.fastPathRejoin, pic.u.vr));
        repatcher.repatch(labels.getInlineShapeData(pic.fastPathStart, pic.shapeGuard),
                          NULL);
        repatcher.relink(labels.getInlineShapeJump(pic.fastPathStart.labelAtOffset(pic.shapeGuard)),
                         pic.slowPathStart);

        FunctionPtr target(JS_FUNC_TO_DATA_PTR(void *, ic::SetProp));
        repatcher.relink(pic.slowPathCall, target);
    }

    LookupStatus patchInline(const Shape *shape)
    {
        JS_ASSERT(!pic.inlinePathPatched);
        JaegerSpew(JSpew_PICs, "patch setprop inline at %p\n", pic.fastPathStart.executableAddress());

        Repatcher repatcher(f.jit());
        SetPropLabels &labels = pic.setPropLabels();

        int32 offset;
        if (obj->isFixedSlot(shape->slot())) {
            CodeLocationInstruction istr = labels.getDslotsLoad(pic.fastPathRejoin, pic.u.vr);
            repatcher.repatchLoadPtrToLEA(istr);

            
            
            
            
            
            
            
            int32 diff = int32(JSObject::getFixedSlotOffset(0)) -
                         int32(JSObject::offsetOfSlots());
            JS_ASSERT(diff != 0);
            offset = (int32(shape->slot()) * sizeof(Value)) + diff;
        } else {
            offset = obj->dynamicSlotIndex(shape->slot()) * sizeof(Value);
        }

        repatcher.repatch(labels.getInlineShapeData(pic.fastPathStart, pic.shapeGuard),
                          obj->lastProperty());
        repatcher.patchAddressOffsetForValueStore(labels.getInlineValueStore(pic.fastPathRejoin),
                                                  offset, pic.u.vr.isTypeKnown());

        pic.inlinePathPatched = true;

        return Lookup_Cacheable;
    }

    int getLastStubSecondShapeGuard() const {
        return lastStubSecondShapeGuard ? POST_INST_OFFSET(lastStubSecondShapeGuard) : 0;
    }

    void patchPreviousToHere(CodeLocationLabel cs)
    {
        Repatcher repatcher(pic.lastCodeBlock(f.jit()));
        CodeLocationLabel label = pic.lastPathStart();

        
        
        
        if (pic.stubsGenerated) {
            repatcher.relink(pic.setPropLabels().getStubShapeJump(label), cs);
        } else {
            CodeLocationLabel shapeGuard = label.labelAtOffset(pic.shapeGuard);
            repatcher.relink(pic.setPropLabels().getInlineShapeJump(shapeGuard), cs);
        }
        if (int secondGuardOffset = getLastStubSecondShapeGuard())
            repatcher.relink(label.jumpAtOffset(secondGuardOffset), cs);
    }

    LookupStatus generateStub(const Shape *initialShape, const Shape *shape, bool adding)
    {
        if (hadGC())
            return Lookup_Uncacheable;

        
        Vector<Jump, 8> slowExits(cx);
        Vector<Jump, 8> otherGuards(cx);

        Assembler masm;

        
        if (pic.shapeNeedsRemat()) {
            masm.loadShape(pic.objReg, pic.shapeReg);
            pic.shapeRegHasBaseShape = true;
        }

        Label start = masm.label();
        Jump shapeGuard = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                         ImmPtr(initialShape));

        Label stubShapeJumpLabel = masm.label();

        pic.setPropLabels().setStubShapeJump(masm, start, stubShapeJumpLabel);

        if (pic.typeMonitored) {
            









            Jump typeGuard = masm.branchPtr(Assembler::NotEqual,
                                            Address(pic.objReg, JSObject::offsetOfType()),
                                            ImmPtr(obj->getType(cx)));
            if (!otherGuards.append(typeGuard))
                return error();
        }

        JS_ASSERT_IF(!shape->hasDefaultSetter(), obj->isCall());

        MaybeJump skipOver;

        if (adding) {
            JS_ASSERT(shape->hasSlot());
            pic.shapeRegHasBaseShape = false;

            
            JSObject *proto = obj->getProto();
            RegisterID lastReg = pic.objReg;
            while (proto) {
                masm.loadPtr(Address(lastReg, JSObject::offsetOfType()), pic.shapeReg);
                masm.loadPtr(Address(pic.shapeReg, offsetof(types::TypeObject, proto)), pic.shapeReg);
                Jump protoGuard = masm.guardShape(pic.shapeReg, proto);
                if (!otherGuards.append(protoGuard))
                    return error();

                proto = proto->getProto();
                lastReg = pic.shapeReg;
            }

            if (pic.kind == ic::PICInfo::SETMETHOD) {
                



                JS_ASSERT(shape->isMethod());
                JSObject *funobj = obj->nativeGetMethod(shape);
                if (pic.u.vr.isConstant()) {
                    JS_ASSERT(funobj == &pic.u.vr.value().toObject());
                } else {
                    Jump mismatchedFunction =
                        masm.branchPtr(Assembler::NotEqual, pic.u.vr.dataReg(), ImmPtr(funobj));
                    if (!slowExits.append(mismatchedFunction))
                        return error();
                }
            }

            if (obj->isFixedSlot(shape->slot())) {
                Address address(pic.objReg,
                                JSObject::getFixedSlotOffset(shape->slot()));
                masm.storeValue(pic.u.vr, address);
            } else {
                





                masm.loadPtr(Address(pic.objReg, JSObject::offsetOfSlots()), pic.shapeReg);
                Address address(pic.shapeReg, obj->dynamicSlotIndex(shape->slot()) * sizeof(Value));
                masm.storeValue(pic.u.vr, address);
            }

            JS_ASSERT(shape == obj->lastProperty());
            JS_ASSERT(shape != initialShape);

            
            masm.storePtr(ImmPtr(shape), Address(pic.objReg, JSObject::offsetOfShape()));
        } else if (shape->hasDefaultSetter()) {
            JS_ASSERT(!shape->isMethod());
            Address address = masm.objPropAddress(obj, pic.objReg, shape->slot());
            masm.storeValue(pic.u.vr, address);
        } else {
            
            
            
            
            
            
            
            JSFunction *fun = obj->asCall().getCalleeFunction();
            uint16 slot = uint16(shape->shortid());

            
            masm.loadObjPrivate(pic.objReg, pic.shapeReg, obj->numFixedSlots());
            Jump escapedFrame = masm.branchTestPtr(Assembler::Zero, pic.shapeReg, pic.shapeReg);

            {
                Address addr(pic.shapeReg, shape->setterOp() == SetCallArg
                                           ? StackFrame::offsetOfFormalArg(fun, slot)
                                           : StackFrame::offsetOfFixed(slot));
                masm.storeValue(pic.u.vr, addr);
                skipOver = masm.jump();
            }

            escapedFrame.linkTo(masm.label(), &masm);
            {
                if (shape->setterOp() == SetCallVar)
                    slot += fun->nargs;

                slot += CallObject::RESERVED_SLOTS;
                Address address = masm.objPropAddress(obj, pic.objReg, slot);

                masm.storeValue(pic.u.vr, address);
            }

            pic.shapeRegHasBaseShape = false;
        }

        Jump done = masm.jump();

        
        MaybeJump slowExit;
        if (otherGuards.length()) {
            for (Jump *pj = otherGuards.begin(); pj != otherGuards.end(); ++pj)
                pj->linkTo(masm.label(), &masm);
            slowExit = masm.jump();
            pic.secondShapeGuard = masm.distanceOf(masm.label()) - masm.distanceOf(start);
        } else {
            pic.secondShapeGuard = 0;
        }

        pic.updatePCCounters(cx, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        buffer.link(shapeGuard, pic.slowPathStart);
        if (slowExit.isSet())
            buffer.link(slowExit.get(), pic.slowPathStart);
        for (Jump *pj = slowExits.begin(); pj != slowExits.end(); ++pj)
            buffer.link(*pj, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);
        if (skipOver.isSet())
            buffer.link(skipOver.get(), pic.fastPathRejoin);
        CodeLocationLabel cs = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generate setprop stub %p %p %d at %p\n",
                   (void*)&pic,
                   initialShape,
                   pic.stubsGenerated,
                   cs.executableAddress());

        
        
        
        patchPreviousToHere(cs);

        pic.stubsGenerated++;
        pic.updateLastPath(buffer, start);

        if (pic.stubsGenerated == MAX_PIC_STUBS)
            disable("max stubs reached");

        return Lookup_Cacheable;
    }

    bool updateMonitoredTypes()
    {
        JS_ASSERT(pic.typeMonitored);

        RecompilationMonitor monitor(cx);
        jsid id = ATOM_TO_JSID(atom);

        if (!obj->getType(cx)->unknownProperties()) {
            types::AutoEnterTypeInference enter(cx);
            types::TypeSet *types = obj->getType(cx)->getProperty(cx, types::MakeTypeId(cx, id), true);
            if (!types)
                return false;
            pic.rhsTypes->addSubset(cx, types);
        }

        return !monitor.recompiled();
    }

    LookupStatus update()
    {
        JS_ASSERT(pic.hit);

        if (obj->isDenseArray())
            return disable("dense array");
        if (!obj->isNative())
            return disable("non-native");
        if (obj->watched())
            return disable("watchpoint");

        Class *clasp = obj->getClass();

        if (clasp->setProperty != JS_StrictPropertyStub)
            return disable("set property hook");
        if (clasp->ops.lookupProperty)
            return disable("ops lookup property hook");
        if (clasp->ops.setProperty)
            return disable("ops set property hook");

        jsid id = ATOM_TO_JSID(atom);

        JSObject *holder;
        JSProperty *prop = NULL;

        
        RecompilationMonitor monitor(cx);
        if (!obj->lookupProperty(cx, id, &holder, &prop))
            return error();
        if (monitor.recompiled())
            return Lookup_Uncacheable;

        
        if (prop && holder != obj) {
            const Shape *shape = (const Shape *) prop;

            if (!holder->isNative())
                return disable("non-native holder");

            if (!shape->writable())
                return disable("readonly");
            if (!shape->hasDefaultSetter() || !shape->hasDefaultGetter())
                return disable("getter/setter in prototype");
            if (shape->hasShortID())
                return disable("short ID in prototype");
            if (!shape->hasSlot())
                return disable("missing slot");

            prop = NULL;
        }

        if (!prop) {
            
            if (obj->isDelegate())
                return disable("delegate");
            if (!obj->isExtensible())
                return disable("not extensible");

            if (clasp->addProperty != JS_PropertyStub)
                return disable("add property hook");
            if (clasp->ops.defineProperty)
                return disable("ops define property hook");

            uint32 index;
            if (js_IdIsIndex(id, &index))
                return disable("index");

            const Shape *initialShape = obj->lastProperty();
            uint32 slots = obj->numDynamicSlots();

            uintN flags = 0;
            PropertyOp getter = clasp->getProperty;

            if (pic.kind == ic::PICInfo::SETMETHOD) {
                if (!obj->canHaveMethodBarrier())
                    return disable("can't have method barrier");

                JSObject *funobj = &f.regs.sp[-1].toObject();
                if (funobj != funobj->getFunctionPrivate())
                    return disable("mismatched function");

                flags |= Shape::METHOD;
            }

            




            id = js_CheckForStringIndex(id);
            const Shape *shape =
                obj->putProperty(cx, id, getter, clasp->setProperty,
                                 SHAPE_INVALID_SLOT, JSPROP_ENUMERATE, flags, 0);
            if (!shape)
                return error();
            if (flags & Shape::METHOD)
                obj->nativeSetSlot(shape->slot(), f.regs.sp[-1]);

            if (monitor.recompiled())
                return Lookup_Uncacheable;

            




            if (obj->inDictionaryMode())
                return disable("dictionary");

            if (!shape->hasDefaultSetter())
                return disable("adding non-default setter");
            if (!shape->hasSlot())
                return disable("adding invalid slot");

            











            if (obj->numDynamicSlots() != slots)
                return disable("insufficient slot capacity");

            if (pic.typeMonitored && !updateMonitoredTypes())
                return Lookup_Uncacheable;

            return generateStub(initialShape, shape, true);
        }

        const Shape *shape = (const Shape *) prop;
        if (pic.kind == ic::PICInfo::SETMETHOD && !shape->isMethod())
            return disable("set method on non-method shape");
        if (!shape->writable())
            return disable("readonly");
        if (shape->isMethod())
            return disable("method");

        if (shape->hasDefaultSetter()) {
            if (!shape->hasSlot())
                return disable("invalid slot");
            if (pic.typeMonitored && !updateMonitoredTypes())
                return Lookup_Uncacheable;
        } else {
            if (shape->hasSetterValue())
                return disable("scripted setter");
            if (shape->setterOp() != SetCallArg &&
                shape->setterOp() != SetCallVar) {
                return disable("setter");
            }
            JS_ASSERT(obj->isCall());
            if (pic.typeMonitored) {
                









                RecompilationMonitor monitor(cx);
                JSFunction *fun = obj->asCall().getCalleeFunction();
                JSScript *script = fun->script();
                uint16 slot = uint16(shape->shortid());
                if (!script->ensureHasTypes(cx, fun))
                    return error();
                {
                    types::AutoEnterTypeInference enter(cx);
                    if (shape->setterOp() == SetCallArg)
                        pic.rhsTypes->addSubset(cx, types::TypeScript::ArgTypes(script, slot));
                    else
                        pic.rhsTypes->addSubset(cx, types::TypeScript::LocalTypes(script, slot));
                }
                if (monitor.recompiled())
                    return Lookup_Uncacheable;
            }
        }

        JS_ASSERT(obj == holder);
        if (!pic.inlinePathPatched &&
            shape->hasDefaultSetter() &&
            !pic.typeMonitored &&
            !obj->isDenseArray()) {
            return patchInline(shape);
        }

        return generateStub(obj->lastProperty(), shape, false);
    }
};

static bool
IsCacheableProtoChain(JSObject *obj, JSObject *holder)
{
    while (obj != holder) {
        




        JSObject *proto = obj->getProto();
        if (!proto || !proto->isNative())
            return false;
        obj = proto;
    }
    return true;
}

template <typename IC>
struct GetPropertyHelper {
    
    JSContext   *cx;
    JSObject    *obj;
    JSAtom      *atom;
    IC          &ic;
    VMFrame     &f;

    
    
    JSObject    *aobj;
    JSObject    *holder;
    JSProperty  *prop;

    
    
    const Shape *shape;

    GetPropertyHelper(JSContext *cx, JSObject *obj, JSAtom *atom, IC &ic, VMFrame &f)
      : cx(cx), obj(obj), atom(atom), ic(ic), f(f), holder(NULL), prop(NULL), shape(NULL)
    { }

  public:
    LookupStatus bind() {
        RecompilationMonitor monitor(cx);
        bool global = (js_CodeSpec[*f.pc()].format & JOF_GNAME);
        if (!js_FindProperty(cx, ATOM_TO_JSID(atom), global, &obj, &holder, &prop))
            return ic.error(cx);
        if (monitor.recompiled())
            return Lookup_Uncacheable;
        if (!prop)
            return ic.disable(cx, "lookup failed");
        if (!obj->isNative())
            return ic.disable(cx, "non-native");
        if (!IsCacheableProtoChain(obj, holder))
            return ic.disable(cx, "non-native holder");
        shape = (const Shape *)prop;
        return Lookup_Cacheable;
    }

    LookupStatus lookup() {
        JSObject *aobj = js_GetProtoIfDenseArray(obj);
        if (!aobj->isNative())
            return ic.disable(cx, "non-native");

        RecompilationMonitor monitor(cx);
        if (!aobj->lookupProperty(cx, ATOM_TO_JSID(atom), &holder, &prop))
            return ic.error(cx);
        if (monitor.recompiled())
            return Lookup_Uncacheable;

        if (!prop)
            return ic.disable(cx, "lookup failed");
        if (!IsCacheableProtoChain(obj, holder))
            return ic.disable(cx, "non-native holder");
        shape = (const Shape *)prop;
        return Lookup_Cacheable;
    }

    LookupStatus testForGet() {
        if (!shape->hasDefaultGetter()) {
            if (shape->isMethod()) {
                if (!ic.isCallOp())
                    return ic.disable(cx, "method valued shape");
            } else {
                if (shape->hasGetterValue())
                    return ic.disable(cx, "getter value shape");
                if (shape->hasSlot() && holder != obj)
                    return ic.disable(cx, "slotful getter hook through prototype");
                if (!ic.canCallHook)
                    return ic.disable(cx, "can't call getter hook");
                if (f.regs.inlined())
                    return ic.disable(cx, "hook called from inline frame");
            }
        } else if (!shape->hasSlot()) {
            return ic.disable(cx, "no slot");
        }

        return Lookup_Cacheable;
    }

    LookupStatus lookupAndTest() {
        LookupStatus status = lookup();
        if (status != Lookup_Cacheable)
            return status;
        return testForGet();
    }
};

class GetPropCompiler : public PICStubCompiler
{
    JSObject    *obj;
    JSAtom      *atom;
    int         lastStubSecondShapeGuard;

  public:
    GetPropCompiler(VMFrame &f, JSScript *script, JSObject *obj, ic::PICInfo &pic, JSAtom *atom,
                    VoidStubPIC stub)
      : PICStubCompiler(pic.kind == ic::PICInfo::CALL ? "callprop" : "getprop", f, script, pic,
                        JS_FUNC_TO_DATA_PTR(void *, stub)),
        obj(obj),
        atom(atom),
        lastStubSecondShapeGuard(pic.secondShapeGuard)
    { }

    int getLastStubSecondShapeGuard() const {
        return lastStubSecondShapeGuard ? POST_INST_OFFSET(lastStubSecondShapeGuard) : 0;
    }

    static void reset(Repatcher &repatcher, ic::PICInfo &pic)
    {
        GetPropLabels &labels = pic.getPropLabels();
        repatcher.repatchLEAToLoadPtr(labels.getDslotsLoad(pic.fastPathRejoin));
        repatcher.repatch(labels.getInlineShapeData(pic.getFastShapeGuard()), NULL);
        repatcher.relink(labels.getInlineShapeJump(pic.getFastShapeGuard()), pic.slowPathStart);

        if (pic.hasTypeCheck()) {
            
            repatcher.relink(labels.getInlineTypeJump(pic.fastPathStart), pic.getSlowTypeCheck());
        }

        VoidStubPIC stub;
        switch (pic.kind) {
          case ic::PICInfo::GET:
            stub = ic::GetProp;
            break;
          case ic::PICInfo::CALL:
            stub = ic::CallProp;
            break;
          default:
            JS_NOT_REACHED("invalid pic kind for GetPropCompiler::reset");
            return;
        }

        FunctionPtr target(JS_FUNC_TO_DATA_PTR(void *, stub));
        repatcher.relink(pic.slowPathCall, target);
    }

    LookupStatus generateArgsLengthStub()
    {
        Assembler masm;

        Jump notArgs = masm.guardShape(pic.objReg, obj);

        masm.load32(Address(pic.objReg, JSObject::getFixedSlotOffset(ArgumentsObject::INITIAL_LENGTH_SLOT)), pic.objReg);
        masm.move(pic.objReg, pic.shapeReg);
        Jump overridden = masm.branchTest32(Assembler::NonZero, pic.shapeReg,
                                            Imm32(ArgumentsObject::LENGTH_OVERRIDDEN_BIT));
        masm.rshift32(Imm32(ArgumentsObject::PACKED_BITS_COUNT), pic.objReg);

        masm.move(ImmType(JSVAL_TYPE_INT32), pic.shapeReg);
        Jump done = masm.jump();

        pic.updatePCCounters(cx, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        buffer.link(notArgs, pic.slowPathStart);
        buffer.link(overridden, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);

        CodeLocationLabel start = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generate args length stub at %p\n",
                   start.executableAddress());

        patchPreviousToHere(start);

        disable("args length done");

        return Lookup_Cacheable;
    }

    LookupStatus generateArrayLengthStub()
    {
        Assembler masm;

        masm.loadObjClass(pic.objReg, pic.shapeReg);
        Jump isDense = masm.testClass(Assembler::Equal, pic.shapeReg, &ArrayClass);
        Jump notArray = masm.testClass(Assembler::NotEqual, pic.shapeReg, &SlowArrayClass);

        isDense.linkTo(masm.label(), &masm);
        masm.loadPtr(Address(pic.objReg, JSObject::offsetOfElements()), pic.objReg);
        masm.load32(Address(pic.objReg, ObjectElements::offsetOfLength()), pic.objReg);
        Jump oob = masm.branch32(Assembler::Above, pic.objReg, Imm32(JSVAL_INT_MAX));
        masm.move(ImmType(JSVAL_TYPE_INT32), pic.shapeReg);
        Jump done = masm.jump();

        pic.updatePCCounters(cx, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        buffer.link(notArray, pic.slowPathStart);
        buffer.link(oob, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);

        CodeLocationLabel start = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generate array length stub at %p\n",
                   start.executableAddress());

        patchPreviousToHere(start);

        disable("array length done");

        return Lookup_Cacheable;
    }

    LookupStatus generateStringObjLengthStub()
    {
        Assembler masm;

        Jump notStringObj = masm.guardShape(pic.objReg, obj);

        masm.loadPayload(Address(pic.objReg, JSObject::getPrimitiveThisOffset()), pic.objReg);
        masm.loadPtr(Address(pic.objReg, JSString::offsetOfLengthAndFlags()), pic.objReg);
        masm.urshift32(Imm32(JSString::LENGTH_SHIFT), pic.objReg);
        masm.move(ImmType(JSVAL_TYPE_INT32), pic.shapeReg);
        Jump done = masm.jump();

        pic.updatePCCounters(cx, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        buffer.link(notStringObj, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);

        CodeLocationLabel start = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generate string object length stub at %p\n",
                   start.executableAddress());

        patchPreviousToHere(start);

        disable("string object length done");

        return Lookup_Cacheable;
    }

    LookupStatus generateStringCallStub()
    {
        JS_ASSERT(pic.hasTypeCheck());
        JS_ASSERT(pic.kind == ic::PICInfo::CALL);

        if (!f.fp()->script()->hasGlobal())
            return disable("String.prototype without compile-and-go global");

        GetPropertyHelper<GetPropCompiler> getprop(cx, obj, atom, *this, f);
        LookupStatus status = getprop.lookupAndTest();
        if (status != Lookup_Cacheable)
            return status;
        if (getprop.obj != getprop.holder)
            return disable("proto walk on String.prototype");
        if (!getprop.shape->hasDefaultGetterOrIsMethod())
            return disable("getter hook on String.prototype");
        if (hadGC())
            return Lookup_Uncacheable;

        Assembler masm;

        
        Jump notString = masm.branchPtr(Assembler::NotEqual, pic.typeReg(),
                                        ImmType(JSVAL_TYPE_STRING));

        






        uint32 thisvOffset = uint32(f.regs.sp - f.fp()->slots()) - 1;
        Address thisv(JSFrameReg, sizeof(StackFrame) + thisvOffset * sizeof(Value));
        masm.storeValueFromComponents(ImmType(JSVAL_TYPE_STRING),
                                      pic.objReg, thisv);

        





        masm.move(ImmPtr(obj), pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump shapeMismatch = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                            ImmPtr(obj->lastProperty()));
        masm.loadObjProp(obj, pic.objReg, getprop.shape, pic.shapeReg, pic.objReg);

        Jump done = masm.jump();

        pic.updatePCCounters(cx, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        buffer.link(notString, pic.getSlowTypeCheck());
        buffer.link(shapeMismatch, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);

        CodeLocationLabel cs = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generate string call stub at %p\n",
                   cs.executableAddress());

        
        if (pic.hasTypeCheck()) {
            Repatcher repatcher(f.jit());
            repatcher.relink(pic.getPropLabels().getInlineTypeJump(pic.fastPathStart), cs);
        }

        
        disable("generated string call stub");

        return Lookup_Cacheable;
    }

    LookupStatus generateStringLengthStub()
    {
        JS_ASSERT(pic.hasTypeCheck());

        Assembler masm;
        Jump notString = masm.branchPtr(Assembler::NotEqual, pic.typeReg(),
                                        ImmType(JSVAL_TYPE_STRING));
        masm.loadPtr(Address(pic.objReg, JSString::offsetOfLengthAndFlags()), pic.objReg);
        
        masm.urshift32(Imm32(JSString::LENGTH_SHIFT), pic.objReg);
        masm.move(ImmType(JSVAL_TYPE_INT32), pic.shapeReg);
        Jump done = masm.jump();

        pic.updatePCCounters(cx, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        buffer.link(notString, pic.getSlowTypeCheck());
        buffer.link(done, pic.fastPathRejoin);

        CodeLocationLabel start = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generate string length stub at %p\n",
                   start.executableAddress());

        if (pic.hasTypeCheck()) {
            Repatcher repatcher(f.jit());
            repatcher.relink(pic.getPropLabels().getInlineTypeJump(pic.fastPathStart), start);
        }

        disable("generated string length stub");

        return Lookup_Cacheable;
    }

    LookupStatus patchInline(JSObject *holder, const Shape *shape)
    {
        spew("patch", "inline");
        Repatcher repatcher(f.jit());
        GetPropLabels &labels = pic.getPropLabels();

        int32 offset;
        if (holder->isFixedSlot(shape->slot())) {
            CodeLocationInstruction istr = labels.getDslotsLoad(pic.fastPathRejoin);
            repatcher.repatchLoadPtrToLEA(istr);

            
            
            
            
            
            
            
            int32 diff = int32(JSObject::getFixedSlotOffset(0)) -
                         int32(JSObject::offsetOfSlots());
            JS_ASSERT(diff != 0);
            offset  = (int32(shape->slot()) * sizeof(Value)) + diff;
        } else {
            offset = holder->dynamicSlotIndex(shape->slot()) * sizeof(Value);
        }

        repatcher.repatch(labels.getInlineShapeData(pic.getFastShapeGuard()), obj->lastProperty());
        repatcher.patchAddressOffsetForValueLoad(labels.getValueLoad(pic.fastPathRejoin), offset);

        pic.inlinePathPatched = true;

        return Lookup_Cacheable;
    }

    void generateGetterStub(Assembler &masm, const Shape *shape,
                            Label start, const Vector<Jump, 8> &shapeMismatches)
    {
        



        JS_ASSERT(pic.canCallHook);
        PropertyOp getter = shape->getterOp();

        masm.storePtr(ImmPtr((void *) REJOIN_NATIVE_GETTER),
                      FrameAddress(offsetof(VMFrame, stubRejoin)));

        Registers tempRegs = Registers::tempCallRegMask();
        if (tempRegs.hasReg(Registers::ClobberInCall))
            tempRegs.takeReg(Registers::ClobberInCall);

        
        RegisterID holdObjReg = pic.objReg;
        if (tempRegs.hasReg(pic.objReg)) {
            tempRegs.takeReg(pic.objReg);
        } else {
            holdObjReg = tempRegs.takeAnyReg().reg();
            masm.move(pic.objReg, holdObjReg);
        }

        RegisterID t0 = tempRegs.takeAnyReg().reg();
        masm.bumpStubCounter(f.script(), f.pc(), t0);

        





        int32 vpOffset = (char *) f.regs.sp - (char *) f.fp();
        if (shape->hasSlot()) {
            masm.loadObjProp(obj, holdObjReg, shape,
                             Registers::ClobberInCall, t0);
            masm.storeValueFromComponents(Registers::ClobberInCall, t0, Address(JSFrameReg, vpOffset));
        } else {
            masm.storeValue(UndefinedValue(), Address(JSFrameReg, vpOffset));
        }

        int32 initialFrameDepth = f.regs.sp - f.fp()->slots();
        masm.setupFallibleABICall(cx->typeInferenceEnabled(), f.regs.pc, initialFrameDepth);

        
#ifdef JS_CPU_X86
        RegisterID cxReg = tempRegs.takeAnyReg().reg();
#else
        RegisterID cxReg = Registers::ArgReg0;
#endif
        masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), cxReg);

        
        RegisterID vpReg = t0;
        masm.addPtr(Imm32(vpOffset), JSFrameReg, vpReg);

        masm.restoreStackBase();
        masm.setupABICall(Registers::NormalCall, 4);
        masm.storeArg(3, vpReg);
        masm.storeArg(2, ImmPtr((void *) JSID_BITS(shape->getUserId())));
        masm.storeArg(1, holdObjReg);
        masm.storeArg(0, cxReg);

        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, getter), false);

        NativeStubLinker::FinalJump done;
        if (!NativeStubEpilogue(f, masm, &done, 0, vpOffset, pic.shapeReg, pic.objReg))
            return;
        NativeStubLinker linker(masm, f.jit(), f.regs.pc, done);
        if (!linker.init(f.cx))
            THROW();

        if (!linker.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !linker.verifyRange(f.jit())) {
            disable("code memory is out of range");
            return;
        }

        linker.patchJump(pic.fastPathRejoin);

        linkerEpilogue(linker, start, shapeMismatches);
    }

    LookupStatus generateStub(JSObject *holder, const Shape *shape)
    {
        Vector<Jump, 8> shapeMismatches(cx);

        Assembler masm;

        Label start;
        Jump shapeGuardJump;
        Jump argsLenGuard;

        bool setStubShapeOffset = true;
        if (obj->isDenseArray()) {
            start = masm.label();
            shapeGuardJump = masm.branchPtr(Assembler::NotEqual,
                                            Address(pic.objReg, JSObject::offsetOfShape()),
                                            ImmPtr(obj->lastProperty()));

            



#ifndef JS_HAS_IC_LABELS
            setStubShapeOffset = false;
#endif
        } else {
            if (pic.shapeNeedsRemat()) {
                masm.loadShape(pic.objReg, pic.shapeReg);
                pic.shapeRegHasBaseShape = true;
            }

            start = masm.label();
            shapeGuardJump = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                            ImmPtr(obj->lastProperty()));
        }
        Label stubShapeJumpLabel = masm.label();

        if (!shapeMismatches.append(shapeGuardJump))
            return error();

        RegisterID holderReg = pic.objReg;
        if (obj != holder) {
            
            holderReg = pic.shapeReg;
            masm.move(ImmPtr(holder), holderReg);
            pic.shapeRegHasBaseShape = false;

            
            Jump j = masm.guardShape(holderReg, holder);
            if (!shapeMismatches.append(j))
                return error();

            pic.secondShapeGuard = masm.distanceOf(masm.label()) - masm.distanceOf(start);
        } else {
            pic.secondShapeGuard = 0;
        }

        if (!shape->hasDefaultGetterOrIsMethod()) {
            generateGetterStub(masm, shape, start, shapeMismatches);
            if (setStubShapeOffset)
                pic.getPropLabels().setStubShapeJump(masm, start, stubShapeJumpLabel);
            return Lookup_Cacheable;
        }

        
        masm.loadObjProp(holder, holderReg, shape, pic.shapeReg, pic.objReg);
        Jump done = masm.jump();

        pic.updatePCCounters(cx, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        
        buffer.link(done, pic.fastPathRejoin);

        linkerEpilogue(buffer, start, shapeMismatches);

        if (setStubShapeOffset)
            pic.getPropLabels().setStubShapeJump(masm, start, stubShapeJumpLabel);
        return Lookup_Cacheable;
    }

    void linkerEpilogue(LinkerHelper &buffer, Label start, const Vector<Jump, 8> &shapeMismatches)
    {
        
        for (Jump *pj = shapeMismatches.begin(); pj != shapeMismatches.end(); ++pj)
            buffer.link(*pj, pic.slowPathStart);

        CodeLocationLabel cs = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generated %s stub at %p\n", type, cs.executableAddress());

        patchPreviousToHere(cs);

        pic.stubsGenerated++;
        pic.updateLastPath(buffer, start);

        if (pic.stubsGenerated == MAX_PIC_STUBS)
            disable("max stubs reached");
        if (obj->isDenseArray())
            disable("dense array");
    }

    void patchPreviousToHere(CodeLocationLabel cs)
    {
        Repatcher repatcher(pic.lastCodeBlock(f.jit()));
        CodeLocationLabel label = pic.lastPathStart();

        
        
        
        int shapeGuardJumpOffset;
        if (pic.stubsGenerated)
            shapeGuardJumpOffset = pic.getPropLabels().getStubShapeJumpOffset();
        else
            shapeGuardJumpOffset = pic.shapeGuard + pic.getPropLabels().getInlineShapeJumpOffset();
        int secondGuardOffset = getLastStubSecondShapeGuard();

        JaegerSpew(JSpew_PICs, "Patching previous (%d stubs) (start %p) (offset %d) (second %d)\n",
                   (int) pic.stubsGenerated, label.executableAddress(),
                   shapeGuardJumpOffset, secondGuardOffset);

        repatcher.relink(label.jumpAtOffset(shapeGuardJumpOffset), cs);
        if (secondGuardOffset)
            repatcher.relink(label.jumpAtOffset(secondGuardOffset), cs);
    }

    LookupStatus update()
    {
        JS_ASSERT(pic.hit);

        GetPropertyHelper<GetPropCompiler> getprop(cx, obj, atom, *this, f);
        LookupStatus status = getprop.lookupAndTest();
        if (status != Lookup_Cacheable)
            return status;
        if (hadGC())
            return Lookup_Uncacheable;

        if (obj == getprop.holder &&
            getprop.shape->hasDefaultGetterOrIsMethod() &&
            !pic.inlinePathPatched) {
            return patchInline(getprop.holder, getprop.shape);
        }

        return generateStub(getprop.holder, getprop.shape);
    }
};

class ScopeNameCompiler : public PICStubCompiler
{
  private:
    typedef Vector<Jump, 8> JumpList;

    JSObject *scopeChain;
    JSAtom *atom;
    GetPropertyHelper<ScopeNameCompiler> getprop;
    ScopeNameCompiler *thisFromCtor() { return this; }

    void patchPreviousToHere(CodeLocationLabel cs)
    {
        ScopeNameLabels &       labels = pic.scopeNameLabels();
        Repatcher               repatcher(pic.lastCodeBlock(f.jit()));
        CodeLocationLabel       start = pic.lastPathStart();
        JSC::CodeLocationJump   jump;

        
        if (pic.stubsGenerated)
            jump = labels.getStubJump(start);
        else
            jump = labels.getInlineJump(start);
        repatcher.relink(jump, cs);
    }

    LookupStatus walkScopeChain(Assembler &masm, JumpList &fails)
    {
        
        JSObject *tobj = scopeChain;

        
        JS_ASSERT_IF(pic.kind == ic::PICInfo::XNAME, tobj && tobj == getprop.holder);
        JS_ASSERT_IF(pic.kind == ic::PICInfo::XNAME, getprop.obj == tobj);

        while (tobj && tobj != getprop.holder) {
            if (!IsCacheableNonGlobalScope(tobj))
                return disable("non-cacheable scope chain object");
            JS_ASSERT(tobj->isNative());

            if (tobj != scopeChain) {
                
                Jump j = masm.branchTestPtr(Assembler::Zero, pic.objReg, pic.objReg);
                if (!fails.append(j))
                    return error();
            }

            
            masm.loadShape(pic.objReg, pic.shapeReg);
            Jump j = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                    ImmPtr(tobj->lastProperty()));
            if (!fails.append(j))
                return error();

            
            Address parent(pic.objReg, offsetof(JSObject, parent));
            masm.loadPtr(parent, pic.objReg);

            tobj = tobj->getParent();
        }

        if (tobj != getprop.holder)
            return disable("scope chain walk terminated early");

        return Lookup_Cacheable;
    }

  public:
    ScopeNameCompiler(VMFrame &f, JSScript *script, JSObject *scopeChain, ic::PICInfo &pic,
                      JSAtom *atom, VoidStubPIC stub)
      : PICStubCompiler("name", f, script, pic, JS_FUNC_TO_DATA_PTR(void *, stub)),
        scopeChain(scopeChain), atom(atom),
        getprop(f.cx, NULL, atom, *thisFromCtor(), f)
    { }

    static void reset(Repatcher &repatcher, ic::PICInfo &pic)
    {
        ScopeNameLabels &labels = pic.scopeNameLabels();

        
        JSC::CodeLocationJump inlineJump = labels.getInlineJump(pic.fastPathStart);
        repatcher.relink(inlineJump, pic.slowPathStart);

        VoidStubPIC stub;
        switch (pic.kind) {
          case ic::PICInfo::NAME:
            stub = ic::Name;
            break;
          case ic::PICInfo::XNAME:
            stub = ic::XName;
            break;
          case ic::PICInfo::CALLNAME:
            stub = ic::CallName;
            break;
          default:
            JS_NOT_REACHED("Invalid pic kind in ScopeNameCompiler::reset");
            return;
        }
        FunctionPtr target(JS_FUNC_TO_DATA_PTR(void *, stub));
        repatcher.relink(pic.slowPathCall, target);
    }

    LookupStatus generateGlobalStub(JSObject *obj)
    {
        Assembler masm;
        JumpList fails(cx);
        ScopeNameLabels &labels = pic.scopeNameLabels();

        
        if (pic.kind == ic::PICInfo::NAME || pic.kind == ic::PICInfo::CALLNAME)
            masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), pic.objReg);

        JS_ASSERT(obj == getprop.holder);
        JS_ASSERT(getprop.holder == scopeChain->getGlobal());

        LookupStatus status = walkScopeChain(masm, fails);
        if (status != Lookup_Cacheable)
            return status;

        
        MaybeJump finalNull;
        if (pic.kind == ic::PICInfo::NAME || pic.kind == ic::PICInfo::CALLNAME)
            finalNull = masm.branchTestPtr(Assembler::Zero, pic.objReg, pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump finalShape = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                         ImmPtr(getprop.holder->lastProperty()));

        masm.loadObjProp(obj, pic.objReg, getprop.shape, pic.shapeReg, pic.objReg);

        




        if (pic.kind == ic::PICInfo::CALLNAME) {
            
            Value *thisVp = &cx->regs().sp[1];
            Address thisSlot(JSFrameReg, StackFrame::offsetOfFixed(thisVp - cx->fp()->slots()));
            masm.storeValue(UndefinedValue(), thisSlot);
        }

        Jump done = masm.jump();

        
        for (Jump *pj = fails.begin(); pj != fails.end(); ++pj)
            pj->linkTo(masm.label(), &masm);
        if (finalNull.isSet())
            finalNull.get().linkTo(masm.label(), &masm);
        finalShape.linkTo(masm.label(), &masm);
        Label failLabel = masm.label();
        Jump failJump = masm.jump();

        pic.updatePCCounters(cx, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        buffer.link(failJump, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);
        CodeLocationLabel cs = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generated %s global stub at %p\n", type, cs.executableAddress());
        spew("NAME stub", "global");

        patchPreviousToHere(cs);

        pic.stubsGenerated++;
        pic.updateLastPath(buffer, failLabel);
        labels.setStubJump(masm, failLabel, failJump);

        if (pic.stubsGenerated == MAX_PIC_STUBS)
            disable("max stubs reached");

        return Lookup_Cacheable;
    }

    enum CallObjPropKind {
        ARG,
        VAR
    };

    LookupStatus generateCallStub(JSObject *obj)
    {
        Assembler masm;
        Vector<Jump, 8> fails(cx);
        ScopeNameLabels &labels = pic.scopeNameLabels();

        
        if (pic.kind == ic::PICInfo::NAME || pic.kind == ic::PICInfo::CALLNAME)
            masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), pic.objReg);

        JS_ASSERT(obj == getprop.holder);
        JS_ASSERT(getprop.holder != scopeChain->getGlobal());

        CallObjPropKind kind;
        const Shape *shape = getprop.shape;
        if (shape->getterOp() == GetCallArg) {
            kind = ARG;
        } else if (shape->getterOp() == GetCallVar) {
            kind = VAR;
        } else {
            return disable("unhandled callobj sprop getter");
        }

        LookupStatus status = walkScopeChain(masm, fails);
        if (status != Lookup_Cacheable)
            return status;

        
        MaybeJump finalNull;
        if (pic.kind == ic::PICInfo::NAME || pic.kind == ic::PICInfo::CALLNAME)
            finalNull = masm.branchTestPtr(Assembler::Zero, pic.objReg, pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump finalShape = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                         ImmPtr(getprop.holder->lastProperty()));

        




        if (pic.kind == ic::PICInfo::CALLNAME) {
            JS_ASSERT(obj->isCall());
            Value *thisVp = &cx->regs().sp[1];
            Address thisSlot(JSFrameReg, StackFrame::offsetOfFixed(thisVp - cx->fp()->slots()));
            masm.storeValue(UndefinedValue(), thisSlot);
        }

        
        masm.loadObjPrivate(pic.objReg, pic.shapeReg, getprop.holder->numFixedSlots());

        JSFunction *fun = getprop.holder->asCall().getCalleeFunction();
        uint16 slot = uint16(shape->shortid());

        Jump skipOver;
        Jump escapedFrame = masm.branchTestPtr(Assembler::Zero, pic.shapeReg, pic.shapeReg);

        
        {
            Address addr(pic.shapeReg, kind == ARG ? StackFrame::offsetOfFormalArg(fun, slot)
                                                   : StackFrame::offsetOfFixed(slot));
            masm.loadPayload(addr, pic.objReg);
            masm.loadTypeTag(addr, pic.shapeReg);
            skipOver = masm.jump();
        }

        escapedFrame.linkTo(masm.label(), &masm);

        {
            if (kind == VAR)
                slot += fun->nargs;

            slot += CallObject::RESERVED_SLOTS;
            Address address = masm.objPropAddress(obj, pic.objReg, slot);

            
            masm.loadValueAsComponents(address, pic.shapeReg, pic.objReg);
        }

        skipOver.linkTo(masm.label(), &masm);
        Jump done = masm.jump();

        
        for (Jump *pj = fails.begin(); pj != fails.end(); ++pj)
            pj->linkTo(masm.label(), &masm);
        if (finalNull.isSet())
            finalNull.get().linkTo(masm.label(), &masm);
        finalShape.linkTo(masm.label(), &masm);
        Label failLabel = masm.label();
        Jump failJump = masm.jump();

        pic.updatePCCounters(cx, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        buffer.link(failJump, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);
        CodeLocationLabel cs = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generated %s call stub at %p\n", type, cs.executableAddress());

        patchPreviousToHere(cs);

        pic.stubsGenerated++;
        pic.updateLastPath(buffer, failLabel);
        labels.setStubJump(masm, failLabel, failJump);

        if (pic.stubsGenerated == MAX_PIC_STUBS)
            disable("max stubs reached");

        return Lookup_Cacheable;
    }

    LookupStatus updateForName()
    {
        
        LookupStatus status = getprop.bind();
        if (status != Lookup_Cacheable)
            return status;

        return update(getprop.obj);
    }

    LookupStatus updateForXName()
    {
        
        getprop.obj = scopeChain;
        LookupStatus status = getprop.lookup();
        if (status != Lookup_Cacheable)
            return status;

        return update(getprop.obj);
    }

    LookupStatus update(JSObject *obj)
    {
        if (obj != getprop.holder)
            return disable("property is on proto of a scope object");

        if (obj->isCall())
            return generateCallStub(obj);

        LookupStatus status = getprop.testForGet();
        if (status != Lookup_Cacheable)
            return status;

        if (!obj->getParent())
            return generateGlobalStub(obj);

        return disable("scope object not handled yet");
    }

    bool retrieve(Value *vp, Value *thisvp, PICInfo::Kind kind)
    {
        JSObject *obj = getprop.obj;
        JSObject *holder = getprop.holder;
        const JSProperty *prop = getprop.prop;

        if (!prop) {
            
            if (kind == ic::PICInfo::NAME) {
                JSOp op2 = js_GetOpcode(cx, f.script(), f.pc() + JSOP_NAME_LENGTH);
                if (op2 == JSOP_TYPEOF) {
                    vp->setUndefined();
                    return true;
                }
            }
            ReportAtomNotDefined(cx, atom);
            return false;
        }

        
        
        if (!getprop.shape) {
            if (!obj->getGeneric(cx, ATOM_TO_JSID(atom), vp))
                return false;
            if (thisvp)
                return ComputeImplicitThis(cx, obj, *vp, thisvp);
            return true;
        }

        const Shape *shape = getprop.shape;
        JSObject *normalized = obj;
        if (obj->isWith() && !shape->hasDefaultGetter())
            normalized = js_UnwrapWithObject(cx, obj);
        NATIVE_GET(cx, normalized, holder, shape, JSGET_METHOD_BARRIER, vp, return false);
        if (thisvp)
            return ComputeImplicitThis(cx, normalized, *vp, thisvp);
        return true;
    }
};

class BindNameCompiler : public PICStubCompiler
{
    JSObject *scopeChain;
    JSAtom *atom;

  public:
    BindNameCompiler(VMFrame &f, JSScript *script, JSObject *scopeChain, ic::PICInfo &pic,
                      JSAtom *atom, VoidStubPIC stub)
      : PICStubCompiler("bind", f, script, pic, JS_FUNC_TO_DATA_PTR(void *, stub)),
        scopeChain(scopeChain), atom(atom)
    { }

    static void reset(Repatcher &repatcher, ic::PICInfo &pic)
    {
        BindNameLabels &labels = pic.bindNameLabels();

        
        JSC::CodeLocationJump inlineJump = labels.getInlineJump(pic.getFastShapeGuard());
        repatcher.relink(inlineJump, pic.slowPathStart);

        
        FunctionPtr target(JS_FUNC_TO_DATA_PTR(void *, ic::BindName));
        repatcher.relink(pic.slowPathCall, target);
    }

    void patchPreviousToHere(CodeLocationLabel cs)
    {
        BindNameLabels &labels = pic.bindNameLabels();
        Repatcher repatcher(pic.lastCodeBlock(f.jit()));
        JSC::CodeLocationJump jump;

        
        if (pic.stubsGenerated)
            jump = labels.getStubJump(pic.lastPathStart());
        else
            jump = labels.getInlineJump(pic.getFastShapeGuard());
        repatcher.relink(jump, cs);
    }

    LookupStatus generateStub(JSObject *obj)
    {
        Assembler masm;
        Vector<Jump, 8> fails(cx);

        BindNameLabels &labels = pic.bindNameLabels();

        
        masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump firstShape = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                         ImmPtr(scopeChain->lastProperty()));

        
        JSObject *tobj = scopeChain;
        Address parent(pic.objReg, offsetof(JSObject, parent));
        while (tobj && tobj != obj) {
            if (!IsCacheableNonGlobalScope(tobj))
                return disable("non-cacheable obj in scope chain");
            masm.loadPtr(parent, pic.objReg);
            Jump nullTest = masm.branchTestPtr(Assembler::Zero, pic.objReg, pic.objReg);
            if (!fails.append(nullTest))
                return error();
            masm.loadShape(pic.objReg, pic.shapeReg);
            Jump shapeTest = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                            ImmPtr(tobj->lastProperty()));
            if (!fails.append(shapeTest))
                return error();
            tobj = tobj->getParent();
        }
        if (tobj != obj)
            return disable("indirect hit");

        Jump done = masm.jump();

        
        for (Jump *pj = fails.begin(); pj != fails.end(); ++pj)
            pj->linkTo(masm.label(), &masm);
        firstShape.linkTo(masm.label(), &masm);
        Label failLabel = masm.label();
        Jump failJump = masm.jump();

        pic.updatePCCounters(cx, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        buffer.link(failJump, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);
        CodeLocationLabel cs = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generated %s stub at %p\n", type, cs.executableAddress());

        patchPreviousToHere(cs);

        pic.stubsGenerated++;
        pic.updateLastPath(buffer, failLabel);
        labels.setStubJump(masm, failLabel, failJump);

        if (pic.stubsGenerated == MAX_PIC_STUBS)
            disable("max stubs reached");

        return Lookup_Cacheable;
    }

    JSObject *update()
    {
        JS_ASSERT(scopeChain->getParent());
        RecompilationMonitor monitor(cx);

        JSObject *obj = js_FindIdentifierBase(cx, scopeChain, ATOM_TO_JSID(atom));

        if (monitor.recompiled())
            return obj;

        if (!obj) {
            disable("error");
            return obj;
        }

        if (!pic.hit) {
            spew("first hit", "nop");
            pic.hit = true;
            return obj;
        }

        LookupStatus status = generateStub(obj);
        if (status == Lookup_Error) {
            disable("error");
            return NULL;
        }

        return obj;
    }
};

static void JS_FASTCALL
DisabledGetPropIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::GetProp(f);
}

static void JS_FASTCALL
DisabledGetPropICNoCache(VMFrame &f, ic::PICInfo *pic)
{
    stubs::GetPropNoCache(f, pic->atom);
}

void JS_FASTCALL
ic::GetProp(VMFrame &f, ic::PICInfo *pic)
{
    JSScript *script = f.fp()->script();

    JSAtom *atom = pic->atom;
    if (atom == f.cx->runtime->atomState.lengthAtom) {
        if (f.regs.sp[-1].isString()) {
            GetPropCompiler cc(f, script, NULL, *pic, NULL, DisabledGetPropIC);
            LookupStatus status = cc.generateStringLengthStub();
            if (status == Lookup_Error)
                THROW();
            JSString *str = f.regs.sp[-1].toString();
            f.regs.sp[-1].setInt32(str->length());
            return;
        } else if (f.regs.sp[-1].isMagic(JS_LAZY_ARGUMENTS)) {
            f.regs.sp[-1].setInt32(f.regs.fp()->numActualArgs());
            return;
        } else if (!f.regs.sp[-1].isPrimitive()) {
            JSObject *obj = &f.regs.sp[-1].toObject();
            if (obj->isArray() ||
                (obj->isArguments() && !obj->asArguments()->hasOverriddenLength()) ||
                obj->isString()) {
                GetPropCompiler cc(f, script, obj, *pic, NULL, DisabledGetPropIC);
                if (obj->isArray()) {
                    LookupStatus status = cc.generateArrayLengthStub();
                    if (status == Lookup_Error)
                        THROW();
                    f.regs.sp[-1].setNumber(obj->getArrayLength());
                } else if (obj->isArguments()) {
                    LookupStatus status = cc.generateArgsLengthStub();
                    if (status == Lookup_Error)
                        THROW();
                    f.regs.sp[-1].setInt32(int32_t(obj->asArguments()->initialLength()));
                } else if (obj->isString()) {
                    LookupStatus status = cc.generateStringObjLengthStub();
                    if (status == Lookup_Error)
                        THROW();
                    JSString *str = obj->getPrimitiveThis().toString();
                    f.regs.sp[-1].setInt32(str->length());
                }
                return;
            }
        }
        atom = f.cx->runtime->atomState.lengthAtom;
    }

    




    RecompilationMonitor monitor(f.cx);

    JSObject *obj = ValueToObject(f.cx, &f.regs.sp[-1]);
    if (!obj)
        THROW();

    if (!monitor.recompiled() && pic->shouldUpdate(f.cx)) {
        VoidStubPIC stub = pic->usePropCache
                           ? DisabledGetPropIC
                           : DisabledGetPropICNoCache;
        GetPropCompiler cc(f, script, obj, *pic, atom, stub);
        if (!cc.update()) {
            cc.disable("error");
            THROW();
        }
    }

    Value v;
    if (!obj->getGeneric(f.cx, ATOM_TO_JSID(atom), &v))
        THROW();

    f.regs.sp[-1] = v;
}

void JS_FASTCALL
ic::GetPropNoCache(VMFrame &f, ic::PICInfo *pic)
{
    




    GetProp(f, pic);
}

template <JSBool strict>
static void JS_FASTCALL
DisabledSetPropIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::SetName<strict>(f, pic->atom);
}

template <JSBool strict>
static void JS_FASTCALL
DisabledSetPropICNoCache(VMFrame &f, ic::PICInfo *pic)
{
    stubs::SetPropNoCache<strict>(f, pic->atom);
}

void JS_FASTCALL
ic::SetProp(VMFrame &f, ic::PICInfo *pic)
{
    JSScript *script = f.fp()->script();
    JS_ASSERT(pic->isSet());

    VoidStubPIC stub = pic->usePropCache
                       ? STRICT_VARIANT(DisabledSetPropIC)
                       : STRICT_VARIANT(DisabledSetPropICNoCache);

    
    JSAtom *atom = pic->atom;
    VoidStubAtom nstub = pic->usePropCache
                         ? STRICT_VARIANT(stubs::SetName)
                         : STRICT_VARIANT(stubs::SetPropNoCache);

    RecompilationMonitor monitor(f.cx);

    JSObject *obj = ValueToObject(f.cx, &f.regs.sp[-2]);
    if (!obj)
        THROW();

    
    
    if (!monitor.recompiled() && pic->shouldUpdate(f.cx)) {
        SetPropCompiler cc(f, script, obj, *pic, atom, stub);
        LookupStatus status = cc.update();
        if (status == Lookup_Error)
            THROW();
    }

    nstub(f, atom);
}

static void JS_FASTCALL
DisabledCallPropIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::CallProp(f, pic->atom);
}

void JS_FASTCALL
ic::CallProp(VMFrame &f, ic::PICInfo *pic)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;

    JSScript *script = f.fp()->script();
    RecompilationMonitor monitor(cx);

    Value lval;
    lval = regs.sp[-1];

    
    jsid id = ATOM_TO_JSID(pic->atom);

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
            THROW();
        }
        JSObject *pobj;
        if (!js_GetClassPrototype(cx, NULL, protoKey, &pobj))
            THROW();
        objv.setObject(*pobj);
    }

    JSObject *aobj = js_GetProtoIfDenseArray(&objv.toObject());
    Value rval;

    PropertyCacheEntry *entry;
    JSObject *obj2;
    JSAtom *atom;
    JS_PROPERTY_CACHE(cx).test(cx, f.pc(), aobj, obj2, entry, atom);
    if (!atom) {
        NATIVE_GET(cx, &objv.toObject(), obj2, entry->prop, JSGET_NO_METHOD_BARRIER, &rval,
                   THROW());
        




        regs.sp++;
        regs.sp[-2] = rval;
        regs.sp[-1] = lval;
    } else {
        



        regs.sp++;
        regs.sp[-1].setNull();
        if (lval.isObject()) {
            if (!js_GetMethod(cx, &objv.toObject(), id,
                              JS_LIKELY(!objv.toObject().getOps()->getProperty)
                              ? JSGET_CACHE_RESULT | JSGET_NO_METHOD_BARRIER
                              : JSGET_NO_METHOD_BARRIER,
                              &rval)) {
                THROW();
            }
            regs.sp[-1] = objv;
            regs.sp[-2] = rval;
        } else {
            JS_ASSERT(!objv.toObject().getOps()->getProperty);
            if (!js_GetPropertyHelper(cx, &objv.toObject(), id,
                                      JSGET_CACHE_RESULT | JSGET_NO_METHOD_BARRIER,
                                      &rval)) {
                THROW();
            }
            regs.sp[-1] = lval;
            regs.sp[-2] = rval;
        }
    }

#if JS_HAS_NO_SUCH_METHOD
    if (JS_UNLIKELY(rval.isPrimitive()) && regs.sp[-1].isObject()) {
        regs.sp[-2].setString(JSID_TO_STRING(id));
        if (!OnUnknownMethod(cx, regs.sp - 2))
            THROW();
    }
#endif

    if (monitor.recompiled())
        return;

    GetPropCompiler cc(f, script, &objv.toObject(), *pic, pic->atom, DisabledCallPropIC);
    if (lval.isObject()) {
        if (pic->shouldUpdate(cx)) {
            LookupStatus status = cc.update();
            if (status == Lookup_Error)
                THROW();
        }
    } else if (lval.isString()) {
        LookupStatus status = cc.generateStringCallStub();
        if (status == Lookup_Error)
            THROW();
    } else {
        cc.disable("non-string primitive");
    }
}

static void JS_FASTCALL
DisabledNameIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::Name(f);
}

static void JS_FASTCALL
DisabledXNameIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::GetProp(f);
}

void JS_FASTCALL
ic::XName(VMFrame &f, ic::PICInfo *pic)
{
    JSScript *script = f.fp()->script();

    
    JSObject *obj = &f.regs.sp[-1].toObject();

    ScopeNameCompiler cc(f, script, obj, *pic, pic->atom, DisabledXNameIC);

    LookupStatus status = cc.updateForXName();
    if (status == Lookup_Error)
        THROW();

    Value rval;
    if (!cc.retrieve(&rval, NULL, PICInfo::XNAME))
        THROW();
    f.regs.sp[-1] = rval;
}

void JS_FASTCALL
ic::Name(VMFrame &f, ic::PICInfo *pic)
{
    JSScript *script = f.fp()->script();

    ScopeNameCompiler cc(f, script, &f.fp()->scopeChain(), *pic, pic->atom, DisabledNameIC);

    LookupStatus status = cc.updateForName();
    if (status == Lookup_Error)
        THROW();

    Value rval;
    if (!cc.retrieve(&rval, NULL, PICInfo::NAME))
        THROW();
    f.regs.sp[0] = rval;
}

static void JS_FASTCALL
DisabledCallNameIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::CallName(f);
}

void JS_FASTCALL
ic::CallName(VMFrame &f, ic::PICInfo *pic)
{
    JSScript *script = f.fp()->script();

    ScopeNameCompiler cc(f, script, &f.fp()->scopeChain(), *pic, pic->atom, DisabledCallNameIC);

    LookupStatus status = cc.updateForName();
    if (status == Lookup_Error)
        THROW();

    Value rval, thisval;
    if (!cc.retrieve(&rval, &thisval, PICInfo::CALLNAME))
        THROW();

    f.regs.sp[0] = rval;
    f.regs.sp[1] = thisval;
}

static void JS_FASTCALL
DisabledBindNameIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::BindName(f);
}

static void JS_FASTCALL
DisabledBindNameICNoCache(VMFrame &f, ic::PICInfo *pic)
{
    stubs::BindNameNoCache(f, pic->atom);
}

void JS_FASTCALL
ic::BindName(VMFrame &f, ic::PICInfo *pic)
{
    JSScript *script = f.fp()->script();

    VoidStubPIC stub = pic->usePropCache
                       ? DisabledBindNameIC
                       : DisabledBindNameICNoCache;
    BindNameCompiler cc(f, script, &f.fp()->scopeChain(), *pic, pic->atom, stub);

    JSObject *obj = cc.update();
    if (!obj)
        THROW();

    f.regs.sp[0].setObject(*obj);
}

bool
BaseIC::isCallOp()
{
    return !!(js_CodeSpec[op].format & JOF_CALLOP);
}

void
BaseIC::spew(JSContext *cx, const char *event, const char *message)
{
#ifdef JS_METHODJIT_SPEW
    JaegerSpew(JSpew_PICs, "%s %s: %s (%s: %d)\n",
               js_CodeName[op], event, message, cx->fp()->script()->filename, CurrentLine(cx));
#endif
}


inline uint32 frameCountersOffset(JSContext *cx)
{
    uint32 offset = 0;
    if (cx->regs().inlined()) {
        offset += cx->fp()->script()->length;
        uint32 index = cx->regs().inlined()->inlineIndex;
        InlineFrame *frames = cx->fp()->jit()->inlineFrames();
        for (unsigned i = 0; i < index; i++)
            offset += frames[i].fun->script()->length;
    }

    jsbytecode *pc;
    JSScript *script = cx->stack.currentScript(&pc);
    offset += pc - script->code;

    return offset;
}

LookupStatus
BaseIC::disable(JSContext *cx, const char *reason, void *stub)
{
    JITScript *jit = cx->fp()->jit();
    if (jit->pcLengths) {
        uint32 offset = frameCountersOffset(cx);
        jit->pcLengths[offset].picsLength = 0;
    }

    spew(cx, "disabled", reason);
    Repatcher repatcher(jit);
    repatcher.relink(slowPathCall, FunctionPtr(stub));
    return Lookup_Uncacheable;
}

void
BaseIC::updatePCCounters(JSContext *cx, Assembler &masm)
{
    JITScript *jit = cx->fp()->jit();
    if (jit->pcLengths) {
        uint32 offset = frameCountersOffset(cx);
        jit->pcLengths[offset].picsLength += masm.size();
    }
}

bool
BaseIC::shouldUpdate(JSContext *cx)
{
    if (!hit) {
        hit = true;
        spew(cx, "ignored", "first hit");
        return false;
    }
    JS_ASSERT(stubsGenerated < MAX_PIC_STUBS);
    return true;
}

static void JS_FASTCALL
DisabledGetElem(VMFrame &f, ic::GetElementIC *ic)
{
    stubs::GetElem(f);
}

static void JS_FASTCALL
DisabledCallElem(VMFrame &f, ic::GetElementIC *ic)
{
    stubs::CallElem(f);
}

bool
GetElementIC::shouldUpdate(JSContext *cx)
{
    if (!hit) {
        hit = true;
        spew(cx, "ignored", "first hit");
        return false;
    }
    JS_ASSERT(stubsGenerated < MAX_GETELEM_IC_STUBS);
    return true;
}

LookupStatus
GetElementIC::disable(JSContext *cx, const char *reason)
{
    slowCallPatched = true;
    void *stub = (op == JSOP_GETELEM)
                 ? JS_FUNC_TO_DATA_PTR(void *, DisabledGetElem)
                 : JS_FUNC_TO_DATA_PTR(void *, DisabledCallElem);
    BaseIC::disable(cx, reason, stub);
    return Lookup_Uncacheable;
}

LookupStatus
GetElementIC::error(JSContext *cx)
{
    disable(cx, "error");
    return Lookup_Error;
}

void
GetElementIC::purge(Repatcher &repatcher)
{
    
    if (inlineTypeGuardPatched)
        repatcher.relink(fastPathStart.jumpAtOffset(inlineTypeGuard), slowPathStart);
    if (inlineShapeGuardPatched)
        repatcher.relink(fastPathStart.jumpAtOffset(inlineShapeGuard), slowPathStart);

    if (slowCallPatched) {
        if (op == JSOP_GETELEM) {
            repatcher.relink(slowPathCall,
                             FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, ic::GetElement)));
        } else if (op == JSOP_CALLELEM) {
            repatcher.relink(slowPathCall,
                             FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, ic::CallElement)));
        }
    }

    reset();
}

LookupStatus
GetElementIC::attachGetProp(VMFrame &f, JSContext *cx, JSObject *obj, const Value &v, jsid id, Value *vp)
{
    JS_ASSERT(v.isString());

    GetPropertyHelper<GetElementIC> getprop(cx, obj, JSID_TO_ATOM(id), *this, f);
    LookupStatus status = getprop.lookupAndTest();
    if (status != Lookup_Cacheable)
        return status;

    
    
    
    if (cx->typeInferenceEnabled() && !forcedTypeBarrier)
        return disable(cx, "string element access may not have type barrier");

    Assembler masm;

    
    MaybeJump atomTypeGuard;
    if (hasInlineTypeGuard() && !inlineTypeGuardPatched) {
        
        
        
        
        
        JS_ASSERT(!idRemat.isTypeKnown());
        atomTypeGuard = masm.testString(Assembler::NotEqual, typeReg);
    } else {
        
        
        
        JS_ASSERT_IF(!hasInlineTypeGuard(), idRemat.knownType() == JSVAL_TYPE_STRING);
    }

    
    if (!obj->isDenseArray() && !typeRegHasBaseShape) {
        masm.loadShape(objReg, typeReg);
        typeRegHasBaseShape = true;
    }

    MaybeJump atomIdGuard;
    if (!idRemat.isConstant())
        atomIdGuard = masm.branchPtr(Assembler::NotEqual, idRemat.dataReg(), ImmPtr(v.toString()));

    
    Jump shapeGuard = masm.branchPtr(Assembler::NotEqual, typeReg, ImmPtr(obj->lastProperty()));

    
    MaybeJump protoGuard;
    JSObject *holder = getprop.holder;
    RegisterID holderReg = objReg;
    if (obj != holder) {
        
        holderReg = typeReg;
        masm.move(ImmPtr(holder), holderReg);
        typeRegHasBaseShape = false;

        
        protoGuard = masm.guardShape(holderReg, holder);
    }

    if (op == JSOP_CALLELEM) {
        
        Value *thisVp = &cx->regs().sp[-1];
        Address thisSlot(JSFrameReg, StackFrame::offsetOfFixed(thisVp - cx->fp()->slots()));
        masm.storeValueFromComponents(ImmType(JSVAL_TYPE_OBJECT), objReg, thisSlot);
    }

    
    const Shape *shape = getprop.shape;
    masm.loadObjProp(holder, holderReg, shape, typeReg, objReg);

    Jump done = masm.jump();

    updatePCCounters(cx, masm);

    PICLinker buffer(masm, *this);
    if (!buffer.init(cx))
        return error(cx);

    if (hasLastStringStub && !buffer.verifyRange(lastStringStub))
        return disable(cx, "code memory is out of range");
    if (!buffer.verifyRange(cx->fp()->jit()))
        return disable(cx, "code memory is out of range");

    
    buffer.maybeLink(atomIdGuard, slowPathStart);
    buffer.maybeLink(atomTypeGuard, slowPathStart);
    buffer.link(shapeGuard, slowPathStart);
    buffer.maybeLink(protoGuard, slowPathStart);
    buffer.link(done, fastPathRejoin);

    CodeLocationLabel cs = buffer.finalize();
#if DEBUG
    char *chars = DeflateString(cx, v.toString()->getChars(cx), v.toString()->length());
    JaegerSpew(JSpew_PICs, "generated %s stub at %p for atom %p (\"%s\") shape %p (%s: %d)\n",
               js_CodeName[op], cs.executableAddress(), (void*)JSID_TO_ATOM(id), chars,
               holder->lastProperty(), cx->fp()->script()->filename, CurrentLine(cx));
    cx->free_(chars);
#endif

    
    if (shouldPatchInlineTypeGuard() || shouldPatchUnconditionalShapeGuard()) {
        Repatcher repatcher(cx->fp()->jit());

        if (shouldPatchInlineTypeGuard()) {
            
            
            JS_ASSERT(!inlineTypeGuardPatched);
            JS_ASSERT(atomTypeGuard.isSet());

            repatcher.relink(fastPathStart.jumpAtOffset(inlineTypeGuard), cs);
            inlineTypeGuardPatched = true;
        }

        if (shouldPatchUnconditionalShapeGuard()) {
            
            
            
            
            JS_ASSERT(!hasInlineTypeGuard());

            repatcher.relink(fastPathStart.jumpAtOffset(inlineShapeGuard), cs);
            inlineShapeGuardPatched = true;
        }
    }

    
    if (hasLastStringStub) {
        Repatcher repatcher(lastStringStub);
        CodeLocationLabel stub(lastStringStub.start());
        if (atomGuard)
            repatcher.relink(stub.jumpAtOffset(atomGuard), cs);
        repatcher.relink(stub.jumpAtOffset(firstShapeGuard), cs);
        if (secondShapeGuard)
            repatcher.relink(stub.jumpAtOffset(secondShapeGuard), cs);
    }

    
    hasLastStringStub = true;
    lastStringStub = JITCode(cs.executableAddress(), buffer.size());
    if (atomIdGuard.isSet()) {
        atomGuard = buffer.locationOf(atomIdGuard.get()) - cs;
        JS_ASSERT(atomGuard == buffer.locationOf(atomIdGuard.get()) - cs);
        JS_ASSERT(atomGuard);
    } else {
        atomGuard = 0;
    }
    if (protoGuard.isSet()) {
        secondShapeGuard = buffer.locationOf(protoGuard.get()) - cs;
        JS_ASSERT(secondShapeGuard == buffer.locationOf(protoGuard.get()) - cs);
        JS_ASSERT(secondShapeGuard);
    } else {
        secondShapeGuard = 0;
    }
    firstShapeGuard = buffer.locationOf(shapeGuard) - cs;
    JS_ASSERT(firstShapeGuard == buffer.locationOf(shapeGuard) - cs);
    JS_ASSERT(firstShapeGuard);

    stubsGenerated++;

    if (stubsGenerated == MAX_GETELEM_IC_STUBS)
        disable(cx, "max stubs reached");

    
    *vp = holder->getSlot(shape->slot());

    return Lookup_Cacheable;
}

LookupStatus
GetElementIC::attachArguments(JSContext *cx, JSObject *obj, const Value &v, jsid id, Value *vp)
{
    if (!v.isInt32())
        return disable(cx, "arguments object with non-integer key");

    if (op == JSOP_CALLELEM)
        return disable(cx, "arguments object with call");

    JS_ASSERT(hasInlineTypeGuard() || idRemat.knownType() == JSVAL_TYPE_INT32);

    Assembler masm;

    Jump shapeGuard = masm.guardShape(objReg, obj);

    masm.move(objReg, typeReg);
    masm.load32(Address(objReg, JSObject::getFixedSlotOffset(ArgumentsObject::INITIAL_LENGTH_SLOT)), 
                objReg);
    Jump overridden = masm.branchTest32(Assembler::NonZero, objReg,
                                        Imm32(ArgumentsObject::LENGTH_OVERRIDDEN_BIT));
    masm.rshift32(Imm32(ArgumentsObject::PACKED_BITS_COUNT), objReg);

    Jump outOfBounds;
    if (idRemat.isConstant()) {
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, objReg, Imm32(v.toInt32()));
    } else {
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, objReg, idRemat.dataReg());
    }

    masm.loadPayload(Address(typeReg, JSObject::getFixedSlotOffset(ArgumentsObject::DATA_SLOT)), objReg);
    if (idRemat.isConstant()) {
        Address slot(objReg, offsetof(ArgumentsData, slots) + v.toInt32() * sizeof(Value));
        masm.loadTypeTag(slot, objReg);
    } else {
        BaseIndex slot(objReg, idRemat.dataReg(), Assembler::JSVAL_SCALE, 
                       offsetof(ArgumentsData, slots));
        masm.loadTypeTag(slot, objReg);
    }    
    Jump holeCheck = masm.branchPtr(Assembler::Equal, objReg, ImmType(JSVAL_TYPE_MAGIC));

    Address privateData(typeReg, JSObject::getPrivateDataOffset(ArgumentsObject::NFIXED_SLOTS));
    Jump liveArguments = masm.branchPtr(Assembler::NotEqual, privateData, ImmPtr(0));
   
    masm.loadPrivate(Address(typeReg, JSObject::getFixedSlotOffset(ArgumentsObject::DATA_SLOT)), objReg);

    if (idRemat.isConstant()) {
        Address slot(objReg, offsetof(ArgumentsData, slots) + v.toInt32() * sizeof(Value));
        masm.loadValueAsComponents(slot, typeReg, objReg);           
    } else {
        BaseIndex slot(objReg, idRemat.dataReg(), Assembler::JSVAL_SCALE, 
                       offsetof(ArgumentsData, slots));
        masm.loadValueAsComponents(slot, typeReg, objReg);
    }

    Jump done = masm.jump();

    liveArguments.linkTo(masm.label(), &masm);

    masm.loadPtr(privateData, typeReg);

    Address fun(typeReg, StackFrame::offsetOfExec());
    masm.loadPtr(fun, objReg);

    Address nargs(objReg, offsetof(JSFunction, nargs));
    masm.load16(nargs, objReg);

    Jump notFormalArg;
    if (idRemat.isConstant())
        notFormalArg = masm.branch32(Assembler::BelowOrEqual, objReg, Imm32(v.toInt32()));
    else
        notFormalArg = masm.branch32(Assembler::BelowOrEqual, objReg, idRemat.dataReg());

    masm.lshift32(Imm32(3), objReg); 
    masm.subPtr(objReg, typeReg); 

    Label loadFromStack = masm.label();
    masm.move(typeReg, objReg);

    if (idRemat.isConstant()) {
        Address frameEntry(objReg, v.toInt32() * sizeof(Value));
        masm.loadValueAsComponents(frameEntry, typeReg, objReg);
    } else {
        BaseIndex frameEntry(objReg, idRemat.dataReg(), Assembler::JSVAL_SCALE);
        masm.loadValueAsComponents(frameEntry, typeReg, objReg);
    }    
    Jump done2 = masm.jump();

    notFormalArg.linkTo(masm.label(), &masm);

    masm.push(typeReg);

    Address argsObject(typeReg, StackFrame::offsetOfArgs());
    masm.loadPtr(argsObject, typeReg);

    masm.load32(Address(typeReg, JSObject::getFixedSlotOffset(ArgumentsObject::INITIAL_LENGTH_SLOT)), 
                typeReg); 
    masm.rshift32(Imm32(ArgumentsObject::PACKED_BITS_COUNT), typeReg); 

    

    masm.addPtr(typeReg, objReg);
    masm.addPtr(Imm32(2), objReg);
    masm.lshiftPtr(Imm32(3), objReg);

    masm.pop(typeReg);
    masm.subPtr(objReg, typeReg);

    masm.jump(loadFromStack);

    updatePCCounters(cx, masm);

    PICLinker buffer(masm, *this);

    if (!buffer.init(cx))
        return error(cx);

    if (!buffer.verifyRange(cx->fp()->jit()))
        return disable(cx, "code memory is out of range");

    buffer.link(shapeGuard, slowPathStart);
    buffer.link(overridden, slowPathStart);
    buffer.link(outOfBounds, slowPathStart);
    buffer.link(holeCheck, slowPathStart);
    buffer.link(done, fastPathRejoin);    
    buffer.link(done2, fastPathRejoin);
    
    CodeLocationLabel cs = buffer.finalizeCodeAddendum();

    JaegerSpew(JSpew_PICs, "generated getelem arguments stub at %p\n", cs.executableAddress());

    Repatcher repatcher(cx->fp()->jit());
    repatcher.relink(fastPathStart.jumpAtOffset(inlineShapeGuard), cs);

    JS_ASSERT(!shouldPatchUnconditionalShapeGuard());
    JS_ASSERT(!inlineShapeGuardPatched);

    inlineShapeGuardPatched = true;
    stubsGenerated++;

    if (stubsGenerated == MAX_GETELEM_IC_STUBS)
        disable(cx, "max stubs reached");

    disable(cx, "generated arguments stub");

    if (!obj->getGeneric(cx, id, vp))
        return Lookup_Error;

    return Lookup_Cacheable;
}

#if defined JS_METHODJIT_TYPED_ARRAY
LookupStatus
GetElementIC::attachTypedArray(JSContext *cx, JSObject *obj, const Value &v, jsid id, Value *vp)
{
    if (!v.isInt32())
        return disable(cx, "typed array with string key");

    if (op == JSOP_CALLELEM)
        return disable(cx, "typed array with call");

    
    
    JS_ASSERT(hasInlineTypeGuard() || idRemat.knownType() == JSVAL_TYPE_INT32);

    Assembler masm;

    
    Jump shapeGuard = masm.guardShape(objReg, obj);

    
    Jump outOfBounds;
    Address typedArrayLength(objReg, TypedArray::lengthOffset());
    if (idRemat.isConstant()) {
        JS_ASSERT(idRemat.value().toInt32() == v.toInt32());
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, typedArrayLength, Imm32(v.toInt32()));
    } else {
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, typedArrayLength, idRemat.dataReg());
    }

    
    masm.loadPtr(Address(objReg, TypedArray::dataOffset()), objReg);

    Int32Key key = idRemat.isConstant()
                 ? Int32Key::FromConstant(v.toInt32())
                 : Int32Key::FromRegister(idRemat.dataReg());

    JSObject *tarray = js::TypedArray::getTypedArray(obj);
    MaybeRegisterID tempReg;
    masm.loadFromTypedArray(TypedArray::getType(tarray), objReg, key, typeReg, objReg, tempReg);

    Jump done = masm.jump();

    updatePCCounters(cx, masm);

    PICLinker buffer(masm, *this);
    if (!buffer.init(cx))
        return error(cx);

    if (!buffer.verifyRange(cx->fp()->jit()))
        return disable(cx, "code memory is out of range");

    buffer.link(shapeGuard, slowPathStart);
    buffer.link(outOfBounds, slowPathStart);
    buffer.link(done, fastPathRejoin);

    CodeLocationLabel cs = buffer.finalizeCodeAddendum();
    JaegerSpew(JSpew_PICs, "generated getelem typed array stub at %p\n", cs.executableAddress());

    
    
    JS_ASSERT(!shouldPatchUnconditionalShapeGuard());
    JS_ASSERT(!inlineShapeGuardPatched);

    Repatcher repatcher(cx->fp()->jit());
    repatcher.relink(fastPathStart.jumpAtOffset(inlineShapeGuard), cs);
    inlineShapeGuardPatched = true;

    stubsGenerated++;

    
    
    if (stubsGenerated == MAX_GETELEM_IC_STUBS)
        disable(cx, "max stubs reached");

    disable(cx, "generated typed array stub");

    
    if (!obj->getGeneric(cx, id, vp))
        return Lookup_Error;

    return Lookup_Cacheable;
}
#endif 

LookupStatus
GetElementIC::update(VMFrame &f, JSContext *cx, JSObject *obj, const Value &v, jsid id, Value *vp)
{
    





    if (v.isString() && js_CheckForStringIndex(id) == id)
        return attachGetProp(f, cx, obj, v, id, vp);

#if defined JS_METHODJIT_TYPED_ARRAY
    









    if (!cx->typeInferenceEnabled() && js_IsTypedArray(obj))
        return attachTypedArray(cx, obj, v, id, vp);
#endif

    return disable(cx, "unhandled object and key type");
}

void JS_FASTCALL
ic::CallElement(VMFrame &f, ic::GetElementIC *ic)
{
    JSContext *cx = f.cx;

    
    if (!f.regs.sp[-2].isObject()) {
        ic->disable(cx, "non-object");
        stubs::CallElem(f);
        return;
    }

    Value thisv = f.regs.sp[-2];
    JSObject *thisObj = ValuePropertyBearer(cx, thisv, -2);
    if (!thisObj)
        THROW();

    jsid id;
    Value idval = f.regs.sp[-1];
    if (idval.isInt32() && INT_FITS_IN_JSID(idval.toInt32()))
        id = INT_TO_JSID(idval.toInt32());
    else if (!js_InternNonIntElementId(cx, thisObj, idval, &id))
        THROW();

    if (ic->shouldUpdate(cx)) {
#ifdef DEBUG
        f.regs.sp[-2] = MagicValue(JS_GENERIC_MAGIC);
#endif
        LookupStatus status = ic->update(f, cx, thisObj, idval, id, &f.regs.sp[-2]);
        if (status != Lookup_Uncacheable) {
            if (status == Lookup_Error)
                THROW();

            
            JS_ASSERT(!f.regs.sp[-2].isMagic());
            f.regs.sp[-1].setObject(*thisObj);
            return;
        }
    }

    
    if (!js_GetMethod(cx, thisObj, id, JSGET_NO_METHOD_BARRIER, &f.regs.sp[-2]))
        THROW();

#if JS_HAS_NO_SUCH_METHOD
    if (JS_UNLIKELY(f.regs.sp[-2].isPrimitive()) && thisv.isObject()) {
        f.regs.sp[-2] = f.regs.sp[-1];
        f.regs.sp[-1].setObject(*thisObj);
        if (!OnUnknownMethod(cx, f.regs.sp - 2))
            THROW();
    } else
#endif
    {
        f.regs.sp[-1] = thisv;
    }
}

void JS_FASTCALL
ic::GetElement(VMFrame &f, ic::GetElementIC *ic)
{
    JSContext *cx = f.cx;

    
    if (!f.regs.sp[-2].isObject()) {
        ic->disable(cx, "non-object");
        stubs::GetElem(f);
        return;
    }

    Value idval = f.regs.sp[-1];

    RecompilationMonitor monitor(cx);

    JSObject *obj = ValueToObject(cx, &f.regs.sp[-2]);
    if (!obj)
        THROW();

    jsid id;
    if (idval.isInt32() && INT_FITS_IN_JSID(idval.toInt32())) {
        id = INT_TO_JSID(idval.toInt32());
    } else {
        if (!js_InternNonIntElementId(cx, obj, idval, &id))
            THROW();
    }

    if (!monitor.recompiled() && ic->shouldUpdate(cx)) {
#ifdef DEBUG
        f.regs.sp[-2] = MagicValue(JS_GENERIC_MAGIC);
#endif
        LookupStatus status = ic->update(f, cx, obj, idval, id, &f.regs.sp[-2]);
        if (status != Lookup_Uncacheable) {
            if (status == Lookup_Error)
                THROW();

            
            JS_ASSERT(!f.regs.sp[-2].isMagic());
            return;
        }
    }

    if (!obj->getGeneric(cx, id, &f.regs.sp[-2]))
        THROW();
}

#define APPLY_STRICTNESS(f, s)                          \
    (FunctionTemplateConditional(s, f<true>, f<false>))

LookupStatus
SetElementIC::disable(JSContext *cx, const char *reason)
{
    slowCallPatched = true;
    VoidStub stub = APPLY_STRICTNESS(stubs::SetElem, strictMode);
    BaseIC::disable(cx, reason, JS_FUNC_TO_DATA_PTR(void *, stub));
    return Lookup_Uncacheable;
}

LookupStatus
SetElementIC::error(JSContext *cx)
{
    disable(cx, "error");
    return Lookup_Error;
}

void
SetElementIC::purge(Repatcher &repatcher)
{
    
    if (inlineShapeGuardPatched)
        repatcher.relink(fastPathStart.jumpAtOffset(inlineShapeGuard), slowPathStart);
    if (inlineHoleGuardPatched)
        repatcher.relink(fastPathStart.jumpAtOffset(inlineHoleGuard), slowPathStart);

    if (slowCallPatched) {
        void *stub = JS_FUNC_TO_DATA_PTR(void *, APPLY_STRICTNESS(ic::SetElement, strictMode));
        repatcher.relink(slowPathCall, FunctionPtr(stub));
    }

    reset();
}

LookupStatus
SetElementIC::attachHoleStub(JSContext *cx, JSObject *obj, int32 keyval)
{
    if (keyval < 0)
        return disable(cx, "negative key index");

    
    
    
    
    
    JS_ASSERT((jsuint)keyval >= obj->getDenseArrayInitializedLength() ||
              obj->getDenseArrayElement(keyval).isMagic(JS_ARRAY_HOLE));

    if (js_PrototypeHasIndexedProperties(cx, obj))
        return disable(cx, "prototype has indexed properties");

    Assembler masm;

    Vector<Jump, 4> fails(cx);

    
    
    
    
    
    for (JSObject *pobj = obj->getProto(); pobj; pobj = pobj->getProto()) {
        if (!pobj->isNative())
            return disable(cx, "non-native array prototype");
        masm.move(ImmPtr(pobj), objReg);
        Jump j = masm.guardShape(objReg, pobj);
        if (!fails.append(j))
            return error(cx);
    }

    
    masm.rematPayload(StateRemat::FromInt32(objRemat), objReg);

    
    masm.loadPtr(Address(objReg, JSObject::offsetOfElements()), objReg);

    Int32Key key = hasConstantKey ? Int32Key::FromConstant(keyValue) : Int32Key::FromRegister(keyReg);

    
    fails.append(masm.guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                       objReg, key, Assembler::NotEqual));

    
    fails.append(masm.guardArrayExtent(ObjectElements::offsetOfCapacity(),
                                       objReg, key, Assembler::BelowOrEqual));

    masm.bumpKey(key, 1);

    
    masm.storeKey(key, Address(objReg, ObjectElements::offsetOfInitializedLength()));
    Jump lengthGuard = masm.guardArrayExtent(ObjectElements::offsetOfLength(),
                                             objReg, key, Assembler::AboveOrEqual);
    masm.storeKey(key, Address(objReg, ObjectElements::offsetOfLength()));
    lengthGuard.linkTo(masm.label(), &masm);

    masm.bumpKey(key, -1);

    
    if (hasConstantKey) {
        Address slot(objReg, keyValue * sizeof(Value));
        masm.storeValue(vr, slot);
    } else {
        BaseIndex slot(objReg, keyReg, Assembler::JSVAL_SCALE);
        masm.storeValue(vr, slot);
    }

    Jump done = masm.jump();

    JS_ASSERT(!execPool);
    JS_ASSERT(!inlineHoleGuardPatched);

    LinkerHelper buffer(masm, JSC::METHOD_CODE);
    execPool = buffer.init(cx);
    if (!execPool)
        return error(cx);

    if (!buffer.verifyRange(cx->fp()->jit()))
        return disable(cx, "code memory is out of range");

    
    for (size_t i = 0; i < fails.length(); i++)
        buffer.link(fails[i], slowPathStart);
    buffer.link(done, fastPathRejoin);

    CodeLocationLabel cs = buffer.finalize();
    JaegerSpew(JSpew_PICs, "generated dense array hole stub at %p\n", cs.executableAddress());

    Repatcher repatcher(cx->fp()->jit());
    repatcher.relink(fastPathStart.jumpAtOffset(inlineHoleGuard), cs);
    inlineHoleGuardPatched = true;

    disable(cx, "generated dense array hole stub");

    return Lookup_Cacheable;
}

#if defined JS_METHODJIT_TYPED_ARRAY
LookupStatus
SetElementIC::attachTypedArray(JSContext *cx, JSObject *obj, int32 key)
{
    
    JS_ASSERT(!inlineShapeGuardPatched);

    Assembler masm;

    
    Jump shapeGuard = masm.guardShape(objReg, obj);

    
    Jump outOfBounds;
    Address typedArrayLength(objReg, TypedArray::lengthOffset());
    if (hasConstantKey)
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, typedArrayLength, Imm32(keyValue));
    else
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, typedArrayLength, keyReg);

    
    masm.rematPayload(StateRemat::FromInt32(objRemat), objReg);

    
    masm.loadPtr(Address(objReg, TypedArray::dataOffset()), objReg);

    JSObject *tarray = js::TypedArray::getTypedArray(obj);
    int shift = js::TypedArray::slotWidth(obj);
    if (hasConstantKey) {
        Address addr(objReg, keyValue * shift);
        if (!StoreToTypedArray(cx, masm, tarray, addr, vr, volatileMask))
            return error(cx);
    } else {
        Assembler::Scale scale = Assembler::TimesOne;
        switch (shift) {
          case 2:
            scale = Assembler::TimesTwo;
            break;
          case 4:
            scale = Assembler::TimesFour;
            break;
          case 8:
            scale = Assembler::TimesEight;
            break;
        }
        BaseIndex addr(objReg, keyReg, scale);
        if (!StoreToTypedArray(cx, masm, tarray, addr, vr, volatileMask))
            return error(cx);
    }

    Jump done = masm.jump();

    
    
    
    JS_ASSERT(!execPool);
    LinkerHelper buffer(masm, JSC::METHOD_CODE);
    execPool = buffer.init(cx);
    if (!execPool)
        return error(cx);

    if (!buffer.verifyRange(cx->fp()->jit()))
        return disable(cx, "code memory is out of range");

    
    buffer.link(shapeGuard, slowPathStart);
    buffer.link(outOfBounds, fastPathRejoin);
    buffer.link(done, fastPathRejoin);
    masm.finalize(buffer);

    CodeLocationLabel cs = buffer.finalizeCodeAddendum();
    JaegerSpew(JSpew_PICs, "generated setelem typed array stub at %p\n", cs.executableAddress());

    Repatcher repatcher(cx->fp()->jit());
    repatcher.relink(fastPathStart.jumpAtOffset(inlineShapeGuard), cs);
    inlineShapeGuardPatched = true;

    stubsGenerated++;

    
    
    if (stubsGenerated == MAX_GETELEM_IC_STUBS)
        disable(cx, "max stubs reached");

    disable(cx, "generated typed array stub");

    return Lookup_Cacheable;
}
#endif 

LookupStatus
SetElementIC::update(JSContext *cx, const Value &objval, const Value &idval)
{
    if (!objval.isObject())
        return disable(cx, "primitive lval");
    if (!idval.isInt32())
        return disable(cx, "non-int32 key");

    JSObject *obj = &objval.toObject();
    int32 key = idval.toInt32();

    if (obj->isDenseArray())
        return attachHoleStub(cx, obj, key);

#if defined JS_METHODJIT_TYPED_ARRAY
    
    if (!cx->typeInferenceEnabled() && js_IsTypedArray(obj))
        return attachTypedArray(cx, obj, key);
#endif

    return disable(cx, "unsupported object type");
}

template<JSBool strict>
void JS_FASTCALL
ic::SetElement(VMFrame &f, ic::SetElementIC *ic)
{
    JSContext *cx = f.cx;

    if (ic->shouldUpdate(cx)) {
        LookupStatus status = ic->update(cx, f.regs.sp[-3], f.regs.sp[-2]);
        if (status == Lookup_Error)
            THROW();
    }

    stubs::SetElem<strict>(f);
}

template void JS_FASTCALL ic::SetElement<true>(VMFrame &f, SetElementIC *ic);
template void JS_FASTCALL ic::SetElement<false>(VMFrame &f, SetElementIC *ic);

void
JITScript::purgePICs()
{
    if (!nPICs && !nGetElems && !nSetElems)
        return;

    Repatcher repatcher(this);

    ic::PICInfo *pics_ = pics();
    for (uint32 i = 0; i < nPICs; i++) {
        ic::PICInfo &pic = pics_[i];
        switch (pic.kind) {
          case ic::PICInfo::SET:
          case ic::PICInfo::SETMETHOD:
            SetPropCompiler::reset(repatcher, pic);
            break;
          case ic::PICInfo::NAME:
          case ic::PICInfo::XNAME:
          case ic::PICInfo::CALLNAME:
            ScopeNameCompiler::reset(repatcher, pic);
            break;
          case ic::PICInfo::BIND:
            BindNameCompiler::reset(repatcher, pic);
            break;
          case ic::PICInfo::CALL: 
          case ic::PICInfo::GET:
            GetPropCompiler::reset(repatcher, pic);
            break;
          default:
            JS_NOT_REACHED("Unhandled PIC kind");
            break;
        }
        pic.reset();
    }

    ic::GetElementIC *getElems_ = getElems();
    ic::SetElementIC *setElems_ = setElems();
    for (uint32 i = 0; i < nGetElems; i++)
        getElems_[i].purge(repatcher);
    for (uint32 i = 0; i < nSetElems; i++)
        setElems_[i].purge(repatcher);
}

void
ic::PurgePICs(JSContext *cx, JSScript *script)
{
    if (script->jitNormal)
        script->jitNormal->purgePICs();
    if (script->jitCtor)
        script->jitCtor->purgePICs();
}

#endif 

