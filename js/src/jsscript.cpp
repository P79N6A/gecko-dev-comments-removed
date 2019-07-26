












































#include <string.h>
#include "jstypes.h"
#include "jsutil.h"
#include "jscrashreport.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbgapi.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsscope.h"
#include "jsscript.h"

#include "frontend/BytecodeEmitter.h"
#include "frontend/Parser.h"
#include "js/MemoryMetrics.h"
#include "methodjit/MethodJIT.h"
#include "ion/IonCode.h"
#include "methodjit/Retcon.h"
#include "vm/Debugger.h"
#include "vm/Xdr.h"

#include "jsinferinlines.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::gc;
using namespace js::frontend;

namespace js {

BindingKind
Bindings::lookup(JSContext *cx, JSAtom *name, unsigned *indexp) const
{
    if (!lastBinding)
        return NONE;

    Shape **spp;
    Shape *shape = Shape::search(cx, lastBinding, ATOM_TO_JSID(name), &spp);
    if (!shape)
        return NONE;

    if (indexp)
        *indexp = shape->shortid();

    if (shape->getter() == CallObject::getArgOp)
        return ARGUMENT;

    return shape->writable() ? VARIABLE : CONSTANT;
}

bool
Bindings::add(JSContext *cx, JSAtom *name, BindingKind kind)
{
    if (!ensureShape(cx))
        return false;

    if (nargs + nvars == BINDING_COUNT_LIMIT) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             (kind == ARGUMENT)
                             ? JSMSG_TOO_MANY_FUN_ARGS
                             : JSMSG_TOO_MANY_LOCALS);
        return false;
    }

    




    unsigned attrs = JSPROP_ENUMERATE | JSPROP_PERMANENT;

    uint16_t *indexp;
    PropertyOp getter;
    StrictPropertyOp setter;
    uint32_t slot = CallObject::RESERVED_SLOTS;

    if (kind == ARGUMENT) {
        JS_ASSERT(nvars == 0);
        indexp = &nargs;
        getter = CallObject::getArgOp;
        setter = CallObject::setArgOp;
        slot += nargs;
    } else {
        JS_ASSERT(kind == VARIABLE || kind == CONSTANT);

        indexp = &nvars;
        getter = CallObject::getVarOp;
        setter = CallObject::setVarOp;
        if (kind == CONSTANT)
            attrs |= JSPROP_READONLY;
        slot += nargs + nvars;
    }

    jsid id;
    if (!name) {
        JS_ASSERT(kind == ARGUMENT); 
        id = INT_TO_JSID(nargs);
    } else {
        id = ATOM_TO_JSID(name);
    }

    StackBaseShape base(&CallClass, NULL, BaseShape::VAROBJ);
    base.updateGetterSetter(attrs, getter, setter);

    UnownedBaseShape *nbase = BaseShape::getUnowned(cx, base);
    if (!nbase)
        return NULL;

    StackShape child(nbase, id, slot, 0, attrs, Shape::HAS_SHORTID, *indexp);

    
    Shape *shape = lastBinding->getChildBinding(cx, child);
    if (!shape)
        return false;

    lastBinding = shape;
    ++*indexp;
    return true;
}

Shape *
Bindings::callObjectShape(JSContext *cx) const
{
    if (!hasDup())
        return lastShape();

    




    Vector<const Shape *> shapes(cx);
    HashSet<jsid> seen(cx);
    if (!seen.init())
        return NULL;

    for (Shape::Range r = lastShape()->all(); !r.empty(); r.popFront()) {
        const Shape &s = r.front();
        HashSet<jsid>::AddPtr p = seen.lookupForAdd(s.propid());
        if (!p) {
            if (!seen.add(p, s.propid()))
                return NULL;
            if (!shapes.append(&s))
                return NULL;
        }
    }

    


    RootedVarShape shape(cx);
    shape = initialShape(cx);
    for (int i = shapes.length() - 1; i >= 0; --i) {
        shape = shape->getChildBinding(cx, shapes[i]);
        if (!shape)
            return NULL;
    }

    return shape;
}

bool
Bindings::getLocalNameArray(JSContext *cx, Vector<JSAtom *> *namesp)
{
    JS_ASSERT(lastBinding);
    JS_ASSERT(count() > 0);

    Vector<JSAtom *> &names = *namesp;
    JS_ASSERT(names.empty());

    unsigned n = count();
    if (!names.growByUninitialized(n))
        return false;

#ifdef DEBUG
    JSAtom * const POISON = reinterpret_cast<JSAtom *>(0xdeadbeef);
    for (unsigned i = 0; i < n; i++)
        names[i] = POISON;
#endif

    for (Shape::Range r = lastBinding->all(); !r.empty(); r.popFront()) {
        const Shape &shape = r.front();
        unsigned index = uint16_t(shape.shortid());

        if (shape.getter() == CallObject::getArgOp) {
            JS_ASSERT(index < nargs);
        } else {
            JS_ASSERT(index < nvars);
            index += nargs;
        }

        if (JSID_IS_ATOM(shape.propid())) {
            names[index] = JSID_TO_ATOM(shape.propid());
        } else {
            JS_ASSERT(JSID_IS_INT(shape.propid()));
            JS_ASSERT(shape.getter() == CallObject::getArgOp);
            names[index] = NULL;
        }
    }

#ifdef DEBUG
    for (unsigned i = 0; i < n; i++)
        JS_ASSERT(names[i] != POISON);
#endif

    return true;
}

const Shape *
Bindings::lastArgument() const
{
    JS_ASSERT(lastBinding);

    const js::Shape *shape = lastVariable();
    if (nvars > 0) {
        while (shape->previous() && shape->getter() != CallObject::getArgOp)
            shape = shape->previous();
    }
    return shape;
}

const Shape *
Bindings::lastVariable() const
{
    JS_ASSERT(lastBinding);
    return lastBinding;
}

void
Bindings::makeImmutable()
{
    JS_ASSERT(lastBinding);
    JS_ASSERT(!lastBinding->inDictionary());
}

void
Bindings::trace(JSTracer *trc)
{
    if (lastBinding)
        MarkShape(trc, &lastBinding, "shape");
}

} 

template<XDRMode mode>
static bool
XDRScriptConst(XDRState<mode> *xdr, HeapValue *vp)
{
    




    enum ConstTag {
        SCRIPT_INT     = 0,
        SCRIPT_DOUBLE  = 1,
        SCRIPT_STRING  = 2,
        SCRIPT_TRUE    = 3,
        SCRIPT_FALSE   = 4,
        SCRIPT_NULL    = 5,
        SCRIPT_VOID    = 6
    };

    uint32_t tag;
    if (mode == XDR_ENCODE) {
        if (vp->isInt32()) {
            tag = SCRIPT_INT;
        } else if (vp->isDouble()) {
            tag = SCRIPT_DOUBLE;
        } else if (vp->isString()) {
            tag = SCRIPT_STRING;
        } else if (vp->isTrue()) {
            tag = SCRIPT_TRUE;
        } else if (vp->isFalse()) {
            tag = SCRIPT_FALSE;
        } else if (vp->isNull()) {
            tag = SCRIPT_NULL;
        } else {
            JS_ASSERT(vp->isUndefined());
            tag = SCRIPT_VOID;
        }
    }

    if (!xdr->codeUint32(&tag))
        return false;

    switch (tag) {
      case SCRIPT_INT: {
        uint32_t i;
        if (mode == XDR_ENCODE)
            i = uint32_t(vp->toInt32());
        if (!xdr->codeUint32(&i))
            return JS_FALSE;
        if (mode == XDR_DECODE)
            vp->init(Int32Value(int32_t(i)));
        break;
      }
      case SCRIPT_DOUBLE: {
        double d;
        if (mode == XDR_ENCODE)
            d = vp->toDouble();
        if (!xdr->codeDouble(&d))
            return false;
        if (mode == XDR_DECODE)
            vp->init(DoubleValue(d));
        break;
      }
      case SCRIPT_STRING: {
        JSString *str;
        if (mode == XDR_ENCODE)
            str = vp->toString();
        if (!xdr->codeString(&str))
            return false;
        if (mode == XDR_DECODE)
            vp->init(StringValue(str));
        break;
      }
      case SCRIPT_TRUE:
        if (mode == XDR_DECODE)
            vp->init(BooleanValue(true));
        break;
      case SCRIPT_FALSE:
        if (mode == XDR_DECODE)
            vp->init(BooleanValue(false));
        break;
      case SCRIPT_NULL:
        if (mode == XDR_DECODE)
            vp->init(NullValue());
        break;
      case SCRIPT_VOID:
        if (mode == XDR_DECODE)
            vp->init(UndefinedValue());
        break;
    }
    return true;
}

template<XDRMode mode>
bool
js::XDRScript(XDRState<mode> *xdr, JSScript **scriptp, JSScript *parentScript)
{
    enum ScriptBits {
        NoScriptRval,
        SavedCallerFun,
        StrictModeCode,
        ContainsDynamicNameAccess,
        ArgumentsHasLocalBinding,
        NeedsArgsObj,
        OwnFilename,
        ParentFilename,
        IsGenerator
    };

    uint32_t length, lineno, nslots;
    uint32_t natoms, nsrcnotes, ntrynotes, nobjects, nregexps, nconsts, nClosedArgs, nClosedVars, i;
    uint32_t prologLength, version;
    uint32_t nTypeSets = 0;
    uint32_t scriptBits = 0;

    JSContext *cx = xdr->cx();
    JSScript *script;
    nsrcnotes = ntrynotes = natoms = nobjects = nregexps = nconsts = nClosedArgs = nClosedVars = 0;
    jssrcnote *notes = NULL;

    
    uint16_t nargs, nvars;
#if defined(DEBUG) || defined(__GNUC__) 
    script = NULL;
    nargs = nvars = Bindings::BINDING_COUNT_LIMIT;
#endif
    uint32_t argsVars;
    if (mode == XDR_ENCODE) {
        script = *scriptp;
        JS_ASSERT_IF(parentScript, parentScript->compartment() == script->compartment());
    
        
        JS_ASSERT(!JSScript::isValidOffset(script->globalsOffset));

        nargs = script->bindings.numArgs();
        nvars = script->bindings.numVars();
        argsVars = (nargs << 16) | nvars;
    }
    if (!xdr->codeUint32(&argsVars))
        return false;
    if (mode == XDR_DECODE) {
        nargs = argsVars >> 16;
        nvars = argsVars & 0xFFFF;
    }
    JS_ASSERT(nargs != Bindings::BINDING_COUNT_LIMIT);
    JS_ASSERT(nvars != Bindings::BINDING_COUNT_LIMIT);

    Bindings bindings(cx);
    uint32_t nameCount = nargs + nvars;
    if (nameCount > 0) {
        LifoAllocScope las(&cx->tempLifoAlloc());

        







        unsigned bitmapLength = JS_HOWMANY(nameCount, JS_BITS_PER_UINT32);
        uint32_t *bitmap = cx->tempLifoAlloc().newArray<uint32_t>(bitmapLength);
        if (!bitmap) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        Vector<JSAtom *> names(cx);
        if (mode == XDR_ENCODE) {
            if (!script->bindings.getLocalNameArray(cx, &names))
                return false;
            PodZero(bitmap, bitmapLength);
            for (unsigned i = 0; i < nameCount; i++) {
                if (i < nargs && names[i])
                    bitmap[i >> JS_BITS_PER_UINT32_LOG2] |= JS_BIT(i & (JS_BITS_PER_UINT32 - 1));
            }
        }
        for (unsigned i = 0; i < bitmapLength; ++i) {
            if (!xdr->codeUint32(&bitmap[i]))
                return false;
        }

        for (unsigned i = 0; i < nameCount; i++) {
            if (i < nargs &&
                !(bitmap[i >> JS_BITS_PER_UINT32_LOG2] & JS_BIT(i & (JS_BITS_PER_UINT32 - 1))))
            {
                if (mode == XDR_DECODE) {
                    uint16_t dummy;
                    if (!bindings.addDestructuring(cx, &dummy))
                        return false;
                } else {
                    JS_ASSERT(!names[i]);
                }
                continue;
            }

            JSAtom *name;
            if (mode == XDR_ENCODE)
                name = names[i];
            if (!XDRAtom(xdr, &name))
                return false;
            if (mode == XDR_DECODE) {
                BindingKind kind = (i < nargs)
                                   ? ARGUMENT
                                   : (bitmap[i >> JS_BITS_PER_UINT32_LOG2] &
                                      JS_BIT(i & (JS_BITS_PER_UINT32 - 1))
                                     ? CONSTANT
                                     : VARIABLE);
                if (!bindings.add(cx, name, kind))
                    return false;
            }
        }
    }

    if (mode == XDR_DECODE) {
        if (!bindings.ensureShape(cx))
            return false;
        bindings.makeImmutable();
    }

    if (mode == XDR_ENCODE)
        length = script->length;
    if (!xdr->codeUint32(&length))
        return JS_FALSE;

    if (mode == XDR_ENCODE) {
        prologLength = script->mainOffset;
        JS_ASSERT(script->getVersion() != JSVERSION_UNKNOWN);
        version = (uint32_t)script->getVersion() | (script->nfixed << 16);
        lineno = script->lineno;
        nslots = (uint32_t)script->nslots;
        nslots = (uint32_t)((script->staticLevel << 16) | script->nslots);
        natoms = script->natoms;

        notes = script->notes();
        nsrcnotes = script->numNotes();

        if (JSScript::isValidOffset(script->constsOffset))
            nconsts = script->consts()->length;
        if (JSScript::isValidOffset(script->objectsOffset))
            nobjects = script->objects()->length;
        if (JSScript::isValidOffset(script->regexpsOffset))
            nregexps = script->regexps()->length;
        if (JSScript::isValidOffset(script->trynotesOffset))
            ntrynotes = script->trynotes()->length;
        
        nClosedArgs = script->numClosedArgs();
        nClosedVars = script->numClosedVars();

        nTypeSets = script->nTypeSets;

        if (script->noScriptRval)
            scriptBits |= (1 << NoScriptRval);
        if (script->savedCallerFun)
            scriptBits |= (1 << SavedCallerFun);
        if (script->strictModeCode)
            scriptBits |= (1 << StrictModeCode);
        if (script->bindingsAccessedDynamically)
            scriptBits |= (1 << ContainsDynamicNameAccess);
        if (script->argumentsHasLocalBinding())
            scriptBits |= (1 << ArgumentsHasLocalBinding);
        if (script->analyzedArgsUsage() && script->needsArgsObj())
            scriptBits |= (1 << NeedsArgsObj);
        if (script->filename) {
            scriptBits |= (parentScript && parentScript->filename == script->filename)
                          ? (1 << ParentFilename)
                          : (1 << OwnFilename);
        }
        if (script->isGenerator)
            scriptBits |= (1 << IsGenerator);

        JS_ASSERT(!script->compileAndGo);
        JS_ASSERT(!script->hasSingletons);
    }

    if (!xdr->codeUint32(&prologLength))
        return JS_FALSE;
    if (!xdr->codeUint32(&version))
        return JS_FALSE;

    



    if (!xdr->codeUint32(&natoms))
        return JS_FALSE;
    if (!xdr->codeUint32(&nsrcnotes))
        return JS_FALSE;
    if (!xdr->codeUint32(&ntrynotes))
        return JS_FALSE;
    if (!xdr->codeUint32(&nobjects))
        return JS_FALSE;
    if (!xdr->codeUint32(&nregexps))
        return JS_FALSE;
    if (!xdr->codeUint32(&nconsts))
        return JS_FALSE;
    if (!xdr->codeUint32(&nClosedArgs))
        return JS_FALSE;
    if (!xdr->codeUint32(&nClosedVars))
        return JS_FALSE;
    if (!xdr->codeUint32(&nTypeSets))
        return JS_FALSE;
    if (!xdr->codeUint32(&scriptBits))
        return JS_FALSE;

    if (mode == XDR_DECODE) {
        
        JSVersion version_ = JSVersion(version & JS_BITMASK(16));
        JS_ASSERT((version_ & VersionFlags::FULL_MASK) == unsigned(version_));
        script = JSScript::NewScript(cx, length, nsrcnotes, natoms, nobjects,
                                     nregexps, ntrynotes, nconsts, 0, nClosedArgs,
                                     nClosedVars, nTypeSets, version_);
        if (!script)
            return JS_FALSE;

        script->bindings.transfer(cx, &bindings);
        JS_ASSERT(!script->mainOffset);
        script->mainOffset = prologLength;
        script->nfixed = uint16_t(version >> 16);

        
        notes = script->notes();
        *scriptp = script;

        if (scriptBits & (1 << NoScriptRval))
            script->noScriptRval = true;
        if (scriptBits & (1 << SavedCallerFun))
            script->savedCallerFun = true;
        if (scriptBits & (1 << StrictModeCode))
            script->strictModeCode = true;
        if (scriptBits & (1 << ContainsDynamicNameAccess))
            script->bindingsAccessedDynamically = true;
        if (scriptBits & (1 << ArgumentsHasLocalBinding)) {
            PropertyName *arguments = cx->runtime->atomState.argumentsAtom;
            unsigned slot;
            DebugOnly<BindingKind> kind = script->bindings.lookup(cx, arguments, &slot);
            JS_ASSERT(kind == VARIABLE || kind == CONSTANT);
            script->setArgumentsHasLocalBinding(slot);
        }
        if (scriptBits & (1 << NeedsArgsObj))
            script->setNeedsArgsObj(true);
        if (scriptBits & (1 << IsGenerator))
            script->isGenerator = true;
    }

    JS_STATIC_ASSERT(sizeof(jsbytecode) == 1);
    JS_STATIC_ASSERT(sizeof(jssrcnote) == 1);
    if (!xdr->codeBytes(script->code, length) ||
        !xdr->codeBytes(notes, nsrcnotes) ||
        !xdr->codeUint32(&lineno) ||
        !xdr->codeUint32(&nslots)) {
        return false;
    }

    if (scriptBits & (1 << OwnFilename)) {
        const char *filename;
        if (mode == XDR_ENCODE)
            filename = script->filename;
        if (!xdr->codeCString(&filename))
            return false;
        if (mode == XDR_DECODE) {
            script->filename = SaveScriptFilename(cx, filename);
            if (!script->filename)
                return false;
        }
    } else if (scriptBits & (1 << ParentFilename)) {
        JS_ASSERT(parentScript);
        if (mode == XDR_DECODE)
            script->filename = parentScript->filename;
    }

    if (mode == XDR_DECODE) {
        script->lineno = lineno;
        script->nslots = uint16_t(nslots);
        script->staticLevel = uint16_t(nslots >> 16);
        xdr->initScriptPrincipals(script);
    }

    for (i = 0; i != natoms; ++i) {
        if (mode == XDR_DECODE) {
            JSAtom *tmp = NULL;
            if (!XDRAtom(xdr, &tmp))
                return false;
            script->atoms[i].init(tmp);
        } else {
            JSAtom *tmp = script->atoms[i];
            if (!XDRAtom(xdr, &tmp))
                return false;
        }
    }

    





    for (i = 0; i != nobjects; ++i) {
        HeapPtr<JSObject> *objp = &script->objects()->vector[i];
        uint32_t isBlock;
        if (mode == XDR_ENCODE) {
            JSObject *obj = *objp;
            JS_ASSERT(obj->isFunction() || obj->isStaticBlock());
            isBlock = obj->isBlock() ? 1 : 0;
        }
        if (!xdr->codeUint32(&isBlock))
            return false;
        if (isBlock == 0) {
            JSObject *tmp = *objp;
            if (!XDRInterpretedFunction(xdr, &tmp, parentScript))
                return false;
            *objp = tmp;
        } else {
            JS_ASSERT(isBlock == 1);
            StaticBlockObject *tmp = static_cast<StaticBlockObject *>(objp->get());
            if (!XDRStaticBlockObject(xdr, script, &tmp))
                return false;
            *objp = tmp;
        }
    }
    for (i = 0; i != nregexps; ++i) {
        if (!XDRScriptRegExpObject(xdr, &script->regexps()->vector[i]))
            return false;
    }
    for (i = 0; i != nClosedArgs; ++i) {
        if (!xdr->codeUint32(&script->closedArgs()->vector[i]))
            return false;
    }
    for (i = 0; i != nClosedVars; ++i) {
        if (!xdr->codeUint32(&script->closedVars()->vector[i]))
            return false;
    }

    if (ntrynotes != 0) {
        



        JSTryNote *tn, *tnfirst;
        uint32_t kindAndDepth;
        JS_STATIC_ASSERT(sizeof(tn->kind) == sizeof(uint8_t));
        JS_STATIC_ASSERT(sizeof(tn->stackDepth) == sizeof(uint16_t));

        tnfirst = script->trynotes()->vector;
        JS_ASSERT(script->trynotes()->length == ntrynotes);
        tn = tnfirst + ntrynotes;
        do {
            --tn;
            if (mode == XDR_ENCODE) {
                kindAndDepth = (uint32_t(tn->kind) << 16)
                               | uint32_t(tn->stackDepth);
            }
            if (!xdr->codeUint32(&kindAndDepth) ||
                !xdr->codeUint32(&tn->start) ||
                !xdr->codeUint32(&tn->length)) {
                return false;
            }
            if (mode == XDR_DECODE) {
                tn->kind = uint8_t(kindAndDepth >> 16);
                tn->stackDepth = uint16_t(kindAndDepth);
            }
        } while (tn != tnfirst);
    }

    if (nconsts) {
        HeapValue *vector = script->consts()->vector;
        for (i = 0; i != nconsts; ++i) {
            if (!XDRScriptConst(xdr, &vector[i]))
                return false;
        }
    }

    if (mode == XDR_DECODE) {
        if (cx->hasRunOption(JSOPTION_PCCOUNT))
            (void) script->initScriptCounts(cx);
        *scriptp = script;
    }

    return true;
}

template bool
js::XDRScript(XDRState<XDR_ENCODE> *xdr, JSScript **scriptp, JSScript *parentScript);

template bool
js::XDRScript(XDRState<XDR_DECODE> *xdr, JSScript **scriptp, JSScript *parentScript);

bool
JSScript::initScriptCounts(JSContext *cx)
{
    JS_ASSERT(!hasScriptCounts);

    size_t n = 0;

    jsbytecode *pc, *next;
    for (pc = code; pc < code + length; pc = next) {
        n += PCCounts::numCounts(JSOp(*pc));
        next = pc + GetBytecodeLength(pc);
    }

    size_t bytes = (length * sizeof(PCCounts)) + (n * sizeof(double));
    char *cursor = (char *) cx->calloc_(bytes);
    if (!cursor)
        return false;

    
    ScriptCountsMap *map = compartment()->scriptCountsMap;
    if (!map) {
        map = cx->new_<ScriptCountsMap>();
        if (!map || !map->init()) {
            cx->free_(cursor);
            cx->delete_(map);
            return false;
        }
        compartment()->scriptCountsMap = map;
    }

    DebugOnly<char *> base = cursor;

    ScriptCounts scriptCounts;
    scriptCounts.pcCountsVector = (PCCounts *) cursor;
    cursor += length * sizeof(PCCounts);

    for (pc = code; pc < code + length; pc = next) {
        scriptCounts.pcCountsVector[pc - code].counts = (double *) cursor;
        size_t capacity = PCCounts::numCounts(JSOp(*pc));
#ifdef DEBUG
        scriptCounts.pcCountsVector[pc - code].capacity = capacity;
#endif
        cursor += capacity * sizeof(double);
        next = pc + GetBytecodeLength(pc);
    }

    if (!map->putNew(this, scriptCounts)) {
        cx->free_(cursor);
        cx->delete_(map);
        return false;
    }
    hasScriptCounts = true; 

    JS_ASSERT(size_t(cursor - base) == bytes);

    
    InterpreterFrames *frames;
    for (frames = cx->runtime->interpreterFrames; frames; frames = frames->older)
        frames->enableInterruptsIfRunning(this);

    return true;
}

js::PCCounts
JSScript::getPCCounts(jsbytecode *pc) {
    JS_ASSERT(hasScriptCounts);
    JS_ASSERT(size_t(pc - code) < length);
    ScriptCountsMap *map = compartment()->scriptCountsMap;
    JS_ASSERT(map);
    ScriptCountsMap::Ptr p = map->lookup(this);
    JS_ASSERT(p);
    return p->value.pcCountsVector[pc - code];
}

ScriptCounts
JSScript::releaseScriptCounts()
{
    JS_ASSERT(hasScriptCounts);
    ScriptCountsMap *map = compartment()->scriptCountsMap;
    JS_ASSERT(map);
    ScriptCountsMap::Ptr p = map->lookup(this);
    JS_ASSERT(p);
    ScriptCounts counts = p->value;
    map->remove(p);
    hasScriptCounts = false;
    return counts;
}

void
JSScript::destroyScriptCounts(FreeOp *fop)
{
    if (hasScriptCounts) {
        ScriptCounts scriptCounts = releaseScriptCounts();
        fop->free_(scriptCounts.pcCountsVector);
    }
}





const char *
js::SaveScriptFilename(JSContext *cx, const char *filename)
{
    JSCompartment *comp = cx->compartment;

    ScriptFilenameTable::AddPtr p = comp->scriptFilenameTable.lookupForAdd(filename);
    if (!p) {
        size_t size = offsetof(ScriptFilenameEntry, filename) + strlen(filename) + 1;
        ScriptFilenameEntry *entry = (ScriptFilenameEntry *) cx->malloc_(size);
        if (!entry)
            return NULL;
        entry->marked = false;
        strcpy(entry->filename, filename);

        if (!comp->scriptFilenameTable.add(p, entry)) {
            Foreground::free_(entry);
            JS_ReportOutOfMemory(cx);
            return NULL;
        }
    }

    ScriptFilenameEntry *sfe = *p;
#ifdef JSGC_INCREMENTAL
    





    if (comp->needsBarrier() && !sfe->marked)
        sfe->marked = true;
#endif

    return sfe->filename;
}




#define FILENAME_TO_SFE(fn) \
    ((ScriptFilenameEntry *) ((fn) - offsetof(ScriptFilenameEntry, filename)))

void
js::MarkScriptFilename(const char *filename)
{
    ScriptFilenameEntry *sfe = FILENAME_TO_SFE(filename);
    sfe->marked = true;
}

void
js::SweepScriptFilenames(JSCompartment *comp)
{
    ScriptFilenameTable &table = comp->scriptFilenameTable;
    for (ScriptFilenameTable::Enum e(table); !e.empty(); e.popFront()) {
        ScriptFilenameEntry *entry = e.front();
        if (entry->marked) {
            entry->marked = false;
        } else if (!comp->rt->gcKeepAtoms) {
            Foreground::free_(entry);
            e.removeFront();
        }
    }
}

void
js::FreeScriptFilenames(JSCompartment *comp)
{
    ScriptFilenameTable &table = comp->scriptFilenameTable;
    for (ScriptFilenameTable::Enum e(table); !e.empty(); e.popFront())
        Foreground::free_(e.front());

    table.clear();
}



















































#define KEEPS_JSVAL_ALIGNMENT(T) \
    (JS_ALIGNMENT_OF(jsval) % JS_ALIGNMENT_OF(T) == 0 && \
     sizeof(T) % sizeof(jsval) == 0)

#define HAS_JSVAL_ALIGNMENT(T) \
    (JS_ALIGNMENT_OF(jsval) == JS_ALIGNMENT_OF(T) && \
     sizeof(T) == sizeof(jsval))

#define NO_PADDING_BETWEEN_ENTRIES(T1, T2) \
    (JS_ALIGNMENT_OF(T1) % JS_ALIGNMENT_OF(T2) == 0)







JS_STATIC_ASSERT(KEEPS_JSVAL_ALIGNMENT(JSConstArray));
JS_STATIC_ASSERT(KEEPS_JSVAL_ALIGNMENT(JSObjectArray));     
JS_STATIC_ASSERT(KEEPS_JSVAL_ALIGNMENT(JSTryNoteArray));
JS_STATIC_ASSERT(KEEPS_JSVAL_ALIGNMENT(GlobalSlotArray));
JS_STATIC_ASSERT(KEEPS_JSVAL_ALIGNMENT(ClosedSlotArray));   


JS_STATIC_ASSERT(HAS_JSVAL_ALIGNMENT(HeapValue));
JS_STATIC_ASSERT(NO_PADDING_BETWEEN_ENTRIES(HeapValue, JSAtom *));
JS_STATIC_ASSERT(NO_PADDING_BETWEEN_ENTRIES(JSAtom *, HeapPtrObject));
JS_STATIC_ASSERT(NO_PADDING_BETWEEN_ENTRIES(HeapPtrObject, HeapPtrObject));
JS_STATIC_ASSERT(NO_PADDING_BETWEEN_ENTRIES(HeapPtrObject, JSTryNote));
JS_STATIC_ASSERT(NO_PADDING_BETWEEN_ENTRIES(JSTryNote, GlobalSlotArray::Entry));
JS_STATIC_ASSERT(NO_PADDING_BETWEEN_ENTRIES(GlobalSlotArray::Entry, uint32_t));
JS_STATIC_ASSERT(NO_PADDING_BETWEEN_ENTRIES(uint32_t, uint32_t));
JS_STATIC_ASSERT(NO_PADDING_BETWEEN_ENTRIES(uint32_t, jsbytecode));
JS_STATIC_ASSERT(NO_PADDING_BETWEEN_ENTRIES(jsbytecode, jssrcnote));







JS_STATIC_ASSERT(sizeof(JSConstArray) +
                 sizeof(JSObjectArray) +
                 sizeof(JSObjectArray) +
                 sizeof(JSTryNoteArray) +
                 sizeof(js::GlobalSlotArray) +
                 sizeof(js::ClosedSlotArray)
                 < JSScript::INVALID_OFFSET);
JS_STATIC_ASSERT(JSScript::INVALID_OFFSET <= 255);

JSScript *
JSScript::NewScript(JSContext *cx, uint32_t length, uint32_t nsrcnotes, uint32_t natoms,
                    uint32_t nobjects, uint32_t nregexps,
                    uint32_t ntrynotes, uint32_t nconsts, uint32_t nglobals,
                    uint16_t nClosedArgs, uint16_t nClosedVars, uint32_t nTypeSets, JSVersion version)
{
    size_t size = 0;

    if (nconsts != 0)
        size += sizeof(JSConstArray) + nconsts * sizeof(Value);
    size += sizeof(JSAtom *) * natoms;
    if (nobjects != 0)
        size += sizeof(JSObjectArray) + nobjects * sizeof(JSObject *);
    if (nregexps != 0)
        size += sizeof(JSObjectArray) + nregexps * sizeof(JSObject *);
    if (ntrynotes != 0)
        size += sizeof(JSTryNoteArray) + ntrynotes * sizeof(JSTryNote);
    if (nglobals != 0)
        size += sizeof(GlobalSlotArray) + nglobals * sizeof(GlobalSlotArray::Entry);
    if (nClosedArgs != 0)
        size += sizeof(ClosedSlotArray) + nClosedArgs * sizeof(uint32_t);
    if (nClosedVars != 0)
        size += sizeof(ClosedSlotArray) + nClosedVars * sizeof(uint32_t);

    size += length * sizeof(jsbytecode);
    size += nsrcnotes * sizeof(jssrcnote);

    



    JS_STATIC_ASSERT(sizeof(Value) == sizeof(double));
    uint8_t *data = static_cast<uint8_t *>(cx->calloc_(JS_ROUNDUP(size, sizeof(Value))));
    if (!data)
        return NULL;

    JSScript *script = js_NewGCScript(cx);
    if (!script) {
        Foreground::free_(data);
        return NULL;
    }

    PodZero(script);
    script->data  = data;
    script->length = length;
    script->version = version;
    new (&script->bindings) Bindings(cx);

    uint8_t *cursor = data;
    if (nconsts != 0) {
        script->constsOffset = uint8_t(cursor - data);
        cursor += sizeof(JSConstArray);
    } else {
        script->constsOffset = JSScript::INVALID_OFFSET;
    }
    if (nobjects != 0) {
        script->objectsOffset = uint8_t(cursor - data);
        cursor += sizeof(JSObjectArray);
    } else {
        script->objectsOffset = JSScript::INVALID_OFFSET;
    }
    if (nregexps != 0) {
        script->regexpsOffset = uint8_t(cursor - data);
        cursor += sizeof(JSObjectArray);
    } else {
        script->regexpsOffset = JSScript::INVALID_OFFSET;
    }
    if (ntrynotes != 0) {
        script->trynotesOffset = uint8_t(cursor - data);
        cursor += sizeof(JSTryNoteArray);
    } else {
        script->trynotesOffset = JSScript::INVALID_OFFSET;
    }
    if (nglobals != 0) {
        script->globalsOffset = uint8_t(cursor - data);
        cursor += sizeof(GlobalSlotArray);
    } else {
        script->globalsOffset = JSScript::INVALID_OFFSET;
    }
    if (nClosedArgs != 0) {
        script->closedArgsOffset = uint8_t(cursor - data);
        cursor += sizeof(ClosedSlotArray);
    } else {
        script->closedArgsOffset = JSScript::INVALID_OFFSET;
    }
    JS_ASSERT(cursor - data < 0xFF);
    if (nClosedVars != 0) {
        script->closedVarsOffset = uint8_t(cursor - data);
        cursor += sizeof(ClosedSlotArray);
    } else {
        script->closedVarsOffset = JSScript::INVALID_OFFSET;
    }

    if (nconsts != 0) {
        JS_ASSERT(reinterpret_cast<uintptr_t>(cursor) % sizeof(jsval) == 0);
        script->consts()->length = nconsts;
        script->consts()->vector = (HeapValue *)cursor;
        cursor += nconsts * sizeof(script->consts()->vector[0]);
    }

    if (natoms != 0) {
        script->natoms = natoms;
        script->atoms = reinterpret_cast<HeapPtrAtom *>(cursor);
        cursor += natoms * sizeof(script->atoms[0]);
    }

    if (nobjects != 0) {
        script->objects()->length = nobjects;
        script->objects()->vector = (HeapPtr<JSObject> *)cursor;
        cursor += nobjects * sizeof(script->objects()->vector[0]);
    }

    if (nregexps != 0) {
        script->regexps()->length = nregexps;
        script->regexps()->vector = (HeapPtr<JSObject> *)cursor;
        cursor += nregexps * sizeof(script->regexps()->vector[0]);
    }

    if (ntrynotes != 0) {
        script->trynotes()->length = ntrynotes;
        script->trynotes()->vector = reinterpret_cast<JSTryNote *>(cursor);
        size_t vectorSize = ntrynotes * sizeof(script->trynotes()->vector[0]);
#ifdef DEBUG
        memset(cursor, 0, vectorSize);
#endif
        cursor += vectorSize;
    }

    if (nglobals != 0) {
        script->globals()->length = nglobals;
        script->globals()->vector = reinterpret_cast<GlobalSlotArray::Entry *>(cursor);
        cursor += nglobals * sizeof(script->globals()->vector[0]);
    }

    if (nClosedArgs != 0) {
        script->closedArgs()->length = nClosedArgs;
        script->closedArgs()->vector = reinterpret_cast<uint32_t *>(cursor);
        cursor += nClosedArgs * sizeof(script->closedArgs()->vector[0]);
    }

    if (nClosedVars != 0) {
        script->closedVars()->length = nClosedVars;
        script->closedVars()->vector = reinterpret_cast<uint32_t *>(cursor);
        cursor += nClosedVars * sizeof(script->closedVars()->vector[0]);
    }

    JS_ASSERT(nTypeSets <= UINT16_MAX);
    script->nTypeSets = uint16_t(nTypeSets);

    script->code = (jsbytecode *)cursor;
    JS_ASSERT(cursor + length * sizeof(jsbytecode) + nsrcnotes * sizeof(jssrcnote) == data + size);

#ifdef DEBUG
    script->id_ = 0;
#endif

    JS_ASSERT(script->getVersion() == version);
    return script;
}

JSScript *
JSScript::NewScriptFromEmitter(JSContext *cx, BytecodeEmitter *bce)
{
    uint32_t mainLength, prologLength, nfixed;
    JSScript *script;
    const char *filename;
    JSFunction *fun;

    
    JS_ASSERT(bce->atomIndices->count() <= INDEX_LIMIT);
    JS_ASSERT(bce->objectList.length <= INDEX_LIMIT);
    JS_ASSERT(bce->regexpList.length <= INDEX_LIMIT);

    mainLength = bce->offset();
    prologLength = bce->prologOffset();

    if (!bce->bindings.ensureShape(cx))
        return NULL;

    uint32_t nsrcnotes = uint32_t(bce->countFinalSourceNotes());
    uint16_t nClosedArgs = uint16_t(bce->closedArgs.length());
    JS_ASSERT(nClosedArgs == bce->closedArgs.length());
    uint16_t nClosedVars = uint16_t(bce->closedVars.length());
    JS_ASSERT(nClosedVars == bce->closedVars.length());
    script = NewScript(cx, prologLength + mainLength, nsrcnotes,
                       bce->atomIndices->count(), bce->objectList.length,
                       bce->regexpList.length, bce->ntrynotes, bce->constList.length(),
                       bce->globalUses.length(), nClosedArgs, nClosedVars,
                       bce->typesetCount, bce->version());
    if (!script)
        return NULL;

    bce->bindings.makeImmutable();

    JS_ASSERT(script->mainOffset == 0);
    script->mainOffset = prologLength;
    PodCopy<jsbytecode>(script->code, bce->prologBase(), prologLength);
    PodCopy<jsbytecode>(script->main(), bce->base(), mainLength);
    nfixed = bce->inFunction() ? bce->bindings.numVars() : 0;
    JS_ASSERT(nfixed < SLOTNO_LIMIT);
    script->nfixed = uint16_t(nfixed);
    InitAtomMap(cx, bce->atomIndices.getMap(), script->atoms);

    filename = bce->parser->tokenStream.getFilename();
    if (filename) {
        script->filename = SaveScriptFilename(cx, filename);
        if (!script->filename)
            return NULL;
    }
    script->lineno = bce->firstLine;
    if (script->nfixed + bce->maxStackDepth >= JS_BIT(16)) {
        ReportCompileErrorNumber(cx, bce->tokenStream(), NULL, JSREPORT_ERROR, JSMSG_NEED_DIET,
                                 "script");
        return NULL;
    }
    script->nslots = script->nfixed + bce->maxStackDepth;
    script->staticLevel = uint16_t(bce->staticLevel);
    script->principals = bce->parser->principals;

    if (script->principals)
        JS_HoldPrincipals(script->principals);

    
    script->originPrincipals = bce->parser->originPrincipals;
    if (!script->originPrincipals)
        script->originPrincipals = script->principals;
    if (script->originPrincipals)
        JS_HoldPrincipals(script->originPrincipals);

    script->sourceMap = (jschar *) bce->parser->tokenStream.releaseSourceMap();

    if (!FinishTakingSrcNotes(cx, bce, script->notes()))
        return NULL;
    if (bce->ntrynotes != 0)
        FinishTakingTryNotes(bce, script->trynotes());
    if (bce->objectList.length != 0)
        bce->objectList.finish(script->objects());
    if (bce->regexpList.length != 0)
        bce->regexpList.finish(script->regexps());
    if (bce->constList.length() != 0)
        bce->constList.finish(script->consts());
    if (bce->flags & TCF_NO_SCRIPT_RVAL)
        script->noScriptRval = true;
    if (bce->flags & TCF_STRICT_MODE_CODE)
        script->strictModeCode = true;
    if (bce->flags & TCF_COMPILE_N_GO) {
        script->compileAndGo = true;
        const StackFrame *fp = bce->parser->callerFrame;
        if (fp && fp->isFunctionFrame())
            script->savedCallerFun = true;
    }
    if (bce->bindingsAccessedDynamically())
        script->bindingsAccessedDynamically = true;
    if (bce->flags & TCF_HAS_SINGLETONS)
        script->hasSingletons = true;
    if (bce->flags & TCF_FUN_IS_GENERATOR)
        script->isGenerator = true;

    if (bce->argumentsHasLocalBinding()) {
        script->setArgumentsHasLocalBinding(bce->argumentsLocalSlot());
        if (bce->definitelyNeedsArgsObj())
            script->setNeedsArgsObj(true);
    }

    if (bce->globalUses.length()) {
        PodCopy<GlobalSlotArray::Entry>(script->globals()->vector, &bce->globalUses[0],
                                        bce->globalUses.length());
    }

    if (nClosedArgs)
        PodCopy<uint32_t>(script->closedArgs()->vector, &bce->closedArgs[0], nClosedArgs);
    if (nClosedVars)
        PodCopy<uint32_t>(script->closedVars()->vector, &bce->closedVars[0], nClosedVars);

    script->bindings.transfer(cx, &bce->bindings);

    fun = NULL;
    if (bce->inFunction()) {
        



        fun = bce->fun();
        JS_ASSERT(fun->isInterpreted());
        JS_ASSERT(!fun->script());
        if (bce->flags & TCF_FUN_HEAVYWEIGHT)
            fun->flags |= JSFUN_HEAVYWEIGHT;

        


        bool singleton =
            cx->typeInferenceEnabled() &&
            bce->parent &&
            bce->parent->compiling() &&
            bce->parent->asBytecodeEmitter()->checkSingletonContext();

        if (!script->typeSetFunction(cx, fun, singleton))
            return NULL;

        fun->setScript(script);
        script->globalObject = fun->getParent() ? &fun->getParent()->global() : NULL;
    } else {
        



        if (bce->flags & TCF_NEED_SCRIPT_GLOBAL)
            script->globalObject = GetCurrentGlobal(cx);
    }

    
    js_CallNewScriptHook(cx, script, fun);
    if (!bce->parent) {
        GlobalObject *compileAndGoGlobal = NULL;
        if (script->compileAndGo) {
            compileAndGoGlobal = script->globalObject;
            if (!compileAndGoGlobal)
                compileAndGoGlobal = &bce->scopeChain()->global();
        }
        Debugger::onNewScript(cx, script, compileAndGoGlobal);
    }

    if (cx->hasRunOption(JSOPTION_PCCOUNT))
        (void) script->initScriptCounts(cx);

    return script;
}

size_t
JSScript::computedSizeOfData()
{
    uint8_t *dataEnd = code + length * sizeof(jsbytecode) + numNotes() * sizeof(jssrcnote);
    JS_ASSERT(dataEnd >= data);
    return dataEnd - data;
}

size_t
JSScript::sizeOfData(JSMallocSizeOfFun mallocSizeOf)
{
    return mallocSizeOf(data);
}





uint32_t
JSScript::numNotes()
{
    jssrcnote *sn;
    jssrcnote *notes_ = notes();
    for (sn = notes_; !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn))
        continue;
    return sn - notes_ + 1;    
}

JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun)
{
    JS_ASSERT(!script->callDestroyHook);
    if (JSNewScriptHook hook = cx->runtime->debugHooks.newScriptHook) {
        AutoKeepAtoms keep(cx->runtime);
        hook(cx, script->filename, script->lineno, script, fun,
             cx->runtime->debugHooks.newScriptHookData);
    }
    script->callDestroyHook = true;
}

void
js::CallDestroyScriptHook(FreeOp *fop, JSScript *script)
{
    if (!script->callDestroyHook)
        return;

    if (JSDestroyScriptHook hook = fop->runtime()->debugHooks.destroyScriptHook)
        hook(fop, script, fop->runtime()->debugHooks.destroyScriptHookData);
    script->callDestroyHook = false;
    script->clearTraps(fop);
}

void
JSScript::finalize(FreeOp *fop)
{
    CallDestroyScriptHook(fop, this);

    JS_ASSERT_IF(principals, originPrincipals);
    if (principals)
        JS_DropPrincipals(fop->runtime(), principals);
    if (originPrincipals)
        JS_DropPrincipals(fop->runtime(), originPrincipals);

    if (types)
        types->destroy();

#ifdef JS_METHODJIT
    mjit::ReleaseScriptCode(fop, this);
# ifdef JS_ION
    if (hasIonScript())
        ion::IonScript::Destroy(fop, ion);
# endif
#endif

    destroyScriptCounts(fop);

    if (sourceMap)
        fop->free_(sourceMap);

    if (debug) {
        jsbytecode *end = code + length;
        for (jsbytecode *pc = code; pc < end; pc++) {
            if (BreakpointSite *site = getBreakpointSite(pc)) {
                
                JS_ASSERT(site->firstBreakpoint() == NULL);
                site->clearTrap(fop, NULL, NULL);
                JS_ASSERT(getBreakpointSite(pc) == NULL);
            }
        }
        fop->free_(debug);
    }

    JS_POISON(data, 0xdb, computedSizeOfData());
    fop->free_(data);
}

namespace js {

static const uint32_t GSN_CACHE_THRESHOLD = 100;
static const uint32_t GSN_CACHE_MAP_INIT_SIZE = 20;

void
GSNCache::purge()
{
    code = NULL;
    if (map.initialized())
        map.finish();
}

} 

jssrcnote *
js_GetSrcNoteCached(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    size_t target = pc - script->code;
    if (target >= size_t(script->length))
        return NULL;

    GSNCache *cache = GetGSNCache(cx);
    if (cache->code == script->code) {
        JS_ASSERT(cache->map.initialized());
        GSNCache::Map::Ptr p = cache->map.lookup(pc);
        return p ? p->value : NULL;
    }

    size_t offset = 0;
    jssrcnote *result;
    for (jssrcnote *sn = script->notes(); ; sn = SN_NEXT(sn)) {
        if (SN_IS_TERMINATOR(sn)) {
            result = NULL;
            break;
        }
        offset += SN_DELTA(sn);
        if (offset == target && SN_IS_GETTABLE(sn)) {
            result = sn;
            break;
        }
    }

    if (cache->code != script->code && script->length >= GSN_CACHE_THRESHOLD) {
        unsigned nsrcnotes = 0;
        for (jssrcnote *sn = script->notes(); !SN_IS_TERMINATOR(sn);
             sn = SN_NEXT(sn)) {
            if (SN_IS_GETTABLE(sn))
                ++nsrcnotes;
        }
        if (cache->code) {
            JS_ASSERT(cache->map.initialized());
            cache->map.finish();
            cache->code = NULL;
        }
        if (cache->map.init(nsrcnotes)) {
            pc = script->code;
            for (jssrcnote *sn = script->notes(); !SN_IS_TERMINATOR(sn);
                 sn = SN_NEXT(sn)) {
                pc += SN_DELTA(sn);
                if (SN_IS_GETTABLE(sn))
                    JS_ALWAYS_TRUE(cache->map.put(pc, sn));
            }
            cache->code = script->code;
        }
    }

    return result;
}

unsigned
js::PCToLineNumber(unsigned startLine, jssrcnote *notes, jsbytecode *code, jsbytecode *pc)
{
    unsigned lineno = startLine;

    




    ptrdiff_t offset = 0;
    ptrdiff_t target = pc - code;
    for (jssrcnote *sn = notes; !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        offset += SN_DELTA(sn);
        SrcNoteType type = (SrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            if (offset <= target)
                lineno = (unsigned) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            if (offset <= target)
                lineno++;
        }
        if (offset > target)
            break;
    }

    return lineno;
}

unsigned
js::PCToLineNumber(JSScript *script, jsbytecode *pc)
{
    
    if (!pc)
        return 0;

    return PCToLineNumber(script->lineno, script->notes(), script->code, pc);
}


#define SN_LINE_LIMIT   (SN_3BYTE_OFFSET_FLAG << 16)

jsbytecode *
js_LineNumberToPC(JSScript *script, unsigned target)
{
    ptrdiff_t offset = 0;
    ptrdiff_t best = -1;
    unsigned lineno = script->lineno;
    unsigned bestdiff = SN_LINE_LIMIT;
    for (jssrcnote *sn = script->notes(); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        



        if (lineno == target && offset >= ptrdiff_t(script->mainOffset))
            goto out;
        if (lineno >= target) {
            unsigned diff = lineno - target;
            if (diff < bestdiff) {
                bestdiff = diff;
                best = offset;
            }
        }
        offset += SN_DELTA(sn);
        SrcNoteType type = (SrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            lineno = (unsigned) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            lineno++;
        }
    }
    if (best >= 0)
        offset = best;
out:
    return script->code + offset;
}

JS_FRIEND_API(unsigned)
js_GetScriptLineExtent(JSScript *script)
{
    unsigned lineno = script->lineno;
    unsigned maxLineNo = 0;
    bool counting = true;
    for (jssrcnote *sn = script->notes(); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        SrcNoteType type = (SrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            if (maxLineNo < lineno)
                maxLineNo = lineno;
            lineno = (unsigned) js_GetSrcNoteOffset(sn, 0);
            counting = true;
            if (maxLineNo < lineno)
                maxLineNo = lineno;
            else
                counting = false;
        } else if (type == SRC_NEWLINE) {
            if (counting)
                lineno++;
        }
    }

    if (maxLineNo > lineno)
        lineno = maxLineNo;

    return 1 + lineno - script->lineno;
}

namespace js {

unsigned
CurrentLine(JSContext *cx)
{
    return PCToLineNumber(cx->fp()->script(), cx->regs().pc);
}

void
CurrentScriptFileLineOriginSlow(JSContext *cx, const char **file, unsigned *linenop,
                                JSPrincipals **origin)
{
    FrameRegsIter iter(cx);
    while (!iter.done() && !iter.fp()->isScriptFrame())
        ++iter;

    if (iter.done()) {
        *file = NULL;
        *linenop = 0;
        *origin = NULL;
        return;
    }

    JSScript *script = iter.fp()->script();
    *file = script->filename;
    *linenop = PCToLineNumber(iter.fp()->script(), iter.pc());
    *origin = script->originPrincipals;
}

}  

JSScript *
js::CloneScript(JSContext *cx, JSScript *script)
{
    JS_ASSERT(cx->compartment != script->compartment());

    
    XDREncoder encoder(cx);

    if (!XDRScript(&encoder, &script, NULL))
        return NULL;

    uint32_t nbytes;
    const void *p = encoder.getData(&nbytes);

    
    XDRDecoder decoder(cx, p, nbytes, cx->compartment->principals, script->originPrincipals);

    JSScript *newScript;
    if (!XDRScript(&decoder, &newScript, NULL))
        return NULL;

    return newScript;
}

bool
JSScript::ensureHasDebug(JSContext *cx)
{
    if (debug)
        return true;

    size_t nbytes = offsetof(DebugScript, breakpoints) + length * sizeof(BreakpointSite*);
    debug = (DebugScript *) cx->calloc_(nbytes);
    if (!debug)
        return false;

    




    InterpreterFrames *frames;
    for (frames = cx->runtime->interpreterFrames; frames; frames = frames->older)
        frames->enableInterruptsIfRunning(this);

    return true;
}

void
JSScript::recompileForStepMode(FreeOp *fop)
{
#ifdef JS_METHODJIT
    if (hasJITCode()) {
        mjit::Recompiler::clearStackReferences(fop, this);
        mjit::ReleaseScriptCode(fop, this);
    }
#endif
}

bool
JSScript::tryNewStepMode(JSContext *cx, uint32_t newValue)
{
    JS_ASSERT(debug);

    uint32_t prior = debug->stepMode;
    debug->stepMode = newValue;

    if (!prior != !newValue) {
        
        recompileForStepMode(cx->runtime->defaultFreeOp());

        if (!stepModeEnabled() && !debug->numSites) {
            cx->free_(debug);
            debug = NULL;
        }
    }

    return true;
}

bool
JSScript::setStepModeFlag(JSContext *cx, bool step)
{
    if (!ensureHasDebug(cx))
        return false;

    return tryNewStepMode(cx, (debug->stepMode & stepCountMask) | (step ? stepFlagMask : 0));
}

bool
JSScript::changeStepModeCount(JSContext *cx, int delta)
{
    if (!ensureHasDebug(cx))
        return false;

    assertSameCompartment(cx, this);
    JS_ASSERT_IF(delta > 0, cx->compartment->debugMode());

    uint32_t count = debug->stepMode & stepCountMask;
    JS_ASSERT(((count + delta) & stepCountMask) == count + delta);
    return tryNewStepMode(cx,
                          (debug->stepMode & stepFlagMask) |
                          ((count + delta) & stepCountMask));
}

BreakpointSite *
JSScript::getOrCreateBreakpointSite(JSContext *cx, jsbytecode *pc,
                                    GlobalObject *scriptGlobal)
{
    JS_ASSERT(size_t(pc - code) < length);

    if (!ensureHasDebug(cx))
        return NULL;

    BreakpointSite *&site = debug->breakpoints[pc - code];

    if (!site) {
        site = cx->runtime->new_<BreakpointSite>(this, pc);
        if (!site) {
            js_ReportOutOfMemory(cx);
            return NULL;
        }
        debug->numSites++;
    }

    if (site->scriptGlobal)
        JS_ASSERT_IF(scriptGlobal, site->scriptGlobal == scriptGlobal);
    else
        site->scriptGlobal = scriptGlobal;

    return site;
}

void
JSScript::destroyBreakpointSite(FreeOp *fop, jsbytecode *pc)
{
    JS_ASSERT(unsigned(pc - code) < length);

    BreakpointSite *&site = debug->breakpoints[pc - code];
    JS_ASSERT(site);

    fop->delete_(site);
    site = NULL;

    if (--debug->numSites == 0 && !stepModeEnabled()) {
        fop->free_(debug);
        debug = NULL;
    }
}

void
JSScript::clearBreakpointsIn(FreeOp *fop, js::Debugger *dbg, JSObject *handler)
{
    if (!hasAnyBreakpointsOrStepMode())
        return;

    jsbytecode *end = code + length;
    for (jsbytecode *pc = code; pc < end; pc++) {
        BreakpointSite *site = getBreakpointSite(pc);
        if (site) {
            Breakpoint *nextbp;
            for (Breakpoint *bp = site->firstBreakpoint(); bp; bp = nextbp) {
                nextbp = bp->nextInSite();
                if ((!dbg || bp->debugger == dbg) && (!handler || bp->getHandler() == handler))
                    bp->destroy(fop);
            }
        }
    }
}

void
JSScript::clearTraps(FreeOp *fop)
{
    if (!hasAnyBreakpointsOrStepMode())
        return;

    jsbytecode *end = code + length;
    for (jsbytecode *pc = code; pc < end; pc++) {
        BreakpointSite *site = getBreakpointSite(pc);
        if (site)
            site->clearTrap(fop);
    }
}

void
JSScript::markChildren(JSTracer *trc)
{
    JS_ASSERT_IF(trc->runtime->gcStrictCompartmentChecking, compartment()->isCollecting());

    for (uint32_t i = 0; i < natoms; ++i) {
        if (atoms[i])
            MarkString(trc, &atoms[i], "atom");
    }

    if (JSScript::isValidOffset(objectsOffset)) {
        JSObjectArray *objarray = objects();
        MarkObjectRange(trc, objarray->length, objarray->vector, "objects");
    }

    if (JSScript::isValidOffset(regexpsOffset)) {
        JSObjectArray *objarray = regexps();
        MarkObjectRange(trc, objarray->length, objarray->vector, "objects");
    }

    if (JSScript::isValidOffset(constsOffset)) {
        JSConstArray *constarray = consts();
        MarkValueRange(trc, constarray->length, constarray->vector, "consts");
    }

    if (function())
        MarkObject(trc, &function_, "function");

    if (!isCachedEval && globalObject)
        MarkObject(trc, &globalObject, "object");

    if (IS_GC_MARKING_TRACER(trc) && filename)
        MarkScriptFilename(filename);

    bindings.trace(trc);

    if (types)
        types->trace(trc);

    if (hasAnyBreakpointsOrStepMode()) {
        for (unsigned i = 0; i < length; i++) {
            BreakpointSite *site = debug->breakpoints[i];
            if (site && site->trapHandler)
                MarkValue(trc, &site->trapClosure, "trap closure");
        }
    }

    if (hasIonScript())
        ion::IonScript::Trace(trc, ion);
}

void
JSScript::setArgumentsHasLocalBinding(uint16_t slot)
{
    argsHasLocalBinding_ = true;
    argsSlot_ = slot;
    needsArgsAnalysis_ = true;
}

void
JSScript::setNeedsArgsObj(bool needsArgsObj)
{
    JS_ASSERT(!analyzedArgsUsage());
    JS_ASSERT_IF(needsArgsObj, argumentsHasLocalBinding());
    needsArgsAnalysis_ = false;
    needsArgsObj_ = needsArgsObj;
}

bool
JSScript::applySpeculationFailed(JSContext *cx)
{
    JS_ASSERT(analyzedArgsUsage());
    JS_ASSERT(argumentsHasLocalBinding());
    JS_ASSERT(!needsArgsObj());
    needsArgsObj_ = true;

    const unsigned slot = argumentsLocalSlot();

    











    for (AllFramesIter i(cx->stack.space()); !i.done(); ++i) {
        StackFrame *fp = i.fp();
        if (fp->isFunctionFrame() && fp->script() == this) {
            if (!fp->hasArgsObj()) {
                ArgumentsObject *obj = ArgumentsObject::create(cx, fp);
                if (!obj) {
                    




                    needsArgsObj_ = false;
                    return false;
                }
                fp->localSlot(slot) = ObjectValue(*obj);
            }
        }
    }

#ifdef JS_METHODJIT
    if (hasJITCode()) {
        mjit::Recompiler::clearStackReferences(cx->runtime->defaultFreeOp(), this);
        mjit::ReleaseScriptCode(cx->runtime->defaultFreeOp(), this);
    }
#endif

    if (hasAnalysis() && analysis()->ranInference()) {
        types::AutoEnterTypeInference enter(cx);
        types::TypeScript::MonitorUnknown(cx, this, argumentsBytecode());
    }

    return true;
}

#ifdef DEBUG
bool
JSScript::varIsAliased(unsigned varSlot)
{
    if (bindingsAccessedDynamically)
        return true;

    for (uint32_t i = 0; i < numClosedVars(); ++i) {
        if (closedVars()->vector[i] == varSlot) {
            JS_ASSERT(function()->isHeavyweight());
            return true;
        }
    }

    return false;
}

bool
JSScript::argIsAliased(unsigned argSlot)
{
    return argLivesInCallObject(argSlot) || needsArgsObj();
}

bool
JSScript::argLivesInArgumentsObject(unsigned argSlot)
{
    return needsArgsObj() && !argLivesInCallObject(argSlot);
}

bool
JSScript::argLivesInCallObject(unsigned argSlot)
{
    if (bindingsAccessedDynamically)
        return true;

    for (uint32_t i = 0; i < numClosedArgs(); ++i) {
        if (closedArgs()->vector[i] == argSlot) {
            JS_ASSERT(function()->isHeavyweight());
            return true;
        }
    }

    return false;
}
#endif
