





































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

#if defined JS_POLYIC

using namespace js;
using namespace js::mjit;
using namespace js::mjit::ic;

typedef JSC::FunctionPtr FunctionPtr;
typedef JSC::MacroAssembler::RegisterID RegisterID;
typedef JSC::MacroAssembler::Jump Jump;
typedef JSC::MacroAssembler::Imm32 Imm32;


static const uint32 INLINE_PATH_LENGTH = 64;


#ifndef JS_HAS_IC_LABELS
ICOffsetInitializer::ICOffsetInitializer()
{
    {
        GetPropLabels &labels = PICInfo::getPropLabels_;
#if defined JS_CPU_X86
        labels.dslotsLoadOffset = -15;
        labels.inlineShapeOffset = 6;
        labels.stubShapeJumpOffset = 12;
        labels.inlineValueLoadOffset = -12;
#endif
    }
    {
        SetPropLabels &labels = PICInfo::setPropLabels_;
#if defined JS_CPU_X86
        labels.inlineShapeDataOffset = 6;
        
        labels.inlineShapeJumpOffset = 12;
        labels.stubShapeJumpOffset = 12;
#endif
    }
    {
        BindNameLabels &labels = PICInfo::bindNameLabels_;
#if defined JS_CPU_X86
        labels.inlineJumpOffset = 10;
        labels.stubJumpOffset = 5;
#endif
    }
    {
        ScopeNameLabels &labels = PICInfo::scopeNameLabels_;
#if defined JS_CPU_X86
        labels.inlineJumpOffset = 5;
        labels.stubJumpOffset = 5;
#endif
    }
}

ICOffsetInitializer s_ICOffsetInitializer;
GetPropLabels PICInfo::getPropLabels_;
SetPropLabels PICInfo::setPropLabels_;
BindNameLabels PICInfo::bindNameLabels_;
ScopeNameLabels PICInfo::scopeNameLabels_;
#endif




class PICLinker : public LinkerHelper
{
    ic::BasePolyIC &ic;

  public:
    PICLinker(Assembler &masm, ic::BasePolyIC &ic)
      : LinkerHelper(masm), ic(ic)
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

  public:
    PICStubCompiler(const char *type, VMFrame &f, JSScript *script, ic::PICInfo &pic, void *stub)
      : BaseCompiler(f.cx), type(type), f(f), script(script), pic(pic), stub(stub)
    { }

    bool isCallOp() const {
        if (pic.kind == ic::PICInfo::CALL)
            return true;
        return !!(js_CodeSpec[pic.op].format & JOF_CALLOP);
    }

    LookupStatus error() {
        disable("error");
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

  protected:
    void spew(const char *event, const char *op) {
#ifdef JS_METHODJIT_SPEW
        JaegerSpew(JSpew_PICs, "%s %s: %s (%s: %d)\n",
                   type, event, op, script->filename,
                   js_FramePCToLineNumber(cx, f.fp()));
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
                          int32(INVALID_SHAPE));
        repatcher.relink(labels.getInlineShapeJump(pic.fastPathStart.labelAtOffset(pic.shapeGuard)),
                         pic.slowPathStart);

        FunctionPtr target(JS_FUNC_TO_DATA_PTR(void *, ic::SetProp));
        repatcher.relink(pic.slowPathCall, target);
    }

    LookupStatus patchInline(const Shape *shape, bool inlineSlot)
    {
        JS_ASSERT(!pic.inlinePathPatched);
        JaegerSpew(JSpew_PICs, "patch setprop inline at %p\n", pic.fastPathStart.executableAddress());

        Repatcher repatcher(f.jit());
        SetPropLabels &labels = pic.setPropLabels();

        int32 offset;
        if (inlineSlot) {
            CodeLocationInstruction istr = labels.getDslotsLoad(pic.fastPathRejoin, pic.u.vr);
            repatcher.repatchLoadPtrToLEA(istr);

            
            
            
            
            
            
            
            int32 diff = int32(JSObject::getFixedSlotOffset(0)) -
                         int32(offsetof(JSObject, slots));
            JS_ASSERT(diff != 0);
            offset  = (int32(shape->slot) * sizeof(Value)) + diff;
        } else {
            offset = shape->slot * sizeof(Value);
        }

        repatcher.repatch(labels.getInlineShapeData(pic.fastPathStart, pic.shapeGuard),
                          obj->shape());
        repatcher.patchAddressOffsetForValueStore(labels.getInlineValueStore(pic.fastPathRejoin,
                                                                             pic.u.vr),
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

    LookupStatus generateStub(uint32 initialShape, const Shape *shape, bool adding, bool inlineSlot)
    {
        
        Vector<Jump, 8> slowExits(cx);
        Vector<Jump, 8> otherGuards(cx);

        Assembler masm;

        
        if (pic.shapeNeedsRemat()) {
            masm.loadShape(pic.objReg, pic.shapeReg);
            pic.shapeRegHasBaseShape = true;
        }

        Label start = masm.label();
        Jump shapeGuard = masm.branch32FixedLength(Assembler::NotEqual, pic.shapeReg,
                                                   Imm32(initialShape));

        Label stubShapeJumpLabel = masm.label();

        pic.setPropLabels().setStubShapeJump(masm, start, stubShapeJumpLabel);

        JS_ASSERT_IF(!shape->hasDefaultSetter(), obj->getClass() == &js_CallClass);

        MaybeJump skipOver;

        if (adding) {
            JS_ASSERT(shape->hasSlot());
            pic.shapeRegHasBaseShape = false;

            
            JSObject *proto = obj->getProto();
            RegisterID lastReg = pic.objReg;
            while (proto) {
                masm.loadPtr(Address(lastReg, offsetof(JSObject, proto)), pic.shapeReg);
                Jump protoGuard = masm.guardShape(pic.shapeReg, proto);
                if (!otherGuards.append(protoGuard))
                    return error();

                proto = proto->getProto();
                lastReg = pic.shapeReg;
            }

            if (pic.kind == ic::PICInfo::SETMETHOD) {
                



                JS_ASSERT(shape->isMethod());
                JSObject *funobj = &shape->methodObject();
                if (pic.u.vr.isConstant()) {
                    JS_ASSERT(funobj == &pic.u.vr.value().toObject());
                } else {
                    Jump mismatchedFunction =
                        masm.branchPtr(Assembler::NotEqual, pic.u.vr.dataReg(), ImmPtr(funobj));
                    if (!slowExits.append(mismatchedFunction))
                        return error();
                }
            }

            if (inlineSlot) {
                Address address(pic.objReg,
                                JSObject::getFixedSlotOffset(shape->slot));
                masm.storeValue(pic.u.vr, address);
            } else {
                
                Address capacity(pic.objReg, offsetof(JSObject, capacity));
                masm.load32(capacity, pic.shapeReg);
                Jump overCapacity = masm.branch32(Assembler::LessThanOrEqual, pic.shapeReg,
                                                  Imm32(shape->slot));
                if (!slowExits.append(overCapacity))
                    return error();

                masm.loadPtr(Address(pic.objReg, offsetof(JSObject, slots)), pic.shapeReg);
                Address address(pic.shapeReg, shape->slot * sizeof(Value));
                masm.storeValue(pic.u.vr, address);
            }

            uint32 newShape = obj->shape();
            JS_ASSERT(newShape != initialShape);

            
            masm.storePtr(ImmPtr(shape), Address(pic.objReg, offsetof(JSObject, lastProp)));
            masm.store32(Imm32(newShape), Address(pic.objReg, offsetof(JSObject, objShape)));

            
            if (shape->isMethod()) {
                Address flags(pic.objReg, offsetof(JSObject, flags));

                
                masm.load32(flags, pic.shapeReg);
                masm.or32(Imm32(JSObject::METHOD_BARRIER), pic.shapeReg);
                masm.store32(pic.shapeReg, flags);
            }
        } else if (shape->hasDefaultSetter()) {
            Address address(pic.objReg, JSObject::getFixedSlotOffset(shape->slot));
            if (!inlineSlot) {
                masm.loadPtr(Address(pic.objReg, offsetof(JSObject, slots)), pic.objReg);
                address = Address(pic.objReg, shape->slot * sizeof(Value));
            }

            
            
            if (obj->brandedOrHasMethodBarrier()) {
                masm.loadTypeTag(address, pic.shapeReg);
                Jump skip = masm.testObject(Assembler::NotEqual, pic.shapeReg);
                masm.loadPayload(address, pic.shapeReg);
                Jump rebrand = masm.testFunction(Assembler::Equal, pic.shapeReg);
                if (!slowExits.append(rebrand))
                    return error();
                skip.linkTo(masm.label(), &masm);
                pic.shapeRegHasBaseShape = false;
            }

            masm.storeValue(pic.u.vr, address);
        } else {
            
            
            
            
            
            
            
            JSFunction *fun = obj->getCallObjCalleeFunction();
            uint16 slot = uint16(shape->shortid);

            
            masm.loadObjPrivate(pic.objReg, pic.shapeReg);
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
                masm.loadPtr(Address(pic.objReg, offsetof(JSObject, slots)), pic.objReg);

                Address dslot(pic.objReg, (slot + JSObject::CALL_RESERVED_SLOTS) * sizeof(Value));
                masm.storeValue(pic.u.vr, dslot);
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
        JaegerSpew(JSpew_PICs, "generate setprop stub %p %d %d at %p\n",
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

    LookupStatus update()
    {
        JS_ASSERT(pic.hit);

        if (obj->isDenseArray())
            return disable("dense array");
        if (!obj->isNative())
            return disable("non-native");

        Class *clasp = obj->getClass();

        if (clasp->setProperty != StrictPropertyStub)
            return disable("set property hook");
        if (clasp->ops.lookupProperty)
            return disable("ops lookup property hook");
        if (clasp->ops.setProperty)
            return disable("ops set property hook");

        jsid id = ATOM_TO_JSID(atom);

        JSObject *holder;
        JSProperty *prop = NULL;
        if (!obj->lookupProperty(cx, id, &holder, &prop))
            return error();

        
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

            if (clasp->addProperty != PropertyStub)
                return disable("add property hook");
            if (clasp->ops.defineProperty)
                return disable("ops define property hook");

            uint32 index;
            if (js_IdIsIndex(id, &index))
                return disable("index");

            uint32 initialShape = obj->shape();

            if (!obj->ensureClassReservedSlots(cx))
                return error();

            uint32 slots = obj->numSlots();
            uintN flags = 0;
            PropertyOp getter = clasp->getProperty;

            if (pic.kind == ic::PICInfo::SETMETHOD) {
                if (!obj->canHaveMethodBarrier())
                    return disable("can't have method barrier");

                JSObject *funobj = &f.regs.sp[-1].toObject();
                if (funobj != GET_FUNCTION_PRIVATE(cx, funobj))
                    return disable("mismatched function");

                flags |= Shape::METHOD;
                getter = CastAsPropertyOp(funobj);
            }

            




            id = js_CheckForStringIndex(id);
            const Shape *shape =
                obj->putProperty(cx, id, getter, clasp->setProperty,
                                 SHAPE_INVALID_SLOT, JSPROP_ENUMERATE, flags, 0);
            if (!shape)
                return error();
            if (flags & Shape::METHOD)
                obj->nativeSetSlot(shape->slot, f.regs.sp[-1]);

            




            if (obj->inDictionaryMode())
                return disable("dictionary");

            if (!shape->hasDefaultSetter())
                return disable("adding non-default setter");
            if (!shape->hasSlot())
                return disable("adding invalid slot");

            











            if (obj->numSlots() != slots)
                return disable("insufficient slot capacity");

            return generateStub(initialShape, shape, true, !obj->hasSlotsArray());
        }

        const Shape *shape = (const Shape *) prop;
        if (pic.kind == ic::PICInfo::SETMETHOD && !shape->isMethod())
            return disable("set method on non-method shape");
        if (!shape->writable())
            return disable("readonly");

        if (shape->hasDefaultSetter()) {
            if (!shape->hasSlot())
                return disable("invalid slot");
        } else {
            if (shape->hasSetterValue())
                return disable("scripted setter");
            if (shape->setterOp() != SetCallArg &&
                shape->setterOp() != SetCallVar) {
                return disable("setter");
            }
        }

        JS_ASSERT(obj == holder);
        if (!pic.inlinePathPatched &&
            !obj->brandedOrHasMethodBarrier() &&
            shape->hasDefaultSetter() &&
            !obj->isDenseArray()) {
            return patchInline(shape, !obj->hasSlotsArray());
        } 

        return generateStub(obj->shape(), shape, false, !obj->hasSlotsArray());
    }
};

static bool
IsCacheableProtoChain(JSObject *obj, JSObject *holder)
{
    while (obj != holder) {
        JSObject *proto = obj->getProto();
        if (!proto->isNative())
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

    
    
    JSObject    *aobj;
    JSObject    *holder;
    JSProperty  *prop;
 
    
    
    const Shape *shape;

    GetPropertyHelper(JSContext *cx, JSObject *obj, JSAtom *atom, IC &ic)
      : cx(cx), obj(obj), atom(atom), ic(ic), holder(NULL), prop(NULL), shape(NULL)
    { }

  public:
    LookupStatus bind() {
        if (!js_FindProperty(cx, ATOM_TO_JSID(atom), &obj, &holder, &prop))
            return ic.error(cx);
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
        if (!aobj->lookupProperty(cx, ATOM_TO_JSID(atom), &holder, &prop))
            return ic.error(cx);
        if (!prop)
            return ic.disable(cx, "lookup failed");
        if (!IsCacheableProtoChain(obj, holder))
            return ic.disable(cx, "non-native holder");
        shape = (const Shape *)prop;
        return Lookup_Cacheable;
    }

    LookupStatus testForGet() {
        if (!shape->hasDefaultGetter()) {
            if (!shape->isMethod())
                return ic.disable(cx, "getter");
            if (!ic.isCallOp())
                return ic.disable(cx, "method valued shape");
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
        repatcher.repatch(labels.getInlineShapeData(pic.getFastShapeGuard()),
                          int32(INVALID_SHAPE));
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

        Jump notArgs = masm.testObjClass(Assembler::NotEqual, pic.objReg, obj->getClass());

        masm.loadPtr(Address(pic.objReg, offsetof(JSObject, slots)), pic.objReg);
        masm.load32(Address(pic.objReg, ArgumentsObject::INITIAL_LENGTH_SLOT * sizeof(Value)),
                    pic.objReg);
        masm.move(pic.objReg, pic.shapeReg);
        Jump overridden = masm.branchTest32(Assembler::NonZero, pic.shapeReg,
                                            Imm32(ArgumentsObject::LENGTH_OVERRIDDEN_BIT));
        masm.rshift32(Imm32(ArgumentsObject::PACKED_BITS_COUNT), pic.objReg);
        
        masm.move(ImmType(JSVAL_TYPE_INT32), pic.shapeReg);
        Jump done = masm.jump();

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
        Jump isDense = masm.testClass(Assembler::Equal, pic.shapeReg, &js_ArrayClass);
        Jump notArray = masm.testClass(Assembler::NotEqual, pic.shapeReg, &js_SlowArrayClass);

        isDense.linkTo(masm.label(), &masm);
        masm.load32(Address(pic.objReg, offsetof(JSObject, privateData)), pic.objReg);
        Jump oob = masm.branch32(Assembler::Above, pic.objReg, Imm32(JSVAL_INT_MAX));
        masm.move(ImmType(JSVAL_TYPE_INT32), pic.shapeReg);
        Jump done = masm.jump();

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

        Jump notStringObj = masm.testObjClass(Assembler::NotEqual, pic.objReg, obj->getClass());
        masm.loadPtr(Address(pic.objReg, offsetof(JSObject, slots)), pic.objReg);
        masm.loadPayload(Address(pic.objReg, JSObject::JSSLOT_PRIMITIVE_THIS * sizeof(Value)),
                         pic.objReg);
        masm.loadPtr(Address(pic.objReg, JSString::offsetOfLengthAndFlags()), pic.objReg);
        masm.urshift32(Imm32(JSString::LENGTH_SHIFT), pic.objReg);
        masm.move(ImmType(JSVAL_TYPE_INT32), pic.shapeReg);
        Jump done = masm.jump();

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

        if (!f.fp()->script()->compileAndGo)
            return disable("String.prototype without compile-and-go");

        GetPropertyHelper<GetPropCompiler> getprop(cx, obj, atom, *this);
        LookupStatus status = getprop.lookupAndTest();
        if (status != Lookup_Cacheable)
            return status;
        if (getprop.obj != getprop.holder)
            return disable("proto walk on String.prototype");

        Assembler masm;

        
        Jump notString = masm.branchPtr(Assembler::NotEqual, pic.typeReg(),
                                        ImmType(JSVAL_TYPE_STRING));

        






        uint32 thisvOffset = uint32(f.regs.sp - f.fp()->slots()) - 1;
        Address thisv(JSFrameReg, sizeof(StackFrame) + thisvOffset * sizeof(Value));
        masm.storeValueFromComponents(ImmType(JSVAL_TYPE_STRING),
                                      pic.objReg, thisv);

        





        masm.move(ImmPtr(obj), pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump shapeMismatch = masm.branch32(Assembler::NotEqual, pic.shapeReg,
                                           Imm32(obj->shape()));
        masm.loadObjProp(obj, pic.objReg, getprop.shape, pic.shapeReg, pic.objReg);

        Jump done = masm.jump();

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
        if (!holder->hasSlotsArray()) {
            CodeLocationInstruction istr = labels.getDslotsLoad(pic.fastPathRejoin);
            repatcher.repatchLoadPtrToLEA(istr);

            
            
            
            
            
            
            
            int32 diff = int32(JSObject::getFixedSlotOffset(0)) -
                         int32(offsetof(JSObject, slots));
            JS_ASSERT(diff != 0);
            offset  = (int32(shape->slot) * sizeof(Value)) + diff;
        } else {
            offset = shape->slot * sizeof(Value);
        }

        repatcher.repatch(labels.getInlineShapeData(pic.getFastShapeGuard()), obj->shape());
        repatcher.patchAddressOffsetForValueLoad(labels.getValueLoad(pic.fastPathRejoin), offset);

        pic.inlinePathPatched = true;

        return Lookup_Cacheable;
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
            shapeGuardJump = masm.testObjClass(Assembler::NotEqual, pic.objReg, obj->getClass());

            



#ifndef JS_HAS_IC_LABELS
            setStubShapeOffset = false;
#endif
        } else {
            if (pic.shapeNeedsRemat()) {
                masm.loadShape(pic.objReg, pic.shapeReg);
                pic.shapeRegHasBaseShape = true;
            }

            start = masm.label();
            shapeGuardJump = masm.branch32FixedLength(Assembler::NotEqual, pic.shapeReg,
                                                      Imm32(obj->shape()));
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

        
        masm.loadObjProp(holder, holderReg, shape, pic.shapeReg, pic.objReg);
        Jump done = masm.jump();

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.jit())) ||
            !buffer.verifyRange(f.jit())) {
            return disable("code memory is out of range");
        }

        
        for (Jump *pj = shapeMismatches.begin(); pj != shapeMismatches.end(); ++pj)
            buffer.link(*pj, pic.slowPathStart);

        
        buffer.link(done, pic.fastPathRejoin);
        CodeLocationLabel cs = buffer.finalize();
        JaegerSpew(JSpew_PICs, "generated %s stub at %p\n", type, cs.executableAddress());

        patchPreviousToHere(cs);

        pic.stubsGenerated++;
        pic.updateLastPath(buffer, start);

        if (setStubShapeOffset)
            pic.getPropLabels().setStubShapeJump(masm, start, stubShapeJumpLabel);

        if (pic.stubsGenerated == MAX_PIC_STUBS)
            disable("max stubs reached");
        if (obj->isDenseArray())
            disable("dense array");

        return Lookup_Cacheable;
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
        repatcher.relink(label.jumpAtOffset(shapeGuardJumpOffset), cs);
        if (int secondGuardOffset = getLastStubSecondShapeGuard())
            repatcher.relink(label.jumpAtOffset(secondGuardOffset), cs);
    }

    LookupStatus update()
    {
        JS_ASSERT(pic.hit);

        GetPropertyHelper<GetPropCompiler> getprop(cx, obj, atom, *this);
        LookupStatus status = getprop.lookupAndTest();
        if (status != Lookup_Cacheable)
            return status;

        if (obj == getprop.holder && !pic.inlinePathPatched)
            return patchInline(getprop.holder, getprop.shape);
        
        return generateStub(getprop.holder, getprop.shape);
    }
};

class ScopeNameCompiler : public PICStubCompiler
{
  private:
    typedef Vector<Jump, 8, ContextAllocPolicy> JumpList;

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
            Jump j = masm.branch32(Assembler::NotEqual, pic.shapeReg, Imm32(tobj->shape()));
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
        getprop(f.cx, NULL, atom, *thisFromCtor())
    { }

    static void reset(Repatcher &repatcher, ic::PICInfo &pic)
    {
        ScopeNameLabels &labels = pic.scopeNameLabels();

        
        JSC::CodeLocationJump inlineJump = labels.getInlineJump(pic.fastPathStart);
        repatcher.relink(inlineJump, pic.slowPathStart);

        VoidStubPIC stub = (pic.kind == ic::PICInfo::NAME) ? ic::Name : ic::XName;
        FunctionPtr target(JS_FUNC_TO_DATA_PTR(void *, stub));
        repatcher.relink(pic.slowPathCall, target);
    }

    LookupStatus generateGlobalStub(JSObject *obj)
    {
        Assembler masm;
        JumpList fails(cx);
        ScopeNameLabels &labels = pic.scopeNameLabels();

        
        if (pic.kind == ic::PICInfo::NAME)
            masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), pic.objReg);

        JS_ASSERT(obj == getprop.holder);
        JS_ASSERT(getprop.holder == scopeChain->getGlobal());

        LookupStatus status = walkScopeChain(masm, fails);
        if (status != Lookup_Cacheable)
            return status;

        
        MaybeJump finalNull;
        if (pic.kind == ic::PICInfo::NAME)
            finalNull = masm.branchTestPtr(Assembler::Zero, pic.objReg, pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump finalShape = masm.branch32(Assembler::NotEqual, pic.shapeReg, Imm32(getprop.holder->shape()));

        masm.loadObjProp(obj, pic.objReg, getprop.shape, pic.shapeReg, pic.objReg);
        Jump done = masm.jump();

        
        for (Jump *pj = fails.begin(); pj != fails.end(); ++pj)
            pj->linkTo(masm.label(), &masm);
        if (finalNull.isSet())
            finalNull.get().linkTo(masm.label(), &masm);
        finalShape.linkTo(masm.label(), &masm);
        Label failLabel = masm.label();
        Jump failJump = masm.jump();

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
        Vector<Jump, 8, ContextAllocPolicy> fails(cx);
        ScopeNameLabels &labels = pic.scopeNameLabels();

        
        if (pic.kind == ic::PICInfo::NAME)
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
        if (pic.kind == ic::PICInfo::NAME)
            finalNull = masm.branchTestPtr(Assembler::Zero, pic.objReg, pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump finalShape = masm.branch32(Assembler::NotEqual, pic.shapeReg, Imm32(getprop.holder->shape()));

        
        masm.loadObjPrivate(pic.objReg, pic.shapeReg);

        JSFunction *fun = getprop.holder->getCallObjCalleeFunction();
        uint16 slot = uint16(shape->shortid);

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
            masm.loadPtr(Address(pic.objReg, offsetof(JSObject, slots)), pic.objReg);

            if (kind == VAR)
                slot += fun->nargs;
            Address dslot(pic.objReg, (slot + JSObject::CALL_RESERVED_SLOTS) * sizeof(Value));

            
            masm.loadValueAsComponents(dslot, pic.shapeReg, pic.objReg);
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

        if (obj->getClass() == &js_CallClass)
            return generateCallStub(obj);

        LookupStatus status = getprop.testForGet();
        if (status != Lookup_Cacheable)
            return status;

        if (!obj->getParent())
            return generateGlobalStub(obj);

        return disable("scope object not handled yet");
    }

    bool retrieve(Value *vp)
    {
        JSObject *obj = getprop.obj;
        JSObject *holder = getprop.holder;
        const JSProperty *prop = getprop.prop;

        if (!prop) {
            
            disable("property not found");
            if (pic.kind == ic::PICInfo::NAME) {
                JSOp op2 = js_GetOpcode(cx, script, cx->regs().pc + JSOP_NAME_LENGTH);
                if (op2 == JSOP_TYPEOF) {
                    vp->setUndefined();
                    return true;
                }
            }
            ReportAtomNotDefined(cx, atom);
            return false;
        }

        
        
        if (!getprop.shape)
            return obj->getProperty(cx, ATOM_TO_JSID(atom), vp);

        const Shape *shape = getprop.shape;
        JSObject *normalized = obj;
        if (obj->getClass() == &js_WithClass && !shape->hasDefaultGetter())
            normalized = js_UnwrapWithObject(cx, obj);
        NATIVE_GET(cx, normalized, holder, shape, JSGET_METHOD_BARRIER, vp, return false);

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
        js::Vector<Jump, 8, ContextAllocPolicy> fails(cx);

        BindNameLabels &labels = pic.bindNameLabels();

        
        masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump firstShape = masm.branch32(Assembler::NotEqual, pic.shapeReg,
                                        Imm32(scopeChain->shape()));

        
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
            Jump shapeTest = masm.branch32(Assembler::NotEqual, pic.shapeReg,
                                           Imm32(tobj->shape()));
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

        JSObject *obj = js_FindIdentifierBase(cx, scopeChain, ATOM_TO_JSID(atom));
        if (!obj)
            return obj;

        if (!pic.hit) {
            spew("first hit", "nop");
            pic.hit = true;
            return obj;
        }

        LookupStatus status = generateStub(obj);
        if (status == Lookup_Error)
            return NULL;

        return obj;
    }
};

static void JS_FASTCALL
DisabledLengthIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::Length(f);
}

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
            GetPropCompiler cc(f, script, NULL, *pic, NULL, DisabledLengthIC);
            LookupStatus status = cc.generateStringLengthStub();
            if (status == Lookup_Error)
                THROW();
            JSString *str = f.regs.sp[-1].toString();
            f.regs.sp[-1].setInt32(str->length());
            return;
        } else if (!f.regs.sp[-1].isPrimitive()) {
            JSObject *obj = &f.regs.sp[-1].toObject();
            if (obj->isArray() ||
                (obj->isArguments() && !obj->asArguments()->hasOverriddenLength()) ||
                obj->isString()) {
                GetPropCompiler cc(f, script, obj, *pic, NULL, DisabledLengthIC);
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

    JSObject *obj = ValueToObject(f.cx, &f.regs.sp[-1]);
    if (!obj)
        THROW();

    if (pic->shouldUpdate(f.cx)) {
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
    if (!obj->getProperty(f.cx, ATOM_TO_JSID(atom), &v))
        THROW();
    f.regs.sp[-1] = v;
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
    JSObject *obj = ValueToObject(f.cx, &f.regs.sp[-2]);
    if (!obj)
        THROW();

    JSScript *script = f.fp()->script();
    JS_ASSERT(pic->isSet());

    VoidStubPIC stub = pic->usePropCache
                       ? STRICT_VARIANT(DisabledSetPropIC)
                       : STRICT_VARIANT(DisabledSetPropICNoCache);

    
    
    
    
    
    
    
    if (pic->shouldUpdate(f.cx)) {

        SetPropCompiler cc(f, script, obj, *pic, pic->atom, stub);
        LookupStatus status = cc.update();
        if (status == Lookup_Error)
            THROW();
    }
    
    stub(f, pic);
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
    JS_PROPERTY_CACHE(cx).test(cx, regs.pc, aobj, obj2, entry, atom);
    if (!atom) {
        if (entry->vword.isFunObj()) {
            rval.setObject(entry->vword.toFunObj());
        } else if (entry->vword.isSlot()) {
            uint32 slot = entry->vword.toSlot();
            rval = obj2->nativeGetSlot(slot);
        } else {
            JS_ASSERT(entry->vword.isShape());
            const Shape *shape = entry->vword.toShape();
            NATIVE_GET(cx, &objv.toObject(), obj2, shape, JSGET_NO_METHOD_BARRIER, &rval,
                       THROW());
        }
        regs.sp++;
        regs.sp[-2] = rval;
        regs.sp[-1] = lval;
    } else {
        



        jsid id;
        id = ATOM_TO_JSID(pic->atom);

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

#if JS_HAS_NO_SUCH_METHOD
    if (JS_UNLIKELY(rval.isPrimitive()) && regs.sp[-1].isObject()) {
        regs.sp[-2].setString(pic->atom);
        if (!js_OnUnknownMethod(cx, regs.sp - 2))
            THROW();
    }
#endif
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
    if (!cc.retrieve(&rval))
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
    if (!cc.retrieve(&rval))
        THROW();
    f.regs.sp[0] = rval;
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
    if (!obj) {
        cc.disable("error");
        THROW();
    }

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
               js_CodeName[op], event, message, cx->fp()->script()->filename,
               js_FramePCToLineNumber(cx, cx->fp()));
#endif
}

LookupStatus
BaseIC::disable(JSContext *cx, const char *reason, void *stub)
{
    spew(cx, "disabled", reason);
    Repatcher repatcher(cx->fp()->jit());
    repatcher.relink(slowPathCall, FunctionPtr(stub));
    return Lookup_Uncacheable;
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
    if (inlineClaspGuardPatched)
        repatcher.relink(fastPathStart.jumpAtOffset(inlineClaspGuard), slowPathStart);

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
GetElementIC::attachGetProp(JSContext *cx, JSObject *obj, const Value &v, jsid id, Value *vp)
{
    JS_ASSERT(v.isString());

    GetPropertyHelper<GetElementIC> getprop(cx, obj, JSID_TO_ATOM(id), *this);
    LookupStatus status = getprop.lookupAndTest();
    if (status != Lookup_Cacheable)
        return status;

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

    
    Jump shapeGuard;
    if (obj->isDenseArray()) {
        shapeGuard = masm.testObjClass(Assembler::NotEqual, objReg, obj->getClass());
    } else {
        shapeGuard = masm.branch32(Assembler::NotEqual, typeReg, Imm32(obj->shape()));
    }

    
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
    char *chars = js_DeflateString(cx, v.toString()->getChars(cx), v.toString()->length());
    JaegerSpew(JSpew_PICs, "generated %s stub at %p for atom 0x%x (\"%s\") shape 0x%x (%s: %d)\n",
               js_CodeName[op], cs.executableAddress(), id, chars, holder->shape(),
               cx->fp()->script()->filename, js_FramePCToLineNumber(cx, cx->fp()));
    cx->free_(chars);
#endif

    
    if (shouldPatchInlineTypeGuard() || shouldPatchUnconditionalClaspGuard()) {
        Repatcher repatcher(cx->fp()->jit());

        if (shouldPatchInlineTypeGuard()) {
            
            
            JS_ASSERT(!inlineTypeGuardPatched);
            JS_ASSERT(atomTypeGuard.isSet());

            repatcher.relink(fastPathStart.jumpAtOffset(inlineTypeGuard), cs);
            inlineTypeGuardPatched = true;
        }

        if (shouldPatchUnconditionalClaspGuard()) {
            
            
            
            
            JS_ASSERT(!hasInlineTypeGuard());

            repatcher.relink(fastPathStart.jumpAtOffset(inlineClaspGuard), cs);
            inlineClaspGuardPatched = true;
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

    
    if (shape->isMethod())
        *vp = ObjectValue(shape->methodObject());
    else
        *vp = holder->getSlot(shape->slot);

    return Lookup_Cacheable;
}

#if defined JS_POLYIC_TYPED_ARRAY
LookupStatus
GetElementIC::attachTypedArray(JSContext *cx, JSObject *obj, const Value &v, jsid id, Value *vp)
{
    if (!v.isInt32())
        return disable(cx, "typed array with string key");

    if (op == JSOP_CALLELEM)
        return disable(cx, "typed array with call");

    
    
    JS_ASSERT(hasInlineTypeGuard() || idRemat.knownType() == JSVAL_TYPE_INT32);

    Assembler masm;

    
    Jump claspGuard = masm.testObjClass(Assembler::NotEqual, objReg, obj->getClass());

    
    masm.loadPtr(Address(objReg, offsetof(JSObject, privateData)), objReg);

    
    Jump outOfBounds;
    Address typedArrayLength(objReg, js::TypedArray::lengthOffset());
    if (idRemat.isConstant()) {
        JS_ASSERT(idRemat.value().toInt32() == v.toInt32());
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, typedArrayLength, Imm32(v.toInt32()));
    } else {
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, typedArrayLength, idRemat.dataReg());
    }

    
    masm.loadPtr(Address(objReg, js::TypedArray::dataOffset()), objReg);

    js::TypedArray *tarray = js::TypedArray::fromJSObject(obj);
    int shift = tarray->slotWidth();
    if (idRemat.isConstant()) {
        int32 index = v.toInt32();
        Address addr(objReg, index * shift);
        LoadFromTypedArray(masm, tarray, addr, typeReg, objReg);
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
        BaseIndex addr(objReg, idRemat.dataReg(), scale);
        LoadFromTypedArray(masm, tarray, addr, typeReg, objReg);
    }

    Jump done = masm.jump();

    PICLinker buffer(masm, *this);
    if (!buffer.init(cx))
        return error(cx);

    if (!buffer.verifyRange(cx->fp()->jit()))
        return disable(cx, "code memory is out of range");

    buffer.link(claspGuard, slowPathStart);
    buffer.link(outOfBounds, slowPathStart);
    buffer.link(done, fastPathRejoin);

    CodeLocationLabel cs = buffer.finalizeCodeAddendum();
    JaegerSpew(JSpew_PICs, "generated getelem typed array stub at %p\n", cs.executableAddress());

    
    
    JS_ASSERT(!shouldPatchUnconditionalClaspGuard());
    JS_ASSERT(!inlineClaspGuardPatched);

    Repatcher repatcher(cx->fp()->jit());
    repatcher.relink(fastPathStart.jumpAtOffset(inlineClaspGuard), cs);
    inlineClaspGuardPatched = true;

    stubsGenerated++;

    
    
    if (stubsGenerated == MAX_GETELEM_IC_STUBS)
        disable(cx, "max stubs reached");

    disable(cx, "generated typed array stub");

    
    if (!obj->getProperty(cx, id, vp))
        return Lookup_Error;

    return Lookup_Cacheable;
}
#endif 

LookupStatus
GetElementIC::update(JSContext *cx, JSObject *obj, const Value &v, jsid id, Value *vp)
{
    if (v.isString())
        return attachGetProp(cx, obj, v, id, vp);

#if defined JS_POLYIC_TYPED_ARRAY
    if (js_IsTypedArray(obj))
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
        LookupStatus status = ic->update(cx, thisObj, idval, id, &f.regs.sp[-2]);
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
        if (!js_OnUnknownMethod(cx, f.regs.sp - 2))
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

    JSObject *obj = ValueToObject(cx, &f.regs.sp[-2]);
    if (!obj)
        THROW();

    Value idval = f.regs.sp[-1];

    jsid id;
    if (idval.isInt32() && INT_FITS_IN_JSID(idval.toInt32())) {
        id = INT_TO_JSID(idval.toInt32());
    } else {
        if (!js_InternNonIntElementId(cx, obj, idval, &id))
            THROW();
    }

    if (ic->shouldUpdate(cx)) {
#ifdef DEBUG
        f.regs.sp[-2] = MagicValue(JS_GENERIC_MAGIC);
#endif
        LookupStatus status = ic->update(cx, obj, idval, id, &f.regs.sp[-2]);
        if (status != Lookup_Uncacheable) {
            if (status == Lookup_Error)
                THROW();

            
            JS_ASSERT(!f.regs.sp[-2].isMagic());
            return;
        }
    }

    if (!obj->getProperty(cx, id, &f.regs.sp[-2]))
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
    
    if (inlineClaspGuardPatched)
        repatcher.relink(fastPathStart.jumpAtOffset(inlineClaspGuard), slowPathStart);
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

    
    
    
    
    
    JS_ASSERT((jsuint)keyval >= obj->getDenseArrayCapacity() ||
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

    
    MaybeJump keyGuard;
    if (!hasConstantKey)
        keyGuard = masm.branch32(Assembler::LessThan, keyReg, Imm32(0));

    
    Jump skipUpdate;
    Address arrayLength(objReg, offsetof(JSObject, privateData));
    if (hasConstantKey) {
        skipUpdate = masm.branch32(Assembler::Above, arrayLength, Imm32(keyValue));
        masm.store32(Imm32(keyValue + 1), arrayLength);
    } else {
        skipUpdate = masm.branch32(Assembler::Above, arrayLength, keyReg);
        masm.add32(Imm32(1), keyReg);
        masm.store32(keyReg, arrayLength);
        masm.sub32(Imm32(1), keyReg);
    }
    skipUpdate.linkTo(masm.label(), &masm);

    
    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);
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

    LinkerHelper buffer(masm);
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

#if defined JS_POLYIC_TYPED_ARRAY
LookupStatus
SetElementIC::attachTypedArray(JSContext *cx, JSObject *obj, int32 key)
{
    
    JS_ASSERT(!inlineClaspGuardPatched);

    Assembler masm;

    
    Jump claspGuard = masm.testObjClass(Assembler::NotEqual, objReg, obj->getClass());

    
    masm.loadPtr(Address(objReg, offsetof(JSObject, privateData)), objReg);

    
    Jump outOfBounds;
    Address typedArrayLength(objReg, js::TypedArray::lengthOffset());
    if (hasConstantKey)
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, typedArrayLength, Imm32(keyValue));
    else
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, typedArrayLength, keyReg);

    
    js::TypedArray *tarray = js::TypedArray::fromJSObject(obj);
    masm.loadPtr(Address(objReg, js::TypedArray::dataOffset()), objReg);

    int shift = tarray->slotWidth();
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
    LinkerHelper buffer(masm);
    execPool = buffer.init(cx);
    if (!execPool)
        return error(cx);

    if (!buffer.verifyRange(cx->fp()->jit()))
        return disable(cx, "code memory is out of range");

    
    buffer.link(claspGuard, slowPathStart);
    buffer.link(outOfBounds, fastPathRejoin);
    buffer.link(done, fastPathRejoin);
    masm.finalize(buffer);

    CodeLocationLabel cs = buffer.finalizeCodeAddendum();
    JaegerSpew(JSpew_PICs, "generated setelem typed array stub at %p\n", cs.executableAddress());

    Repatcher repatcher(cx->fp()->jit());
    repatcher.relink(fastPathStart.jumpAtOffset(inlineClaspGuard), cs);
    inlineClaspGuardPatched = true;

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

#if defined JS_POLYIC_TYPED_ARRAY
    if (js_IsTypedArray(obj))
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

