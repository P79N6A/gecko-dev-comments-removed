








































#include "jsscope.h"

#include "CodeGenerator.h"
#include "Ion.h"
#include "IonCaches.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "VMFunctions.h"

#include "jsinterpinlines.h"

#include "vm/Stack.h"
#include "IonFrames-inl.h"

using namespace js;
using namespace js::ion;

void
CodeLocationJump::repoint(IonCode *code, MacroAssembler *masm)
{
    JS_ASSERT(!absolute_);
    size_t new_off = (size_t)raw_;
#ifdef JS_SMALL_BRANCH
    size_t jumpTableEntryOffset = reinterpret_cast<size_t>(jumpTableEntry_);
#endif
    if (masm != NULL) {
#ifdef JS_CPU_X64
        JS_ASSERT((uint64_t)raw_ <= UINT32_MAX);
#endif
        new_off = masm->actualOffset((uintptr_t)raw_);
#ifdef JS_SMALL_BRANCH
        jumpTableEntryOffset = masm->actualIndex(jumpTableEntryOffset);
#endif
    }
    raw_ = code->raw() + new_off;
#ifdef JS_SMALL_BRANCH
    jumpTableEntry_ = Assembler::PatchableJumpAddress(code, (size_t) jumpTableEntryOffset);
#endif
    absolute_ = true;
}

void
CodeLocationLabel::repoint(IonCode *code, MacroAssembler *masm)
{
     JS_ASSERT(!absolute_);
     size_t new_off = (size_t)raw_;
     if (masm != NULL) {
#ifdef JS_CPU_X64
        JS_ASSERT((uint64_t)raw_ <= UINT32_MAX);
#endif
        new_off = masm->actualOffset((uintptr_t)raw_);
     }
     JS_ASSERT(new_off < code->instructionsSize());

     raw_ = code->raw() + new_off;
     absolute_ = true;
}

void
CodeOffsetLabel::fixup(MacroAssembler *masm)
{
     offset_ = masm->actualOffset(offset_);
}

void
CodeOffsetJump::fixup(MacroAssembler *masm)
{
     offset_ = masm->actualOffset(offset_);
#ifdef JS_SMALL_BRANCH
     jumpTableIndex_ = masm->actualIndex(jumpTableIndex_);
#endif
}

static const size_t MAX_STUBS = 16;

static void
GeneratePrototypeGuards(JSContext *cx, MacroAssembler &masm, JSObject *obj, JSObject *holder,
                        Register objectReg, Register scratchReg, Label *failures)
{
    JS_ASSERT(obj != holder);

    if (obj->hasUncacheableProto()) {
        
        
        masm.loadPtr(Address(objectReg, JSObject::offsetOfType()), scratchReg);
        Address proto(scratchReg, offsetof(types::TypeObject, proto));
        masm.branchPtr(Assembler::NotEqual, proto, ImmGCPtr(obj->getProto()), failures);
    }

    JSObject *pobj = obj->getProto();
    while (pobj != holder) {
        if (pobj->hasUncacheableProto()) {
            if (pobj->hasSingletonType()) {
                types::TypeObject *type = pobj->getType(cx);
                masm.movePtr(ImmGCPtr(type), scratchReg);
                Address proto(scratchReg, offsetof(types::TypeObject, proto));
                masm.branchPtr(Assembler::NotEqual, proto, ImmGCPtr(obj->getProto()), failures);
            } else {
                masm.movePtr(ImmGCPtr(pobj), scratchReg);
                Address objType(scratchReg, JSObject::offsetOfType());
                masm.branchPtr(Assembler::NotEqual, objType, ImmGCPtr(pobj->type()), failures);
            }
        }
        pobj = pobj->getProto();
    }
}

struct GetNativePropertyStub
{
    CodeOffsetJump exitOffset;
    CodeOffsetJump rejoinOffset;

    void generate(JSContext *cx, MacroAssembler &masm, JSObject *obj, JSObject *holder,
                  const Shape *shape, Register object, TypedOrValueRegister output,
                  RepatchLabel *failures, Label *nonRepatchFailures = NULL)
    {
        
        
        
        bool multipleFailureJumps = (nonRepatchFailures != NULL) && nonRepatchFailures->used();
        exitOffset = masm.branchPtrWithPatch(Assembler::NotEqual,
                                             Address(object, JSObject::offsetOfShape()),
                                             ImmGCPtr(obj->lastProperty()),
                                             failures);

        bool restoreScratch = false;
        Register scratchReg = Register::FromCode(0); 

        
        
        
        Label prototypeFailures;
        if (obj != holder || !holder->isFixedSlot(shape->slot())) {
            if (output.hasValue()) {
                scratchReg = output.valueReg().scratchReg();
            } else if (output.type() == MIRType_Double) {
                scratchReg = object;
                masm.push(scratchReg);
                restoreScratch = true;
            } else {
                scratchReg = output.typedReg().gpr();
            }
        }

        Register holderReg;
        if (obj != holder) {
            
            GeneratePrototypeGuards(cx, masm, obj, holder, object, scratchReg, &prototypeFailures);

            
            holderReg = scratchReg;
            masm.movePtr(ImmGCPtr(holder), holderReg);
            masm.branchPtr(Assembler::NotEqual,
                           Address(holderReg, JSObject::offsetOfShape()),
                           ImmGCPtr(holder->lastProperty()),
                           &prototypeFailures);
        } else {
            holderReg = object;
        }

        if (holder->isFixedSlot(shape->slot())) {
            Address addr(holderReg, JSObject::getFixedSlotOffset(shape->slot()));
            masm.loadTypedOrValue(addr, output);
        } else {
            masm.loadPtr(Address(holderReg, JSObject::offsetOfSlots()), scratchReg);

            Address addr(scratchReg, holder->dynamicSlotIndex(shape->slot()) * sizeof(Value));
            masm.loadTypedOrValue(addr, output);
        }

        if (restoreScratch)
            masm.pop(scratchReg);

        RepatchLabel rejoin_;
        rejoinOffset = masm.jumpWithPatch(&rejoin_);
        masm.bind(&rejoin_);

        if (obj != holder || multipleFailureJumps) {
            masm.bind(&prototypeFailures);
            if (restoreScratch)
                masm.pop(scratchReg);
            masm.bind(failures);
            if (multipleFailureJumps)
                masm.bind(nonRepatchFailures);
            RepatchLabel exit_;
            exitOffset = masm.jumpWithPatch(&exit_);
            masm.bind(&exit_);
        } else {
            masm.bind(failures);
        }
    }
};

bool
IonCacheGetProperty::attachNative(JSContext *cx, JSObject *obj, JSObject *holder, const Shape *shape)
{
    MacroAssembler masm;
    RepatchLabel failures;

    GetNativePropertyStub getprop;
    getprop.generate(cx, masm, obj, holder, shape, object(), output(), &failures);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    getprop.rejoinOffset.fixup(&masm);
    getprop.exitOffset.fixup(&masm);

    CodeLocationJump rejoinJump(code, getprop.rejoinOffset);
    CodeLocationJump exitJump(code, getprop.exitOffset);
    CodeLocationJump lastJump_ = lastJump();
    PatchJump(lastJump_, CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated native GETPROP stub at %p %s", code->raw(),
            idempotent() ? "(idempotent)" : "(not idempotent)");

    return true;
}

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

static bool
IsCacheableGetProp(JSObject *obj, JSObject *holder, const Shape *shape)
{
    return (shape &&
            IsCacheableProtoChain(obj, holder) &&
            shape->hasSlot() &&
            shape->hasDefaultGetter());
}

static bool
TryAttachNativeStub(JSContext *cx, IonCacheGetProperty &cache, HandleObject obj,
                    HandlePropertyName name, bool *isCacheableNative)
{
    JS_ASSERT(!*isCacheableNative);

    if (!obj->isNative())
        return true;

    
    
    
    if (cache.idempotent() && !obj->hasIdempotentProtoChain())
        return true;

    RootedShape shape(cx);
    RootedObject holder(cx);
    if (!obj->lookupProperty(cx, name, &holder, &shape))
        return false;

    if (!IsCacheableGetProp(obj, holder, shape))
        return true;

    *isCacheableNative = true;

    if (cache.stubCount() < MAX_STUBS) {
        cache.incrementStubCount();

        if (!cache.attachNative(cx, obj, holder, shape))
            return false;
    }

    return true;
}

bool
js::ion::GetPropertyCache(JSContext *cx, size_t cacheIndex, HandleObject obj, Value *vp)
{
    JSScript *topScript = GetTopIonJSScript(cx);
    IonScript *ion = topScript->ionScript();

    IonCacheGetProperty &cache = ion->getCache(cacheIndex).toGetProperty();
    RootedPropertyName name(cx, cache.name());

    JSScript *script;
    jsbytecode *pc;
    cache.getScriptedLocation(&script, &pc);

    
    AutoDetectInvalidation adi(cx, vp, ion);

    
    if (cache.idempotent())
        adi.disable();

    
    
    
    bool isCacheableNative = false;
    if (!TryAttachNativeStub(cx, cache, obj, name, &isCacheableNative))
        return false;

    if (cache.idempotent() && !isCacheableNative) {
        
        
        
        
        
        
        IonSpew(IonSpew_InlineCaches, "Invalidating from idempotent cache %s:%d",
                topScript->filename, topScript->lineno);

        topScript->invalidatedIdempotentCache = true;

        return Invalidate(cx, topScript);
    }

    RootedId id(cx, NameToId(name));
    if (obj->getOps()->getProperty) {
        if (!GetPropertyGenericMaybeCallXML(cx, JSOp(*pc), obj, id, vp))
            return false;
    } else {
        if (!GetPropertyHelper(cx, obj, id, 0, vp))
            return false;
    }

    if (!cache.idempotent()) {
        
        

#if JS_HAS_NO_SUCH_METHOD
        
        if (JSOp(*pc) == JSOP_CALLPROP && JS_UNLIKELY(vp->isPrimitive())) {
            if (!OnUnknownMethod(cx, obj, IdToValue(id), vp))
                return false;
        }
#endif

        
        types::TypeScript::Monitor(cx, script, pc, *vp);
    }

    return true;
}

void
IonCache::updateBaseAddress(IonCode *code, MacroAssembler &masm)
{
    initialJump_.repoint(code, &masm);
    lastJump_.repoint(code, &masm);
    cacheLabel_.repoint(code, &masm);
}

void
IonCache::reset()
{
    PatchJump(initialJump_, cacheLabel_);

    this->stubCount_ = 0;
    this->lastJump_ = initialJump_;
}

bool
IonCacheSetProperty::attachNativeExisting(JSContext *cx, JSObject *obj, const Shape *shape)
{
    MacroAssembler masm;

    RepatchLabel exit_;
    CodeOffsetJump exitOffset =
        masm.branchPtrWithPatch(Assembler::NotEqual,
                                Address(object(), JSObject::offsetOfShape()),
                                ImmGCPtr(obj->lastProperty()),
                                &exit_);
    masm.bind(&exit_);

    if (obj->isFixedSlot(shape->slot())) {
        Address addr(object(), JSObject::getFixedSlotOffset(shape->slot()));

        if (cx->compartment->needsBarrier())
            masm.callPreBarrier(addr, MIRType_Value);

        masm.storeConstantOrRegister(value(), addr);
    } else {
        Register slotsReg = object();
        masm.loadPtr(Address(object(), JSObject::offsetOfSlots()), slotsReg);

        Address addr(slotsReg, obj->dynamicSlotIndex(shape->slot()) * sizeof(Value));

        if (cx->compartment->needsBarrier())
            masm.callPreBarrier(addr, MIRType_Value);

        masm.storeConstantOrRegister(value(), addr);
    }

    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    rejoinOffset.fixup(&masm);
    exitOffset.fixup(&masm);

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump exitJump(code, exitOffset);
    CodeLocationJump lastJump_ = lastJump();
    PatchJump(lastJump_, CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated native SETPROP setting case stub at %p", code->raw());

    return true;
}

bool
IonCacheSetProperty::attachNativeAdding(JSContext *cx, JSObject *obj, const Shape *oldShape, const Shape *newShape,
                                        const Shape *propShape)
{
    MacroAssembler masm;

    Label failures;

    
    masm.branchTestObjShape(Assembler::NotEqual, object(), oldShape, &failures);

    Label protoFailures;
    masm.push(object());    

    JSObject *proto = obj->getProto();
    Register protoReg = object();
    while (proto) {
        Shape *protoShape = proto->lastProperty();

        
        masm.loadPtr(Address(protoReg, JSObject::offsetOfType()), protoReg);
        masm.loadPtr(Address(protoReg, offsetof(types::TypeObject, proto)), protoReg);

        
        masm.branchTestPtr(Assembler::Zero, protoReg, protoReg, &protoFailures);
        masm.branchTestObjShape(Assembler::NotEqual, protoReg, protoShape, &protoFailures);

        proto = proto->getProto();
    }

    masm.pop(object());     

    
    masm.storePtr(ImmGCPtr(newShape), Address(object(), JSObject::offsetOfShape()));

    
    if (obj->isFixedSlot(propShape->slot())) {
        Address addr(object(), JSObject::getFixedSlotOffset(propShape->slot()));
        masm.storeConstantOrRegister(value(), addr);
    } else {
        Register slotsReg = object();

        masm.loadPtr(Address(object(), JSObject::offsetOfSlots()), slotsReg);

        Address addr(slotsReg, obj->dynamicSlotIndex(propShape->slot()) * sizeof(Value));
        masm.storeConstantOrRegister(value(), addr);
    }

    
    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    
    masm.bind(&protoFailures);
    masm.pop(object());
    masm.bind(&failures);

    RepatchLabel exit_;
    CodeOffsetJump exitOffset = masm.jumpWithPatch(&exit_);
    masm.bind(&exit_);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    rejoinOffset.fixup(&masm);
    exitOffset.fixup(&masm);

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump exitJump(code, exitOffset);
    CodeLocationJump lastJump_ = lastJump();
    PatchJump(lastJump_, CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated native SETPROP adding case stub at %p", code->raw());

    return true;
}

static bool
IsPropertyInlineable(JSObject *obj, IonCacheSetProperty &cache)
{
    
    
    if (cache.stubCount() >= MAX_STUBS)
        return false;

    if (!obj->isNative())
        return false;

    if (obj->watched())
        return false;

    return true;
}

static bool
IsPropertySetInlineable(JSContext *cx, HandleObject obj, JSAtom *atom, jsid *pId, const Shape **pShape)
{
    jsid id = AtomToId(atom);
    *pId = id;

    const Shape *shape = obj->nativeLookup(cx, id);
    *pShape = shape;

    if (!shape)
        return false;

    if (!shape->hasSlot())
        return false;

    if (!shape->hasDefaultSetter())
        return false;

    if (!shape->writable())
        return false;

    return true;
}

static bool
IsPropertyAddInlineable(JSContext *cx, HandleObject obj, jsid id, uint32_t oldSlots,
                       const Shape **pShape)
{
    
    if (*pShape)
        return false;

    const Shape *shape = obj->nativeLookup(cx, id);
    if (!shape || shape->inDictionary() || !shape->hasSlot() || !shape->hasDefaultSetter())
        return false;

    
    if (obj->getClass()->resolve != JS_ResolveStub)
        return false;

    
    
    
    for (JSObject *proto = obj->getProto(); proto; proto = proto->getProto()) {
        
        if (!proto->isNative())
            return false;

        
        const Shape *protoShape = proto->nativeLookup(cx, id);
        if (protoShape && !protoShape->hasDefaultSetter())
            return false;

        
        
        if (proto->getClass()->resolve != JS_ResolveStub)
             return false;
    }

    
    
    
    if (obj->numDynamicSlots() != oldSlots)
        return false;

    *pShape = shape;
    return true;
}

bool
js::ion::SetPropertyCache(JSContext *cx, size_t cacheIndex, HandleObject obj, HandleValue value,
                          bool isSetName)
{
    IonScript *ion = GetTopIonJSScript(cx)->ion;
    IonCacheSetProperty &cache = ion->getCache(cacheIndex).toSetProperty();
    RootedPropertyName name(cx, cache.name());
    jsid id;
    const Shape *shape = NULL;

    bool inlinable = IsPropertyInlineable(obj, cache);
    if (inlinable && IsPropertySetInlineable(cx, obj, name, &id, &shape)) {
        cache.incrementStubCount();
        if (!cache.attachNativeExisting(cx, obj, shape))
            return false;
    }

    uint32_t oldSlots = obj->numDynamicSlots();
    const Shape *oldShape = obj->lastProperty();

    
    if (!SetProperty(cx, obj, name, value, cache.strict(), isSetName))
        return false;

    
    
    if (inlinable && IsPropertyAddInlineable(cx, obj, id, oldSlots, &shape)) {
        const Shape *newShape = obj->lastProperty();
        cache.incrementStubCount();
        if (!cache.attachNativeAdding(cx, obj, oldShape, newShape, shape))
            return false;
    }

    return true;
}

bool
IonCacheGetElement::attachGetProp(JSContext *cx, JSObject *obj, const Value &idval, PropertyName *name,
                                  Value *res)
{
    RootedObject holder(cx);
    RootedShape shape(cx);
    if (!obj->lookupProperty(cx, name, &holder, &shape))
        return false;

    if (!IsCacheableGetProp(obj, holder, shape)) {
        IonSpew(IonSpew_InlineCaches, "GETELEM uncacheable property");
        return true;
    }

    JS_ASSERT(idval.isString());

    RepatchLabel failures;
    Label nonRepatchFailures;
    MacroAssembler masm;

    
    ValueOperand val = index().reg().valueReg();
    masm.branchTestValue(Assembler::NotEqual, val, idval, &nonRepatchFailures);

    GetNativePropertyStub getprop;
    getprop.generate(cx, masm, obj, holder, shape, object(), output(), &failures, &nonRepatchFailures);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    getprop.rejoinOffset.fixup(&masm);
    getprop.exitOffset.fixup(&masm);

    CodeLocationJump rejoinJump(code, getprop.rejoinOffset);
    CodeLocationJump exitJump(code, getprop.exitOffset);
    CodeLocationJump lastJump_ = lastJump();
    PatchJump(lastJump_, CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated GETELEM property stub at %p", code->raw());
    return true;
}


static inline Shape *
GetDenseArrayShape(JSContext *cx, JSObject *globalObj)
{
    JSObject *proto = globalObj->global().getOrCreateArrayPrototype(cx);
    if (!proto)
        return NULL;

    return EmptyShape::getInitialShape(cx, &ArrayClass, proto,
                                       proto->getParent(), gc::FINALIZE_OBJECT0);
}

bool
IonCacheGetElement::attachDenseArray(JSContext *cx, JSObject *obj, const Value &idval, Value *res)
{
    JS_ASSERT(obj->isDenseArray());
    JS_ASSERT(idval.isInt32());

    Label failures;
    MacroAssembler masm;

    
    RootedShape shape(cx, GetDenseArrayShape(cx, &script->global()));
    if (!shape)
        return false;
    masm.branchTestObjShape(Assembler::NotEqual, object(), shape, &failures);

    
    ValueOperand val = index().reg().valueReg();
    masm.branchTestInt32(Assembler::NotEqual, val, &failures);

    
    masm.push(object());
    masm.loadPtr(Address(object(), JSObject::offsetOfElements()), object());

    
    ValueOperand out = output().valueReg();
    Register scratchReg = out.scratchReg();
    masm.unboxInt32(val, scratchReg);

    Label hole;

    
    Address initLength(object(), ObjectElements::offsetOfInitializedLength());
    masm.branch32(Assembler::BelowOrEqual, initLength, scratchReg, &hole);

    
    masm.loadValue(BaseIndex(object(), scratchReg, TimesEight), out);

    
    masm.branchTestMagic(Assembler::Equal, out, &hole);

    masm.pop(object());
    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    
    masm.bind(&hole);
    masm.pop(object());
    masm.bind(&failures);

    RepatchLabel exit_;
    CodeOffsetJump exitOffset = masm.jumpWithPatch(&exit_);
    masm.bind(&exit_);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    rejoinOffset.fixup(&masm);
    exitOffset.fixup(&masm);

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump exitJump(code, exitOffset);
    CodeLocationJump lastJump_ = lastJump();
    PatchJump(lastJump_, CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    setHasDenseArrayStub();
    IonSpew(IonSpew_InlineCaches, "Generated GETELEM dense array stub at %p", code->raw());

    return true;
}

bool
js::ion::GetElementCache(JSContext *cx, size_t cacheIndex, JSObject *obj, const Value &idval, Value *res)
{
    IonScript *ion = GetTopIonJSScript(cx)->ionScript();

    IonCacheGetElement &cache = ion->getCache(cacheIndex).toGetElement();

    
    AutoDetectInvalidation adi(cx, res, ion);

    RootedId id(cx);
    if (!FetchElementId(cx, obj, idval, id.address(), res))
        return false;

    if (cache.stubCount() < MAX_STUBS) {
        if (obj->isNative() && cache.monitoredResult()) {
            cache.incrementStubCount();

            uint32_t dummy;
            if (idval.isString() && JSID_IS_ATOM(id) && !JSID_TO_ATOM(id)->isIndex(&dummy)) {
                if (!cache.attachGetProp(cx, obj, idval, JSID_TO_ATOM(id)->asPropertyName(), res))
                    return false;
            }
        } else if (!cache.hasDenseArrayStub() && obj->isDenseArray() && idval.isInt32()) {
            
            cache.incrementStubCount();

            if (!cache.attachDenseArray(cx, obj, idval, res))
                return false;
        }
    }

    JSScript *script;
    jsbytecode *pc;
    cache.getScriptedLocation(&script, &pc);

    if (!GetElementOperation(cx, JSOp(*pc), ObjectValue(*obj), idval, res))
        return false;

    types::TypeScript::Monitor(cx, script, pc, *res);
    return true;
}

bool
IonCacheBindName::attachGlobal(JSContext *cx, JSObject *scopeChain)
{
    JS_ASSERT(scopeChain->isGlobal());

    MacroAssembler masm;

    
    RepatchLabel exit_;
    CodeOffsetJump exitOffset = masm.branchPtrWithPatch(Assembler::NotEqual, scopeChainReg(),
                                                        ImmGCPtr(scopeChain), &exit_);
    masm.bind(&exit_);
    masm.movePtr(ImmGCPtr(scopeChain), outputReg());

    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    rejoinOffset.fixup(&masm);
    exitOffset.fixup(&masm);

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump exitJump(code, exitOffset);
    CodeLocationJump lastJump_ = lastJump();
    PatchJump(lastJump_, CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated BINDNAME global stub at %p", code->raw());
    return true;
}

static inline void
GenerateScopeChainGuard(MacroAssembler &masm, JSObject *scopeObj,
                        Register scopeObjReg, Shape *shape, Label *failures)
{
    if (scopeObj->isCall()) {
        
        
        
        CallObject *callObj = &scopeObj->asCall();
        if (!callObj->isForEval()) {
            JSFunction *fun = &callObj->callee();
            JSScript *script = fun->script();
            if (!script->bindings.extensibleParents() && !script->funHasExtensibleScope)
                return;
        }
    } else if (scopeObj->isGlobal()) {
        
        
        
        if (shape && !shape->configurable())
            return;
    }

    Address shapeAddr(scopeObjReg, JSObject::offsetOfShape());
    masm.branchPtr(Assembler::NotEqual, shapeAddr, ImmGCPtr(scopeObj->lastProperty()), failures);
}

static void
GenerateScopeChainGuards(MacroAssembler &masm, JSObject *scopeChain, JSObject *holder,
                         Register outputReg, Label *failures)
{
    JSObject *tobj = scopeChain;

    
    
    while (true) {
        JS_ASSERT(IsCacheableNonGlobalScope(tobj) || tobj->isGlobal());

        GenerateScopeChainGuard(masm, tobj, outputReg, NULL, failures);
        if (tobj == holder)
            break;

        
        tobj = &tobj->asScope().enclosingScope();
        masm.extractObject(Address(outputReg, ScopeObject::offsetOfEnclosingScope()), outputReg);
    }
}

bool
IonCacheBindName::attachNonGlobal(JSContext *cx, JSObject *scopeChain, JSObject *holder)
{
    JS_ASSERT(IsCacheableNonGlobalScope(scopeChain));

    MacroAssembler masm;

    
    RepatchLabel failures;
    Label nonRepatchFailures;
    CodeOffsetJump exitOffset =
        masm.branchPtrWithPatch(Assembler::NotEqual,
                                Address(scopeChainReg(), JSObject::offsetOfShape()),
                                ImmGCPtr(scopeChain->lastProperty()),
                                &failures);

    if (holder != scopeChain) {
        JSObject *parent = &scopeChain->asScope().enclosingScope();
        masm.extractObject(Address(scopeChainReg(), ScopeObject::offsetOfEnclosingScope()), outputReg());

        GenerateScopeChainGuards(masm, parent, holder, outputReg(), &nonRepatchFailures);
    } else {
        masm.movePtr(scopeChainReg(), outputReg());
    }

    
    
    RepatchLabel rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    
    masm.bind(&failures);
    masm.bind(&nonRepatchFailures);
    if (holder != scopeChain) {
        RepatchLabel exit_;
        exitOffset = masm.jumpWithPatch(&exit_);
        masm.bind(&exit_);
    }

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    rejoinOffset.fixup(&masm);
    exitOffset.fixup(&masm);

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump exitJump(code, exitOffset);
    CodeLocationJump lastJump_ = lastJump();
    PatchJump(lastJump_, CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated BINDNAME non-global stub at %p", code->raw());
    return true;
}

static bool
IsCacheableScopeChain(JSObject *scopeChain, JSObject *holder)
{
    while (true) {
        if (!IsCacheableNonGlobalScope(scopeChain)) {
            IonSpew(IonSpew_InlineCaches, "Non-cacheable object on scope chain");
            return false;
        }

        if (scopeChain == holder)
            return true;

        scopeChain = &scopeChain->asScope().enclosingScope();
        if (!scopeChain) {
            IonSpew(IonSpew_InlineCaches, "Scope chain indirect hit");
            return false;
        }
    }

    JS_NOT_REACHED("Shouldn't get here");
    return false;
}

JSObject *
js::ion::BindNameCache(JSContext *cx, size_t cacheIndex, HandleObject scopeChain)
{
    IonScript *ion = GetTopIonJSScript(cx)->ionScript();
    IonCacheBindName &cache = ion->getCache(cacheIndex).toBindName();
    HandlePropertyName name = cache.name();

    JSObject *holder;
    if (scopeChain->isGlobal()) {
        holder = scopeChain;
    } else {
        holder = FindIdentifierBase(cx, scopeChain, name);
        if (!holder)
            return NULL;
    }

    
    
    if (cache.stubCount() < MAX_STUBS) {
        cache.incrementStubCount();

        if (scopeChain->isGlobal()) {
            if (!cache.attachGlobal(cx, scopeChain))
                return NULL;
        } else if (IsCacheableScopeChain(scopeChain, holder)) {
            if (!cache.attachNonGlobal(cx, scopeChain, holder))
                return NULL;
        } else {
            IonSpew(IonSpew_InlineCaches, "BINDNAME uncacheable scope chain");
        }
    }

    return holder;
}

bool
IonCacheName::attach(JSContext *cx, HandleObject scopeChain, HandleObject holder, Shape *shape)
{
    MacroAssembler masm;
    Label failures;

    Register scratchReg = outputReg().valueReg().scratchReg();

    masm.mov(scopeChainReg(), scratchReg);
    GenerateScopeChainGuards(masm, scopeChain, holder, scratchReg, &failures);

    unsigned slot = shape->slot();
    if (holder->isFixedSlot(slot)) {
        Address addr(scratchReg, JSObject::getFixedSlotOffset(slot));
        masm.loadTypedOrValue(addr, outputReg());
    } else {
        masm.loadPtr(Address(scratchReg, JSObject::offsetOfSlots()), scratchReg);

        Address addr(scratchReg, holder->dynamicSlotIndex(slot) * sizeof(Value));
        masm.loadTypedOrValue(addr, outputReg());
    }

    RepatchLabel rejoin;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin);
    masm.bind(&rejoin);

    CodeOffsetJump exitOffset;

    if (failures.used()) {
        masm.bind(&failures);

        RepatchLabel exit;
        exitOffset = masm.jumpWithPatch(&exit);
        masm.bind(&exit);
    }

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    rejoinOffset.fixup(&masm);
    if (failures.bound())
        exitOffset.fixup(&masm);

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump lastJump_ = lastJump();
    PatchJump(lastJump_, CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    if (failures.bound()) {
        CodeLocationJump exitJump(code, exitOffset);
        PatchJump(exitJump, cacheLabel());
        updateLastJump(exitJump);
    }

    IonSpew(IonSpew_InlineCaches, "Generated NAME stub at %p", code->raw());
    return true;
}

static bool
IsCacheableName(JSContext *cx, HandleObject scopeChain, HandleObject obj, HandleObject holder,
                HandleShape shape)
{
    if (!shape)
        return false;
    if (!obj->isNative())
        return false;
    if (obj != holder)
        return false;

    if (obj->isGlobal()) {
        
        if (!IsCacheableGetProp(obj, holder, shape))
            return false;
    } else if (obj->isCall()) {
        if (!shape->hasDefaultGetter())
            return false;
    } else {
        
        return false;
    }

    RootedObject obj2(cx, scopeChain);
    while (obj2) {
        if (!IsCacheableNonGlobalScope(obj2) && !obj2->isGlobal())
            return false;

        
        if (obj2->isGlobal() || obj2 == obj)
            break;

        obj2 = obj2->enclosingScope();
    }

    return obj == obj2;
}

bool
js::ion::GetNameCache(JSContext *cx, size_t cacheIndex, HandleObject scopeChain, Value *vp)
{
    IonScript *ion = GetTopIonJSScript(cx)->ionScript();

    IonCacheName &cache = ion->getCache(cacheIndex).toName();
    RootedPropertyName name(cx, cache.name());

    JSScript *script;
    jsbytecode *pc;
    cache.getScriptedLocation(&script, &pc);

    RootedObject obj(cx);
    RootedObject holder(cx);
    RootedShape shape(cx);
    if (!FindProperty(cx, name, scopeChain, &obj, &holder, &shape))
        return false;

    if (cache.stubCount() < MAX_STUBS &&
        IsCacheableName(cx, scopeChain, obj, holder, shape))
    {
        if (!cache.attach(cx, scopeChain, obj, shape))
            return false;
        cache.incrementStubCount();
    }

    if (cache.isTypeOf()) {
        if (!FetchName<true>(cx, obj, holder, name, shape, vp))
            return false;
    } else {
        if (!FetchName<false>(cx, obj, holder, name, shape, vp))
            return false;
    }

    
    types::TypeScript::Monitor(cx, script, pc, *vp);

    return true;
}

