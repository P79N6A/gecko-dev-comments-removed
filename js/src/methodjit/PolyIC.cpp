





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
#include "jsinterpinlines.h"
#include "jsautooplen.h"

#include "js/CharacterEncoding.h"

#include "vm/ScopeObject-inl.h"
#include "vm/StringObject-inl.h"

#if defined JS_POLYIC

using namespace js;
using namespace js::mjit;
using namespace js::mjit::ic;

typedef JSC::FunctionPtr FunctionPtr;
typedef JSC::MacroAssembler::RegisterID RegisterID;
typedef JSC::MacroAssembler::Jump Jump;
typedef JSC::MacroAssembler::Imm32 Imm32;


static const uint32_t INLINE_PATH_LENGTH = 64;




class PICLinker : public LinkerHelper
{
    ic::BasePolyIC &ic;

  public:
    PICLinker(Assembler &masm, ic::BasePolyIC &ic)
      : LinkerHelper(masm, JSC::JAEGER_CODE), ic(ic)
    { }

    bool init(JSContext *cx) {
        JSC::ExecutablePool *pool = LinkerHelper::init(cx);
        if (!pool)
            return false;
        if (!ic.addPool(cx, pool)) {
            markVerified();
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
    ic::PICInfo &pic;
    void *stub;
    uint64_t gcNumber;

  public:
    bool canCallHook;

    PICStubCompiler(const char *type, VMFrame &f, ic::PICInfo &pic, void *stub)
      : BaseCompiler(f.cx), type(type), f(f), pic(pic), stub(stub),
        gcNumber(f.cx->runtime->gcNumber), canCallHook(pic.canCallHook)
    { }

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
        return pic.disable(f, reason, stub);
    }

    LookupStatus disable(VMFrame &f, const char *reason) {
        return pic.disable(f, reason, stub);
    }

    bool hadGC() {
        return gcNumber != f.cx->runtime->gcNumber;
    }

  protected:
    void spew(const char *event, const char *op) {
#ifdef JS_METHODJIT_SPEW
        AutoAssertNoGC nogc;
        JaegerSpew(JSpew_PICs, "%s %s: %s (%s: %d)\n",
                   type, event, op, f.script()->filename, CurrentLine(cx));
#endif
    }
};

static bool
GeneratePrototypeGuards(JSContext *cx, Vector<JSC::MacroAssembler::Jump,8> &mismatches, Assembler &masm,
                        JSObject *obj, JSObject *holder,
                        JSC::MacroAssembler::RegisterID objReg,
                        JSC::MacroAssembler::RegisterID scratchReg)
{
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::AbsoluteAddress AbsoluteAddress;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::Jump Jump;

    if (obj->hasUncacheableProto()) {
        masm.loadPtr(Address(objReg, JSObject::offsetOfType()), scratchReg);
        Jump j = masm.branchPtr(Assembler::NotEqual,
                                Address(scratchReg, offsetof(types::TypeObject, proto)),
                                ImmPtr(obj->getTaggedProto().toObjectOrNull()));
        if (!mismatches.append(j))
            return false;
    }

    JSObject *pobj = obj->getTaggedProto().toObjectOrNull();
    while (pobj != holder) {
        if (pobj->hasUncacheableProto()) {
            Jump j;
            if (pobj->hasSingletonType()) {
                types::TypeObject *type = pobj->getType(cx);
                j = masm.branchPtr(Assembler::NotEqual,
                                   AbsoluteAddress(&type->proto),
                                   ImmPtr(pobj->getProto()),
                                   scratchReg);
            } else {
                j = masm.branchPtr(Assembler::NotEqual,
                                   AbsoluteAddress(pobj->addressOfType()),
                                   ImmPtr(pobj->type()),
                                   scratchReg);
            }
            if (!mismatches.append(j))
                return false;
        }
        pobj = pobj->getProto();
    }

    return true;
}

class SetPropCompiler : public PICStubCompiler
{
    RootedObject obj;
    RootedPropertyName name;
    int lastStubSecondShapeGuard;

  public:
    SetPropCompiler(VMFrame &f, JSObject *obj, ic::PICInfo &pic, PropertyName *name,
                    VoidStubPIC stub)
      : PICStubCompiler("setprop", f, pic, JS_FUNC_TO_DATA_PTR(void *, stub)),
        obj(f.cx, obj), name(f.cx, name), lastStubSecondShapeGuard(pic.secondShapeGuard)
    { }

    static void reset(Repatcher &repatcher, ic::PICInfo &pic)
    {
        SetPropLabels &labels = pic.setPropLabels();
        repatcher.repatchLEAToLoadPtr(labels.getDslotsLoad(pic.fastPathRejoin, pic.u.vr));
        repatcher.repatch(labels.getInlineShapeData(pic.fastPathStart, pic.shapeGuard),
                          NULL);
        repatcher.relink(labels.getInlineShapeJump(pic.fastPathStart.labelAtOffset(pic.shapeGuard)),
                         pic.slowPathStart);

        FunctionPtr target(JS_FUNC_TO_DATA_PTR(void *, ic::SetPropOrName));
        repatcher.relink(pic.slowPathCall, target);
    }

    LookupStatus patchInline(UnrootedShape shape)
    {
        JS_ASSERT(!pic.inlinePathPatched);
        JaegerSpew(JSpew_PICs, "patch setprop inline at %p\n", pic.fastPathStart.executableAddress());

        Repatcher repatcher(f.chunk());
        SetPropLabels &labels = pic.setPropLabels();

        int32_t offset;
        if (obj->isFixedSlot(shape->slot())) {
            CodeLocationInstruction istr = labels.getDslotsLoad(pic.fastPathRejoin, pic.u.vr);
            repatcher.repatchLoadPtrToLEA(istr);

            
            
            
            
            
            
            
            int32_t diff = int32_t(JSObject::getFixedSlotOffset(0)) -
                         int32_t(JSObject::offsetOfSlots());
            JS_ASSERT(diff != 0);
            offset = (int32_t(shape->slot()) * sizeof(Value)) + diff;
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
        Repatcher repatcher(pic.lastCodeBlock(f.chunk()));
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

    LookupStatus generateStub(UnrootedShape initialShape, UnrootedShape shape, bool adding)
    {
        if (hadGC())
            return Lookup_Uncacheable;

        
        Vector<Jump, 8> slowExits(cx);
        Vector<Jump, 8> otherGuards(cx);

        MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
        Assembler masm(&sps, &f);

        
        if (pic.shapeNeedsRemat()) {
            masm.loadShape(pic.objReg, pic.shapeReg);
            pic.shapeRegHasBaseShape = true;
        }

        Label start = masm.label();
        Jump shapeGuard = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                         ImmPtr(initialShape));

        Label stubShapeJumpLabel = masm.label();

        pic.setPropLabels().setStubShapeJump(masm, start, stubShapeJumpLabel);

        if (pic.typeMonitored || adding) {
            















            Jump typeGuard = masm.branchPtr(Assembler::NotEqual,
                                            Address(pic.objReg, JSObject::offsetOfType()),
                                            ImmPtr(obj->getType(cx)));
            if (!otherGuards.append(typeGuard))
                return error();
        }

        JS_ASSERT_IF(!shape->hasDefaultSetter(), obj->isCall());

        if (adding) {
            JS_ASSERT(shape->hasSlot());
            pic.shapeRegHasBaseShape = false;

            if (!GeneratePrototypeGuards(cx, otherGuards, masm, obj, NULL,
                                         pic.objReg, pic.shapeReg)) {
                return error();
            }

            
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
            Address address = masm.objPropAddress(obj, pic.objReg, shape->slot());
            masm.storeValue(pic.u.vr, address);
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

        pic.updatePCCounters(f, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !buffer.verifyRange(f.chunk())) {
            return disable("code memory is out of range");
        }

        buffer.link(shapeGuard, pic.slowPathStart);
        if (slowExit.isSet())
            buffer.link(slowExit.get(), pic.slowPathStart);
        for (Jump *pj = slowExits.begin(); pj != slowExits.end(); ++pj)
            buffer.link(*pj, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);
        CodeLocationLabel cs = buffer.finalize(f);
        JaegerSpew(JSpew_PICs, "generate setprop stub %p %p %d at %p\n",
                   (void*)&pic,
                   (void*)initialShape,
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
        jsid id = NameToId(name);

        types::TypeObject *type = obj->getType(cx);
        if (monitor.recompiled())
            return false;

        if (!type->unknownProperties()) {
            types::AutoEnterAnalysis enter(cx);
            types::TypeSet *types = type->getProperty(cx, types::MakeTypeId(cx, id), true);
            if (!types)
                return false;

            jsbytecode *pc;
            RootedScript script(cx, cx->stack.currentScript(&pc));

            if (!JSScript::ensureRanInference(cx, script) || monitor.recompiled())
                return false;

            JS_ASSERT(*pc == JSOP_SETPROP || *pc == JSOP_SETNAME);

            types::StackTypeSet *rhsTypes = script->analysis()->poppedTypes(pc, 0);
            rhsTypes->addSubset(cx, types);
        }

        return !monitor.recompiled();
    }

    LookupStatus update()
    {
        JS_ASSERT(pic.hit);

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

        RootedObject holder(cx);
        RootedShape shape(cx);

        
        RecompilationMonitor monitor(cx);
        if (!JSObject::lookupProperty(cx, obj, name, &holder, &shape))
            return error();
        if (monitor.recompiled())
            return Lookup_Uncacheable;

        
        if (shape && holder != obj) {
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

            shape = NULL;
        }

        if (!shape) {
            
            if (obj->isDelegate())
                return disable("delegate");
            if (!obj->isExtensible())
                return disable("not extensible");

            if (clasp->addProperty != JS_PropertyStub)
                return disable("add property hook");
            if (clasp->ops.defineProperty)
                return disable("ops define property hook");

            



            if (JSOp(*f.pc()) == JSOP_SETNAME)
                return disable("add property under SETNAME");

            



            JSObject *proto = obj;
            while (proto) {
                if (!proto->isNative())
                    return disable("non-native proto");
                proto = proto->getProto();
            }

            RootedShape initialShape(cx, obj->lastProperty());
            uint32_t slots = obj->numDynamicSlots();

            unsigned flags = 0;
            PropertyOp getter = clasp->getProperty;

            




            shape = JSObject::putProperty(cx, obj, name, getter, clasp->setProperty,
                                          SHAPE_INVALID_SLOT, JSPROP_ENUMERATE, flags, 0);
            if (!shape)
                return error();

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

#ifdef JSGC_INCREMENTAL_MJ
            



            if (cx->compartment->compileBarriers())
                return disable("ADDPROP write barrier required");
#endif

            if (pic.typeMonitored && !updateMonitoredTypes())
                return Lookup_Uncacheable;

            return generateStub(initialShape, shape, true);
        }

        if (!shape->writable())
            return disable("readonly");
        if (shape->hasDefaultSetter()) {
            if (!shape->hasSlot())
                return disable("invalid slot");
            if (pic.typeMonitored && !updateMonitoredTypes())
                return Lookup_Uncacheable;
        } else {
            return disable("setter");
        }

        JS_ASSERT(obj == holder);
        if (!pic.inlinePathPatched &&
            shape->hasDefaultSetter() &&
            !pic.typeMonitored)
        {
            pic.setInlinePathShape(obj->lastProperty());
            return patchInline(shape);
        }

        return generateStub(obj->lastProperty(), shape, false);
    }
};

static bool
IsCacheableProtoChain(JSObject *obj, JSObject *holder)
{
    while (obj != holder) {
        




        TaggedProto proto = obj->getTaggedProto();
        if (!proto.isObject() || !proto.toObject()->isNative())
            return false;
        obj = proto.toObject();
    }
    return true;
}

static bool
IsCacheableListBase(JSObject *obj)
{
    if (!obj->isProxy())
        return false;

    BaseProxyHandler *handler = GetProxyHandler(obj);

    if (handler->family() != GetListBaseHandlerFamily())
        return false;

    if (obj->numFixedSlots() <= GetListBaseExpandoSlot())
        return false;

    return true;
}

template <typename IC>
struct GetPropHelper {
    
    JSContext          *cx;
    RootedObject       obj;
    RootedPropertyName name;
    IC                 &ic;
    VMFrame            &f;

    
    
    RootedObject       holder;
    RootedShape        prop;

    
    
    RootedShape        shape;

    GetPropHelper(JSContext *cx, JSObject *obj, PropertyName *name, IC &ic, VMFrame &f)
      : cx(cx), obj(cx, obj), name(cx, name), ic(ic), f(f), holder(cx), prop(cx), shape(cx)
    { }

  public:
    LookupStatus bind() {
        RecompilationMonitor monitor(cx);
        RootedObject scopeChain(cx, cx->stack.currentScriptedScopeChain());
        if (js_CodeSpec[*f.pc()].format & JOF_GNAME)
            scopeChain = &scopeChain->global();
        if (!LookupName(cx, name, scopeChain, &obj, &holder, &prop))
            return ic.error(cx);
        if (monitor.recompiled())
            return Lookup_Uncacheable;
        if (!prop)
            return ic.disable(cx, "lookup failed");
        if (!obj->isNative())
            return ic.disable(cx, "non-native");
        if (!IsCacheableProtoChain(obj, holder))
            return ic.disable(cx, "non-native holder");
        shape = prop;
        return Lookup_Cacheable;
    }

    LookupStatus lookup() {
        RootedObject aobj(cx, obj);
        if (IsCacheableListBase(obj))
            aobj = obj->getTaggedProto().toObjectOrNull();

        if (!aobj->isNative())
            return ic.disable(f, "non-native");

        RecompilationMonitor monitor(cx);
        if (!JSObject::lookupProperty(cx, aobj, name, &holder, &prop))
            return ic.error(cx);
        if (monitor.recompiled())
            return Lookup_Uncacheable;

        if (!prop) {
            



            if (obj->getClass()->getProperty && obj->getClass()->getProperty != JS_PropertyStub)
                return Lookup_Uncacheable;

            




            JSObject *obj2 = obj;
            while (obj2) {
                if (!obj2->isNative())
                    return Lookup_Uncacheable;
                obj2 = obj2->getProto();
            }

#if JS_HAS_NO_SUCH_METHOD
            




            if (*f.pc() == JSOP_CALLPROP)
                return Lookup_Uncacheable;
#endif

            return Lookup_NoProperty;
        }
        if (!IsCacheableProtoChain(obj, holder))
            return ic.disable(f, "non-native holder");
        shape = prop;
        return Lookup_Cacheable;
    }

    LookupStatus testForGet() {
        AutoAssertNoGC nogc;
        if (!shape->hasDefaultGetter()) {
            if (shape->hasGetterValue()) {
                JSObject *getterObj = shape->getterObject();
                if (!getterObj->isFunction() || !getterObj->toFunction()->isNative())
                    return ic.disable(f, "getter object not a native function");
            }
            if (shape->hasSlot() && holder != obj)
                return ic.disable(f, "slotful getter hook through prototype");
            if (!ic.canCallHook)
                return ic.disable(f, "can't call getter hook");
            if (f.regs.inlined()) {
                




                f.script()->uninlineable = true;
                MarkTypeObjectFlags(cx, f.script()->function(),
                                    types::OBJECT_FLAG_UNINLINEABLE);
                return Lookup_Uncacheable;
            }
        } else if (!shape->hasSlot()) {
            return ic.disable(f, "no slot");
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

namespace js {
namespace mjit {

inline void
MarkNotIdempotent(UnrootedScript script, jsbytecode *pc)
{
    if (!script->hasAnalysis())
        return;
    analyze::Bytecode *code = script->analysis()->maybeCode(pc);
    if (!code)
        return;
    code->notIdempotent = true;
}

class GetPropCompiler : public PICStubCompiler
{
    RootedObject obj;
    RootedPropertyName name;
    int lastStubSecondShapeGuard;

  public:
    GetPropCompiler(VMFrame &f, JSObject *obj, ic::PICInfo &pic, PropertyName *name,
                    VoidStubPIC stub)
      : PICStubCompiler("getprop", f, pic,
                        JS_FUNC_TO_DATA_PTR(void *, stub)),
        obj(f.cx, obj),
        name(f.cx, name),
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

        JS_ASSERT(pic.kind == ic::PICInfo::GET);

        FunctionPtr target(JS_FUNC_TO_DATA_PTR(void *, ic::GetProp));
        repatcher.relink(pic.slowPathCall, target);
    }

    LookupStatus generateArrayLengthStub()
    {
        MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
        Assembler masm(&sps, &f);

        masm.loadObjClass(pic.objReg, pic.shapeReg);
        Jump notArray = masm.testClass(Assembler::NotEqual, pic.shapeReg, &ArrayClass);

        masm.loadPtr(Address(pic.objReg, JSObject::offsetOfElements()), pic.objReg);
        masm.load32(Address(pic.objReg, ObjectElements::offsetOfLength()), pic.objReg);
        Jump oob = masm.branch32(Assembler::Above, pic.objReg, Imm32(JSVAL_INT_MAX));
        masm.move(ImmType(JSVAL_TYPE_INT32), pic.shapeReg);
        Jump done = masm.jump();

        pic.updatePCCounters(f, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !buffer.verifyRange(f.chunk())) {
            return disable("code memory is out of range");
        }

        buffer.link(notArray, pic.slowPathStart);
        buffer.link(oob, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);

        CodeLocationLabel start = buffer.finalize(f);
        JaegerSpew(JSpew_PICs, "generate array length stub at %p\n",
                   start.executableAddress());

        patchPreviousToHere(start);

        disable("array length done");

        return Lookup_Cacheable;
    }

    LookupStatus generateStringObjLengthStub()
    {
        MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
        Assembler masm(&sps, &f);

        Jump notStringObj = masm.guardShape(pic.objReg, obj);

        masm.loadPayload(Address(pic.objReg, StringObject::offsetOfLength()), pic.objReg);
        masm.move(ImmType(JSVAL_TYPE_INT32), pic.shapeReg);
        Jump done = masm.jump();

        pic.updatePCCounters(f, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !buffer.verifyRange(f.chunk())) {
            return disable("code memory is out of range");
        }

        buffer.link(notStringObj, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);

        CodeLocationLabel start = buffer.finalize(f);
        JaegerSpew(JSpew_PICs, "generate string object length stub at %p\n",
                   start.executableAddress());

        patchPreviousToHere(start);

        disable("string object length done");

        return Lookup_Cacheable;
    }

    LookupStatus generateStringPropertyStub()
    {
        AssertCanGC();

        if (!f.fp()->script()->compileAndGo)
            return disable("String.prototype without compile-and-go global");

        RecompilationMonitor monitor(f.cx);

        RootedObject obj(f.cx, f.fp()->global().getOrCreateStringPrototype(f.cx));
        if (!obj)
            return error();

        if (monitor.recompiled())
            return Lookup_Uncacheable;

        GetPropHelper<GetPropCompiler> getprop(cx, obj, name, *this, f);
        LookupStatus status = getprop.lookupAndTest();
        if (status != Lookup_Cacheable)
            return status;
        if (getprop.obj != getprop.holder)
            return disable("proto walk on String.prototype");
        if (!getprop.shape->hasDefaultGetter())
            return disable("getter hook on String.prototype");
        if (hadGC())
            return Lookup_Uncacheable;

        MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
        Assembler masm(&sps, &f);

        
        Jump notString = masm.branchPtr(Assembler::NotEqual, pic.typeReg(),
                                        ImmType(JSVAL_TYPE_STRING));

        





        masm.move(ImmPtr(obj), pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump shapeMismatch = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                            ImmPtr(obj->lastProperty()));
        masm.loadObjProp(obj, pic.objReg, getprop.shape, pic.shapeReg, pic.objReg);

        Jump done = masm.jump();

        pic.updatePCCounters(f, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !buffer.verifyRange(f.chunk())) {
            return disable("code memory is out of range");
        }

        buffer.link(notString, pic.getSlowTypeCheck());
        buffer.link(shapeMismatch, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);

        CodeLocationLabel cs = buffer.finalize(f);
        JaegerSpew(JSpew_PICs, "generate string call stub at %p\n",
                   cs.executableAddress());

        
        if (pic.hasTypeCheck()) {
            Repatcher repatcher(f.chunk());
            repatcher.relink(pic.getPropLabels().getInlineTypeJump(pic.fastPathStart), cs);
        }

        
        disable("generated string call stub");
        return Lookup_Cacheable;
    }

    LookupStatus generateStringLengthStub()
    {
        JS_ASSERT(pic.hasTypeCheck());

        MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
        Assembler masm(&sps, &f);
        Jump notString = masm.branchPtr(Assembler::NotEqual, pic.typeReg(),
                                        ImmType(JSVAL_TYPE_STRING));
        masm.loadPtr(Address(pic.objReg, JSString::offsetOfLengthAndFlags()), pic.objReg);
        
        masm.urshift32(Imm32(JSString::LENGTH_SHIFT), pic.objReg);
        masm.move(ImmType(JSVAL_TYPE_INT32), pic.shapeReg);
        Jump done = masm.jump();

        pic.updatePCCounters(f, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !buffer.verifyRange(f.chunk())) {
            return disable("code memory is out of range");
        }

        buffer.link(notString, pic.getSlowTypeCheck());
        buffer.link(done, pic.fastPathRejoin);

        CodeLocationLabel start = buffer.finalize(f);
        JaegerSpew(JSpew_PICs, "generate string length stub at %p\n",
                   start.executableAddress());

        if (pic.hasTypeCheck()) {
            Repatcher repatcher(f.chunk());
            repatcher.relink(pic.getPropLabels().getInlineTypeJump(pic.fastPathStart), start);
        }

        disable("generated string length stub");

        return Lookup_Cacheable;
    }

    LookupStatus patchInline(JSObject *holder, UnrootedShape shape)
    {
        spew("patch", "inline");
        Repatcher repatcher(f.chunk());
        GetPropLabels &labels = pic.getPropLabels();

        int32_t offset;
        if (holder->isFixedSlot(shape->slot())) {
            CodeLocationInstruction istr = labels.getDslotsLoad(pic.fastPathRejoin);
            repatcher.repatchLoadPtrToLEA(istr);

            
            
            
            
            
            
            
            int32_t diff = int32_t(JSObject::getFixedSlotOffset(0)) -
                         int32_t(JSObject::offsetOfSlots());
            JS_ASSERT(diff != 0);
            offset  = (int32_t(shape->slot()) * sizeof(Value)) + diff;
        } else {
            offset = holder->dynamicSlotIndex(shape->slot()) * sizeof(Value);
        }

        repatcher.repatch(labels.getInlineShapeData(pic.getFastShapeGuard()), obj->lastProperty());
        repatcher.patchAddressOffsetForValueLoad(labels.getValueLoad(pic.fastPathRejoin), offset);

        pic.inlinePathPatched = true;

        return Lookup_Cacheable;
    }

    
    void generateGetterStub(Assembler &masm, UnrootedShape shape, jsid userid,
                            Label start, Vector<Jump, 8> &shapeMismatches)
    {
        AutoAssertNoGC nogc;

        



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
        masm.bumpStubCount(f.script(), f.pc(), t0);

        





        int32_t initialFrameDepth = f.regs.sp - f.fp()->slots() + 3;
        int32_t vpOffset = (char *) f.regs.sp - (char *) f.fp();
        int32_t idHandleOffset = (char *) (f.regs.sp + 1) - (char *) f.fp();
        int32_t objHandleOffset = (char *) (f.regs.sp + 2) - (char *) f.fp();

        


        masm.storePtr(holdObjReg, masm.payloadOf(Address(JSFrameReg, objHandleOffset)));
        masm.storePtr(ImmPtr((void *) JSID_BITS(userid)), masm.payloadOf(Address(JSFrameReg, idHandleOffset)));

        







#if JS_BITS_PER_WORD == 32
        masm.storePtr(ImmPtr(NULL), masm.tagOf(Address(JSFrameReg, objHandleOffset)));
        masm.storePtr(ImmPtr(NULL), masm.tagOf(Address(JSFrameReg, idHandleOffset)));
#endif

        if (shape->hasSlot()) {
            masm.loadObjProp(obj, holdObjReg, shape,
                             Registers::ClobberInCall, t0);
            masm.storeValueFromComponents(Registers::ClobberInCall, t0, Address(JSFrameReg, vpOffset));
        } else {
            masm.storeValue(UndefinedValue(), Address(JSFrameReg, vpOffset));
        }

        masm.setupFallibleABICall(cx->typeInferenceEnabled(), f.regs.pc, initialFrameDepth);

        
#ifdef JS_CPU_X86
        RegisterID cxReg = tempRegs.takeAnyReg().reg();
#else
        RegisterID cxReg = Registers::ArgReg0;
#endif
        masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), cxReg);

        
        masm.addPtr(Imm32(vpOffset), JSFrameReg, t0);

        masm.restoreStackBase();
        masm.setupABICall(Registers::NormalCall, 4);
        masm.storeArg(3, t0);
        masm.addPtr(Imm32(idHandleOffset - vpOffset + Assembler::PAYLOAD_OFFSET), t0);
        masm.storeArg(2, t0);
        masm.addPtr(Imm32(objHandleOffset - idHandleOffset), t0);
        masm.storeArg(1, t0);
        masm.storeArg(0, cxReg);

        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, getter), false);

        NativeStubLinker::FinalJump done;
        if (!NativeStubEpilogue(f, masm, &done, 0, vpOffset, pic.shapeReg, pic.objReg))
            return;
        NativeStubLinker linker(masm, f.chunk(), f.regs.pc, done);
        if (!linker.init(f.cx))
            THROW();

        if (!linker.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !linker.verifyRange(f.chunk())) {
            disable("code memory is out of range");
            return;
        }

        linker.patchJump(pic.fastPathRejoin);

        linkerEpilogue(linker, start, shapeMismatches);
    }

    
    void generateNativeGetterStub(Assembler &masm, UnrootedShape shape,
                                  Label start, Vector<Jump, 8> &shapeMismatches)
    {
        AutoAssertNoGC nogc;

        



        JS_ASSERT(pic.canCallHook);

        JSFunction *fun = shape->getterObject()->toFunction();
        Native native = fun->native();

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
        masm.bumpStubCount(f.script(), f.pc(), t0);

        








        int32_t vpOffset = (char *) f.regs.sp - (char *) f.fp();

        masm.storeValue(ObjectValue(*fun), Address(JSFrameReg, vpOffset));
        masm.storeValueFromComponents(ImmType(JSVAL_TYPE_OBJECT), holdObjReg,
                                      Address(JSFrameReg, vpOffset + sizeof(js::Value)));

        



        int32_t initialFrameDepth = f.regs.sp + 2 - f.fp()->slots();
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
        masm.setupABICall(Registers::NormalCall, 3);
        masm.storeArg(2, vpReg);
        masm.storeArg(1, Imm32(0)); 
        masm.storeArg(0, cxReg);

        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, native), false);

        NativeStubLinker::FinalJump done;
        if (!NativeStubEpilogue(f, masm, &done, 0, vpOffset, pic.shapeReg, pic.objReg))
            return;
        NativeStubLinker linker(masm, f.chunk(), f.regs.pc, done);
        if (!linker.init(f.cx))
            THROW();

        if (!linker.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !linker.verifyRange(f.chunk())) {
            disable("code memory is out of range");
            return;
        }

        linker.patchJump(pic.fastPathRejoin);

        linkerEpilogue(linker, start, shapeMismatches);
    }

    LookupStatus generateStub(JSObject *holder, HandleShape shape)
    {
        AssertCanGC();
        Vector<Jump, 8> shapeMismatches(cx);

        MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
        Assembler masm(&sps, &f);

        
        SkipRoot skip(cx, &masm);

        Label start;
        Jump shapeGuardJump;
        Jump argsLenGuard;

        if (pic.shapeNeedsRemat()) {
            masm.loadShape(pic.objReg, pic.shapeReg);
            pic.shapeRegHasBaseShape = true;
        }

        start = masm.label();
        shapeGuardJump = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                        ImmPtr(obj->lastProperty()));
        Label stubShapeJumpLabel = masm.label();

        if (!shapeMismatches.append(shapeGuardJump))
            return error();

        
        if (IsCacheableListBase(obj)) {
            
            
            
            
            
            
            
            
            

            Address handler(pic.objReg, JSObject::getFixedSlotOffset(JSSLOT_PROXY_HANDLER));
            Jump handlerGuard = masm.testPrivate(Assembler::NotEqual, handler, GetProxyHandler(obj));
            if (!shapeMismatches.append(handlerGuard))
                return error();

            Address expandoAddress(pic.objReg, JSObject::getFixedSlotOffset(GetListBaseExpandoSlot()));

            Value expandoValue = obj->getFixedSlot(GetListBaseExpandoSlot());
            JSObject *expando = expandoValue.isObject() ? &expandoValue.toObject() : NULL;

            
            
            
            JS_ASSERT_IF(expando, expando->isNative() && expando->getProto() == NULL);

            if (expando && expando->nativeLookupNoAllocation(name) == NULL) {
                Jump expandoGuard = masm.testObject(Assembler::NotEqual, expandoAddress);
                if (!shapeMismatches.append(expandoGuard))
                    return error();

                masm.loadPayload(expandoAddress, pic.shapeReg);
                pic.shapeRegHasBaseShape = false;

                Jump shapeGuard = masm.branchPtr(Assembler::NotEqual,
                                                 Address(pic.shapeReg, JSObject::offsetOfShape()),
                                                 ImmPtr(expando->lastProperty()));
                if (!shapeMismatches.append(shapeGuard))
                    return error();
            } else {
                Jump expandoGuard = masm.testUndefined(Assembler::NotEqual, expandoAddress);
                if (!shapeMismatches.append(expandoGuard))
                    return error();
            }
        }

        RegisterID holderReg = pic.objReg;
        if (obj != holder) {
            if (!GeneratePrototypeGuards(cx, shapeMismatches, masm, obj, holder,
                                         pic.objReg, pic.shapeReg)) {
                return error();
            }

            if (holder) {
                
                holderReg = pic.shapeReg;
                masm.move(ImmPtr(holder), holderReg);
                pic.shapeRegHasBaseShape = false;

                
                Jump j = masm.guardShape(holderReg, holder);
                if (!shapeMismatches.append(j))
                    return error();
            } else {
                
                
                JSObject *proto = obj->getTaggedProto().toObjectOrNull();
                RegisterID lastReg = pic.objReg;
                while (proto) {
                    masm.loadPtr(Address(lastReg, JSObject::offsetOfType()), pic.shapeReg);
                    masm.loadPtr(Address(pic.shapeReg, offsetof(types::TypeObject, proto)), pic.shapeReg);
                    Jump protoGuard = masm.guardShape(pic.shapeReg, proto);
                    if (!shapeMismatches.append(protoGuard))
                        return error();

                    proto = proto->getProto();
                    lastReg = pic.shapeReg;
                }
            }

            pic.secondShapeGuard = masm.distanceOf(masm.label()) - masm.distanceOf(start);
        } else {
            pic.secondShapeGuard = 0;
        }

        if (shape && !shape->hasDefaultGetter()) {
            MarkNotIdempotent(f.script(), f.pc());

            if (shape->hasGetterValue()) {
                generateNativeGetterStub(masm, shape, start, shapeMismatches);
            } else {
                RootedId userid(cx);
                if (!shape->getUserId(cx, &userid))
                    return error();
                generateGetterStub(masm, shape, userid, start, shapeMismatches);
            }
            pic.getPropLabels().setStubShapeJump(masm, start, stubShapeJumpLabel);
            return Lookup_Cacheable;
        }

        






        if (shape)
            masm.loadObjProp(holder, holderReg, shape, pic.shapeReg, pic.objReg);
        else
            masm.loadValueAsComponents(UndefinedValue(), pic.shapeReg, pic.objReg);

        Jump done = masm.jump();
        pic.updatePCCounters(f, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !buffer.verifyRange(f.chunk())) {
            return disable("code memory is out of range");
        }

        
        buffer.link(done, pic.fastPathRejoin);

        linkerEpilogue(buffer, start, shapeMismatches);

        pic.getPropLabels().setStubShapeJump(masm, start, stubShapeJumpLabel);
        return Lookup_Cacheable;
    }

    void linkerEpilogue(LinkerHelper &buffer, Label start, Vector<Jump, 8> &shapeMismatches)
    {
        
        for (Jump *pj = shapeMismatches.begin(); pj != shapeMismatches.end(); ++pj)
            buffer.link(*pj, pic.slowPathStart);

        CodeLocationLabel cs = buffer.finalize(f);
        JaegerSpew(JSpew_PICs, "generated %s stub at %p\n", type, cs.executableAddress());

        patchPreviousToHere(cs);

        pic.stubsGenerated++;
        pic.updateLastPath(buffer, start);

        if (pic.stubsGenerated == MAX_PIC_STUBS)
            disable("max stubs reached");
    }

    void patchPreviousToHere(CodeLocationLabel cs)
    {
        Repatcher repatcher(pic.lastCodeBlock(f.chunk()));
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

        GetPropHelper<GetPropCompiler> getprop(cx, obj, name, *this, f);
        RecompilationMonitor monitor(cx);
        LookupStatus status = getprop.lookupAndTest();

        if (status != Lookup_Cacheable && status != Lookup_NoProperty) {
            
            if (!monitor.recompiled())
                pic.hadUncacheable = true;
            MarkNotIdempotent(f.script(), f.pc());
            return status;
        }

        
        
        if (!obj->hasIdempotentProtoChain())
            MarkNotIdempotent(f.script(), f.pc());

        
        
        if (!getprop.holder)
            MarkNotIdempotent(f.script(), f.pc());

        if (hadGC())
            return Lookup_Uncacheable;

        if (obj == getprop.holder &&
            getprop.shape->hasDefaultGetter() &&
            !pic.inlinePathPatched)
        {
            pic.setInlinePathShape(obj->lastProperty());
            return patchInline(getprop.holder, getprop.shape);
        }

        return generateStub(getprop.holder, getprop.shape);
    }
};

}  
}  

class ScopeNameCompiler : public PICStubCompiler
{
  private:
    typedef Vector<Jump, 8> JumpList;

    RootedObject scopeChain;
    RootedPropertyName name;
    GetPropHelper<ScopeNameCompiler> getprop;
    ScopeNameCompiler *thisFromCtor() { return this; }

    void patchPreviousToHere(CodeLocationLabel cs)
    {
        ScopeNameLabels &       labels = pic.scopeNameLabels();
        Repatcher               repatcher(pic.lastCodeBlock(f.chunk()));
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

            
            masm.loadShape(pic.objReg, pic.shapeReg);
            Jump j = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                    ImmPtr(tobj->lastProperty()));
            if (!fails.append(j))
                return error();

            
            Address parent(pic.objReg, ScopeObject::offsetOfEnclosingScope());
            masm.loadPayload(parent, pic.objReg);

            tobj = &tobj->asScope().enclosingScope();
        }

        if (tobj != getprop.holder)
            return disable("scope chain walk terminated early");

        return Lookup_Cacheable;
    }

  public:
    ScopeNameCompiler(VMFrame &f, JSObject *scopeChain, ic::PICInfo &pic,
                      PropertyName *name, VoidStubPIC stub)
      : PICStubCompiler("name", f, pic, JS_FUNC_TO_DATA_PTR(void *, stub)),
        scopeChain(f.cx, scopeChain), name(f.cx, name),
        getprop(f.cx, NULL, name, *thisFromCtor(), f)
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
          default:
            JS_NOT_REACHED("Invalid pic kind in ScopeNameCompiler::reset");
            return;
        }
        FunctionPtr target(JS_FUNC_TO_DATA_PTR(void *, stub));
        repatcher.relink(pic.slowPathCall, target);
    }

    LookupStatus generateGlobalStub(JSObject *obj)
    {
        MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
        Assembler masm(&sps, &f);
        JumpList fails(cx);
        ScopeNameLabels &labels = pic.scopeNameLabels();

        
        if (pic.kind == ic::PICInfo::NAME)
            masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), pic.objReg);

        JS_ASSERT(obj == getprop.holder);
        JS_ASSERT(getprop.holder == &scopeChain->global());

        LookupStatus status = walkScopeChain(masm, fails);
        if (status != Lookup_Cacheable)
            return status;

        
        MaybeJump finalNull;
        if (pic.kind == ic::PICInfo::NAME)
            finalNull = masm.branchTestPtr(Assembler::Zero, pic.objReg, pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump finalShape = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                         ImmPtr(getprop.holder->lastProperty()));

        masm.loadObjProp(obj, pic.objReg, getprop.shape, pic.shapeReg, pic.objReg);

        Jump done = masm.jump();

        
        for (Jump *pj = fails.begin(); pj != fails.end(); ++pj)
            pj->linkTo(masm.label(), &masm);
        if (finalNull.isSet())
            finalNull.get().linkTo(masm.label(), &masm);
        finalShape.linkTo(masm.label(), &masm);
        Label failLabel = masm.label();
        Jump failJump = masm.jump();

        pic.updatePCCounters(f, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !buffer.verifyRange(f.chunk())) {
            return disable("code memory is out of range");
        }

        buffer.link(failJump, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);
        CodeLocationLabel cs = buffer.finalize(f);
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
        MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
        Assembler masm(&sps, &f);
        Vector<Jump, 8> fails(cx);
        ScopeNameLabels &labels = pic.scopeNameLabels();

        
        if (pic.kind == ic::PICInfo::NAME)
            masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), pic.objReg);

        JS_ASSERT(obj == getprop.holder);
        JS_ASSERT(getprop.holder != &scopeChain->global());

        UnrootedShape shape = getprop.shape;
        if (!shape->hasDefaultGetter())
            return disable("unhandled callobj sprop getter");

        LookupStatus status = walkScopeChain(masm, fails);
        if (status != Lookup_Cacheable)
            return status;

        
        MaybeJump finalNull;
        if (pic.kind == ic::PICInfo::NAME)
            finalNull = masm.branchTestPtr(Assembler::Zero, pic.objReg, pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump finalShape = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                         ImmPtr(getprop.holder->lastProperty()));
        Address address = masm.objPropAddress(obj, pic.objReg, shape->slot());

        
        masm.loadValueAsComponents(address, pic.shapeReg, pic.objReg);

        Jump done = masm.jump();

        
        for (Jump *pj = fails.begin(); pj != fails.end(); ++pj)
            pj->linkTo(masm.label(), &masm);
        if (finalNull.isSet())
            finalNull.get().linkTo(masm.label(), &masm);
        finalShape.linkTo(masm.label(), &masm);
        Label failLabel = masm.label();
        Jump failJump = masm.jump();

        pic.updatePCCounters(f, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !buffer.verifyRange(f.chunk())) {
            return disable("code memory is out of range");
        }

        buffer.link(failJump, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);
        CodeLocationLabel cs = buffer.finalize(f);
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

        if (obj->isGlobal())
            return generateGlobalStub(obj);

        return disable("scope object not handled yet");
    }

    bool retrieve(MutableHandleValue vp, PICInfo::Kind kind)
    {
        RootedObject obj(cx, getprop.obj);
        RootedObject holder(cx, getprop.holder);
        RootedShape prop(cx, getprop.prop);

        if (!prop) {
            
            if (kind == ic::PICInfo::NAME) {
                JSOp op2 = JSOp(f.pc()[JSOP_NAME_LENGTH]);
                if (op2 == JSOP_TYPEOF) {
                    vp.setUndefined();
                    return true;
                }
            }
            ReportAtomNotDefined(cx, name);
            return false;
        }

        
        
        if (!getprop.shape) {
            if (!JSObject::getProperty(cx, obj, obj, name, vp))
                return false;
            return true;
        }

        RootedShape shape(cx, getprop.shape);
        Rooted<JSObject*> normalized(cx, obj);
        if (obj->isWith() && !shape->hasDefaultGetter())
            normalized = &obj->asWith().object();
        if (shape->isDataDescriptor() && shape->hasDefaultGetter()) {
            
            JS_ASSERT(shape->slot() != SHAPE_INVALID_SLOT || !shape->hasDefaultSetter());
            if (shape->slot() != SHAPE_INVALID_SLOT)
                vp.set(holder->nativeGetSlot(shape->slot()));
            else
                vp.setUndefined();
        } else {
            if (!js_NativeGet(cx, normalized, holder, shape, 0, vp))
                return false;
        }
        return true;
    }
};

class BindNameCompiler : public PICStubCompiler
{
    RootedObject scopeChain;
    RootedPropertyName name;

  public:
    BindNameCompiler(VMFrame &f, JSObject *scopeChain, ic::PICInfo &pic,
                     PropertyName *name, VoidStubPIC stub)
      : PICStubCompiler("bind", f, pic, JS_FUNC_TO_DATA_PTR(void *, stub)),
        scopeChain(f.cx, scopeChain), name(f.cx, name)
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
        Repatcher repatcher(pic.lastCodeBlock(f.chunk()));
        JSC::CodeLocationJump jump;

        
        if (pic.stubsGenerated)
            jump = labels.getStubJump(pic.lastPathStart());
        else
            jump = labels.getInlineJump(pic.getFastShapeGuard());
        repatcher.relink(jump, cs);
    }

    LookupStatus generateStub(JSObject *obj)
    {
        MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
        Assembler masm(&sps, &f);
        Vector<Jump, 8> fails(cx);

        BindNameLabels &labels = pic.bindNameLabels();

        if (!IsCacheableNonGlobalScope(scopeChain))
            return disable("non-cacheable obj at start of scope chain");

        
        masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), pic.objReg);
        masm.loadShape(pic.objReg, pic.shapeReg);
        Jump firstShape = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                         ImmPtr(scopeChain->lastProperty()));

        if (scopeChain != obj) {
            
            JSObject *tobj = &scopeChain->asScope().enclosingScope();
            Address parent(pic.objReg, ScopeObject::offsetOfEnclosingScope());
            while (tobj) {
                if (!IsCacheableNonGlobalScope(tobj))
                    return disable("non-cacheable obj in scope chain");
                masm.loadPayload(parent, pic.objReg);
                masm.loadShape(pic.objReg, pic.shapeReg);
                Jump shapeTest = masm.branchPtr(Assembler::NotEqual, pic.shapeReg,
                                                ImmPtr(tobj->lastProperty()));
                if (!fails.append(shapeTest))
                    return error();
                if (tobj == obj)
                    break;
                tobj = &tobj->asScope().enclosingScope();
            }
            if (tobj != obj)
                return disable("indirect hit");
        }

        Jump done = masm.jump();

        
        for (Jump *pj = fails.begin(); pj != fails.end(); ++pj)
            pj->linkTo(masm.label(), &masm);
        firstShape.linkTo(masm.label(), &masm);
        Label failLabel = masm.label();
        Jump failJump = masm.jump();

        pic.updatePCCounters(f, masm);

        PICLinker buffer(masm, pic);
        if (!buffer.init(cx))
            return error();

        if (!buffer.verifyRange(pic.lastCodeBlock(f.chunk())) ||
            !buffer.verifyRange(f.chunk())) {
            return disable("code memory is out of range");
        }

        buffer.link(failJump, pic.slowPathStart);
        buffer.link(done, pic.fastPathRejoin);
        CodeLocationLabel cs = buffer.finalize(f);
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
        RecompilationMonitor monitor(cx);

        RootedObject scope(cx);
        if (!LookupNameWithGlobalDefault(cx, name, scopeChain, &scope))
            return NULL;

        if (monitor.recompiled())
            return scope;

        if (!pic.hit) {
            spew("first hit", "nop");
            pic.hit = true;
            return scope;
        }

        LookupStatus status = generateStub(scope);
        if (status == Lookup_Error)
            return NULL;

        return scope;
    }
};

static void JS_FASTCALL
DisabledGetPropIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::GetProp(f, pic->name);
}

static void JS_FASTCALL
DisabledGetPropNoCacheIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::GetPropNoCache(f, pic->name);
}

void JS_FASTCALL
ic::GetProp(VMFrame &f, ic::PICInfo *pic)
{
    bool cached = pic->cached;
    VoidStubPIC stub = cached ? DisabledGetPropIC : DisabledGetPropNoCacheIC;

    RootedPropertyName name(f.cx, pic->name);
    if (name == f.cx->names().length) {
        if (IsOptimizedArguments(f.fp(), &f.regs.sp[-1])) {
            f.regs.sp[-1].setInt32(f.regs.fp()->numActualArgs());
            return;
        }
        if (!f.regs.sp[-1].isPrimitive()) {
            JSObject *obj = &f.regs.sp[-1].toObject();
            if (obj->isArray() || obj->isString()) {
                GetPropCompiler cc(f, obj, *pic, NULL, stub);
                if (obj->isArray()) {
                    LookupStatus status = cc.generateArrayLengthStub();
                    if (status == Lookup_Error)
                        THROW();
                    f.regs.sp[-1].setNumber(obj->getArrayLength());
                } else {
                    LookupStatus status = cc.generateStringObjLengthStub();
                    if (status == Lookup_Error)
                        THROW();
                    JSString *str = obj->asString().unbox();
                    f.regs.sp[-1].setInt32(str->length());
                }
                return;
            }
        }
    }

    RootedValue objval(f.cx, f.regs.sp[-1]);

    if (f.regs.sp[-1].isString()) {
        GetPropCompiler cc(f, NULL, *pic, name, stub);
        if (name == f.cx->names().length) {
            LookupStatus status = cc.generateStringLengthStub();
            if (status == Lookup_Error)
                THROW();
            JSString *str = f.regs.sp[-1].toString();
            f.regs.sp[-1].setInt32(str->length());
        } else {
            LookupStatus status = cc.generateStringPropertyStub();
            if (status == Lookup_Error)
                THROW();
            RootedObject obj(f.cx, ToObjectFromStack(f.cx, objval));
            if (!obj)
                THROW();
            MutableHandleValue vp = MutableHandleValue::fromMarkedLocation(&f.regs.sp[-1]);
            if (!JSObject::getProperty(f.cx, obj, obj, name, vp))
                THROW();
        }
        return;
    }

    RecompilationMonitor monitor(f.cx);

    RootedObject obj(f.cx, ToObjectFromStack(f.cx, objval));
    if (!obj)
        THROW();

    if (!monitor.recompiled() && pic->shouldUpdate(f)) {
        GetPropCompiler cc(f, obj, *pic, name, stub);
        if (!cc.update())
            THROW();
    }

    RootedValue v(f.cx);
    if (cached) {
        RootedScript script(f.cx, f.script());
        if (!GetPropertyOperation(f.cx, script, f.pc(), &objval, &v))
            THROW();
    } else {
        if (!JSObject::getProperty(f.cx, obj, obj, name, &v))
            THROW();
    }

    f.regs.sp[-1] = v;
}

static void JS_FASTCALL
DisabledSetPropIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::SetProp(f, pic->name);
}

static void JS_FASTCALL
DisabledSetNameIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::SetName(f, pic->name);
}

void JS_FASTCALL
ic::SetPropOrName(VMFrame &f, ic::PICInfo *pic)
{
    JS_ASSERT(pic->isSet());
    JS_ASSERT(*f.pc() == JSOP_SETPROP || *f.pc() == JSOP_SETNAME);

    
    RootedPropertyName name(f.cx, pic->name);

    RecompilationMonitor monitor(f.cx);

    RootedValue objval(f.cx, f.regs.sp[-2]);
    JSObject *obj = ToObjectFromStack(f.cx, objval);
    if (!obj)
        THROW();

    
    
    if (!monitor.recompiled() && pic->shouldUpdate(f)) {
        VoidStubPIC disabled = *f.pc() == JSOP_SETPROP ? DisabledSetPropIC : DisabledSetNameIC;
        SetPropCompiler cc(f, obj, *pic, name, disabled);
        LookupStatus status = cc.update();
        if (status == Lookup_Error)
            THROW();
        if (status != Lookup_Cacheable && !monitor.recompiled())
            pic->hadUncacheable = true;
    }

    if (*f.pc() == JSOP_SETPROP)
        stubs::SetProp(f, name);
    else
        stubs::SetName(f, name);
}

static void JS_FASTCALL
DisabledNameIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::Name(f);
}

static void JS_FASTCALL
DisabledXNameIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::GetProp(f, pic->name);
}

void JS_FASTCALL
ic::XName(VMFrame &f, ic::PICInfo *pic)
{
    
    JSObject *obj = &f.regs.sp[-1].toObject();

    ScopeNameCompiler cc(f, obj, *pic, pic->name, DisabledXNameIC);

    LookupStatus status = cc.updateForXName();
    if (status == Lookup_Error)
        THROW();

    RootedValue rval(f.cx);
    if (!cc.retrieve(&rval, PICInfo::XNAME))
        THROW();
    f.regs.sp[-1] = rval;
}

void JS_FASTCALL
ic::Name(VMFrame &f, ic::PICInfo *pic)
{
    ScopeNameCompiler cc(f, f.fp()->scopeChain(), *pic, pic->name, DisabledNameIC);

    LookupStatus status = cc.updateForName();
    if (status == Lookup_Error)
        THROW();

    RootedValue rval(f.cx);
    if (!cc.retrieve(&rval, PICInfo::NAME))
        THROW();
    f.regs.sp[0] = rval;
}

static void JS_FASTCALL
DisabledBindNameIC(VMFrame &f, ic::PICInfo *pic)
{
    stubs::BindName(f, pic->name);
}

void JS_FASTCALL
ic::BindName(VMFrame &f, ic::PICInfo *pic)
{
    VoidStubPIC stub = DisabledBindNameIC;
    BindNameCompiler cc(f, f.fp()->scopeChain(), *pic, pic->name, stub);

    JSObject *obj = cc.update();
    if (!obj)
        THROW();

    f.regs.sp[0].setObject(*obj);
}

void
BaseIC::spew(VMFrame &f, const char *event, const char *message)
{
#ifdef JS_METHODJIT_SPEW
    AutoAssertNoGC nogc;
    JaegerSpew(JSpew_PICs, "%s %s: %s (%s: %d)\n",
               js_CodeName[JSOp(*f.pc())], event, message,
               f.cx->fp()->script()->filename, CurrentLine(f.cx));
#endif
}


inline uint32_t
frameCountersOffset(VMFrame &f)
{
    AutoAssertNoGC nogc;

    JSContext *cx = f.cx;

    uint32_t offset = 0;
    if (cx->regs().inlined()) {
        offset += cx->fp()->script()->length;
        uint32_t index = cx->regs().inlined()->inlineIndex;
        InlineFrame *frames = f.chunk()->inlineFrames();
        for (unsigned i = 0; i < index; i++)
            offset += frames[i].fun->nonLazyScript()->length;
    }

    jsbytecode *pc;
    UnrootedScript script = cx->stack.currentScript(&pc);
    offset += pc - script->code;

    return offset;
}

LookupStatus
BaseIC::disable(VMFrame &f, const char *reason, void *stub)
{
    if (f.chunk()->pcLengths) {
        uint32_t offset = frameCountersOffset(f);
        f.chunk()->pcLengths[offset].picsLength = 0;
    }

    disabled = true;

    spew(f, "disabled", reason);
    Repatcher repatcher(f.chunk());
    repatcher.relink(slowPathCall, FunctionPtr(stub));
    return Lookup_Uncacheable;
}

void
BaseIC::updatePCCounters(VMFrame &f, Assembler &masm)
{
    if (f.chunk()->pcLengths) {
        uint32_t offset = frameCountersOffset(f);
        f.chunk()->pcLengths[offset].picsLength += masm.size();
    }
}

bool
BaseIC::shouldUpdate(VMFrame &f)
{
    if (!hit) {
        hit = true;
        spew(f, "ignored", "first hit");
        return false;
    }
    JS_ASSERT(stubsGenerated < MAX_PIC_STUBS);
    return true;
}

void
PICInfo::purge(Repatcher &repatcher)
{
    switch (kind) {
      case SET:
        SetPropCompiler::reset(repatcher, *this);
        break;
      case NAME:
      case XNAME:
        ScopeNameCompiler::reset(repatcher, *this);
        break;
      case BIND:
        BindNameCompiler::reset(repatcher, *this);
        break;
      case GET:
        GetPropCompiler::reset(repatcher, *this);
        break;
      default:
        JS_NOT_REACHED("Unhandled PIC kind");
        break;
    }
    reset();
}

static void JS_FASTCALL
DisabledGetElem(VMFrame &f, ic::GetElementIC *ic)
{
    stubs::GetElem(f);
}

bool
GetElementIC::shouldUpdate(VMFrame &f)
{
    if (!hit) {
        hit = true;
        spew(f, "ignored", "first hit");
        return false;
    }
    JS_ASSERT(stubsGenerated < MAX_GETELEM_IC_STUBS);
    return true;
}

LookupStatus
GetElementIC::disable(VMFrame &f, const char *reason)
{
    slowCallPatched = true;
    void *stub = JS_FUNC_TO_DATA_PTR(void *, DisabledGetElem);
    BaseIC::disable(f, reason, stub);
    return Lookup_Uncacheable;
}

LookupStatus
GetElementIC::error(JSContext *cx)
{
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
        repatcher.relink(slowPathCall,
                         FunctionPtr(JS_FUNC_TO_DATA_PTR(void *, ic::GetElement)));
    }

    reset();
}

LookupStatus
GetElementIC::attachGetProp(VMFrame &f, HandleObject obj, HandleValue v, HandlePropertyName name,
                            MutableHandleValue vp)
{
    JS_ASSERT(v.isString());
    JSContext *cx = f.cx;

    
    if (!obj->isNative())
        return Lookup_Uncacheable;

    GetPropHelper<GetElementIC> getprop(cx, obj, name, *this, f);
    LookupStatus status = getprop.lookupAndTest();
    if (status != Lookup_Cacheable)
        return status;

    
    
    
    if (cx->typeInferenceEnabled() && !forcedTypeBarrier)
        return disable(f, "string element access may not have type barrier");

    MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
    Assembler masm(&sps, &f);

    
    MaybeJump atomTypeGuard;
    if (hasInlineTypeGuard() && !inlineTypeGuardPatched) {
        
        
        
        
        
        JS_ASSERT(!idRemat.isTypeKnown());
        atomTypeGuard = masm.testString(Assembler::NotEqual, typeReg);
    } else {
        
        
        
        JS_ASSERT_IF(!hasInlineTypeGuard(), idRemat.knownType() == JSVAL_TYPE_STRING);
    }

    
    if (!typeRegHasBaseShape) {
        masm.loadShape(objReg, typeReg);
        typeRegHasBaseShape = true;
    }

    MaybeJump atomIdGuard;
    if (!idRemat.isConstant())
        atomIdGuard = masm.branchPtr(Assembler::NotEqual, idRemat.dataReg(), ImmPtr(v.toString()));

    
    Jump shapeGuard = masm.branchPtr(Assembler::NotEqual, typeReg, ImmPtr(obj->lastProperty()));

    Vector<Jump, 8> otherGuards(cx);

    
    MaybeJump protoGuard;
    JSObject *holder = getprop.holder;
    RegisterID holderReg = objReg;
    if (obj != holder) {
        if (!GeneratePrototypeGuards(cx, otherGuards, masm, obj, holder, objReg, typeReg))
            return error(cx);

        
        holderReg = typeReg;
        masm.move(ImmPtr(holder), holderReg);
        typeRegHasBaseShape = false;

        
        protoGuard = masm.guardShape(holderReg, holder);
    }

    
    RootedShape shape(cx, getprop.shape);
    masm.loadObjProp(holder, holderReg, shape, typeReg, objReg);

    Jump done = masm.jump();

    updatePCCounters(f, masm);

    PICLinker buffer(masm, *this);
    if (!buffer.init(cx))
        return error(cx);

    if (hasLastStringStub && !buffer.verifyRange(lastStringStub))
        return disable(f, "code memory is out of range");
    if (!buffer.verifyRange(f.chunk()))
        return disable(f, "code memory is out of range");

    
    buffer.maybeLink(atomIdGuard, slowPathStart);
    buffer.maybeLink(atomTypeGuard, slowPathStart);
    buffer.link(shapeGuard, slowPathStart);
    buffer.maybeLink(protoGuard, slowPathStart);
    for (Jump *pj = otherGuards.begin(); pj != otherGuards.end(); ++pj)
        buffer.link(*pj, slowPathStart);
    buffer.link(done, fastPathRejoin);

    CodeLocationLabel cs = buffer.finalize(f);
#if DEBUG
    Latin1CharsZ latin1 = LossyTwoByteCharsToNewLatin1CharsZ(cx, v.toString()->ensureLinear(cx)->range());
    JaegerSpew(JSpew_PICs, "generated %s stub at %p for atom %p (\"%s\") shape %p (%s: %d)\n",
               js_CodeName[JSOp(*f.pc())], cs.executableAddress(), (void*)name, latin1.get(),
               (void*)holder->lastProperty(), cx->fp()->script()->filename,
               CurrentLine(cx));
    JS_free(latin1);
#endif

    
    if (shouldPatchInlineTypeGuard() || shouldPatchUnconditionalShapeGuard()) {
        Repatcher repatcher(f.chunk());

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
        disable(f, "max stubs reached");

    
    vp.set(holder->getSlot(shape->slot()));

    return Lookup_Cacheable;
}

#if defined JS_METHODJIT_TYPED_ARRAY
LookupStatus
GetElementIC::attachTypedArray(VMFrame &f, HandleObject obj, HandleValue v, HandleId id, MutableHandleValue vp)
{
    JSContext *cx = f.cx;

    if (!v.isInt32())
        return disable(f, "typed array with string key");

    if (JSOp(*f.pc()) == JSOP_CALLELEM)
        return disable(f, "typed array with call");

    
    
    JS_ASSERT(hasInlineTypeGuard() || idRemat.knownType() == JSVAL_TYPE_INT32);

    MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
    Assembler masm(&sps, &f);

    
    Jump shapeGuard = masm.guardShape(objReg, obj);

    
    Jump outOfBounds;
    Address typedArrayLength = masm.payloadOf(Address(objReg, TypedArray::lengthOffset()));
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

    if (!masm.supportsFloatingPoint() &&
        (TypedArray::type(obj) == TypedArray::TYPE_FLOAT32 ||
         TypedArray::type(obj) == TypedArray::TYPE_FLOAT64 ||
         TypedArray::type(obj) == TypedArray::TYPE_UINT32))
    {
        return disable(f, "fpu not supported");
    }

    MaybeRegisterID tempReg;
    masm.loadFromTypedArray(TypedArray::type(obj), objReg, key, typeReg, objReg, tempReg);

    Jump done = masm.jump();

    updatePCCounters(f, masm);

    PICLinker buffer(masm, *this);
    if (!buffer.init(cx))
        return error(cx);

    if (!buffer.verifyRange(f.chunk()))
        return disable(f, "code memory is out of range");

    buffer.link(shapeGuard, slowPathStart);
    buffer.link(outOfBounds, slowPathStart);
    buffer.link(done, fastPathRejoin);

    CodeLocationLabel cs = buffer.finalizeCodeAddendum();
    JaegerSpew(JSpew_PICs, "generated getelem typed array stub at %p\n", cs.executableAddress());

    
    
    JS_ASSERT(!shouldPatchUnconditionalShapeGuard());
    JS_ASSERT(!inlineShapeGuardPatched);

    Repatcher repatcher(f.chunk());
    repatcher.relink(fastPathStart.jumpAtOffset(inlineShapeGuard), cs);
    inlineShapeGuardPatched = true;

    stubsGenerated++;

    
    
    if (stubsGenerated == MAX_GETELEM_IC_STUBS)
        disable(f, "max stubs reached");

    disable(f, "generated typed array stub");

    
    Rooted<jsid> idRoot(cx, id);
    if (!JSObject::getGeneric(cx, obj, obj, idRoot, vp))
        return Lookup_Error;

    return Lookup_Cacheable;
}
#endif 

LookupStatus
GetElementIC::update(VMFrame &f, HandleObject obj, HandleValue v, HandleId id, MutableHandleValue vp)
{
    




    uint32_t dummy;
    if (v.isString() && JSID_IS_ATOM(id) && !JSID_TO_ATOM(id)->isIndex(&dummy)) {
        RootedPropertyName name(f.cx, JSID_TO_ATOM(id)->asPropertyName());
        return attachGetProp(f, obj, v, name, vp);
    }

#if defined JS_METHODJIT_TYPED_ARRAY
    









    if (!f.cx->typeInferenceEnabled() && obj->isTypedArray())
        return attachTypedArray(f, obj, v, id, vp);
#endif

    return disable(f, "unhandled object and key type");
}

void JS_FASTCALL
ic::GetElement(VMFrame &f, ic::GetElementIC *ic)
{
    JSContext *cx = f.cx;

    
    if (!f.regs.sp[-2].isObject()) {
        ic->disable(f, "non-object");
        stubs::GetElem(f);
        return;
    }

    RootedValue idval(cx, f.regs.sp[-1]);

    RecompilationMonitor monitor(cx);

    RootedValue objval(f.cx, f.regs.sp[-2]);
    RootedObject obj(cx, ToObjectFromStack(cx, objval));
    if (!obj)
        THROW();

#if JS_HAS_XML_SUPPORT
    
    
    if (obj->isXML()) {
        ic->disable(f, "XML object");
        stubs::GetElem(f);
        return;
    }
#endif

    Rooted<jsid> id(cx);
    if (idval.isInt32() && INT_FITS_IN_JSID(idval.toInt32())) {
        id = INT_TO_JSID(idval.toInt32());
    } else {
        if (!InternNonIntElementId(cx, obj, idval, &id))
            THROW();
    }

    MutableHandleValue res = MutableHandleValue::fromMarkedLocation(&f.regs.sp[-2]);

    if (!monitor.recompiled() && ic->shouldUpdate(f)) {
#ifdef DEBUG
        f.regs.sp[-2] = MagicValue(JS_GENERIC_MAGIC);
#endif
        LookupStatus status = ic->update(f, obj, idval, id, res);
        if (status != Lookup_Uncacheable && status != Lookup_NoProperty) {
            if (status == Lookup_Error)
                THROW();

            
            JS_ASSERT(!f.regs.sp[-2].isMagic());
            return;
        }
    }

    if (!JSObject::getGeneric(cx, obj, obj, id, res))
        THROW();

#if JS_HAS_NO_SUCH_METHOD
    if (*f.pc() == JSOP_CALLELEM && JS_UNLIKELY(f.regs.sp[-2].isPrimitive())) {
        if (!OnUnknownMethod(cx, obj, idval, res))
            THROW();
    }
#endif
}

#define APPLY_STRICTNESS(f, s)                          \
    (FunctionTemplateConditional(s, f<true>, f<false>))

LookupStatus
SetElementIC::disable(VMFrame &f, const char *reason)
{
    slowCallPatched = true;
    VoidStub stub = APPLY_STRICTNESS(stubs::SetElem, strictMode);
    BaseIC::disable(f, reason, JS_FUNC_TO_DATA_PTR(void *, stub));
    return Lookup_Uncacheable;
}

LookupStatus
SetElementIC::error(JSContext *cx)
{
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

#if defined JS_METHODJIT_TYPED_ARRAY
LookupStatus
SetElementIC::attachTypedArray(VMFrame &f, JSObject *obj, int32_t key)
{
    
    JS_ASSERT(!inlineShapeGuardPatched);

    JSContext *cx = f.cx;
    MJITInstrumentation sps(&f.cx->runtime->spsProfiler);
    Assembler masm(&sps, &f);

    
    masm.rematPayload(StateRemat::FromInt32(objRemat), objReg);

    
    Jump shapeGuard = masm.guardShape(objReg, obj);

    
    Jump outOfBounds;
    Address typedArrayLength = masm.payloadOf(Address(objReg, TypedArray::lengthOffset()));
    if (hasConstantKey)
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, typedArrayLength, Imm32(keyValue));
    else
        outOfBounds = masm.branch32(Assembler::BelowOrEqual, typedArrayLength, keyReg);

    
    masm.loadPtr(Address(objReg, TypedArray::dataOffset()), objReg);

    if (!masm.supportsFloatingPoint() &&
        (TypedArray::type(obj) == TypedArray::TYPE_FLOAT32 ||
         TypedArray::type(obj) == TypedArray::TYPE_FLOAT64))
    {
        return disable(f, "fpu not supported");
    }

    int shift = TypedArray::slotWidth(obj);
    if (hasConstantKey) {
        Address addr(objReg, keyValue * shift);
        if (!StoreToTypedArray(cx, masm, obj, addr, vr, volatileMask))
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
        if (!StoreToTypedArray(cx, masm, obj, addr, vr, volatileMask))
            return error(cx);
    }

    Jump done = masm.jump();

    
    
    
    JS_ASSERT(!execPool);
    LinkerHelper buffer(masm, JSC::JAEGER_CODE);
    execPool = buffer.init(cx);
    if (!execPool)
        return error(cx);

    if (!buffer.verifyRange(f.chunk()))
        return disable(f, "code memory is out of range");

    
    buffer.link(shapeGuard, slowPathStart);
    buffer.link(outOfBounds, fastPathRejoin);
    buffer.link(done, fastPathRejoin);
    masm.finalize(buffer);

    CodeLocationLabel cs = buffer.finalizeCodeAddendum();
    JaegerSpew(JSpew_PICs, "generated setelem typed array stub at %p\n", cs.executableAddress());

    Repatcher repatcher(f.chunk());
    repatcher.relink(fastPathStart.jumpAtOffset(inlineShapeGuard), cs);
    inlineShapeGuardPatched = true;

    stubsGenerated++;

    
    
    if (stubsGenerated == MAX_GETELEM_IC_STUBS)
        disable(f, "max stubs reached");

    disable(f, "generated typed array stub");

    return Lookup_Cacheable;
}
#endif 

LookupStatus
SetElementIC::update(VMFrame &f, const Value &objval, const Value &idval)
{
    if (!objval.isObject())
        return disable(f, "primitive lval");
    if (!idval.isInt32())
        return disable(f, "non-int32 key");

    JSObject *obj = &objval.toObject();
    int32_t key = idval.toInt32();

#if defined JS_METHODJIT_TYPED_ARRAY
    
    if (!f.cx->typeInferenceEnabled() && obj->isTypedArray())
        return attachTypedArray(f, obj, key);
#endif

    return disable(f, "unsupported object type");
}

bool
SetElementIC::shouldUpdate(VMFrame &f)
{
    if (!hit) {
        hit = true;
        spew(f, "ignored", "first hit");
        return false;
    }
#ifdef JSGC_INCREMENTAL_MJ
    JS_ASSERT(!f.cx->compartment->compileBarriers());
#endif
    JS_ASSERT(stubsGenerated < MAX_PIC_STUBS);
    return true;
}

template<JSBool strict>
void JS_FASTCALL
ic::SetElement(VMFrame &f, ic::SetElementIC *ic)
{
    if (ic->shouldUpdate(f)) {
        LookupStatus status = ic->update(f, f.regs.sp[-3], f.regs.sp[-2]);
        if (status == Lookup_Error)
            THROW();
    }

    stubs::SetElem<strict>(f);
}

template void JS_FASTCALL ic::SetElement<true>(VMFrame &f, SetElementIC *ic);
template void JS_FASTCALL ic::SetElement<false>(VMFrame &f, SetElementIC *ic);

#endif 

