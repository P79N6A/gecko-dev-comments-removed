






#include "vm/SavedStacks.h"

#include "jsapi.h"
#include "jscompartment.h"
#include "jsfriendapi.h"
#include "jsnum.h"

#include "vm/GlobalObject.h"
#include "vm/StringBuffer.h"

#include "jsobjinlines.h"

using mozilla::AddToHash;
using mozilla::HashString;

namespace js {

 HashNumber
SavedFrame::HashPolicy::hash(const Lookup &lookup)
{
    return AddToHash(HashString(lookup.source->chars(), lookup.source->length()),
                     lookup.line,
                     lookup.column,
                     lookup.functionDisplayName,
                     SavedFramePtrHasher::hash(lookup.parent),
                     JSPrincipalsPtrHasher::hash(lookup.principals));
}

 bool
SavedFrame::HashPolicy::match(SavedFrame *existing, const Lookup &lookup)
{
    if (existing->getLine() != lookup.line)
        return false;

    if (existing->getColumn() != lookup.column)
        return false;

    if (existing->getParent() != lookup.parent)
        return false;

    if (existing->getPrincipals() != lookup.principals)
        return false;

    JSAtom *source = existing->getSource();
    if (source->length() != lookup.source->length())
        return false;
    if (source != lookup.source)
        return false;

    JSAtom *functionDisplayName = existing->getFunctionDisplayName();
    if (functionDisplayName) {
        if (!lookup.functionDisplayName)
            return false;
        if (functionDisplayName->length() != lookup.functionDisplayName->length())
            return false;
        if (0 != CompareAtoms(functionDisplayName, lookup.functionDisplayName))
            return false;
    } else if (lookup.functionDisplayName) {
        return false;
    }

    return true;
}

 void
SavedFrame::HashPolicy::rekey(Key &key, const Key &newKey)
{
    key = newKey;
}

 const Class SavedFrame::class_ = {
    "SavedFrame",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(SavedFrame::JSSLOT_COUNT),

    JS_PropertyStub,       
    JS_DeletePropertyStub, 
    JS_PropertyStub,       
    JS_StrictPropertyStub, 
    JS_EnumerateStub,      
    JS_ResolveStub,        
    JS_ConvertStub,        

    SavedFrame::finalize   
};

 void
SavedFrame::finalize(FreeOp *fop, JSObject *obj)
{
    JSPrincipals *p = obj->as<SavedFrame>().getPrincipals();
    if (p) {
        JSRuntime *rt = obj->runtimeFromMainThread();
        JS_DropPrincipals(rt, p);
    }
}

JSAtom *
SavedFrame::getSource()
{
    const Value &v = getReservedSlot(JSSLOT_SOURCE);
    JSString *s = v.toString();
    return &s->asAtom();
}

size_t
SavedFrame::getLine()
{
    const Value &v = getReservedSlot(JSSLOT_LINE);
    return v.toInt32();
}

size_t
SavedFrame::getColumn()
{
    const Value &v = getReservedSlot(JSSLOT_COLUMN);
    return v.toInt32();
}

JSAtom *
SavedFrame::getFunctionDisplayName()
{
    const Value &v = getReservedSlot(JSSLOT_FUNCTIONDISPLAYNAME);
    if (v.isNull())
        return nullptr;
    JSString *s = v.toString();
    return &s->asAtom();
}

SavedFrame *
SavedFrame::getParent()
{
    const Value &v = getReservedSlot(JSSLOT_PARENT);
    return v.isObject() ? &v.toObject().as<SavedFrame>() : nullptr;
}

JSPrincipals *
SavedFrame::getPrincipals()
{
    const Value &v = getReservedSlot(JSSLOT_PRINCIPALS);
    if (v.isUndefined())
        return nullptr;
    return static_cast<JSPrincipals *>(v.toPrivate());
}

void
SavedFrame::initFromLookup(Lookup &lookup)
{
    JS_ASSERT(lookup.source);
    JS_ASSERT(getReservedSlot(JSSLOT_SOURCE).isUndefined());
    setReservedSlot(JSSLOT_SOURCE, StringValue(lookup.source));

    setReservedSlot(JSSLOT_LINE, NumberValue(lookup.line));
    setReservedSlot(JSSLOT_COLUMN, NumberValue(lookup.column));
    setReservedSlot(JSSLOT_FUNCTIONDISPLAYNAME,
                    lookup.functionDisplayName
                        ? StringValue(lookup.functionDisplayName)
                        : NullValue());
    setReservedSlot(JSSLOT_PARENT, ObjectOrNullValue(lookup.parent));
    setReservedSlot(JSSLOT_PRIVATE_PARENT, PrivateValue(lookup.parent));

    JS_ASSERT(getReservedSlot(JSSLOT_PRINCIPALS).isUndefined());
    if (lookup.principals)
        JS_HoldPrincipals(lookup.principals);
    setReservedSlot(JSSLOT_PRINCIPALS, PrivateValue(lookup.principals));
}

bool
SavedFrame::parentMoved()
{
    const Value &v = getReservedSlot(JSSLOT_PRIVATE_PARENT);
    JSObject *p = static_cast<JSObject *>(v.toPrivate());
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
    JSAtom *source = getSource();
    return StringEqualsAscii(source, "self-hosted");
}

 bool
SavedFrame::construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NO_CONSTRUCTOR,
                         "SavedFrame");
    return false;
}

 SavedFrame *
SavedFrame::checkThis(JSContext *cx, CallArgs &args, const char *fnName)
{
    const Value &thisValue = args.thisv();

    if (!thisValue.isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_NONNULL_OBJECT);
        return nullptr;
    }

    JSObject &thisObject = thisValue.toObject();
    if (!thisObject.is<SavedFrame>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             SavedFrame::class_.name, fnName, thisObject.getClass()->name);
        return nullptr;
    }

    
    
    
    if (thisObject.getReservedSlot(JSSLOT_SOURCE).isNull()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             SavedFrame::class_.name, fnName, "prototype object");
        return nullptr;
    }

    return &thisObject.as<SavedFrame>();
}












#define THIS_SAVEDFRAME(cx, argc, vp, fnName, args, frame)         \
    CallArgs args = CallArgsFromVp(argc, vp);                      \
    Rooted<SavedFrame *> frame(cx, checkThis(cx, args, fnName));   \
    if (!frame)                                                    \
        return false

 bool
SavedFrame::sourceProperty(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get source)", args, frame);
    args.rval().setString(frame->getSource());
    return true;
}

 bool
SavedFrame::lineProperty(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get line)", args, frame);
    uint32_t line = frame->getLine();
    args.rval().setNumber(line);
    return true;
}

 bool
SavedFrame::columnProperty(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get column)", args, frame);
    uint32_t column = frame->getColumn();
    args.rval().setNumber(column);
    return true;
}

 bool
SavedFrame::functionDisplayNameProperty(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get functionDisplayName)", args, frame);
    RootedAtom name(cx, frame->getFunctionDisplayName());
    if (name)
        args.rval().setString(name);
    else
        args.rval().setNull();
    return true;
}

 bool
SavedFrame::parentProperty(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "(get parent)", args, frame);
    JSSubsumesOp subsumes = cx->runtime()->securityCallbacks->subsumes;
    JSPrincipals *principals = cx->compartment()->principals;

    do
        frame = frame->getParent();
    while (frame && principals && subsumes &&
           !subsumes(principals, frame->getPrincipals()));

    args.rval().setObjectOrNull(frame);
    return true;
}

 const JSPropertySpec SavedFrame::properties[] = {
    JS_PSG("source", SavedFrame::sourceProperty, 0),
    JS_PSG("line", SavedFrame::lineProperty, 0),
    JS_PSG("column", SavedFrame::columnProperty, 0),
    JS_PSG("functionDisplayName", SavedFrame::functionDisplayNameProperty, 0),
    JS_PSG("parent", SavedFrame::parentProperty, 0),
    JS_PS_END
};

 bool
SavedFrame::toStringMethod(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_SAVEDFRAME(cx, argc, vp, "toString", args, frame);
    StringBuffer sb(cx);
    JSSubsumesOp subsumes = cx->runtime()->securityCallbacks->subsumes;
    JSPrincipals *principals = cx->compartment()->principals;

    do {
        if (principals && subsumes && !subsumes(principals, frame->getPrincipals()))
            continue;
        if (frame->isSelfHosted())
            continue;

        RootedAtom name(cx, frame->getFunctionDisplayName());
        if ((name && !sb.append(name))
            || !sb.append('@')
            || !sb.append(frame->getSource())
            || !sb.append(':')
            || !NumberValueToStringBuffer(cx, NumberValue(frame->getLine()), sb)
            || !sb.append(':')
            || !NumberValueToStringBuffer(cx, NumberValue(frame->getColumn()), sb)
            || !sb.append('\n')) {
            return false;
        }
    } while ((frame = frame->getParent()));

    args.rval().setString(sb.finishString());
    return true;
}

 const JSFunctionSpec SavedFrame::methods[] = {
    JS_FN("constructor", SavedFrame::construct, 0, 0),
    JS_FN("toString", SavedFrame::toStringMethod, 0, 0),
    JS_FS_END
};

bool
SavedStacks::init()
{
    return frames.init();
}

bool
SavedStacks::saveCurrentStack(JSContext *cx, MutableHandle<SavedFrame*> frame)
{
    JS_ASSERT(initialized());
    JS_ASSERT(&cx->compartment()->savedStacks() == this);

    ScriptFrameIter iter(cx);
    return insertFrames(cx, iter, frame);
}

void
SavedStacks::sweep(JSRuntime *rt)
{
    if (frames.initialized()) {
        for (SavedFrame::Set::Enum e(frames); !e.empty(); e.popFront()) {
            JSObject *obj = static_cast<JSObject *>(e.front());
            JSObject *temp = obj;

            if (IsObjectAboutToBeFinalized(&obj)) {
                e.removeFront();
            } else {
                SavedFrame *frame = &obj->as<SavedFrame>();
                bool parentMoved = frame->parentMoved();

                if (parentMoved) {
                    frame->updatePrivateParent();
                }

                if (obj != temp || parentMoved) {
                    Rooted<SavedFrame*> parent(rt, frame->getParent());
                    e.rekeyFront(SavedFrame::Lookup(frame->getSource(),
                                                    frame->getLine(),
                                                    frame->getColumn(),
                                                    frame->getFunctionDisplayName(),
                                                    parent,
                                                    frame->getPrincipals()),
                                 ReadBarriered<SavedFrame *>(frame));
                }
            }
        }
    }

    if (savedFrameProto && IsObjectAboutToBeFinalized(&savedFrameProto)) {
        savedFrameProto = nullptr;
    }
}

uint32_t
SavedStacks::count()
{
    JS_ASSERT(initialized());
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
SavedStacks::insertFrames(JSContext *cx, ScriptFrameIter &iter, MutableHandle<SavedFrame*> frame)
{
    if (iter.done()) {
        frame.set(nullptr);
        return true;
    }

    
    
    
    
    
    
    JS_CHECK_RECURSION_DONT_REPORT(cx, return false);

    ScriptFrameIter thisFrame(iter);
    Rooted<SavedFrame*> parentFrame(cx);
    if (!insertFrames(cx, ++iter, &parentFrame))
        return false;

    RootedScript script(cx, thisFrame.script());
    RootedFunction callee(cx, thisFrame.maybeCallee());
    const char *filename = script->filename();
    if (!filename)
        filename = "";
    RootedAtom source(cx, Atomize(cx, filename, strlen(filename)));
    if (!source)
        return false;
    uint32_t column;
    uint32_t line = PCToLineNumber(script, thisFrame.pc(), &column);

    SavedFrame::Lookup lookup(source,
                              line,
                              column,
                              callee ? callee->displayAtom() : nullptr,
                              parentFrame,
                              thisFrame.compartment()->principals);

    frame.set(getOrCreateSavedFrame(cx, lookup));
    return frame.get() != nullptr;
}

SavedFrame *
SavedStacks::getOrCreateSavedFrame(JSContext *cx, SavedFrame::Lookup &lookup)
{
    SavedFrame::Set::AddPtr p = frames.lookupForAdd(lookup);
    if (p)
        return *p;

    Rooted<SavedFrame *> frame(cx, createFrameFromLookup(cx, lookup));
    if (!frame)
        return nullptr;

    if (!frames.relookupOrAdd(p, lookup, frame))
        return nullptr;

    return frame;
}

JSObject *
SavedStacks::getOrCreateSavedFramePrototype(JSContext *cx)
{
    if (savedFrameProto)
        return savedFrameProto;

    Rooted<GlobalObject *> global(cx, cx->compartment()->maybeGlobal());
    if (!global)
        return nullptr;

    RootedObject proto(cx, NewObjectWithGivenProto(cx, &SavedFrame::class_,
                                                   global->getOrCreateObjectPrototype(cx),
                                                   global));
    if (!proto
        || !JS_DefineProperties(cx, proto, SavedFrame::properties)
        || !JS_DefineFunctions(cx, proto, SavedFrame::methods))
        return nullptr;

    savedFrameProto = proto;
    
    
    savedFrameProto->setReservedSlot(SavedFrame::JSSLOT_SOURCE, NullValue());
    return savedFrameProto;
}

SavedFrame *
SavedStacks::createFrameFromLookup(JSContext *cx, SavedFrame::Lookup &lookup)
{
    RootedObject proto(cx, getOrCreateSavedFramePrototype(cx));
    if (!proto)
        return nullptr;

    JS_ASSERT(proto->compartment() == cx->compartment());

    RootedObject global(cx, cx->compartment()->maybeGlobal());
    if (!global)
        return nullptr;

    JS_ASSERT(global->compartment() == cx->compartment());

    RootedObject frameObj(cx, NewObjectWithGivenProto(cx, &SavedFrame::class_, proto, global));
    if (!frameObj)
        return nullptr;

    SavedFrame &f = frameObj->as<SavedFrame>();
    f.initFromLookup(lookup);

    return &f;
}

bool
SavedStacksMetadataCallback(JSContext *cx, JSObject **pmetadata)
{
    Rooted<SavedFrame *> frame(cx);
    if (!cx->compartment()->savedStacks().saveCurrentStack(cx, &frame))
        return false;
    *pmetadata = frame;
    return true;
}

} 
