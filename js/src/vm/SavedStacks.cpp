





#include "vm/SavedStacks.h"

#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Maybe.h"

#include <math.h>

#include "jsapi.h"
#include "jscompartment.h"
#include "jsfriendapi.h"
#include "jshashutil.h"
#include "jsmath.h"
#include "jsnum.h"
#include "jsscript.h"
#include "prmjtime.h"

#include "gc/Marking.h"
#include "gc/Rooting.h"
#include "js/Vector.h"
#include "vm/Debugger.h"
#include "vm/StringBuffer.h"

#include "jscntxtinlines.h"

#include "vm/NativeObject-inl.h"

using mozilla::AddToHash;
using mozilla::DebugOnly;
using mozilla::HashString;
using mozilla::Maybe;

namespace js {




const unsigned ASYNC_STACK_MAX_FRAME_COUNT = 60;

struct SavedFrame::Lookup {
    Lookup(JSAtom* source, uint32_t line, uint32_t column,
           JSAtom* functionDisplayName, JSAtom* asyncCause, SavedFrame* parent,
           JSPrincipals* principals)
      : source(source),
        line(line),
        column(column),
        functionDisplayName(functionDisplayName),
        asyncCause(asyncCause),
        parent(parent),
        principals(principals)
    {
        MOZ_ASSERT(source);
    }

    explicit Lookup(SavedFrame& savedFrame)
      : source(savedFrame.getSource()),
        line(savedFrame.getLine()),
        column(savedFrame.getColumn()),
        functionDisplayName(savedFrame.getFunctionDisplayName()),
        asyncCause(savedFrame.getAsyncCause()),
        parent(savedFrame.getParent()),
        principals(savedFrame.getPrincipals())
    {
        MOZ_ASSERT(source);
    }

    JSAtom*      source;
    uint32_t     line;
    uint32_t     column;
    JSAtom*      functionDisplayName;
    JSAtom*      asyncCause;
    SavedFrame*  parent;
    JSPrincipals* principals;

    void trace(JSTracer* trc) {
        TraceManuallyBarrieredEdge(trc, &source, "SavedFrame::Lookup::source");
        if (functionDisplayName) {
            TraceManuallyBarrieredEdge(trc, &functionDisplayName,
                                       "SavedFrame::Lookup::functionDisplayName");
        }
        if (asyncCause) {
            TraceManuallyBarrieredEdge(trc, &asyncCause,
                                       "SavedFrame::Lookup::asyncCause");
        }
        if (parent) {
            gc::MarkObjectUnbarriered(trc, &parent,
                                      "SavedFrame::Lookup::parent");
        }
    }
};

class MOZ_STACK_CLASS SavedFrame::AutoLookupVector : public JS::CustomAutoRooter {
  public:
    explicit AutoLookupVector(JSContext* cx)
      : JS::CustomAutoRooter(cx),
        lookups(cx)
    { }

    typedef Vector<Lookup, 20> LookupVector;
    inline LookupVector* operator->() { return &lookups; }
    inline HandleLookup operator[](size_t i) { return HandleLookup(lookups[i]); }

  private:
    LookupVector lookups;

    virtual void trace(JSTracer* trc) {
        for (size_t i = 0; i < lookups.length(); i++)
            lookups[i].trace(trc);
    }
};

 HashNumber
SavedFrame::HashPolicy::hash(const Lookup& lookup)
{
    JS::AutoCheckCannotGC nogc;
    
    
    
    return AddToHash(lookup.line,
                     lookup.column,
                     lookup.source,
                     lookup.functionDisplayName,
                     lookup.asyncCause,
                     SavedFramePtrHasher::hash(lookup.parent),
                     JSPrincipalsPtrHasher::hash(lookup.principals));
}

 bool
SavedFrame::HashPolicy::match(SavedFrame* existing, const Lookup& lookup)
{
    if (existing->getLine() != lookup.line)
        return false;

    if (existing->getColumn() != lookup.column)
        return false;

    if (existing->getParent() != lookup.parent)
        return false;

    if (existing->getPrincipals() != lookup.principals)
        return false;

    JSAtom* source = existing->getSource();
    if (source != lookup.source)
        return false;

    JSAtom* functionDisplayName = existing->getFunctionDisplayName();
    if (functionDisplayName != lookup.functionDisplayName)
        return false;

    JSAtom* asyncCause = existing->getAsyncCause();
    if (asyncCause != lookup.asyncCause)
        return false;

    return true;
}

 void
SavedFrame::HashPolicy::rekey(Key& key, const Key& newKey)
{
    key = newKey;
}

 bool
SavedFrame::finishSavedFrameInit(JSContext* cx, HandleObject ctor, HandleObject proto)
{
    
    
    proto->as<NativeObject>().setReservedSlot(SavedFrame::JSSLOT_SOURCE, NullValue());

    return FreezeObject(cx, proto);
}

 const Class SavedFrame::class_ = {
    "SavedFrame",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(SavedFrame::JSSLOT_COUNT) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_SavedFrame) |
    JSCLASS_IS_ANONYMOUS,
    nullptr,                    
    nullptr,                    
    nullptr,                    
    nullptr,                    
    nullptr,                    
    nullptr,                    
    nullptr,                    
    SavedFrame::finalize,       
    nullptr,                    
    nullptr,                    
    nullptr,                    
    nullptr,                    

    
    {
        GenericCreateConstructor<SavedFrame::construct, 0, JSFunction::FinalizeKind>,
        GenericCreatePrototype,
        SavedFrame::staticFunctions,
        nullptr,
        SavedFrame::protoFunctions,
        SavedFrame::protoAccessors,
        SavedFrame::finishSavedFrameInit,
        ClassSpec::DontDefineConstructor
    }
};

 const JSFunctionSpec
SavedFrame::staticFunctions[] = {
    JS_FS_END
};

 const JSFunctionSpec
SavedFrame::protoFunctions[] = {
    JS_FN("constructor", SavedFrame::construct, 0, 0),
    JS_FN("toString", SavedFrame::toStringMethod, 0, 0),
    JS_FS_END
};

 const JSPropertySpec
SavedFrame::protoAccessors[] = {
    JS_PSG("source", SavedFrame::sourceProperty, 0),
    JS_PSG("line", SavedFrame::lineProperty, 0),
    JS_PSG("column", SavedFrame::columnProperty, 0),
    JS_PSG("functionDisplayName", SavedFrame::functionDisplayNameProperty, 0),
    JS_PSG("asyncCause", SavedFrame::asyncCauseProperty, 0),
    JS_PSG("asyncParent", SavedFrame::asyncParentProperty, 0),
    JS_PSG("parent", SavedFrame::parentProperty, 0),
    JS_PS_END
};

 void
SavedFrame::finalize(FreeOp* fop, JSObject* obj)
{
    JSPrincipals* p = obj->as<SavedFrame>().getPrincipals();
    if (p) {
        JSRuntime* rt = obj->runtimeFromMainThread();
        JS_DropPrincipals(rt, p);
    }
}

JSAtom*
SavedFrame::getSource()
{
    const Value& v = getReservedSlot(JSSLOT_SOURCE);
    JSString* s = v.toString();
    return &s->asAtom();
}

uint32_t
SavedFrame::getLine()
{
    const Value& v = getReservedSlot(JSSLOT_LINE);
    return v.toInt32();
}

uint32_t
SavedFrame::getColumn()
{
    const Value& v = getReservedSlot(JSSLOT_COLUMN);
    return v.toInt32();
}

JSAtom*
SavedFrame::getFunctionDisplayName()
{
    const Value& v = getReservedSlot(JSSLOT_FUNCTIONDISPLAYNAME);
    if (v.isNull())
        return nullptr;
    JSString* s = v.toString();
    return &s->asAtom();
}

JSAtom*
SavedFrame::getAsyncCause()
{
    const Value& v = getReservedSlot(JSSLOT_ASYNCCAUSE);
    if (v.isNull())
        return nullptr;
    JSString* s = v.toString();
    return &s->asAtom();
}

SavedFrame*
SavedFrame::getParent()
{
    const Value& v = getReservedSlot(JSSLOT_PARENT);
    return v.isObject() ? &v.toObject().as<SavedFrame>() : nullptr;
}

JSPrincipals*
SavedFrame::getPrincipals()
{
    const Value& v = getReservedSlot(JSSLOT_PRINCIPALS);
    if (v.isUndefined())
        return nullptr;
    return static_cast<JSPrincipals*>(v.toPrivate());
}

void
SavedFrame::initFromLookup(SavedFrame::HandleLookup lookup)
{
    MOZ_ASSERT(lookup->source);
    MOZ_ASSERT(getReservedSlot(JSSLOT_SOURCE).isUndefined());
    setReservedSlot(JSSLOT_SOURCE, StringValue(lookup->source));

    setReservedSlot(JSSLOT_LINE, NumberValue(lookup->line));
    setReservedSlot(JSSLOT_COLUMN, NumberValue(lookup->column));
    setReservedSlot(JSSLOT_FUNCTIONDISPLAYNAME,
                    lookup->functionDisplayName
                        ? StringValue(lookup->functionDisplayName)
                        : NullValue());
    setReservedSlot(JSSLOT_ASYNCCAUSE,
                    lookup->asyncCause
                        ? StringValue(lookup->asyncCause)
                        : NullValue());
    setReservedSlot(JSSLOT_PARENT, ObjectOrNullValue(lookup->parent));
    setReservedSlot(JSSLOT_PRIVATE_PARENT, PrivateValue(lookup->parent));

    MOZ_ASSERT(getReservedSlot(JSSLOT_PRINCIPALS).isUndefined());
    if (lookup->principals)
        JS_HoldPrincipals(lookup->principals);
    setReservedSlot(JSSLOT_PRINCIPALS, PrivateValue(lookup->principals));
}

bool
SavedFrame::parentMoved()
{
    const Value& v = getReservedSlot(JSSLOT_PRIVATE_PARENT);
    JSObject* p = static_cast<JSObject*>(v.toPrivate());
    return p == getParent();
}

void
SavedFrame::updatePrivateParent()
{
    setReservedSlot(JSSLOT_PRIVATE_PARENT, PrivateValue(getParent()));
}

bool
SavedFrame::isSelfHosted()
{
    JSAtom* source = getSource();
    return StringEqualsAscii(source, "self-hosted");
}

 bool
SavedFrame::construct(JSContext* cx, unsigned argc, Value* vp)
{
    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NO_CONSTRUCTOR,
                         "SavedFrame");
    return false;
}






static SavedFrame*
GetFirstSubsumedFrame(JSContext* cx, HandleSavedFrame frame, bool& skippedAsync)
{
    skippedAsync = false;

    JSSubsumesOp subsumes = cx->runtime()->securityCallbacks->subsumes;
    if (!subsumes)
        return frame;

    JSPrincipals* principals = cx->compartment()->principals();

    RootedSavedFrame rootedFrame(cx, frame);
    while (rootedFrame && !subsumes(principals, rootedFrame->getPrincipals())) {
        if (rootedFrame->getAsyncCause())
            skippedAsync = true;
        rootedFrame = rootedFrame->getParent();
    }

    return rootedFrame;
}

JS_FRIEND_API(JSObject*)
GetFirstSubsumedSavedFrame(JSContext* cx, HandleObject savedFrame)
{
    if (!savedFrame)
        return nullptr;
    bool skippedAsync;
    RootedSavedFrame frame(cx, &savedFrame->as<SavedFrame>());
    return GetFirstSubsumedFrame(cx, frame, skippedAsync);
}

 bool
SavedFrame::checkThis(JSContext* cx, CallArgs& args, const char* fnName,
                      MutableHandleObject frame)
{
    const Value& thisValue = args.thisv();

    if (!thisValue.isObject()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NOT_NONNULL_OBJECT, InformalValueTypeName(thisValue));
        return false;
    }

    JSObject* thisObject = CheckedUnwrap(&thisValue.toObject());
    if (!thisObject || !thisObject->is<SavedFrame>()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             SavedFrame::class_.name, fnName,
                             thisObject ? thisObject->getClass()->name : "object");
        return false;
    }

    
    
    
    if (thisObject->as<SavedFrame>().getReservedSlot(JSSLOT_SOURCE).isNull()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             SavedFrame::class_.name, fnName, "prototype object");
        return false;
    }

    
    
    
    frame.set(&thisValue.toObject());
    return true;
}












#define THIS_SAVEDFRAME(cx, argc, vp, fnName, args, frame)             \
    CallArgs args = CallArgsFromVp(argc, vp);                          \
    RootedObject frame(cx);                                            \
    if (!checkThis(cx, args, fnName, &frame))                          \
        return false;

} 

namespace JS {

namespace {













class MOZ_STACK_CLASS AutoMaybeEnterFrameCompartment
{
public:
    AutoMaybeEnterFrameCompartment(JSContext* cx,
                                   HandleObject obj
                                   MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        
        
        if (obj && cx->compartment() != obj->compartment())
        {
            JSSubsumesOp subsumes = cx->runtime()->securityCallbacks->subsumes;
            if (subsumes && subsumes(cx->compartment()->principals(),
                                     obj->compartment()->principals()))
            {
                ac_.emplace(cx, obj);
            }
        }
    }

 private:
    Maybe<JSAutoCompartment> ac_;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

static inline js::SavedFrame*
UnwrapSavedFrame(JSContext* cx, HandleObject obj, bool& skippedAsync)
{
    if (!obj)
        return nullptr;
    RootedObject savedFrameObj(cx, CheckedUnwrap(obj));
    MOZ_ASSERT(savedFrameObj);
    MOZ_ASSERT(js::SavedFrame::isSavedFrameAndNotProto(*savedFrameObj));
    js::RootedSavedFrame frame(cx, &savedFrameObj->as<js::SavedFrame>());
    return GetFirstSubsumedFrame(cx, frame, skippedAsync);
}

JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameSource(JSContext* cx, HandleObject savedFrame, MutableHandleString sourcep)
{
    AutoMaybeEnterFrameCompartment ac(cx, savedFrame);
    bool skippedAsync;
    js::RootedSavedFrame frame(cx, UnwrapSavedFrame(cx, savedFrame, skippedAsync));
    if (!frame) {
        sourcep.set(cx->runtime()->emptyString);
        return SavedFrameResult::AccessDenied;
    }
    sourcep.set(frame->getSource());
    return SavedFrameResult::Ok;
}

JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameLine(JSContext* cx, HandleObject savedFrame, uint32_t* linep)
{
    MOZ_ASSERT(linep);
    AutoMaybeEnterFrameCompartment ac(cx, savedFrame);
    bool skippedAsync;
    js::RootedSavedFrame frame(cx, UnwrapSavedFrame(cx, savedFrame, skippedAsync));
    if (!frame) {
        *linep = 0;
        return SavedFrameResult::AccessDenied;
    }
    *linep = frame->getLine();
    return SavedFrameResult::Ok;
}

JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameColumn(JSContext* cx, HandleObject savedFrame, uint32_t* columnp)
{
    MOZ_ASSERT(columnp);
    AutoMaybeEnterFrameCompartment ac(cx, savedFrame);
    bool skippedAsync;
    js::RootedSavedFrame frame(cx, UnwrapSavedFrame(cx, savedFrame, skippedAsync));
    if (!frame) {
        *columnp = 0;
        return SavedFrameResult::AccessDenied;
    }
    *columnp = frame->getColumn();
    return SavedFrameResult::Ok;
}

JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameFunctionDisplayName(JSContext* cx, HandleObject savedFrame, MutableHandleString namep)
{
    AutoMaybeEnterFrameCompartment ac(cx, savedFrame);
    bool skippedAsync;
    js::RootedSavedFrame frame(cx, UnwrapSavedFrame(cx, savedFrame, skippedAsync));
    if (!frame) {
        namep.set(nullptr);
        return SavedFrameResult::AccessDenied;
    }
    namep.set(frame->getFunctionDisplayName());
    return SavedFrameResult::Ok;
}

JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameAsyncCause(JSContext* cx, HandleObject savedFrame, MutableHandleString asyncCausep)
{
    AutoMaybeEnterFrameCompartment ac(cx, savedFrame);
    bool skippedAsync;
    js::RootedSavedFrame frame(cx, UnwrapSavedFrame(cx, savedFrame, skippedAsync));
    if (!frame) {
        asyncCausep.set(nullptr);
        return SavedFrameResult::AccessDenied;
    }
    asyncCausep.set(frame->getAsyncCause());
    if (!asyncCausep && skippedAsync)
        asyncCausep.set(cx->names().Async);
    return SavedFrameResult::Ok;
}

JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameAsyncParent(JSContext* cx, HandleObject savedFrame, MutableHandleObject asyncParentp)
{
    AutoMaybeEnterFrameCompartment ac(cx, savedFrame);
    bool skippedAsync;
    js::RootedSavedFrame frame(cx, UnwrapSavedFrame(cx, savedFrame, skippedAsync));
    if (!frame) {
        asyncParentp.set(nullptr);
        return SavedFrameResult::AccessDenied;
    }
    js::RootedSavedFrame parent(cx, frame->getParent());

    
    
    
    js::RootedSavedFrame subsumedParent(cx, GetFirstSubsumedFrame(cx, parent, skippedAsync));

    
    
    
    if (subsumedParent && (subsumedParent->getAsyncCause() || skippedAsync))
        asyncParentp.set(parent);
    else
        asyncParentp.set(nullptr);
    return SavedFrameResult::Ok;
}

JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameParent(JSContext* cx, HandleObject savedFrame, MutableHandleObject parentp)
{
    AutoMaybeEnterFrameCompartment ac(cx, savedFrame);
    bool skippedAsync;
    js::RootedSavedFrame frame(cx, UnwrapSavedFrame(cx, savedFrame, skippedAsync));
    if (!frame) {
        parentp.set(nullptr);
        return SavedFrameResult::AccessDenied;
    }
    js::RootedSavedFrame parent(cx, frame->getParent());

    
    
    
    js::RootedSavedFrame subsumedParent(cx, GetFirstSubsumedFrame(cx, parent, skippedAsync));

    
    
    
    if (subsumedParent && !(subsumedParent->getAsyncCause() || skippedAsync))
        parentp.set(parent);
    else
        parentp.set(nullptr);
    return SavedFrameResult::Ok;
}

JS_PUBLIC_API(bool)
BuildStackString(JSContext* cx, HandleObject stack, MutableHandleString stringp)
{
    js::StringBuffer sb(cx);

    
    
    
    
    {
        AutoMaybeEnterFrameCompartment ac(cx, stack);
        bool skippedAsync;
        js::RootedSavedFrame frame(cx, UnwrapSavedFrame(cx, stack, skippedAsync));
        if (!frame) {
            stringp.set(cx->runtime()->emptyString);
            return true;
        }

        DebugOnly<JSSubsumesOp> subsumes = cx->runtime()->securityCallbacks->subsumes;
        DebugOnly<JSPrincipals*> principals = cx->compartment()->principals();

        js::RootedSavedFrame parent(cx);
        do {
            MOZ_ASSERT_IF(subsumes, (*subsumes)(principals, frame->getPrincipals()));

            if (!frame->isSelfHosted()) {
                RootedString asyncCause(cx, frame->getAsyncCause());
                if (!asyncCause && skippedAsync)
                    asyncCause.set(cx->names().Async);

                js::RootedAtom name(cx, frame->getFunctionDisplayName());
                if ((asyncCause && (!sb.append(asyncCause) || !sb.append('*')))
                    || (name && !sb.append(name))
                    || !sb.append('@')
                    || !sb.append(frame->getSource())
                    || !sb.append(':')
                    || !NumberValueToStringBuffer(cx, NumberValue(frame->getLine()), sb)
                    || !sb.append(':')
                    || !NumberValueToStringBuffer(cx, NumberValue(frame->getColumn()), sb)
                    || !sb.append('\n'))
                {
                    return false;
                }
            }

            parent = frame->getParent();
            frame = js::GetFirstSubsumedFrame(cx, parent, skippedAsync);
        } while (frame);
    }

    JSString* str = sb.finishString();
    if (!str)
        return false;
    assertSameCompartment(cx, str);
    stringp.set(str);
    return true;
}

} 

namespace js {

 bool
SavedFrame::sourceProperty(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get source)", args, frame);
    RootedString source(cx);
    if (JS::GetSavedFrameSource(cx, frame, &source) == JS::SavedFrameResult::Ok)
        args.rval().setString(source);
    else
        args.rval().setNull();
    return true;
}

 bool
SavedFrame::lineProperty(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get line)", args, frame);
    uint32_t line;
    if (JS::GetSavedFrameLine(cx, frame, &line) == JS::SavedFrameResult::Ok)
        args.rval().setNumber(line);
    else
        args.rval().setNull();
    return true;
}

 bool
SavedFrame::columnProperty(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get column)", args, frame);
    uint32_t column;
    if (JS::GetSavedFrameColumn(cx, frame, &column) == JS::SavedFrameResult::Ok)
        args.rval().setNumber(column);
    else
        args.rval().setNull();
    return true;
}

 bool
SavedFrame::functionDisplayNameProperty(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get functionDisplayName)", args, frame);
    RootedString name(cx);
    JS::SavedFrameResult result = JS::GetSavedFrameFunctionDisplayName(cx, frame, &name);
    if (result == JS::SavedFrameResult::Ok && name)
        args.rval().setString(name);
    else
        args.rval().setNull();
    return true;
}

 bool
SavedFrame::asyncCauseProperty(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get asyncCause)", args, frame);
    RootedString asyncCause(cx);
    JS::SavedFrameResult result = JS::GetSavedFrameAsyncCause(cx, frame, &asyncCause);
    if (result == JS::SavedFrameResult::Ok && asyncCause)
        args.rval().setString(asyncCause);
    else
        args.rval().setNull();
    return true;
}

 bool
SavedFrame::asyncParentProperty(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get asyncParent)", args, frame);
    RootedObject asyncParent(cx);
    (void) JS::GetSavedFrameAsyncParent(cx, frame, &asyncParent);
    args.rval().setObjectOrNull(asyncParent);
    return true;
}

 bool
SavedFrame::parentProperty(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get parent)", args, frame);
    RootedObject parent(cx);
    (void) JS::GetSavedFrameParent(cx, frame, &parent);
    args.rval().setObjectOrNull(parent);
    return true;
}

 bool
SavedFrame::toStringMethod(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "toString", args, frame);
    RootedString string(cx);
    if (!JS::BuildStackString(cx, frame, &string))
        return false;
    args.rval().setString(string);
    return true;
}

bool
SavedStacks::init()
{
    if (!pcLocationMap.init())
        return false;

    return frames.init();
}

bool
SavedStacks::saveCurrentStack(JSContext* cx, MutableHandleSavedFrame frame, unsigned maxFrameCount)
{
    MOZ_ASSERT(initialized());
    assertSameCompartment(cx, this);

    if (creatingSavedFrame) {
        frame.set(nullptr);
        return true;
    }

    FrameIter iter(cx, FrameIter::ALL_CONTEXTS, FrameIter::GO_THROUGH_SAVED);
    return insertFrames(cx, iter, frame, maxFrameCount);
}

void
SavedStacks::sweep(JSRuntime* rt)
{
    if (frames.initialized()) {
        for (SavedFrame::Set::Enum e(frames); !e.empty(); e.popFront()) {
            JSObject* obj = e.front().unbarrieredGet();
            JSObject* temp = obj;

            if (IsObjectAboutToBeFinalized(&obj)) {
                e.removeFront();
            } else {
                SavedFrame* frame = &obj->as<SavedFrame>();
                bool parentMoved = frame->parentMoved();

                if (parentMoved) {
                    frame->updatePrivateParent();
                }

                if (obj != temp || parentMoved) {
                    e.rekeyFront(SavedFrame::Lookup(*frame),
                                 ReadBarriered<SavedFrame*>(frame));
                }
            }
        }
    }

    sweepPCLocationMap();
}

void
SavedStacks::trace(JSTracer* trc)
{
    if (!pcLocationMap.initialized())
        return;

    
    for (PCLocationMap::Enum e(pcLocationMap); !e.empty(); e.popFront()) {
        LocationValue& loc = e.front().value();
        TraceEdge(trc, &loc.source, "SavedStacks::PCLocationMap's memoized script source name");
    }
}

uint32_t
SavedStacks::count()
{
    MOZ_ASSERT(initialized());
    return frames.count();
}

void
SavedStacks::clear()
{
    frames.clear();
}

size_t
SavedStacks::sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf)
{
    return frames.sizeOfExcludingThis(mallocSizeOf);
}

bool
SavedStacks::insertFrames(JSContext* cx, FrameIter& iter, MutableHandleSavedFrame frame,
                          unsigned maxFrameCount)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    Activation* asyncActivation = nullptr;
    RootedSavedFrame asyncStack(cx, nullptr);
    RootedString asyncCause(cx, nullptr);

    
    SavedFrame::AutoLookupVector stackChain(cx);
    while (!iter.done()) {
        Activation& activation = *iter.activation();

        if (!asyncActivation) {
            asyncStack = activation.asyncStack();
            if (asyncStack) {
                
                
                
                
                
                asyncCause = activation.asyncCause();
                asyncActivation = &activation;
            }
        } else if (asyncActivation != &activation) {
            
            
            break;
        }

        AutoLocationValueRooter location(cx);

        {
            AutoCompartment ac(cx, iter.compartment());
            if (!cx->compartment()->savedStacks().getLocation(cx, iter, &location))
                return false;
        }

        
        
        if (!stackChain->growByUninitialized(1)) {
            ReportOutOfMemory(cx);
            return false;
        }
        new (&stackChain->back()) SavedFrame::Lookup(
          location->source,
          location->line,
          location->column,
          iter.isNonEvalFunctionFrame() ? iter.functionDisplayAtom() : nullptr,
          nullptr,
          nullptr,
          iter.compartment()->principals()
        );

        ++iter;

        
        if (maxFrameCount == 0)
            continue;

        if (maxFrameCount == 1) {
            
            
            asyncStack.set(nullptr);
            break;
        }

        maxFrameCount--;
    }

    
    
    
    RootedSavedFrame parentFrame(cx, nullptr);
    if (asyncStack && !adoptAsyncStack(cx, asyncStack, asyncCause, &parentFrame, maxFrameCount))
        return false;

    
    
    for (size_t i = stackChain->length(); i != 0; i--) {
        SavedFrame::HandleLookup lookup = stackChain[i-1];
        lookup->parent = parentFrame;
        parentFrame.set(getOrCreateSavedFrame(cx, lookup));
        if (!parentFrame)
            return false;
    }

    frame.set(parentFrame);
    return true;
}

bool
SavedStacks::adoptAsyncStack(JSContext* cx, HandleSavedFrame asyncStack,
                             HandleString asyncCause,
                             MutableHandleSavedFrame adoptedStack,
                             unsigned maxFrameCount)
{
    RootedAtom asyncCauseAtom(cx, AtomizeString(cx, asyncCause));
    if (!asyncCauseAtom)
        return false;

    
    
    
    
    if (maxFrameCount == 0)
        maxFrameCount = ASYNC_STACK_MAX_FRAME_COUNT;

    
    SavedFrame::AutoLookupVector stackChain(cx);
    SavedFrame* currentSavedFrame = asyncStack;
    for (unsigned i = 0; i < maxFrameCount && currentSavedFrame; i++) {
        
        
        if (!stackChain->growByUninitialized(1)) {
            ReportOutOfMemory(cx);
            return false;
        }
        new (&stackChain->back()) SavedFrame::Lookup(*currentSavedFrame);

        
        if (i == 0)
            stackChain->back().asyncCause = asyncCauseAtom;

        currentSavedFrame = currentSavedFrame->getParent();
    }

    
    
    RootedSavedFrame parentFrame(cx, nullptr);
    for (size_t i = stackChain->length(); i != 0; i--) {
        SavedFrame::HandleLookup lookup = stackChain[i-1];
        lookup->parent = parentFrame;
        parentFrame.set(getOrCreateSavedFrame(cx, lookup));
        if (!parentFrame)
            return false;
    }

    adoptedStack.set(parentFrame);
    return true;
}

SavedFrame*
SavedStacks::getOrCreateSavedFrame(JSContext* cx, SavedFrame::HandleLookup lookup)
{
    const SavedFrame::Lookup& lookupInstance = lookup.get();
    DependentAddPtr<SavedFrame::Set> p(cx, frames, lookupInstance);
    if (p)
        return *p;

    RootedSavedFrame frame(cx, createFrameFromLookup(cx, lookup));
    if (!frame)
        return nullptr;

    if (!p.add(cx, frames, lookupInstance, frame)) {
        ReportOutOfMemory(cx);
        return nullptr;
    }

    return frame;
}

SavedFrame*
SavedStacks::createFrameFromLookup(JSContext* cx, SavedFrame::HandleLookup lookup)
{
    RootedGlobalObject global(cx, cx->global());
    assertSameCompartment(cx, global);

    
    
    
    SavedStacks::AutoReentrancyGuard guard(*this);

    RootedNativeObject proto(cx, GlobalObject::getOrCreateSavedFramePrototype(cx, global));
    if (!proto)
        return nullptr;
    assertSameCompartment(cx, proto);

    RootedObject frameObj(cx, NewObjectWithGivenProto(cx, &SavedFrame::class_, proto));
    if (!frameObj)
        return nullptr;

    RootedSavedFrame f(cx, &frameObj->as<SavedFrame>());
    f->initFromLookup(lookup);

    if (!FreezeObject(cx, frameObj))
        return nullptr;

    return f.get();
}




void
SavedStacks::sweepPCLocationMap()
{
    for (PCLocationMap::Enum e(pcLocationMap); !e.empty(); e.popFront()) {
        PCKey key = e.front().key();
        JSScript* script = key.script.get();
        if (IsAboutToBeFinalizedUnbarriered(&script)) {
            e.removeFront();
        } else if (script != key.script.get()) {
            key.script = script;
            e.rekeyFront(key);
        }
    }
}

bool
SavedStacks::getLocation(JSContext* cx, const FrameIter& iter, MutableHandleLocationValue locationp)
{
    
    
    
    
    assertSameCompartment(cx, this, iter.compartment());

    
    
    
    

    if (!iter.hasScript()) {
        if (const char16_t* displayURL = iter.scriptDisplayURL()) {
            locationp->source = AtomizeChars(cx, displayURL, js_strlen(displayURL));
        } else {
            const char* filename = iter.scriptFilename() ? iter.scriptFilename() : "";
            locationp->source = Atomize(cx, filename, strlen(filename));
        }
        if (!locationp->source)
            return false;

        locationp->line = iter.computeLine(&locationp->column);
        
        
        
        locationp->column++;
        return true;
    }

    RootedScript script(cx, iter.script());
    jsbytecode* pc = iter.pc();

    PCKey key(script, pc);
    PCLocationMap::AddPtr p = pcLocationMap.lookupForAdd(key);

    if (!p) {
        RootedAtom source(cx);
        if (const char16_t* displayURL = iter.scriptDisplayURL()) {
            source = AtomizeChars(cx, displayURL, js_strlen(displayURL));
        } else {
            const char* filename = script->filename() ? script->filename() : "";
            source = Atomize(cx, filename, strlen(filename));
        }
        if (!source)
            return false;

        uint32_t column;
        uint32_t line = PCToLineNumber(script, pc, &column);

        
        LocationValue value(source, line, column + 1);
        if (!pcLocationMap.add(p, key, value)) {
            ReportOutOfMemory(cx);
            return false;
        }
    }

    locationp.set(p->value());
    return true;
}

void
SavedStacks::chooseSamplingProbability(JSContext* cx)
{
    GlobalObject::DebuggerVector* dbgs = cx->global()->getDebuggers();
    if (!dbgs || dbgs->empty())
        return;

    Debugger* allocationTrackingDbg = nullptr;
    mozilla::DebugOnly<Debugger**> begin = dbgs->begin();

    for (Debugger** dbgp = dbgs->begin(); dbgp < dbgs->end(); dbgp++) {
        
        
        MOZ_ASSERT(dbgs->begin() == begin);

        if ((*dbgp)->trackingAllocationSites && (*dbgp)->enabled)
            allocationTrackingDbg = *dbgp;
    }

    if (!allocationTrackingDbg)
        return;

    allocationSamplingProbability = allocationTrackingDbg->allocationSamplingProbability;
}

JSObject*
SavedStacksMetadataCallback(JSContext* cx)
{
    SavedStacks& stacks = cx->compartment()->savedStacks();
    if (stacks.allocationSkipCount > 0) {
        stacks.allocationSkipCount--;
        return nullptr;
    }

    stacks.chooseSamplingProbability(cx);
    if (stacks.allocationSamplingProbability == 0.0)
        return nullptr;

    
    
    if (stacks.allocationSamplingProbability != 1.0) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        double notSamplingProb = 1.0 - stacks.allocationSamplingProbability;
        stacks.allocationSkipCount = std::floor(std::log(random_nextDouble(&stacks.rngState)) /
                                                std::log(notSamplingProb));
    }

    RootedSavedFrame frame(cx);
    if (!stacks.saveCurrentStack(cx, &frame))
        CrashAtUnhandlableOOM("SavedStacksMetadataCallback");

    if (!Debugger::onLogAllocationSite(cx, frame, PRMJ_Now()))
        CrashAtUnhandlableOOM("SavedStacksMetadataCallback");

    return frame;
}

JS_FRIEND_API(JSPrincipals*)
GetSavedFramePrincipals(HandleObject savedFrame)
{
    MOZ_ASSERT(savedFrame);
    MOZ_ASSERT(savedFrame->is<SavedFrame>());
    return savedFrame->as<SavedFrame>().getPrincipals();
}

#ifdef JS_CRASH_DIAGNOSTICS
void
CompartmentChecker::check(SavedStacks* stacks)
{
    if (&compartment->savedStacks() != stacks) {
        printf("*** Compartment SavedStacks mismatch: %p vs. %p\n",
               (void*) &compartment->savedStacks(), stacks);
        MOZ_CRASH();
    }
}
#endif 

} 
