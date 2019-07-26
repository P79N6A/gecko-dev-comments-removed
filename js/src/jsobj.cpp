









#include <stdlib.h>
#include <string.h>

#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsutil.h"
#include "jshash.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsonparser.h"
#include "jsopcode.h"
#include "jsprobes.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsdbgapi.h"
#include "json.h"
#include "jswatchpoint.h"
#include "jswrapper.h"
#include "jsxml.h"

#include "builtin/MapObject.h"
#include "frontend/BytecodeCompiler.h"
#include "frontend/Parser.h"
#include "frontend/TreeContext.h"
#include "gc/Marking.h"
#include "js/MemoryMetrics.h"
#include "vm/StringBuffer.h"
#include "vm/Xdr.h"

#include "jsarrayinlines.h"
#include "jsatominlines.h"
#include "jscntxtinlines.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"

#include "vm/MethodGuard-inl.h"

#include "jsautooplen.h"

using namespace mozilla;
using namespace js;
using namespace js::gc;
using namespace js::types;

JS_STATIC_ASSERT(int32_t((JSObject::NELEMENTS_LIMIT - 1) * sizeof(Value)) == int64_t((JSObject::NELEMENTS_LIMIT - 1) * sizeof(Value)));

Class js::ObjectClass = {
    js_Object_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

JS_FRIEND_API(JSObject *)
JS_ObjectToInnerObject(JSContext *cx, JSObject *obj_)
{
    if (!obj_) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INACTIVE);
        return NULL;
    }
    Rooted<JSObject*> obj(cx, obj_);
    return GetInnerObject(cx, obj);
}

JS_FRIEND_API(JSObject *)
JS_ObjectToOuterObject(JSContext *cx, JSObject *obj_)
{
    Rooted<JSObject*> obj(cx, obj_);
    return GetOuterObject(cx, obj);
}

#if JS_HAS_OBJ_PROTO_PROP

static JSBool
obj_getProto(JSContext *cx, HandleObject obj, HandleId id, Value *vp);

static JSBool
obj_setProto(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, Value *vp);

JSPropertySpec object_props[] = {
    {js_proto_str, 0, JSPROP_PERMANENT|JSPROP_SHARED, obj_getProto, obj_setProto},
    {0,0,0,0,0}
};

static JSBool
obj_getProto(JSContext *cx, HandleObject obj, HandleId id, Value *vp)
{
    
    unsigned attrs;
    RootedId nid(cx, NameToId(cx->runtime->atomState.protoAtom));
    return CheckAccess(cx, obj, nid, JSACC_PROTO, vp, &attrs);
}

size_t sSetProtoCalled = 0;

static JSBool
obj_setProto(JSContext *cx, HandleObject obj, HandleId id, JSBool strict, Value *vp)
{
    if (!cx->runningWithTrustedPrincipals())
        ++sSetProtoCalled;

    
    if (!obj->isExtensible()) {
        obj->reportNotExtensible(cx);
        return false;
    }

    if (!vp->isObjectOrNull())
        return true;

    RootedObject pobj(cx, vp->toObjectOrNull());
    unsigned attrs;
    RootedId nid(cx, NameToId(cx->runtime->atomState.protoAtom));
    if (!CheckAccess(cx, obj, nid, JSAccessMode(JSACC_PROTO|JSACC_WRITE), vp, &attrs))
        return false;

    return SetProto(cx, obj, pobj, true);
}

#else  

#define object_props NULL

#endif 

static bool
MarkSharpObjects(JSContext *cx, HandleObject obj, JSIdArray **idap, JSSharpInfo *value)
{
    JS_CHECK_RECURSION(cx, return false);

    JSIdArray *ida;

    JSSharpObjectMap *map = &cx->sharpObjectMap;
    JS_ASSERT(map->depth >= 1);
    JSSharpInfo sharpid;
    JSSharpTable::Ptr p = map->table.lookup(obj);
    if (!p) {
        if (!map->table.put(obj.value(), sharpid))
            return false;

        ida = JS_Enumerate(cx, obj);
        if (!ida)
            return false;

        bool ok = true;
        RootedId id(cx);
        for (int i = 0, length = ida->length; i < length; i++) {
            id = ida->vector[i];
            JSObject *obj2;
            JSProperty *prop;
            ok = obj->lookupGeneric(cx, id, &obj2, &prop);
            if (!ok)
                break;
            if (!prop)
                continue;
            bool hasGetter, hasSetter;
            RootedValue valueRoot(cx), setterRoot(cx);
            Value &value = valueRoot.reference();
            Value &setter = setterRoot.reference();
            if (obj2->isNative()) {
                const Shape *shape = (Shape *) prop;
                hasGetter = shape->hasGetterValue();
                hasSetter = shape->hasSetterValue();
                if (hasGetter)
                    value = shape->getterValue();
                if (hasSetter)
                    setter = shape->setterValue();
            } else {
                hasGetter = hasSetter = false;
            }
            if (hasSetter) {
                
                if (hasGetter && value.isObject()) {
                    Rooted<JSObject*> vobj(cx, &value.toObject());
                    ok = MarkSharpObjects(cx, vobj, NULL, NULL);
                    if (!ok)
                        break;
                }
                value = setter;
            } else if (!hasGetter) {
                ok = obj->getGeneric(cx, id, &value);
                if (!ok)
                    break;
            }
            if (value.isObject()) {
                Rooted<JSObject*> vobj(cx, &value.toObject());
                if (!MarkSharpObjects(cx, vobj, NULL, NULL)) {
                    ok = false;
                    break;
                }
            }
        }
        if (!ok || !idap)
            JS_DestroyIdArray(cx, ida);
        if (!ok)
            return false;
    } else {
        if (!p->value.hasGen && !p->value.isSharp) {
            p->value.hasGen = true;
        }
        sharpid = p->value;
        ida = NULL;
    }
    if (idap)
        *idap = ida;
    if (value)
        *value = sharpid;
    return true;
}

bool
js_EnterSharpObject(JSContext *cx, HandleObject obj, JSIdArray **idap, bool *alreadySeen, bool *isSharp)
{
    if (!JS_CHECK_OPERATION_LIMIT(cx))
        return false;

    *alreadySeen = false;

    JSSharpObjectMap *map = &cx->sharpObjectMap;

    JS_ASSERT_IF(map->depth == 0, map->table.count() == 0);
    JS_ASSERT_IF(map->table.count() == 0, map->depth == 0);

    JSSharpTable::Ptr p;
    JSSharpInfo sharpid;
    JSIdArray *ida = NULL;

    
    if (map->depth == 0) {
        JS_KEEP_ATOMS(cx->runtime);

        











        map->depth = 1;
        bool success = MarkSharpObjects(cx, obj, &ida, &sharpid);
        JS_ASSERT(map->depth == 1);
        map->depth = 0;
        if (!success)
            goto bad;
        JS_ASSERT(!sharpid.isSharp);
        if (!idap) {
            JS_DestroyIdArray(cx, ida);
            ida = NULL;
        }
    } else {
        






        p = map->table.lookup(obj);
        if (!p) {
            if (!map->table.put(obj.value(), sharpid))
                goto bad;
            goto out;
        }
        sharpid = p->value;
    }

    if (sharpid.isSharp || sharpid.hasGen)
        *alreadySeen = true;

out:
    if (!sharpid.isSharp) {
        if (idap && !ida) {
            ida = JS_Enumerate(cx, obj);
            if (!ida)
                goto bad;
        }
        map->depth++;
    }

    if (idap)
        *idap = ida;
    *isSharp = sharpid.isSharp;
    return true;

bad:
    
    if (map->depth == 0) {
        JS_UNKEEP_ATOMS(cx->runtime);
        map->sharpgen = 0;
        map->table.clear();
    }
    return false;
}

void
js_LeaveSharpObject(JSContext *cx, JSIdArray **idap)
{
    JSSharpObjectMap *map = &cx->sharpObjectMap;
    JS_ASSERT(map->depth > 0);
    if (--map->depth == 0) {
        JS_UNKEEP_ATOMS(cx->runtime);
        map->sharpgen = 0;
        map->table.clear();
    }
    if (idap) {
        if (JSIdArray *ida = *idap) {
            JS_DestroyIdArray(cx, ida);
            *idap = NULL;
        }
    }
}

void
js_TraceSharpMap(JSTracer *trc, JSSharpObjectMap *map)
{
    JS_ASSERT(map->depth > 0);

    



















    for (JSSharpTable::Range r = map->table.all(); !r.empty(); r.popFront()) {
        JSObject *tmp = r.front().key;
        MarkObjectRoot(trc, &tmp, "sharp table entry");
        JS_ASSERT(tmp == r.front().key);
    }
}

#if JS_HAS_TOSOURCE
static JSBool
obj_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    bool comma = false;
    const jschar *vchars;
    size_t vlength;
    Value *val;
    JSString *gsop[2];

    JS_CHECK_RECURSION(cx, return JS_FALSE);

    Value localroot[4];
    PodArrayZero(localroot);
    AutoArrayRooter tvr(cx, ArrayLength(localroot), localroot);

    
    bool outermost = (cx->sharpObjectMap.depth == 0);

    RootedObject obj(cx, ToObject(cx, &vp[1]));
    if (!obj)
        return false;

    JSIdArray *ida;
    bool alreadySeen = false;
    bool isSharp = false;
    if (!js_EnterSharpObject(cx, obj, &ida, &alreadySeen, &isSharp))
        return false;

    if (!ida) {
        



        JS_ASSERT(alreadySeen);
        JSString *str = js_NewStringCopyZ(cx, "{}");
        if (!str)
            return false;
        vp->setString(str);
        return true;
    }

    JS_ASSERT(!isSharp);
    if (alreadySeen) {
        JSSharpTable::Ptr p = cx->sharpObjectMap.table.lookup(obj);
        JS_ASSERT(p);
        JS_ASSERT(!p->value.isSharp);
        p->value.isSharp = true;
    }

    
    class AutoLeaveSharpObject {
        JSContext *cx;
        JSIdArray *ida;
      public:
        AutoLeaveSharpObject(JSContext *cx, JSIdArray *ida) : cx(cx), ida(ida) {}
        ~AutoLeaveSharpObject() {
            js_LeaveSharpObject(cx, &ida);
        }
    } autoLeaveSharpObject(cx, ida);

    StringBuffer buf(cx);
    if (outermost && !buf.append('('))
        return false;
    if (!buf.append('{'))
        return false;

    




    val = localroot + 2;

    RootedId id(cx);
    for (int i = 0; i < ida->length; i++) {
        
        id = ida->vector[i];
        JSLinearString *idstr;

        JSObject *obj2;
        JSProperty *prop;
        if (!obj->lookupGeneric(cx, id, &obj2, &prop))
            return false;

        



        JSString *s = ToString(cx, IdToValue(id));
        if (!s || !(idstr = s->ensureLinear(cx)))
            return false;

        int valcnt = 0;
        if (prop) {
            bool doGet = true;
            if (obj2->isNative()) {
                const Shape *shape = (Shape *) prop;
                unsigned attrs = shape->attributes();
                if (attrs & JSPROP_GETTER) {
                    doGet = false;
                    val[valcnt] = shape->getterValue();
                    gsop[valcnt] = cx->runtime->atomState.getAtom;
                    valcnt++;
                }
                if (attrs & JSPROP_SETTER) {
                    doGet = false;
                    val[valcnt] = shape->setterValue();
                    gsop[valcnt] = cx->runtime->atomState.setAtom;
                    valcnt++;
                }
            }
            if (doGet) {
                valcnt = 1;
                gsop[0] = NULL;
                if (!obj->getGeneric(cx, id, &val[0]))
                    return false;
            }
        }

        



        if (JSID_IS_ATOM(id)
            ? !IsIdentifier(idstr)
            : (!JSID_IS_INT(id) || JSID_TO_INT(id) < 0)) {
            s = js_QuoteString(cx, idstr, jschar('\''));
            if (!s || !(idstr = s->ensureLinear(cx)))
                return false;
        }

        for (int j = 0; j < valcnt; j++) {
            



            if (gsop[j] && val[j].isUndefined())
                continue;

            
            JSString *valstr = js_ValueToSource(cx, val[j]);
            if (!valstr)
                return false;
            localroot[j].setString(valstr);             
            vchars = valstr->getChars(cx);
            if (!vchars)
                return false;
            vlength = valstr->length();

            



            if (gsop[j] && IsFunctionObject(val[j])) {
                const jschar *start = vchars;
                const jschar *end = vchars + vlength;

                uint8_t parenChomp = 0;
                if (vchars[0] == '(') {
                    vchars++;
                    parenChomp = 1;
                }

                
                if (vchars)
                    vchars = js_strchr_limit(vchars, ' ', end);

                



                if (vchars)
                    vchars = js_strchr_limit(vchars, '(', end);

                if (vchars) {
                    if (*vchars == ' ')
                        vchars++;
                    vlength = end - vchars - parenChomp;
                } else {
                    gsop[j] = NULL;
                    vchars = start;
                }
            }

            if (comma && !buf.append(", "))
                return false;
            comma = true;

            if (gsop[j])
                if (!buf.append(gsop[j]) || !buf.append(' '))
                    return false;

            if (!buf.append(idstr))
                return false;
            if (!buf.append(gsop[j] ? ' ' : ':'))
                return false;

            if (!buf.append(vchars, vlength))
                return false;
        }
    }

    if (!buf.append('}'))
        return false;
    if (outermost && !buf.append(')'))
        return false;

    JSString *str = buf.finishString();
    if (!str)
        return false;
    vp->setString(str);
    return true;
}
#endif 

namespace js {

JSString *
obj_toStringHelper(JSContext *cx, JSObject *obj)
{
    if (obj->isProxy())
        return Proxy::obj_toString(cx, obj);

    StringBuffer sb(cx);
    const char *className = obj->getClass()->name;
    if (!sb.append("[object ") || !sb.appendInflated(className, strlen(className)) ||
        !sb.append("]"))
    {
        return NULL;
    }
    return sb.finishString();
}

JSObject *
NonNullObject(JSContext *cx, const Value &v)
{
    if (v.isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return NULL;
    }
    return &v.toObject();
}

const char *
InformalValueTypeName(const Value &v)
{
    if (v.isObject())
        return v.toObject().getClass()->name;
    if (v.isString())
        return "string";
    if (v.isNumber())
        return "number";
    if (v.isBoolean())
        return "boolean";
    if (v.isNull())
        return "null";
    if (v.isUndefined())
        return "undefined";
    return "value";
}

} 


static JSBool
obj_toString(JSContext *cx, unsigned argc, Value *vp)
{
    Value &thisv = vp[1];

    
    if (thisv.isUndefined()) {
        vp->setString(cx->runtime->atomState.objectUndefinedAtom);
        return true;
    }

    
    if (thisv.isNull()) {
        vp->setString(cx->runtime->atomState.objectNullAtom);
        return true;
    }

    
    JSObject *obj = ToObject(cx, &thisv);
    if (!obj)
        return false;

    
    JSString *str = js::obj_toStringHelper(cx, obj);
    if (!str)
        return false;
    vp->setString(str);
    return true;
}


static JSBool
obj_toLocaleString(JSContext *cx, unsigned argc, Value *vp)
{
    JS_CHECK_RECURSION(cx, return false);

    
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    
    RootedId id(cx, NameToId(cx->runtime->atomState.toStringAtom));
    return obj->callMethod(cx, id, 0, NULL, vp);
}

static JSBool
obj_valueOf(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
    vp->setObject(*obj);
    return true;
}


static void
AssertInnerizedScopeChain(JSContext *cx, JSObject &scopeobj)
{
#ifdef DEBUG
    for (JSObject *o = &scopeobj; o; o = o->enclosingScope()) {
        if (JSObjectOp op = o->getClass()->ext.innerObject) {
            Rooted<JSObject*> obj(cx, o);
            JS_ASSERT(op(cx, obj) == o);
        }
    }
#endif
}

#ifndef EVAL_CACHE_CHAIN_LIMIT
# define EVAL_CACHE_CHAIN_LIMIT 4
#endif

void
EvalCache::purge()
{
    





    for (size_t i = 0; i < ArrayLength(table_); ++i) {
        for (JSScript **listHeadp = &table_[i]; *listHeadp; ) {
            JSScript *script = *listHeadp;
            JS_ASSERT(GetGCThingTraceKind(script) == JSTRACE_SCRIPT);
            *listHeadp = script->evalHashLink();
            script->evalHashLink() = NULL;
        }
    }
}

inline JSScript **
EvalCache::bucket(JSLinearString *str)
{
    const jschar *s = str->chars();
    size_t n = str->length();

    if (n > 100)
        n = 100;
    uint32_t h;
    for (h = 0; n; s++, n--)
        h = JS_ROTATE_LEFT32(h, 4) ^ *s;

    h *= JS_GOLDEN_RATIO;
    h >>= 32 - SHIFT;
    JS_ASSERT(h < ArrayLength(table_));
    return &table_[h];
}

static JS_ALWAYS_INLINE JSScript *
EvalCacheLookup(JSContext *cx, JSLinearString *str, StackFrame *caller, unsigned staticLevel,
                JSPrincipals *principals, JSObject &scopeobj, JSScript **bucket)
{
    















    unsigned count = 0;
    JSScript **scriptp = bucket;

    JSVersion version = cx->findVersion();
    JSScript *script;
    JSSubsumePrincipalsOp subsume = cx->runtime->securityCallbacks->subsumePrincipals;
    while ((script = *scriptp) != NULL) {
        if (script->savedCallerFun &&
            script->staticLevel == staticLevel &&
            script->getVersion() == version &&
            !script->hasSingletons &&
            (!subsume || script->principals == principals ||
             (subsume(principals, script->principals) &&
              subsume(script->principals, principals)))) {
            



            JSFunction *fun = script->getCallerFunction();

            if (fun == caller->fun()) {
                



                JSAtom *src = script->atoms[0];

                if (src == str || EqualStrings(src, str)) {
                    





                    JS_ASSERT(script->objects()->length >= 1);
                    if (script->objects()->length == 1 &&
                        !script->hasRegexps()) {
                        JS_ASSERT(staticLevel == script->staticLevel);
                        *scriptp = script->evalHashLink();
                        script->evalHashLink() = NULL;
                        return script;
                    }
                }
            }
        }

        if (++count == EVAL_CACHE_CHAIN_LIMIT)
            return NULL;
        scriptp = &script->evalHashLink();
    }
    return NULL;
}












class EvalScriptGuard
{
    JSContext *cx_;
    JSLinearString *str_;
    JSScript **bucket_;
    Rooted<JSScript*> script_;

  public:
    EvalScriptGuard(JSContext *cx, JSLinearString *str)
      : cx_(cx),
        str_(str),
        script_(cx) {
        bucket_ = cx->runtime->evalCache.bucket(str);
    }

    ~EvalScriptGuard() {
        if (script_) {
            CallDestroyScriptHook(cx_->runtime->defaultFreeOp(), script_);
            script_->isActiveEval = false;
            script_->isCachedEval = true;
            script_->evalHashLink() = *bucket_;
            *bucket_ = script_;
        }
    }

    void lookupInEvalCache(StackFrame *caller, unsigned staticLevel,
                           JSPrincipals *principals, JSObject &scopeobj) {
        if (JSScript *found = EvalCacheLookup(cx_, str_, caller, staticLevel,
                                              principals, scopeobj, bucket_)) {
            js_CallNewScriptHook(cx_, found, NULL);
            script_ = found;
            script_->isCachedEval = false;
            script_->isActiveEval = true;
        }
    }

    void setNewScript(JSScript *script) {
        
        JS_ASSERT(!script_ && script);
        script_ = script;
        script_->isActiveEval = true;
    }

    bool foundScript() {
        return !!script_;
    }

    JSScript *script() const {
        JS_ASSERT(script_);
        return script_;
    }
};


enum EvalType { DIRECT_EVAL = EXECUTE_DIRECT_EVAL, INDIRECT_EVAL = EXECUTE_INDIRECT_EVAL };











static bool
EvalKernel(JSContext *cx, const CallArgs &args, EvalType evalType, StackFrame *caller,
           HandleObject scopeobj)
{
    JS_ASSERT((evalType == INDIRECT_EVAL) == (caller == NULL));
    JS_ASSERT_IF(evalType == INDIRECT_EVAL, scopeobj->isGlobal());
    AssertInnerizedScopeChain(cx, *scopeobj);

    if (!scopeobj->global().isRuntimeCodeGenEnabled(cx)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CSP_BLOCKED_EVAL);
        return false;
    }

    
    if (args.length() < 1) {
        args.rval().setUndefined();
        return true;
    }
    if (!args[0].isString()) {
        args.rval() = args[0];
        return true;
    }
    JSString *str = args[0].toString();

    

    




    unsigned staticLevel;
    RootedValue thisv(cx);
    if (evalType == DIRECT_EVAL) {
        staticLevel = caller->script()->staticLevel + 1;

        




        if (!ComputeThis(cx, caller))
            return false;
        thisv = caller->thisValue();

#ifdef DEBUG
        jsbytecode *callerPC = caller->pcQuadratic(cx);
        JS_ASSERT(callerPC && JSOp(*callerPC) == JSOP_EVAL);
#endif
    } else {
        JS_ASSERT(args.callee().global() == *scopeobj);
        staticLevel = 0;

        
        JSObject *thisobj = scopeobj->thisObject(cx);
        if (!thisobj)
            return false;
        thisv = ObjectValue(*thisobj);
    }

    Rooted<JSLinearString*> linearStr(cx, str->ensureLinear(cx));
    if (!linearStr)
        return false;
    const jschar *chars = linearStr->chars();
    size_t length = linearStr->length();

    SkipRoot skip(cx, &chars);

    










    if (length > 2 &&
        ((chars[0] == '[' && chars[length - 1] == ']') ||
        (chars[0] == '(' && chars[length - 1] == ')')) &&
         (!caller || !caller->script()->strictModeCode))
    {
        







        for (const jschar *cp = &chars[1], *end = &chars[length - 2]; ; cp++) {
            if (*cp == 0x2028 || *cp == 0x2029)
                break;

            if (cp == end) {
                bool isArray = (chars[0] == '[');
                JSONParser parser(cx, isArray ? chars : chars + 1, isArray ? length : length - 2,
                                  JSONParser::StrictJSON, JSONParser::NoError);
                Value tmp;
                if (!parser.parse(&tmp))
                    return false;
                if (tmp.isUndefined())
                    break;
                args.rval() = tmp;
                return true;
            }
        }
    }

    EvalScriptGuard esg(cx, linearStr);

    JSPrincipals *principals = PrincipalsForCompiledCode(args, cx);

    if (evalType == DIRECT_EVAL && caller->isNonEvalFunctionFrame())
        esg.lookupInEvalCache(caller, staticLevel, principals, *scopeobj);

    if (!esg.foundScript()) {
        unsigned lineno;
        const char *filename;
        JSPrincipals *originPrincipals;
        CurrentScriptFileLineOrigin(cx, &filename, &lineno, &originPrincipals,
                                    evalType == DIRECT_EVAL ? CALLED_FROM_JSOP_EVAL
                                                            : NOT_CALLED_FROM_JSOP_EVAL);

        bool compileAndGo = true;
        bool noScriptRval = false;
        bool needScriptGlobal = false;
        JSScript *compiled = frontend::CompileScript(cx, scopeobj, caller,
                                                     principals, originPrincipals,
                                                     compileAndGo, noScriptRval, needScriptGlobal,
                                                     chars, length, filename,
                                                     lineno, cx->findVersion(), linearStr,
                                                     staticLevel);
        if (!compiled)
            return false;

        esg.setNewScript(compiled);
    }

    return ExecuteKernel(cx, esg.script(), *scopeobj, thisv, ExecuteType(evalType),
                         NULL , &args.rval());
}






static inline bool
WarnOnTooManyArgs(JSContext *cx, const CallArgs &args)
{
    if (args.length() > 1) {
        if (JSScript *script = cx->stack.currentScript()) {
            if (!script->warnedAboutTwoArgumentEval) {
                static const char TWO_ARGUMENT_WARNING[] =
                    "Support for eval(code, scopeObject) has been removed. "
                    "Use |with (scopeObject) eval(code);| instead.";
                if (!JS_ReportWarning(cx, TWO_ARGUMENT_WARNING))
                    return false;
                script->warnedAboutTwoArgumentEval = true;
            }
        } else {
            



        }
    }

    return true;
}

namespace js {






JSBool
eval(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (!WarnOnTooManyArgs(cx, args))
        return false;

    Rooted<GlobalObject*> global(cx, &args.callee().global());
    return EvalKernel(cx, args, INDIRECT_EVAL, NULL, global);
}

bool
DirectEval(JSContext *cx, const CallArgs &args)
{
    
    StackFrame *caller = cx->fp();
    JS_ASSERT(caller->isScriptFrame());
    JS_ASSERT(IsBuiltinEvalForScope(caller->scopeChain(), args.calleev()));
    JS_ASSERT(JSOp(*cx->regs().pc) == JSOP_EVAL);

    AutoFunctionCallProbe callProbe(cx, args.callee().toFunction(), caller->script());

    if (!WarnOnTooManyArgs(cx, args))
        return false;

    return EvalKernel(cx, args, DIRECT_EVAL, caller, caller->scopeChain());
}

bool
IsBuiltinEvalForScope(JSObject *scopeChain, const Value &v)
{
    return scopeChain->global().getOriginalEval() == v;
}

bool
IsAnyBuiltinEval(JSFunction *fun)
{
    return fun->maybeNative() == eval;
}

JSPrincipals *
PrincipalsForCompiledCode(const CallReceiver &call, JSContext *cx)
{
    JS_ASSERT(IsAnyBuiltinEval(call.callee().toFunction()) ||
              IsBuiltinFunctionConstructor(call.callee().toFunction()));

    















    return call.callee().principals(cx);
}

}  

#if JS_HAS_OBJ_WATCHPOINT

static JSBool
obj_watch_handler(JSContext *cx, JSObject *obj, jsid id, jsval old,
                  jsval *nvp, void *closure)
{
    JSObject *callable = (JSObject *) closure;
    if (JSSubsumePrincipalsOp subsume = cx->runtime->securityCallbacks->subsumePrincipals) {
        if (JSPrincipals *watcher = callable->principals(cx)) {
            if (JSObject *scopeChain = cx->stack.currentScriptedScopeChain()) {
                if (JSPrincipals *subject = scopeChain->principals(cx)) {
                    if (!subsume(watcher, subject)) {
                        
                        return true;
                    }
                }
            }
        }
    }

    
    AutoResolving resolving(cx, obj, id, AutoResolving::WATCH);
    if (resolving.alreadyStarted())
        return true;

    Value argv[] = { IdToValue(id), old, *nvp };
    return Invoke(cx, ObjectValue(*obj), ObjectOrNullValue(callable), ArrayLength(argv), argv, nvp);
}

static JSBool
obj_watch(JSContext *cx, unsigned argc, Value *vp)
{
    if (argc <= 1) {
        js_ReportMissingArg(cx, *vp, 1);
        return false;
    }

    RootedObject callable(cx, js_ValueToCallableObject(cx, &vp[3], 0));
    if (!callable)
        return false;

    RootedId propid(cx);
    if (!ValueToId(cx, vp[2], propid.address()))
        return false;

    RootedObject obj(cx, ToObject(cx, &vp[1]));
    if (!obj)
        return false;

    Value tmp;
    unsigned attrs;
    if (!CheckAccess(cx, obj, propid, JSACC_WATCH, &tmp, &attrs))
        return false;

    vp->setUndefined();

    if (obj->isDenseArray() && !JSObject::makeDenseArraySlow(cx, obj))
        return false;
    return JS_SetWatchPoint(cx, obj, propid, obj_watch_handler, callable);
}

static JSBool
obj_unwatch(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
    vp->setUndefined();
    jsid id;
    if (argc != 0) {
        if (!ValueToId(cx, vp[2], &id))
            return false;
    } else {
        id = JSID_VOID;
    }
    return JS_ClearWatchPoint(cx, obj, id, NULL, NULL);
}

#endif 







static JSBool
obj_hasOwnProperty(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
    return js_HasOwnPropertyHelper(cx, obj->getOps()->lookupGeneric, argc, vp);
}

JSBool
js_HasOwnPropertyHelper(JSContext *cx, LookupGenericOp lookup, unsigned argc,
                        Value *vp)
{
    RootedId id(cx);
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), id.address()))
        return JS_FALSE;

    RootedObject obj(cx, ToObject(cx, &vp[1]));
    if (!obj)
        return false;
    JSObject *obj2;
    JSProperty *prop;
    if (obj->isProxy()) {
        bool has;
        if (!Proxy::hasOwn(cx, obj, id, &has))
            return false;
        vp->setBoolean(has);
        return true;
    }
    if (!js_HasOwnProperty(cx, lookup, obj, id, &obj2, &prop))
        return JS_FALSE;
    vp->setBoolean(!!prop);
    return JS_TRUE;
}

JSBool
js_HasOwnProperty(JSContext *cx, LookupGenericOp lookup, HandleObject obj, HandleId id,
                  JSObject **objp, JSProperty **propp)
{
    JSAutoResolveFlags rf(cx, JSRESOLVE_QUALIFIED | JSRESOLVE_DETECTING);
    if (lookup) {
        if (!lookup(cx, obj, id, objp, propp))
            return false;
    } else {
        if (!baseops::LookupProperty(cx, obj, id, objp, propp))
            return false;
    }
    if (!*propp)
        return true;

    if (*objp == obj)
        return true;

    JSObject *outer = NULL;
    if (JSObjectOp op = (*objp)->getClass()->ext.outerObject) {
        Rooted<JSObject*> inner(cx, *objp);
        outer = op(cx, inner);
        if (!outer)
            return false;
    }

    if (outer != *objp)
        *propp = NULL;
    return true;
}


static JSBool
obj_isPrototypeOf(JSContext *cx, unsigned argc, Value *vp)
{
    
    if (argc < 1 || !vp[2].isObject()) {
        vp->setBoolean(false);
        return true;
    }

    
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    
    vp->setBoolean(js_IsDelegate(cx, obj, vp[2]));
    return true;
}


static JSBool
obj_propertyIsEnumerable(JSContext *cx, unsigned argc, Value *vp)
{
    
    RootedId id(cx);
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), id.address()))
        return false;

    
    RootedObject obj(cx, ToObject(cx, &vp[1]));
    if (!obj)
        return false;

    
    return js_PropertyIsEnumerable(cx, obj, id, vp);
}

JSBool
js_PropertyIsEnumerable(JSContext *cx, HandleObject obj, HandleId id, Value *vp)
{
    JSObject *pobj;
    JSProperty *prop;
    if (!obj->lookupGeneric(cx, id, &pobj, &prop))
        return false;

    if (!prop) {
        vp->setBoolean(false);
        return true;
    }

    



    if (pobj != obj) {
        vp->setBoolean(false);
        return true;
    }

    unsigned attrs;
    if (!pobj->getGenericAttributes(cx, id, &attrs))
        return false;

    vp->setBoolean((attrs & JSPROP_ENUMERATE) != 0);
    return true;
}

#if OLD_GETTER_SETTER_METHODS

enum DefineType { Getter, Setter };

template<DefineType Type>
static bool
DefineAccessor(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (!BoxNonStrictThis(cx, args))
        return false;

    if (args.length() < 2 || !js_IsCallable(args[1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_GETTER_OR_SETTER,
                             Type == Getter ? js_getter_str : js_setter_str);
        return false;
    }

    RootedId id(cx);
    if (!ValueToId(cx, args[0], id.address()))
        return false;

    RootedObject descObj(cx, NewBuiltinClassInstance(cx, &ObjectClass));
    if (!descObj)
        return false;

    JSAtomState &state = cx->runtime->atomState;
    
    if (!descObj->defineProperty(cx, state.enumerableAtom, BooleanValue(true)))
        return false;

    
    if (!descObj->defineProperty(cx, state.configurableAtom, BooleanValue(true)))
        return false;

    
    PropertyName *acc = (Type == Getter) ? state.getAtom : state.setAtom;
    if (!descObj->defineProperty(cx, acc, args[1]))
        return false;

    RootedObject thisObj(cx, &args.thisv().toObject());

    JSBool dummy;
    if (!js_DefineOwnProperty(cx, thisObj, id, ObjectValue(*descObj), &dummy)) {
        return false;
    }
    args.rval().setUndefined();
    return true;
}

JS_FRIEND_API(JSBool)
js::obj_defineGetter(JSContext *cx, unsigned argc, Value *vp)
{
    return DefineAccessor<Getter>(cx, argc, vp);
}

JS_FRIEND_API(JSBool)
js::obj_defineSetter(JSContext *cx, unsigned argc, Value *vp)
{
    return DefineAccessor<Setter>(cx, argc, vp);
}

static JSBool
obj_lookupGetter(JSContext *cx, unsigned argc, Value *vp)
{
    RootedId id(cx);
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), id.address()))
        return JS_FALSE;
    RootedObject obj(cx, ToObject(cx, &vp[1]));
    if (!obj)
        return JS_FALSE;
    if (obj->isProxy()) {
        
        
        vp->setUndefined();
        PropertyDescriptor desc;
        if (!Proxy::getPropertyDescriptor(cx, obj, id, false, &desc))
            return JS_FALSE;
        if (desc.obj && (desc.attrs & JSPROP_GETTER) && desc.getter)
            *vp = CastAsObjectJsval(desc.getter);
        return JS_TRUE;
    }
    JSObject *pobj;
    JSProperty *prop;
    if (!obj->lookupGeneric(cx, id, &pobj, &prop))
        return JS_FALSE;
    vp->setUndefined();
    if (prop) {
        if (pobj->isNative()) {
            Shape *shape = (Shape *) prop;
            if (shape->hasGetterValue())
                *vp = shape->getterValue();
        }
    }
    return JS_TRUE;
}

static JSBool
obj_lookupSetter(JSContext *cx, unsigned argc, Value *vp)
{
    RootedId id(cx);
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), id.address()))
        return JS_FALSE;
    RootedObject obj(cx, ToObject(cx, &vp[1]));
    if (!obj)
        return JS_FALSE;
    if (obj->isProxy()) {
        
        
        vp->setUndefined();
        PropertyDescriptor desc;
        if (!Proxy::getPropertyDescriptor(cx, obj, id, false, &desc))
            return JS_FALSE;
        if (desc.obj && (desc.attrs & JSPROP_SETTER) && desc.setter)
            *vp = CastAsObjectJsval(desc.setter);
        return JS_TRUE;
    }
    JSObject *pobj;
    JSProperty *prop;
    if (!obj->lookupGeneric(cx, id, &pobj, &prop))
        return JS_FALSE;
    vp->setUndefined();
    if (prop) {
        if (pobj->isNative()) {
            Shape *shape = (Shape *) prop;
            if (shape->hasSetterValue())
                *vp = shape->setterValue();
        }
    }
    return JS_TRUE;
}
#endif 

JSBool
obj_getPrototypeOf(JSContext *cx, unsigned argc, Value *vp)
{
    if (argc == 0) {
        js_ReportMissingArg(cx, *vp, 0);
        return JS_FALSE;
    }

    if (vp[2].isPrimitive()) {
        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, vp[2], NULL);
        if (!bytes)
            return JS_FALSE;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_UNEXPECTED_TYPE, bytes, "not an object");
        JS_free(cx, bytes);
        return JS_FALSE;
    }

    JSObject *obj = &vp[2].toObject();
    unsigned attrs;
    RootedId nid(cx, NameToId(cx->runtime->atomState.protoAtom));
    return CheckAccess(cx, obj, nid, JSACC_PROTO, vp, &attrs);
}

namespace js {

bool
NewPropertyDescriptorObject(JSContext *cx, const PropertyDescriptor *desc, Value *vp)
{
    if (!desc->obj) {
        vp->setUndefined();
        return true;
    }

    
    PropDesc d;
    PropDesc::AutoRooter dRoot(cx, &d);

    d.initFromPropertyDescriptor(*desc);
    if (!d.makeObject(cx))
        return false;
    *vp = d.pd();
    return true;
}

void
PropDesc::initFromPropertyDescriptor(const PropertyDescriptor &desc)
{
    isUndefined_ = false;
    pd_.setUndefined();
    attrs = uint8_t(desc.attrs);
    JS_ASSERT_IF(attrs & JSPROP_READONLY, !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    if (desc.attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        hasGet_ = true;
        get_ = ((desc.attrs & JSPROP_GETTER) && desc.getter)
               ? CastAsObjectJsval(desc.getter)
               : UndefinedValue();
        hasSet_ = true;
        set_ = ((desc.attrs & JSPROP_SETTER) && desc.setter)
               ? CastAsObjectJsval(desc.setter)
               : UndefinedValue();
        hasValue_ = false;
        value_.setUndefined();
        hasWritable_ = false;
    } else {
        hasGet_ = false;
        get_.setUndefined();
        hasSet_ = false;
        set_.setUndefined();
        hasValue_ = true;
        value_ = desc.value;
        hasWritable_ = true;
    }
    hasEnumerable_ = true;
    hasConfigurable_ = true;
}

bool
PropDesc::makeObject(JSContext *cx)
{
    MOZ_ASSERT(!isUndefined());

    RootedObject obj(cx, NewBuiltinClassInstance(cx, &ObjectClass));
    if (!obj)
        return false;

    const JSAtomState &atomState = cx->runtime->atomState;
    if ((hasConfigurable() &&
         !obj->defineProperty(cx, atomState.configurableAtom,
                              BooleanValue((attrs & JSPROP_PERMANENT) == 0))) ||
        (hasEnumerable() &&
         !obj->defineProperty(cx, atomState.enumerableAtom,
                              BooleanValue((attrs & JSPROP_ENUMERATE) != 0))) ||
        (hasGet() &&
         !obj->defineProperty(cx, atomState.getAtom, getterValue())) ||
        (hasSet() &&
         !obj->defineProperty(cx, atomState.setAtom, setterValue())) ||
        (hasValue() &&
         !obj->defineProperty(cx, atomState.valueAtom, value())) ||
        (hasWritable() &&
         !obj->defineProperty(cx, atomState.writableAtom,
                              BooleanValue((attrs & JSPROP_READONLY) == 0))))
    {
        return false;
    }

    pd_.setObject(*obj);
    return true;
}

bool
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, PropertyDescriptor *desc)
{
    if (obj->isProxy())
        return Proxy::getOwnPropertyDescriptor(cx, obj, id, false, desc);

    JSObject *pobj;
    JSProperty *prop;
    if (!js_HasOwnProperty(cx, obj->getOps()->lookupGeneric, obj, id, &pobj, &prop))
        return false;
    if (!prop) {
        desc->obj = NULL;
        return true;
    }

    bool doGet = true;
    if (pobj->isNative()) {
        Shape *shape = (Shape *) prop;
        desc->attrs = shape->attributes();
        if (desc->attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
            doGet = false;
            if (desc->attrs & JSPROP_GETTER)
                desc->getter = CastAsPropertyOp(shape->getterObject());
            if (desc->attrs & JSPROP_SETTER)
                desc->setter = CastAsStrictPropertyOp(shape->setterObject());
        }
    } else {
        if (!pobj->getGenericAttributes(cx, id, &desc->attrs))
            return false;
    }

    if (doGet && !obj->getGeneric(cx, id, &desc->value))
        return false;

    desc->obj = obj;
    return true;
}

bool
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, Value *vp)
{
    AutoPropertyDescriptorRooter desc(cx);
    return GetOwnPropertyDescriptor(cx, obj, id, &desc) &&
           NewPropertyDescriptorObject(cx, &desc, vp);
}

bool
GetFirstArgumentAsObject(JSContext *cx, unsigned argc, Value *vp, const char *method, JSObject **objp)
{
    if (argc == 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             method, "0", "s");
        return false;
    }

    const Value &v = vp[2];
    if (!v.isObject()) {
        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, v, NULL);
        if (!bytes)
            return false;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_UNEXPECTED_TYPE,
                             bytes, "not an object");
        JS_free(cx, bytes);
        return false;
    }

    *objp = &v.toObject();
    return true;
}

} 

static JSBool
obj_getOwnPropertyDescriptor(JSContext *cx, unsigned argc, Value *vp)
{
    RootedObject obj(cx);
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.getOwnPropertyDescriptor", obj.address()))
        return JS_FALSE;
    RootedId id(cx);
    if (!ValueToId(cx, argc >= 2 ? vp[3] : UndefinedValue(), id.address()))
        return JS_FALSE;
    return GetOwnPropertyDescriptor(cx, obj, id, vp);
}

static JSBool
obj_keys(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.keys", &obj))
        return false;

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, obj, JSITER_OWNONLY, &props))
        return false;

    AutoValueVector vals(cx);
    if (!vals.reserve(props.length()))
        return false;
    for (size_t i = 0, len = props.length(); i < len; i++) {
        jsid id = props[i];
        if (JSID_IS_STRING(id)) {
            vals.infallibleAppend(StringValue(JSID_TO_STRING(id)));
        } else if (JSID_IS_INT(id)) {
            JSString *str = Int32ToString(cx, JSID_TO_INT(id));
            if (!str)
                return false;
            vals.infallibleAppend(StringValue(str));
        } else {
            JS_ASSERT(JSID_IS_OBJECT(id));
        }
    }

    JS_ASSERT(props.length() <= UINT32_MAX);
    JSObject *aobj = NewDenseCopiedArray(cx, uint32_t(vals.length()), vals.begin());
    if (!aobj)
        return false;
    vp->setObject(*aobj);

    return true;
}

static bool
HasProperty(JSContext *cx, HandleObject obj, HandleId id, Value *vp, bool *foundp)
{
    if (!obj->hasProperty(cx, id, foundp, JSRESOLVE_QUALIFIED | JSRESOLVE_DETECTING))
        return false;
    if (!*foundp) {
        vp->setUndefined();
        return true;
    }

    





    return !!obj->getGeneric(cx, id, vp);
}

bool
PropDesc::initialize(JSContext *cx, const Value &origval, bool checkAccessors)
{
    Value v = origval;

    
    if (v.isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return false;
    }
    RootedObject desc(cx, &v.toObject());

    
    pd_ = v;

    isUndefined_ = false;

    



    attrs = JSPROP_PERMANENT | JSPROP_READONLY;

    bool found = false;
    RootedId id(cx);

    
    id = NameToId(cx->runtime->atomState.enumerableAtom);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasEnumerable_ = true;
        if (js_ValueToBoolean(v))
            attrs |= JSPROP_ENUMERATE;
    }

    
    id = NameToId(cx->runtime->atomState.configurableAtom);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasConfigurable_ = true;
        if (js_ValueToBoolean(v))
            attrs &= ~JSPROP_PERMANENT;
    }

    
    id = NameToId(cx->runtime->atomState.valueAtom);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasValue_ = true;
        value_ = v;
    }

    
    id = NameToId(cx->runtime->atomState.writableAtom);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasWritable_ = true;
        if (js_ValueToBoolean(v))
            attrs &= ~JSPROP_READONLY;
    }

    
    id = NameToId(cx->runtime->atomState.getAtom);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasGet_ = true;
        get_ = v;
        attrs |= JSPROP_GETTER | JSPROP_SHARED;
        attrs &= ~JSPROP_READONLY;
        if (checkAccessors && !checkGetter(cx))
            return false;
    }

    
    id = NameToId(cx->runtime->atomState.setAtom);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasSet_ = true;
        set_ = v;
        attrs |= JSPROP_SETTER | JSPROP_SHARED;
        attrs &= ~JSPROP_READONLY;
        if (checkAccessors && !checkSetter(cx))
            return false;
    }

    
    if ((hasGet() || hasSet()) && (hasValue() || hasWritable())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INVALID_DESCRIPTOR);
        return false;
    }

    JS_ASSERT_IF(attrs & JSPROP_READONLY, !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));

    return true;
}

namespace js {

bool
Throw(JSContext *cx, jsid id, unsigned errorNumber)
{
    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount == 1);

    JSString *idstr = IdToString(cx, id);
    if (!idstr)
       return false;
    JSAutoByteString bytes(cx, idstr);
    if (!bytes)
        return false;
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, errorNumber, bytes.ptr());
    return false;
}

bool
Throw(JSContext *cx, JSObject *obj, unsigned errorNumber)
{
    if (js_ErrorFormatString[errorNumber].argCount == 1) {
        js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,
                                 JSDVG_IGNORE_STACK, ObjectValue(*obj),
                                 NULL, NULL, NULL);
    } else {
        JS_ASSERT(js_ErrorFormatString[errorNumber].argCount == 0);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, errorNumber);
    }
    return false;
}

} 

static JSBool
Reject(JSContext *cx, unsigned errorNumber, bool throwError, jsid id, bool *rval)
{
    if (throwError)
        return Throw(cx, id, errorNumber);

    *rval = false;
    return true;
}

static JSBool
Reject(JSContext *cx, JSObject *obj, unsigned errorNumber, bool throwError, bool *rval)
{
    if (throwError)
        return Throw(cx, obj, errorNumber);

    *rval = false;
    return JS_TRUE;
}





bool
js::CheckDefineProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                        PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    if (!obj->isNative())
        return true;

    
    
    AutoPropertyDescriptorRooter desc(cx);
    if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
        return false;

    
    
    
    if (desc.obj && (desc.attrs & JSPROP_PERMANENT)) {
        
        
        
        if (getter != desc.getter ||
            setter != desc.setter ||
            (attrs != desc.attrs && attrs != (desc.attrs | JSPROP_READONLY)))
        {
            return Throw(cx, id, JSMSG_CANT_REDEFINE_PROP);
        }

        
        
        if ((desc.attrs & (JSPROP_GETTER | JSPROP_SETTER | JSPROP_READONLY)) == JSPROP_READONLY) {
            bool same;
            if (!SameValue(cx, value, desc.value, &same))
                return false;
            if (!same)
                return JSObject::reportReadOnly(cx, id);
        }
    }
    return true;
}

static JSBool
DefinePropertyOnObject(JSContext *cx, HandleObject obj, HandleId id, const PropDesc &desc,
                       bool throwError, bool *rval)
{
    
    JSProperty *current;
    RootedObject obj2(cx);
    JS_ASSERT(!obj->getOps()->lookupGeneric);
    if (!js_HasOwnProperty(cx, NULL, obj, id, obj2.address(), &current))
        return JS_FALSE;

    JS_ASSERT(!obj->getOps()->defineProperty);

    
    if (!current) {
        if (!obj->isExtensible())
            return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);

        *rval = true;

        if (desc.isGenericDescriptor() || desc.isDataDescriptor()) {
            JS_ASSERT(!obj->getOps()->defineProperty);
            Value v = desc.hasValue() ? desc.value() : UndefinedValue();
            return baseops::DefineGeneric(cx, obj, id, &v,
                                          JS_PropertyStub, JS_StrictPropertyStub,
                                          desc.attributes());
        }

        JS_ASSERT(desc.isAccessorDescriptor());

        



        Value dummy;
        unsigned dummyAttrs;
        if (!CheckAccess(cx, obj, id, JSACC_WATCH, &dummy, &dummyAttrs))
            return JS_FALSE;

        Value tmp = UndefinedValue();
        return baseops::DefineGeneric(cx, obj, id, &tmp,
                                      desc.getter(), desc.setter(), desc.attributes());
    }

    
    Value v = UndefinedValue();

    JS_ASSERT(obj == obj2);

    Rooted<const Shape *> shape(cx, reinterpret_cast<Shape *>(current));
    do {
        if (desc.isAccessorDescriptor()) {
            if (!shape->isAccessorDescriptor())
                break;

            if (desc.hasGet()) {
                bool same;
                if (!SameValue(cx, desc.getterValue(), shape->getterOrUndefined(), &same))
                    return false;
                if (!same)
                    break;
            }

            if (desc.hasSet()) {
                bool same;
                if (!SameValue(cx, desc.setterValue(), shape->setterOrUndefined(), &same))
                    return false;
                if (!same)
                    break;
            }
        } else {
            






            if (shape->isDataDescriptor()) {
                









                if (!shape->configurable() &&
                    (!shape->hasDefaultGetter() || !shape->hasDefaultSetter()) &&
                    desc.isDataDescriptor() &&
                    (desc.hasWritable() ? desc.writable() : shape->writable()))
                {
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
                }

                if (!js_NativeGet(cx, obj, obj2, shape, 0, &v))
                    return JS_FALSE;
            }

            if (desc.isDataDescriptor()) {
                if (!shape->isDataDescriptor())
                    break;

                bool same;
                if (desc.hasValue()) {
                    if (!SameValue(cx, desc.value(), v, &same))
                        return false;
                    if (!same) {
                        















                        if (!shape->configurable() &&
                            (!shape->hasDefaultGetter() || !shape->hasDefaultSetter()))
                        {
                            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
                        }
                        break;
                    }
                }
                if (desc.hasWritable() && desc.writable() != shape->writable())
                    break;
            } else {
                
                JS_ASSERT(desc.isGenericDescriptor());
            }
        }

        if (desc.hasConfigurable() && desc.configurable() != shape->configurable())
            break;
        if (desc.hasEnumerable() && desc.enumerable() != shape->enumerable())
            break;

        
        *rval = true;
        return true;
    } while (0);

    
    if (!shape->configurable()) {
        if ((desc.hasConfigurable() && desc.configurable()) ||
            (desc.hasEnumerable() && desc.enumerable() != shape->enumerable())) {
            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
        }
    }

    bool callDelProperty = false;

    if (desc.isGenericDescriptor()) {
        
    } else if (desc.isDataDescriptor() != shape->isDataDescriptor()) {
        
        if (!shape->configurable())
            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
    } else if (desc.isDataDescriptor()) {
        
        JS_ASSERT(shape->isDataDescriptor());
        if (!shape->configurable() && !shape->writable()) {
            if (desc.hasWritable() && desc.writable())
                return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            if (desc.hasValue()) {
                bool same;
                if (!SameValue(cx, desc.value(), v, &same))
                    return false;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }
        }

        callDelProperty = !shape->hasDefaultGetter() || !shape->hasDefaultSetter();
    } else {
        
        JS_ASSERT(desc.isAccessorDescriptor() && shape->isAccessorDescriptor());
        if (!shape->configurable()) {
            if (desc.hasSet()) {
                bool same;
                if (!SameValue(cx, desc.setterValue(), shape->setterOrUndefined(), &same))
                    return false;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }

            if (desc.hasGet()) {
                bool same;
                if (!SameValue(cx, desc.getterValue(), shape->getterOrUndefined(), &same))
                    return false;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }
        }
    }

    
    unsigned attrs;
    PropertyOp getter;
    StrictPropertyOp setter;
    if (desc.isGenericDescriptor()) {
        unsigned changed = 0;
        if (desc.hasConfigurable())
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable())
            changed |= JSPROP_ENUMERATE;

        attrs = (shape->attributes() & ~changed) | (desc.attributes() & changed);
        getter = shape->getter();
        setter = shape->setter();
    } else if (desc.isDataDescriptor()) {
        unsigned unchanged = 0;
        if (!desc.hasConfigurable())
            unchanged |= JSPROP_PERMANENT;
        if (!desc.hasEnumerable())
            unchanged |= JSPROP_ENUMERATE;
        
        if (!desc.hasWritable() && shape->isDataDescriptor())
            unchanged |= JSPROP_READONLY;

        if (desc.hasValue())
            v = desc.value();
        attrs = (desc.attributes() & ~unchanged) | (shape->attributes() & unchanged);
        getter = JS_PropertyStub;
        setter = JS_StrictPropertyStub;
    } else {
        JS_ASSERT(desc.isAccessorDescriptor());

        



        Value dummy;
        if (!CheckAccess(cx, obj2, id, JSACC_WATCH, &dummy, &attrs))
             return JS_FALSE;

        
        unsigned changed = 0;
        if (desc.hasConfigurable())
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable())
            changed |= JSPROP_ENUMERATE;
        if (desc.hasGet())
            changed |= JSPROP_GETTER | JSPROP_SHARED | JSPROP_READONLY;
        if (desc.hasSet())
            changed |= JSPROP_SETTER | JSPROP_SHARED | JSPROP_READONLY;

        attrs = (desc.attributes() & changed) | (shape->attributes() & ~changed);
        if (desc.hasGet()) {
            getter = desc.getter();
        } else {
            getter = (shape->hasDefaultGetter() && !shape->hasGetterValue())
                     ? JS_PropertyStub
                     : shape->getter();
        }
        if (desc.hasSet()) {
            setter = desc.setter();
        } else {
            setter = (shape->hasDefaultSetter() && !shape->hasSetterValue())
                     ? JS_StrictPropertyStub
                     : shape->setter();
        }
    }

    *rval = true;

    








    if (callDelProperty) {
        Value dummy = UndefinedValue();
        if (!CallJSPropertyOp(cx, obj2->getClass()->delProperty, obj2, id, &dummy))
            return false;
    }

    return baseops::DefineGeneric(cx, obj, id, &v, getter, setter, attrs);
}

static JSBool
DefinePropertyOnArray(JSContext *cx, HandleObject obj, HandleId id, const PropDesc &desc,
                      bool throwError, bool *rval)
{
    






    if (obj->isDenseArray() && !JSObject::makeDenseArraySlow(cx, obj))
        return JS_FALSE;

    uint32_t oldLen = obj->getArrayLength();

    if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        






        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_DEFINE_ARRAY_LENGTH);
        return JS_FALSE;
    }

    uint32_t index;
    if (js_IdIsIndex(id, &index)) {
        




        if (!DefinePropertyOnObject(cx, obj, id, desc, false, rval))
            return JS_FALSE;
        if (!*rval)
            return Reject(cx, obj, JSMSG_CANT_DEFINE_ARRAY_INDEX, throwError, rval);

        if (index >= oldLen) {
            JS_ASSERT(index != UINT32_MAX);
            obj->setArrayLength(cx, index + 1);
        }

        *rval = true;
        return JS_TRUE;
    }

    return DefinePropertyOnObject(cx, obj, id, desc, throwError, rval);
}

namespace js {

bool
DefineProperty(JSContext *cx, HandleObject obj, HandleId id, const PropDesc &desc, bool throwError,
               bool *rval)
{
    if (obj->isArray())
        return DefinePropertyOnArray(cx, obj, id, desc, throwError, rval);

    if (obj->getOps()->lookupGeneric) {
        if (obj->isProxy())
            return Proxy::defineProperty(cx, obj, id, desc.pd());
        return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);
    }

    return DefinePropertyOnObject(cx, obj, id, desc, throwError, rval);
}

} 

JSBool
js_DefineOwnProperty(JSContext *cx, HandleObject obj, HandleId id, const Value &descriptor, JSBool *bp)
{
    AutoPropDescArrayRooter descs(cx);
    PropDesc *desc = descs.append();
    if (!desc || !desc->initialize(cx, descriptor))
        return false;

    bool rval;
    if (!DefineProperty(cx, obj, id, *desc, true, &rval))
        return false;
    *bp = !!rval;
    return true;
}


static JSBool
obj_defineProperty(JSContext *cx, unsigned argc, Value *vp)
{
    RootedObject obj(cx);
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.defineProperty", obj.address()))
        return false;

    RootedId id(cx);
    if (!ValueToId(cx, argc >= 2 ? vp[3] : UndefinedValue(), id.address()))
        return JS_FALSE;

    const Value descval = argc >= 3 ? vp[4] : UndefinedValue();

    JSBool junk;
    if (!js_DefineOwnProperty(cx, obj, id, descval, &junk))
        return false;

    vp->setObject(*obj);
    return true;
}

namespace js {

bool
ReadPropertyDescriptors(JSContext *cx, JSObject *props, bool checkAccessors,
                        AutoIdVector *ids, AutoPropDescArrayRooter *descs)
{
    if (!GetPropertyNames(cx, props, JSITER_OWNONLY, ids))
        return false;

    RootedId id(cx);
    for (size_t i = 0, len = ids->length(); i < len; i++) {
        id = (*ids)[i];
        PropDesc* desc = descs->append();
        Value v;
        if (!desc || !props->getGeneric(cx, id, &v) || !desc->initialize(cx, v, checkAccessors))
            return false;
    }
    return true;
}

} 

static bool
DefineProperties(JSContext *cx, HandleObject obj, JSObject *props)
{
    AutoIdVector ids(cx);
    AutoPropDescArrayRooter descs(cx);
    if (!ReadPropertyDescriptors(cx, props, true, &ids, &descs))
        return false;

    bool dummy;
    for (size_t i = 0, len = ids.length(); i < len; i++) {
        if (!DefineProperty(cx, obj, Handle<jsid>::fromMarkedLocation(&ids[i]), descs[i], true, &dummy))
            return false;
    }

    return true;
}

extern JSBool
js_PopulateObject(JSContext *cx, HandleObject newborn, JSObject *props)
{
    return DefineProperties(cx, newborn, props);
}


static JSBool
obj_defineProperties(JSContext *cx, unsigned argc, Value *vp)
{
    
    RootedObject obj(cx);
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.defineProperties", obj.address()))
        return false;
    vp->setObject(*obj);

    
    if (argc < 2) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "Object.defineProperties", "0", "s");
        return false;
    }
    JSObject *props = ToObject(cx, &vp[3]);
    if (!props)
        return false;

    
    return DefineProperties(cx, obj, props);
}


static JSBool
obj_create(JSContext *cx, unsigned argc, Value *vp)
{
    if (argc == 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "Object.create", "0", "s");
        return false;
    }

    CallArgs args = CallArgsFromVp(argc, vp);
    const Value &v = args[0];
    if (!v.isObjectOrNull()) {
        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, v, NULL);
        if (!bytes)
            return false;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_UNEXPECTED_TYPE,
                             bytes, "not an object or null");
        JS_free(cx, bytes);
        return false;
    }

    JSObject *proto = v.toObjectOrNull();
#if JS_HAS_XML_SUPPORT
    if (proto && proto->isXML()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_XML_PROTO_FORBIDDEN);
        return false;
    }
#endif

    



    RootedObject obj(cx);
    obj = NewObjectWithGivenProto(cx, &ObjectClass, proto, &args.callee().global());
    if (!obj)
        return false;

    
    MarkTypeObjectUnknownProperties(cx, obj->type());

    
    if (args.hasDefined(1)) {
        if (args[1].isPrimitive()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
            return false;
        }

        if (!DefineProperties(cx, obj, &args[1].toObject()))
            return false;
    }

    
    args.rval().setObject(*obj);
    return true;
}

static JSBool
obj_getOwnPropertyNames(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.getOwnPropertyNames", &obj))
        return false;

    AutoIdVector keys(cx);
    if (!GetPropertyNames(cx, obj, JSITER_OWNONLY | JSITER_HIDDEN, &keys))
        return false;

    AutoValueVector vals(cx);
    if (!vals.resize(keys.length()))
        return false;

    for (size_t i = 0, len = keys.length(); i < len; i++) {
         jsid id = keys[i];
         if (JSID_IS_INT(id)) {
             JSString *str = Int32ToString(cx, JSID_TO_INT(id));
             if (!str)
                 return false;
             vals[i].setString(str);
         } else if (JSID_IS_ATOM(id)) {
             vals[i].setString(JSID_TO_STRING(id));
         } else {
             vals[i].setObject(*JSID_TO_OBJECT(id));
         }
    }

    JSObject *aobj = NewDenseCopiedArray(cx, vals.length(), vals.begin());
    if (!aobj)
        return false;

    vp->setObject(*aobj);
    return true;
}

static JSBool
obj_isExtensible(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.isExtensible", &obj))
        return false;

    vp->setBoolean(obj->isExtensible());
    return true;
}

static JSBool
obj_preventExtensions(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.preventExtensions", &obj))
        return false;

    vp->setObject(*obj);
    if (!obj->isExtensible())
        return true;

    return obj->preventExtensions(cx);
}

 inline unsigned
JSObject::getSealedOrFrozenAttributes(unsigned attrs, ImmutabilityType it)
{
    
    if (it == FREEZE && !(attrs & (JSPROP_GETTER | JSPROP_SETTER)))
        return JSPROP_PERMANENT | JSPROP_READONLY;
    return JSPROP_PERMANENT;
}

bool
JSObject::sealOrFreeze(JSContext *cx, ImmutabilityType it)
{
    assertSameCompartment(cx, this);
    JS_ASSERT(it == SEAL || it == FREEZE);

    RootedObject self(cx, this);

    if (isExtensible() && !preventExtensions(cx))
        return false;

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, self, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    
    JS_ASSERT(!self->isDenseArray());

    if (self->isNative() && !self->inDictionaryMode()) {
        






        Shape *last = EmptyShape::getInitialShape(cx, self->getClass(),
                                                  self->getProto(),
                                                  self->getParent(),
                                                  self->getAllocKind(),
                                                  self->lastProperty()->getObjectFlags());
        if (!last)
            return false;

        
        AutoShapeVector shapes(cx);
        for (Shape::Range r = self->lastProperty()->all(); !r.empty(); r.popFront()) {
            if (!shapes.append(&r.front()))
                return false;
        }
        Reverse(shapes.begin(), shapes.end());

        for (size_t i = 0; i < shapes.length(); i++) {
            StackShape child(shapes[i]);
            child.attrs |= getSealedOrFrozenAttributes(child.attrs, it);

            if (!JSID_IS_EMPTY(child.propid))
                MarkTypePropertyConfigured(cx, self, child.propid);

            last = cx->propertyTree().getChild(cx, last, self->numFixedSlots(), child);
            if (!last)
                return false;
        }

        JS_ASSERT(self->lastProperty()->slotSpan() == last->slotSpan());
        JS_ALWAYS_TRUE(self->setLastProperty(cx, last));
    } else {
        RootedId id(cx);
        for (size_t i = 0; i < props.length(); i++) {
            id = props[i];

            unsigned attrs;
            if (!self->getGenericAttributes(cx, id, &attrs))
                return false;

            unsigned new_attrs = getSealedOrFrozenAttributes(attrs, it);

            
            if ((attrs | new_attrs) == attrs)
                continue;

            attrs |= new_attrs;
            if (!self->setGenericAttributes(cx, id, &attrs))
                return false;
        }
    }

    return true;
}

bool
JSObject::isSealedOrFrozen(JSContext *cx, ImmutabilityType it, bool *resultp)
{
    if (isExtensible()) {
        *resultp = false;
        return true;
    }

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, this, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    RootedId id(cx);
    for (size_t i = 0, len = props.length(); i < len; i++) {
        id = props[i];

        unsigned attrs;
        if (!getGenericAttributes(cx, id, &attrs))
            return false;

        




        if (!(attrs & JSPROP_PERMANENT) ||
            (it == FREEZE && !(attrs & (JSPROP_READONLY | JSPROP_GETTER | JSPROP_SETTER))))
        {
            *resultp = false;
            return true;
        }
    }

    
    *resultp = true;
    return true;
}

static JSBool
obj_freeze(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.freeze", &obj))
        return false;

    vp->setObject(*obj);

    return obj->freeze(cx);
}

static JSBool
obj_isFrozen(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.preventExtensions", &obj))
        return false;

    bool frozen;
    if (!obj->isFrozen(cx, &frozen))
        return false;
    vp->setBoolean(frozen);
    return true;
}

static JSBool
obj_seal(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.seal", &obj))
        return false;

    vp->setObject(*obj);

    return obj->seal(cx);
}

static JSBool
obj_isSealed(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.isSealed", &obj))
        return false;

    bool sealed;
    if (!obj->isSealed(cx, &sealed))
        return false;
    vp->setBoolean(sealed);
    return true;
}

JSFunctionSpec object_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,             obj_toSource,                0,0),
#endif
    JS_FN(js_toString_str,             obj_toString,                0,0),
    JS_FN(js_toLocaleString_str,       obj_toLocaleString,          0,0),
    JS_FN(js_valueOf_str,              obj_valueOf,                 0,0),
#if JS_HAS_OBJ_WATCHPOINT
    JS_FN(js_watch_str,                obj_watch,                   2,0),
    JS_FN(js_unwatch_str,              obj_unwatch,                 1,0),
#endif
    JS_FN(js_hasOwnProperty_str,       obj_hasOwnProperty,          1,0),
    JS_FN(js_isPrototypeOf_str,        obj_isPrototypeOf,           1,0),
    JS_FN(js_propertyIsEnumerable_str, obj_propertyIsEnumerable,    1,0),
#if OLD_GETTER_SETTER_METHODS
    JS_FN(js_defineGetter_str,         js::obj_defineGetter,        2,0),
    JS_FN(js_defineSetter_str,         js::obj_defineSetter,        2,0),
    JS_FN(js_lookupGetter_str,         obj_lookupGetter,            1,0),
    JS_FN(js_lookupSetter_str,         obj_lookupSetter,            1,0),
#endif
    JS_FS_END
};

JSFunctionSpec object_static_methods[] = {
    JS_FN("getPrototypeOf",            obj_getPrototypeOf,          1,0),
    JS_FN("getOwnPropertyDescriptor",  obj_getOwnPropertyDescriptor,2,0),
    JS_FN("keys",                      obj_keys,                    1,0),
    JS_FN("defineProperty",            obj_defineProperty,          3,0),
    JS_FN("defineProperties",          obj_defineProperties,        2,0),
    JS_FN("create",                    obj_create,                  2,0),
    JS_FN("getOwnPropertyNames",       obj_getOwnPropertyNames,     1,0),
    JS_FN("isExtensible",              obj_isExtensible,            1,0),
    JS_FN("preventExtensions",         obj_preventExtensions,       1,0),
    JS_FN("freeze",                    obj_freeze,                  1,0),
    JS_FN("isFrozen",                  obj_isFrozen,                1,0),
    JS_FN("seal",                      obj_seal,                    1,0),
    JS_FN("isSealed",                  obj_isSealed,                1,0),
    JS_FS_END
};

JSBool
js_Object(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj;
    if (argc == 0) {
        
        obj = NULL;
    } else {
        
        if (!js_ValueToObjectOrNull(cx, vp[2], &obj))
            return JS_FALSE;
    }
    if (!obj) {
        
        JS_ASSERT(!argc || vp[2].isNull() || vp[2].isUndefined());
        gc::AllocKind kind = NewObjectGCKind(cx, &ObjectClass);
        obj = NewBuiltinClassInstance(cx, &ObjectClass, kind);
        if (!obj)
            return JS_FALSE;
        jsbytecode *pc;
        JSScript *script = cx->stack.currentScript(&pc);
        if (script) {
            
            if (!types::SetInitializerObjectType(cx, script, pc, obj))
                return JS_FALSE;
        }
    }
    vp->setObject(*obj);
    return JS_TRUE;
}

static inline JSObject *
NewObject(JSContext *cx, Class *clasp, types::TypeObject *type_, JSObject *parent,
          gc::AllocKind kind)
{
    JS_ASSERT(clasp != &ArrayClass);
    JS_ASSERT_IF(clasp == &FunctionClass,
                 kind == JSFunction::FinalizeKind || kind == JSFunction::ExtendedFinalizeKind);
    JS_ASSERT_IF(parent, parent->global() == cx->compartment->global());

    RootedTypeObject type(cx, type_);

    RootedShape shape(cx);
    shape = EmptyShape::getInitialShape(cx, clasp, type->proto, parent, kind);
    if (!shape)
        return NULL;

    HeapSlot *slots;
    if (!PreallocateObjectDynamicSlots(cx, shape, &slots))
        return NULL;

    JSObject *obj = JSObject::create(cx, kind, shape, type, slots);
    if (!obj) {
        cx->free_(slots);
        return NULL;
    }

    



    if (clasp->trace && !(clasp->flags & JSCLASS_IMPLEMENTS_BARRIERS))
        cx->runtime->gcIncrementalEnabled = false;

    Probes::createObject(cx, obj);
    return obj;
}

JSObject *
js::NewObjectWithGivenProto(JSContext *cx, js::Class *clasp, JSObject *proto_, JSObject *parent_,
                            gc::AllocKind kind)
{
    RootedObject proto(cx, proto_), parent(cx, parent_);

    if (CanBeFinalizedInBackground(kind, clasp))
        kind = GetBackgroundAllocKind(kind);

    NewObjectCache &cache = cx->runtime->newObjectCache;

    NewObjectCache::EntryIndex entry = -1;
    if (proto && (!parent || parent == proto->getParent()) && !proto->isGlobal()) {
        if (cache.lookupProto(clasp, proto, kind, &entry))
            return cache.newObjectFromHit(cx, entry);
    }

    types::TypeObject *type = proto ? proto->getNewType(cx) : cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    



    if (!parent && proto)
        parent = proto->getParent();

    JSObject *obj = NewObject(cx, clasp, type, parent, kind);
    if (!obj)
        return NULL;

    if (entry != -1 && !obj->hasDynamicSlots())
        cache.fillProto(entry, clasp, proto, kind, obj);

    return obj;
}

JSObject *
js::NewObjectWithClassProto(JSContext *cx, js::Class *clasp, JSObject *proto, JSObject *parent_,
                            gc::AllocKind kind)
{
    if (proto)
        return NewObjectWithGivenProto(cx, clasp, proto, parent_, kind);

    RootedObject parent(cx, parent_);

    if (CanBeFinalizedInBackground(kind, clasp))
        kind = GetBackgroundAllocKind(kind);

    if (!parent)
        parent = GetCurrentGlobal(cx);

    








    JSProtoKey protoKey = GetClassProtoKey(clasp);

    NewObjectCache &cache = cx->runtime->newObjectCache;

    NewObjectCache::EntryIndex entry = -1;
    if (parent->isGlobal() && protoKey != JSProto_Null) {
        if (cache.lookupGlobal(clasp, &parent->asGlobal(), kind, &entry))
            return cache.newObjectFromHit(cx, entry);
    }

    if (!FindProto(cx, clasp, parent, &proto))
        return NULL;

    types::TypeObject *type = proto->getNewType(cx);
    if (!type)
        return NULL;

    JSObject *obj = NewObject(cx, clasp, type, parent, kind);
    if (!obj)
        return NULL;

    if (entry != -1 && !obj->hasDynamicSlots())
        cache.fillGlobal(entry, clasp, &parent->asGlobal(), kind, obj);

    return obj;
}

JSObject *
js::NewObjectWithType(JSContext *cx, HandleTypeObject type, JSObject *parent, gc::AllocKind kind)
{
    JS_ASSERT(type->proto->hasNewType(type));
    JS_ASSERT(parent);

    JS_ASSERT(kind <= gc::FINALIZE_OBJECT_LAST);
    if (CanBeFinalizedInBackground(kind, &ObjectClass))
        kind = GetBackgroundAllocKind(kind);

    NewObjectCache &cache = cx->runtime->newObjectCache;

    NewObjectCache::EntryIndex entry = -1;
    if (parent == type->proto->getParent()) {
        if (cache.lookupType(&ObjectClass, type, kind, &entry))
            return cache.newObjectFromHit(cx, entry);
    }

    JSObject *obj = NewObject(cx, &ObjectClass, type, parent, kind);
    if (!obj)
        return NULL;

    if (entry != -1 && !obj->hasDynamicSlots())
        cache.fillType(entry, &ObjectClass, type, kind, obj);

    return obj;
}

JSObject *
js::NewReshapedObject(JSContext *cx, HandleTypeObject type, JSObject *parent,
                      gc::AllocKind kind, const Shape *shape)
{
    RootedObject res(cx, NewObjectWithType(cx, type, parent, kind));
    if (!res)
        return NULL;

    if (shape->isEmptyShape())
        return res;

    
    js::AutoIdVector ids(cx);
    for (unsigned i = 0; i <= shape->slot(); i++) {
        if (!ids.append(JSID_VOID))
            return NULL;
    }
    const js::Shape *nshape = shape;
    while (!nshape->isEmptyShape()) {
        ids[nshape->slot()] = nshape->propid();
        nshape = nshape->previous();
    }

    
    RootedId id(cx);
    for (unsigned i = 0; i < ids.length(); i++) {
        id = ids[i];
        if (!DefineNativeProperty(cx, res, id, js::UndefinedValue(), NULL, NULL,
                                  JSPROP_ENUMERATE, 0, 0, DNP_SKIP_TYPE)) {
            return NULL;
        }
    }
    JS_ASSERT(!res->inDictionaryMode());

    return res;
}

JSObject*
js_CreateThis(JSContext *cx, Class *newclasp, JSObject *callee)
{
    Value protov;
    if (!callee->getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &protov))
        return NULL;

    JSObject *proto = protov.isObjectOrNull() ? protov.toObjectOrNull() : NULL;
    JSObject *parent = callee->getParent();
    gc::AllocKind kind = NewObjectGCKind(cx, newclasp);
    return NewObjectWithClassProto(cx, newclasp, proto, parent, kind);
}

static inline JSObject *
CreateThisForFunctionWithType(JSContext *cx, HandleTypeObject type, JSObject *parent)
{
    if (type->newScript) {
        




        gc::AllocKind kind = type->newScript->allocKind;
        JSObject *res = NewObjectWithType(cx, type, parent, kind);
        if (res)
            JS_ALWAYS_TRUE(res->setLastProperty(cx, (Shape *) type->newScript->shape.get()));
        return res;
    }

    gc::AllocKind kind = NewObjectGCKind(cx, &ObjectClass);
    return NewObjectWithType(cx, type, parent, kind);
}

JSObject *
js_CreateThisForFunctionWithProto(JSContext *cx, HandleObject callee, JSObject *proto)
{
    JSObject *res;

    if (proto) {
        RootedTypeObject type(cx, proto->getNewType(cx, callee->toFunction()));
        if (!type)
            return NULL;
        res = CreateThisForFunctionWithType(cx, type, callee->getParent());
    } else {
        gc::AllocKind kind = NewObjectGCKind(cx, &ObjectClass);
        res = NewObjectWithClassProto(cx, &ObjectClass, proto, callee->getParent(), kind);
    }

    if (res && cx->typeInferenceEnabled())
        TypeScript::SetThis(cx, callee->toFunction()->script(), types::Type::ObjectType(res));

    return res;
}

JSObject *
js_CreateThisForFunction(JSContext *cx, HandleObject callee, bool newType)
{
    Value protov;
    if (!callee->getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &protov))
        return NULL;
    JSObject *proto;
    if (protov.isObject())
        proto = &protov.toObject();
    else
        proto = NULL;
    JSObject *obj = js_CreateThisForFunctionWithProto(cx, callee, proto);

    if (obj && newType) {
        



        obj->clear(cx);
        if (!obj->setSingletonType(cx))
            return NULL;

        JSScript *calleeScript = callee->toFunction()->script();
        TypeScript::SetThis(cx, calleeScript, types::Type::ObjectType(obj));
    }

    return obj;
}






static bool
Detecting(JSContext *cx, jsbytecode *pc)
{
    
    JSOp op = JSOp(*pc);
    if (js_CodeSpec[op].format & JOF_DETECTING)
        return true;

    JSAtom *atom;

    JSScript *script = cx->stack.currentScript();
    jsbytecode *endpc = script->code + script->length;
    JS_ASSERT(script->code <= pc && pc < endpc);

    if (op == JSOP_NULL) {
        



        if (++pc < endpc) {
            op = JSOp(*pc);
            return op == JSOP_EQ || op == JSOP_NE;
        }
        return false;
    }

    if (op == JSOP_GETGNAME || op == JSOP_NAME) {
        




        atom = script->getAtom(GET_UINT32_INDEX(pc));
        if (atom == cx->runtime->atomState.typeAtoms[JSTYPE_VOID] &&
            (pc += js_CodeSpec[op].length) < endpc) {
            op = JSOp(*pc);
            return op == JSOP_EQ || op == JSOP_NE || op == JSOP_STRICTEQ || op == JSOP_STRICTNE;
        }
    }

    return false;
}





unsigned
js_InferFlags(JSContext *cx, unsigned defaultFlags)
{
    const JSCodeSpec *cs;
    uint32_t format;
    unsigned flags = 0;

    jsbytecode *pc;
    JSScript *script = cx->stack.currentScript(&pc);
    if (!script || !pc)
        return defaultFlags;

    cs = &js_CodeSpec[*pc];
    format = cs->format;
    if (JOF_MODE(format) != JOF_NAME)
        flags |= JSRESOLVE_QUALIFIED;
    if (format & JOF_SET) {
        flags |= JSRESOLVE_ASSIGNING;
    } else if (cs->length >= 0) {
        pc += cs->length;
        if (pc < script->code + script->length && Detecting(cx, pc))
            flags |= JSRESOLVE_DETECTING;
    }
    if (format & JOF_DECLARING)
        flags |= JSRESOLVE_DECLARING;
    return flags;
}

JSBool
JSObject::nonNativeSetProperty(JSContext *cx, HandleId id, js::Value *vp, JSBool strict)
{
    RootedObject self(cx, this);
    if (JS_UNLIKELY(watched())) {
        WatchpointMap *wpmap = cx->compartment->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, self, id, vp))
            return false;
    }
    return self->getOps()->setGeneric(cx, self, id, vp, strict);
}

JSBool
JSObject::nonNativeSetElement(JSContext *cx, uint32_t index, js::Value *vp, JSBool strict)
{
    RootedObject self(cx, this);
    if (JS_UNLIKELY(watched())) {
        RootedId id(cx);
        if (!IndexToId(cx, index, id.address()))
            return false;

        WatchpointMap *wpmap = cx->compartment->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, self, id, vp))
            return false;
    }
    return self->getOps()->setElement(cx, self, index, vp, strict);
}

bool
JSObject::deleteByValue(JSContext *cx, const Value &property, Value *rval, bool strict)
{
    uint32_t index;
    if (IsDefinitelyIndex(property, &index))
        return deleteElement(cx, index, rval, strict);

    Value propval = property;
    Rooted<SpecialId> sid(cx);
    if (ValueIsSpecial(this, &propval, sid.address(), cx))
        return deleteSpecial(cx, sid, rval, strict);

    RootedObject self(cx, this);

    JSAtom *name = ToAtom(cx, propval);
    if (!name)
        return false;

    if (name->isIndex(&index))
        return self->deleteElement(cx, index, rval, false);

    Rooted<PropertyName*> propname(cx, name->asPropertyName());
    return self->deleteProperty(cx, propname, rval, false);
}

JS_FRIEND_API(bool)
JS_CopyPropertiesFrom(JSContext *cx, JSObject *target, JSObject *obj)
{
    
    JS_ASSERT(target->isNative() == obj->isNative());
    if (!target->isNative())
        return true;

    AutoShapeVector shapes(cx);
    for (Shape::Range r(obj->lastProperty()); !r.empty(); r.popFront()) {
        if (!shapes.append(&r.front()))
            return false;
    }

    size_t n = shapes.length();
    while (n > 0) {
        const Shape *shape = shapes[--n];
        unsigned attrs = shape->attributes();
        PropertyOp getter = shape->getter();
        StrictPropertyOp setter = shape->setter();
        AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);
        if ((attrs & JSPROP_GETTER) && !cx->compartment->wrap(cx, &getter))
            return false;
        if ((attrs & JSPROP_SETTER) && !cx->compartment->wrap(cx, &setter))
            return false;
        Value v = shape->hasSlot() ? obj->getSlot(shape->slot()) : UndefinedValue();
        if (!cx->compartment->wrap(cx, &v))
            return false;
        Rooted<jsid> id(cx, shape->propid());
        if (!target->defineGeneric(cx, id, v, getter, setter, attrs))
            return false;
    }
    return true;
}

static bool
CopySlots(JSContext *cx, JSObject *from, JSObject *to)
{
    JS_ASSERT(!from->isNative() && !to->isNative());
    JS_ASSERT(from->getClass() == to->getClass());

    size_t n = 0;
    if (from->isWrapper() &&
        (Wrapper::wrapperHandler(from)->flags() &
         Wrapper::CROSS_COMPARTMENT)) {
        to->setSlot(0, from->getSlot(0));
        to->setSlot(1, from->getSlot(1));
        n = 2;
    }

    size_t span = JSCLASS_RESERVED_SLOTS(from->getClass());
    for (; n < span; ++n) {
        Value v = from->getSlot(n);
        if (!cx->compartment->wrap(cx, &v))
            return false;
        to->setSlot(n, v);
    }
    return true;
}

JS_FRIEND_API(JSObject *)
JS_CloneObject(JSContext *cx, JSObject *obj_, JSObject *proto_, JSObject *parent_)
{
    RootedObject obj(cx, obj_);
    RootedObject proto(cx, proto_);
    RootedObject parent(cx, parent_);

    



    if (!obj->isNative()) {
        if (obj->isDenseArray()) {
            if (!JSObject::makeDenseArraySlow(cx, obj))
                return NULL;
        } else if (!obj->isProxy()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_CANT_CLONE_OBJECT);
            return NULL;
        }
    }
    JSObject *clone = NewObjectWithGivenProto(cx, obj->getClass(), proto, parent, obj->getAllocKind());
    if (!clone)
        return NULL;
    if (obj->isNative()) {
        if (clone->isFunction() && (obj->compartment() != clone->compartment())) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_CANT_CLONE_OBJECT);
            return NULL;
        }

        if (obj->hasPrivate())
            clone->setPrivate(obj->getPrivate());
    } else {
        JS_ASSERT(obj->isProxy());
        if (!CopySlots(cx, obj, clone))
            return NULL;
    }

    return clone;
}

struct JSObject::TradeGutsReserved {
    JSContext *cx;
    Vector<Value> avals;
    Vector<Value> bvals;
    int newafixed;
    int newbfixed;
    Shape *newashape;
    Shape *newbshape;
    HeapSlot *newaslots;
    HeapSlot *newbslots;

    TradeGutsReserved(JSContext *cx)
        : cx(cx), avals(cx), bvals(cx),
          newafixed(0), newbfixed(0),
          newashape(NULL), newbshape(NULL),
          newaslots(NULL), newbslots(NULL)
    {}

    ~TradeGutsReserved()
    {
        if (newaslots)
            cx->free_(newaslots);
        if (newbslots)
            cx->free_(newbslots);
    }
};

bool
JSObject::ReserveForTradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                              TradeGutsReserved &reserved)
{
    





    if (a->sizeOfThis() == b->sizeOfThis())
        return true;

    





    if (a->isNative()) {
        if (!a->generateOwnShape(cx))
            return false;
    } else {
        reserved.newbshape = EmptyShape::getInitialShape(cx, a->getClass(),
                                                         a->getProto(), a->getParent(),
                                                         b->getAllocKind());
        if (!reserved.newbshape)
            return false;
    }
    if (b->isNative()) {
        if (!b->generateOwnShape(cx))
            return false;
    } else {
        reserved.newashape = EmptyShape::getInitialShape(cx, b->getClass(),
                                                         b->getProto(), b->getParent(),
                                                         a->getAllocKind());
        if (!reserved.newashape)
            return false;
    }

    

    if (!reserved.avals.reserve(a->slotSpan()))
        return false;
    if (!reserved.bvals.reserve(b->slotSpan()))
        return false;

    JS_ASSERT(a->elements == emptyObjectElements);
    JS_ASSERT(b->elements == emptyObjectElements);

    





    reserved.newafixed = a->numFixedSlots();
    reserved.newbfixed = b->numFixedSlots();

    if (a->hasPrivate()) {
        reserved.newafixed++;
        reserved.newbfixed--;
    }
    if (b->hasPrivate()) {
        reserved.newbfixed++;
        reserved.newafixed--;
    }

    JS_ASSERT(reserved.newafixed >= 0);
    JS_ASSERT(reserved.newbfixed >= 0);

    





    unsigned adynamic = dynamicSlotsCount(reserved.newafixed, b->slotSpan());
    unsigned bdynamic = dynamicSlotsCount(reserved.newbfixed, a->slotSpan());

    if (adynamic) {
        reserved.newaslots = (HeapSlot *) cx->malloc_(sizeof(HeapSlot) * adynamic);
        if (!reserved.newaslots)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(reserved.newaslots, adynamic);
    }
    if (bdynamic) {
        reserved.newbslots = (HeapSlot *) cx->malloc_(sizeof(HeapSlot) * bdynamic);
        if (!reserved.newbslots)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(reserved.newbslots, bdynamic);
    }

    return true;
}

void
JSObject::TradeGuts(JSContext *cx, JSObject *a, JSObject *b, TradeGutsReserved &reserved)
{
    JS_ASSERT(a->compartment() == b->compartment());
    JS_ASSERT(a->isFunction() == b->isFunction());

    
    JS_ASSERT_IF(a->isFunction(), a->sizeOfThis() == b->sizeOfThis());

    




    JS_ASSERT(!a->isRegExp() && !b->isRegExp());

    



    JS_ASSERT(!a->isDenseArray() && !b->isDenseArray());
    JS_ASSERT(!a->isArrayBuffer() && !b->isArrayBuffer());

#ifdef JSGC_INCREMENTAL
    




    JSCompartment *comp = a->compartment();
    if (comp->needsBarrier()) {
        MarkChildren(comp->barrierTracer(), a);
        MarkChildren(comp->barrierTracer(), b);
    }
#endif

    
    const size_t size = a->sizeOfThis();
    if (size == b->sizeOfThis()) {
        




        char tmp[tl::Max<sizeof(JSFunction), sizeof(JSObject_Slots16)>::result];
        JS_ASSERT(size <= sizeof(tmp));

        js_memcpy(tmp, a, size);
        js_memcpy(a, b, size);
        js_memcpy(b, tmp, size);

#ifdef JSGC_GENERATIONAL
        



        JSCompartment *comp = cx->compartment;
        for (size_t i = 0; i < a->numFixedSlots(); ++i) {
            HeapSlot::writeBarrierPost(comp, a, i);
            HeapSlot::writeBarrierPost(comp, b, i);
        }
#endif
    } else {
        





        unsigned acap = a->slotSpan();
        unsigned bcap = b->slotSpan();

        for (size_t i = 0; i < acap; i++)
            reserved.avals.infallibleAppend(a->getSlot(i));

        for (size_t i = 0; i < bcap; i++)
            reserved.bvals.infallibleAppend(b->getSlot(i));

        
        if (a->hasDynamicSlots())
            cx->free_(a->slots);
        if (b->hasDynamicSlots())
            cx->free_(b->slots);

        void *apriv = a->hasPrivate() ? a->getPrivate() : NULL;
        void *bpriv = b->hasPrivate() ? b->getPrivate() : NULL;

        char tmp[sizeof(JSObject)];
        js_memcpy(&tmp, a, sizeof tmp);
        js_memcpy(a, b, sizeof tmp);
        js_memcpy(b, &tmp, sizeof tmp);

        if (a->isNative())
            a->shape_->setNumFixedSlots(reserved.newafixed);
        else
            a->shape_ = reserved.newashape;

        a->slots = reserved.newaslots;
        a->initSlotRange(0, reserved.bvals.begin(), bcap);
        if (a->hasPrivate())
            a->initPrivate(bpriv);

        if (b->isNative())
            b->shape_->setNumFixedSlots(reserved.newbfixed);
        else
            b->shape_ = reserved.newbshape;

        b->slots = reserved.newbslots;
        b->initSlotRange(0, reserved.avals.begin(), acap);
        if (b->hasPrivate())
            b->initPrivate(apriv);

        
        reserved.newaslots = NULL;
        reserved.newbslots = NULL;
    }

#ifdef JSGC_GENERATIONAL
    Shape::writeBarrierPost(a->shape_, &a->shape_);
    Shape::writeBarrierPost(b->shape_, &b->shape_);
    types::TypeObject::writeBarrierPost(a->type_, &a->type_);
    types::TypeObject::writeBarrierPost(b->type_, &b->type_);
#endif

    if (a->inDictionaryMode())
        a->lastProperty()->listp = &a->shape_;
    if (b->inDictionaryMode())
        b->lastProperty()->listp = &b->shape_;
}







bool
JSObject::swap(JSContext *cx, JSObject *other)
{
    if (this->compartment() == other->compartment()) {
        TradeGutsReserved reserved(cx);
        if (!ReserveForTradeGuts(cx, this, other, reserved))
            return false;
        TradeGuts(cx, this, other, reserved);
        return true;
    }

    JSObject *thisClone;
    JSObject *otherClone;
    {
        AutoCompartment ac(cx, other);
        if (!ac.enter())
            return false;
        thisClone = JS_CloneObject(cx, this, other->getProto(), other->getParent());
        if (!thisClone || !JS_CopyPropertiesFrom(cx, thisClone, this))
            return false;
    }
    {
        AutoCompartment ac(cx, this);
        if (!ac.enter())
            return false;
        otherClone = JS_CloneObject(cx, other, other->getProto(), other->getParent());
        if (!otherClone || !JS_CopyPropertiesFrom(cx, otherClone, other))
            return false;
    }

    TradeGutsReserved reservedThis(cx);
    TradeGutsReserved reservedOther(cx);

    if (!ReserveForTradeGuts(cx, this, otherClone, reservedThis) ||
        !ReserveForTradeGuts(cx, other, thisClone, reservedOther)) {
        return false;
    }

    TradeGuts(cx, this, otherClone, reservedThis);
    TradeGuts(cx, other, thisClone, reservedOther);

    return true;
}

static bool
DefineStandardSlot(JSContext *cx, JSObject *obj, JSProtoKey key, JSAtom *atom,
                   const Value &v, uint32_t attrs, bool &named)
{
    RootedId id(cx, AtomToId(atom));

    if (key != JSProto_Null) {
        




        JS_ASSERT(obj->isGlobal());
        JS_ASSERT(obj->isNative());

        const Shape *shape = obj->nativeLookup(cx, id);
        if (!shape) {
            uint32_t slot = 2 * JSProto_LIMIT + key;
            obj->setReservedSlot(slot, v);
            if (!obj->addProperty(cx, id, JS_PropertyStub, JS_StrictPropertyStub, slot, attrs, 0, 0))
                return false;
            AddTypePropertyId(cx, obj, id, v);

            named = true;
            return true;
        }
    }

    named = obj->defineGeneric(cx, id, v, JS_PropertyStub, JS_StrictPropertyStub, attrs);
    return named;
}

namespace js {

static void
SetClassObject(JSObject *obj, JSProtoKey key, JSObject *cobj, JSObject *proto)
{
    JS_ASSERT(!obj->getParent());
    if (!obj->isGlobal())
        return;

    obj->setReservedSlot(key, ObjectOrNullValue(cobj));
    obj->setReservedSlot(JSProto_LIMIT + key, ObjectOrNullValue(proto));
}

static void
ClearClassObject(JSContext *cx, JSObject *obj, JSProtoKey key)
{
    JS_ASSERT(!obj->getParent());
    if (!obj->isGlobal())
        return;

    obj->setSlot(key, UndefinedValue());
    obj->setSlot(JSProto_LIMIT + key, UndefinedValue());
}

JSObject *
DefineConstructorAndPrototype(JSContext *cx, HandleObject obj, JSProtoKey key, HandleAtom atom,
                              JSObject *protoProto, Class *clasp,
                              Native constructor, unsigned nargs,
                              JSPropertySpec *ps, JSFunctionSpec *fs,
                              JSPropertySpec *static_ps, JSFunctionSpec *static_fs,
                              JSObject **ctorp, AllocKind ctorKind)
{
    





















    






    RootedObject proto(cx);
    proto = NewObjectWithClassProto(cx, clasp, protoProto, obj);
    if (!proto)
        return NULL;

    if (!proto->setSingletonType(cx))
        return NULL;

    if (clasp == &ArrayClass && !JSObject::makeDenseArraySlow(cx, proto))
        return NULL;

    
    RootedObject ctor(cx);
    bool named = false;
    bool cached = false;
    if (!constructor) {
        





        if (!(clasp->flags & JSCLASS_IS_ANONYMOUS) || !obj->isGlobal() || key == JSProto_Null) {
            uint32_t attrs = (clasp->flags & JSCLASS_IS_ANONYMOUS)
                           ? JSPROP_READONLY | JSPROP_PERMANENT
                           : 0;
            if (!DefineStandardSlot(cx, obj, key, atom, ObjectValue(*proto), attrs, named))
                goto bad;
        }

        ctor = proto;
    } else {
        





        RootedFunction fun(cx);
        fun = js_NewFunction(cx, NULL, constructor, nargs, JSFUN_CONSTRUCTOR, obj, atom,
                             ctorKind);
        if (!fun)
            goto bad;

        




        if (key != JSProto_Null) {
            SetClassObject(obj, key, fun, proto);
            cached = true;
        }

        if (!DefineStandardSlot(cx, obj, key, atom, ObjectValue(*fun), 0, named))
            goto bad;

        





        ctor = fun;
        if (!LinkConstructorAndPrototype(cx, ctor, proto))
            goto bad;

        
        if (ctor->getClass() == clasp && !ctor->splicePrototype(cx, proto))
            goto bad;
    }

    if (!DefinePropertiesAndBrand(cx, proto, ps, fs) ||
        (ctor != proto && !DefinePropertiesAndBrand(cx, ctor, static_ps, static_fs)))
    {
        goto bad;
    }

    if (clasp->flags & (JSCLASS_FREEZE_PROTO|JSCLASS_FREEZE_CTOR)) {
        JS_ASSERT_IF(ctor == proto, !(clasp->flags & JSCLASS_FREEZE_CTOR));
        if (proto && (clasp->flags & JSCLASS_FREEZE_PROTO) && !proto->freeze(cx))
            goto bad;
        if (ctor && (clasp->flags & JSCLASS_FREEZE_CTOR) && !ctor->freeze(cx))
            goto bad;
    }

    
    if (!cached && key != JSProto_Null)
        SetClassObject(obj, key, ctor, proto);

    if (ctorp)
        *ctorp = ctor;
    return proto;

bad:
    if (named) {
        Value rval;
        obj->deleteByValue(cx, StringValue(atom), &rval, false);
    }
    if (cached)
        ClearClassObject(cx, obj, key);
    return NULL;
}










bool
IsStandardClassResolved(JSObject *obj, js::Class *clasp)
{
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);

    
    return (obj->getReservedSlot(key) != UndefinedValue());
}

void
MarkStandardClassInitializedNoProto(JSObject *obj, js::Class *clasp)
{
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);

    



    if (obj->getReservedSlot(key) == UndefinedValue())
        obj->setSlot(key, BooleanValue(true));
}

}

JSObject *
js_InitClass(JSContext *cx, HandleObject obj, JSObject *protoProto_,
             Class *clasp, Native constructor, unsigned nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs,
             JSObject **ctorp, AllocKind ctorKind)
{
    RootedObject protoProto(cx, protoProto_);

    RootedAtom atom(cx);
    atom = js_Atomize(cx, clasp->name, strlen(clasp->name));
    if (!atom)
        return NULL;

    











    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);
    if (key != JSProto_Null &&
        !protoProto &&
        !js_GetClassPrototype(cx, obj, JSProto_Object, protoProto.address())) {
        return NULL;
    }

    return DefineConstructorAndPrototype(cx, obj, key, atom, protoProto, clasp, constructor, nargs,
                                         ps, fs, static_ps, static_fs, ctorp, ctorKind);
}

inline bool
JSObject::updateSlotsForSpan(JSContext *cx, size_t oldSpan, size_t newSpan)
{
    JS_ASSERT(oldSpan != newSpan);

    size_t oldCount = dynamicSlotsCount(numFixedSlots(), oldSpan);
    size_t newCount = dynamicSlotsCount(numFixedSlots(), newSpan);

    if (oldSpan < newSpan) {
        if (oldCount < newCount && !growSlots(cx, oldCount, newCount))
            return false;

        if (newSpan == oldSpan + 1)
            initSlotUnchecked(oldSpan, UndefinedValue());
        else
            initializeSlotRange(oldSpan, newSpan - oldSpan);
    } else {
        
        prepareSlotRangeForOverwrite(newSpan, oldSpan);
        invalidateSlotRange(newSpan, oldSpan - newSpan);

        if (oldCount > newCount)
            shrinkSlots(cx, oldCount, newCount);
    }

    return true;
}

bool
JSObject::setLastProperty(JSContext *cx, const js::Shape *shape)
{
    JS_ASSERT(!inDictionaryMode());
    JS_ASSERT(!shape->inDictionary());
    JS_ASSERT(shape->compartment() == compartment());
    JS_ASSERT(shape->numFixedSlots() == numFixedSlots());

    size_t oldSpan = lastProperty()->slotSpan();
    size_t newSpan = shape->slotSpan();

    if (oldSpan == newSpan) {
        shape_ = const_cast<js::Shape *>(shape);
        return true;
    }

    if (!updateSlotsForSpan(cx, oldSpan, newSpan))
        return false;

    shape_ = const_cast<js::Shape *>(shape);
    return true;
}

bool
JSObject::setSlotSpan(JSContext *cx, uint32_t span)
{
    JS_ASSERT(inDictionaryMode());
    js::BaseShape *base = lastProperty()->base();

    size_t oldSpan = base->slotSpan();

    if (oldSpan == span)
        return true;

    if (!updateSlotsForSpan(cx, oldSpan, span))
        return false;

    base->setSlotSpan(span);
    return true;
}

bool
JSObject::growSlots(JSContext *cx, uint32_t oldCount, uint32_t newCount)
{
    JS_ASSERT(newCount > oldCount);
    JS_ASSERT(newCount >= SLOT_CAPACITY_MIN);
    JS_ASSERT(!isDenseArray());

    




    JS_ASSERT(newCount < NELEMENTS_LIMIT);

    size_t oldSize = Probes::objectResizeActive() ? computedSizeOfThisSlotsElements() : 0;
    size_t newSize = oldSize + (newCount - oldCount) * sizeof(Value);

    





    if (!hasLazyType() && !oldCount && type()->newScript) {
        gc::AllocKind kind = type()->newScript->allocKind;
        unsigned newScriptSlots = gc::GetGCKindSlots(kind);
        if (newScriptSlots == numFixedSlots() && gc::TryIncrementAllocKind(&kind)) {
            Rooted<TypeObject*> typeObj(cx, type());
            JSObject *obj = NewReshapedObject(cx, typeObj,
                                              getParent(), kind,
                                              typeObj->newScript->shape);
            if (!obj)
                return false;

            typeObj->newScript->allocKind = kind;
            typeObj->newScript->shape = obj->lastProperty();
            typeObj->markStateChange(cx);
        }
    }

    if (!oldCount) {
        slots = (HeapSlot *) cx->malloc_(newCount * sizeof(HeapSlot));
        if (!slots)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(slots, newCount);
        if (Probes::objectResizeActive())
            Probes::resizeObject(cx, this, oldSize, newSize);
        return true;
    }

    HeapSlot *newslots = (HeapSlot*) cx->realloc_(slots, oldCount * sizeof(HeapSlot),
                                                  newCount * sizeof(HeapSlot));
    if (!newslots)
        return false;  

    bool changed = slots != newslots;
    slots = newslots;

    Debug_SetSlotRangeToCrashOnTouch(slots + oldCount, newCount - oldCount);

    
    if (changed && isGlobal())
        types::MarkObjectStateChange(cx, this);

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, this, oldSize, newSize);

    return true;
}

void
JSObject::shrinkSlots(JSContext *cx, uint32_t oldCount, uint32_t newCount)
{
    JS_ASSERT(newCount < oldCount);
    JS_ASSERT(!isDenseArray());

    





    if (isCall())
        return;

    size_t oldSize = Probes::objectResizeActive() ? computedSizeOfThisSlotsElements() : 0;
    size_t newSize = oldSize - (oldCount - newCount) * sizeof(Value);

    if (newCount == 0) {
        cx->free_(slots);
        slots = NULL;
        if (Probes::objectResizeActive())
            Probes::resizeObject(cx, this, oldSize, newSize);
        return;
    }

    JS_ASSERT(newCount >= SLOT_CAPACITY_MIN);

    HeapSlot *newslots = (HeapSlot *) cx->realloc_(slots, newCount * sizeof(HeapSlot));
    if (!newslots)
        return;  

    bool changed = slots != newslots;
    slots = newslots;

    
    if (changed && isGlobal())
        types::MarkObjectStateChange(cx, this);

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, this, oldSize, newSize);
}

bool
JSObject::growElements(JSContext *cx, unsigned newcap)
{
    JS_ASSERT(isDenseArray());

    






    static const size_t CAPACITY_DOUBLING_MAX = 1024 * 1024;
    static const size_t CAPACITY_CHUNK = CAPACITY_DOUBLING_MAX / sizeof(Value);

    uint32_t oldcap = getDenseArrayCapacity();
    JS_ASSERT(oldcap <= newcap);

    size_t oldSize = Probes::objectResizeActive() ? computedSizeOfThisSlotsElements() : 0;

    uint32_t nextsize = (oldcap <= CAPACITY_DOUBLING_MAX)
                      ? oldcap * 2
                      : oldcap + (oldcap >> 3);

    uint32_t actualCapacity = JS_MAX(newcap, nextsize);
    if (actualCapacity >= CAPACITY_CHUNK)
        actualCapacity = JS_ROUNDUP(actualCapacity, CAPACITY_CHUNK);
    else if (actualCapacity < SLOT_CAPACITY_MIN)
        actualCapacity = SLOT_CAPACITY_MIN;

    
    if (actualCapacity >= NELEMENTS_LIMIT || actualCapacity < oldcap || actualCapacity < newcap) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    uint32_t initlen = getDenseArrayInitializedLength();
    uint32_t newAllocated = actualCapacity + ObjectElements::VALUES_PER_HEADER;

    ObjectElements *newheader;
    if (hasDynamicElements()) {
        uint32_t oldAllocated = oldcap + ObjectElements::VALUES_PER_HEADER;
        newheader = (ObjectElements *)
            cx->realloc_(getElementsHeader(), oldAllocated * sizeof(Value),
                         newAllocated * sizeof(Value));
        if (!newheader)
            return false;  
    } else {
        newheader = (ObjectElements *) cx->malloc_(newAllocated * sizeof(Value));
        if (!newheader)
            return false;  
        js_memcpy(newheader, getElementsHeader(),
                  (ObjectElements::VALUES_PER_HEADER + initlen) * sizeof(Value));
    }

    newheader->capacity = actualCapacity;
    elements = newheader->elements();

    Debug_SetSlotRangeToCrashOnTouch(elements + initlen, actualCapacity - initlen);

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, this, oldSize, computedSizeOfThisSlotsElements());

    return true;
}

void
JSObject::shrinkElements(JSContext *cx, unsigned newcap)
{
    JS_ASSERT(isDenseArray());

    uint32_t oldcap = getDenseArrayCapacity();
    JS_ASSERT(newcap <= oldcap);

    size_t oldSize = Probes::objectResizeActive() ? computedSizeOfThisSlotsElements() : 0;

    
    if (oldcap <= SLOT_CAPACITY_MIN || !hasDynamicElements())
        return;

    newcap = Max(newcap, SLOT_CAPACITY_MIN);

    uint32_t newAllocated = newcap + ObjectElements::VALUES_PER_HEADER;

    ObjectElements *newheader = (ObjectElements *)
        cx->realloc_(getElementsHeader(), newAllocated * sizeof(Value));
    if (!newheader)
        return;  

    newheader->capacity = newcap;
    elements = newheader->elements();

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, this, oldSize, computedSizeOfThisSlotsElements());
}

static JSObject *
js_InitNullClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(0);
    return NULL;
}

#define JS_PROTO(name,code,init) extern JSObject *init(JSContext *, JSObject *);
#include "jsproto.tbl"
#undef JS_PROTO

static JSClassInitializerOp lazy_prototype_init[JSProto_LIMIT] = {
#define JS_PROTO(name,code,init) init,
#include "jsproto.tbl"
#undef JS_PROTO
};

namespace js {

bool
SetProto(JSContext *cx, HandleObject obj, HandleObject proto, bool checkForCycles)
{
    JS_ASSERT_IF(!checkForCycles, obj != proto);
    JS_ASSERT(obj->isExtensible());

#if JS_HAS_XML_SUPPORT
    if (proto && proto->isXML()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_XML_PROTO_FORBIDDEN);
        return false;
    }
#endif

    


















    RootedObject oldproto(cx, obj);
    while (oldproto && oldproto->isNative()) {
        if (oldproto->hasSingletonType()) {
            if (!oldproto->generateOwnShape(cx))
                return false;
        } else {
            if (!oldproto->setUncacheableProto(cx))
                return false;
        }
        oldproto = oldproto->getProto();
    }

    if (checkForCycles) {
        for (JSObject *obj2 = proto; obj2; obj2 = obj2->getProto()) {
            if (obj2 == obj) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CYCLIC_VALUE,
                                     js_proto_str);
                return false;
            }
        }
    }

    if (obj->hasSingletonType()) {
        



        if (!obj->splicePrototype(cx, proto))
            return false;
        MarkTypeObjectUnknownProperties(cx, obj->type());
        return true;
    }

    if (proto && !proto->setNewTypeUnknown(cx))
        return false;

    TypeObject *type = proto
        ? proto->getNewType(cx, NULL)
        : cx->compartment->getEmptyType(cx);
    if (!type)
        return false;

    







    MarkTypeObjectUnknownProperties(cx, obj->type(), true);
    MarkTypeObjectUnknownProperties(cx, type, true);

    obj->setType(type);
    return true;
}

}

JSBool
js_GetClassObject(JSContext *cx, JSObject *obj_, JSProtoKey key,
                  JSObject **objp)
{
    RootedObject obj(cx, obj_);

    obj = &obj->global();
    if (!obj->isGlobal()) {
        *objp = NULL;
        return true;
    }

    Value v = obj->getReservedSlot(key);
    if (v.isObject()) {
        *objp = &v.toObject();
        return true;
    }

    AutoResolving resolving(cx, obj, NameToId(cx->runtime->atomState.classAtoms[key]));
    if (resolving.alreadyStarted()) {
        
        *objp = NULL;
        return true;
    }

    JSObject *cobj = NULL;
    if (JSClassInitializerOp init = lazy_prototype_init[key]) {
        if (!init(cx, obj))
            return false;
        v = obj->getReservedSlot(key);
        if (v.isObject())
            cobj = &v.toObject();
    }

    *objp = cobj;
    return true;
}

JSBool
js_FindClassObject(JSContext *cx, JSObject *start, JSProtoKey protoKey,
                   Value *vp, Class *clasp)
{
    JSObject *cobj, *pobj;
    RootedId id(cx);
    JSProperty *prop;
    const Shape *shape;

    RootedObject obj(cx);

    if (start) {
        obj = &start->global();
        obj = GetInnerObject(cx, obj);
    } else {
        obj = GetGlobalForScopeChain(cx);
    }
    if (!obj)
        return false;

    if (protoKey != JSProto_Null) {
        JS_ASSERT(JSProto_Null < protoKey);
        JS_ASSERT(protoKey < JSProto_LIMIT);
        if (!js_GetClassObject(cx, obj, protoKey, &cobj))
            return false;
        if (cobj) {
            vp->setObject(*cobj);
            return JS_TRUE;
        }
        id = NameToId(cx->runtime->atomState.classAtoms[protoKey]);
    } else {
        JSAtom *atom = js_Atomize(cx, clasp->name, strlen(clasp->name));
        if (!atom)
            return false;
        id = AtomToId(atom);
    }

    JS_ASSERT(obj->isNative());
    if (!LookupPropertyWithFlags(cx, obj, id, 0, &pobj, &prop))
        return false;
    Value v = UndefinedValue();
    if (prop && pobj->isNative()) {
        shape = (Shape *) prop;
        if (shape->hasSlot()) {
            v = pobj->nativeGetSlot(shape->slot());
            if (v.isPrimitive())
                v.setUndefined();
        }
    }
    *vp = v;
    return true;
}

bool
JSObject::allocSlot(JSContext *cx, uint32_t *slotp)
{
    uint32_t slot = slotSpan();
    JS_ASSERT(slot >= JSSLOT_FREE(getClass()));

    



    if (inDictionaryMode()) {
        ShapeTable &table = lastProperty()->table();
        uint32_t last = table.freelist;
        if (last != SHAPE_INVALID_SLOT) {
#ifdef DEBUG
            JS_ASSERT(last < slot);
            uint32_t next = getSlot(last).toPrivateUint32();
            JS_ASSERT_IF(next != SHAPE_INVALID_SLOT, next < slot);
#endif

            *slotp = last;

            const Value &vref = getSlot(last);
            table.freelist = vref.toPrivateUint32();
            setSlot(last, UndefinedValue());
            return true;
        }
    }

    if (slot >= SHAPE_MAXIMUM_SLOT) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    *slotp = slot;

    if (inDictionaryMode() && !setSlotSpan(cx, slot + 1))
        return false;

    return true;
}

void
JSObject::freeSlot(JSContext *cx, uint32_t slot)
{
    JS_ASSERT(slot < slotSpan());

    if (inDictionaryMode()) {
        uint32_t &last = lastProperty()->table().freelist;

        
        JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan() && last != slot);

        



        if (JSSLOT_FREE(getClass()) <= slot) {
            JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan());
            setSlot(slot, PrivateUint32Value(last));
            last = slot;
            return;
        }
    }
    setSlot(slot, UndefinedValue());
}

static bool
PurgeProtoChain(JSContext *cx, JSObject *obj_, jsid id_)
{
    const Shape *shape;

    RootedObject obj(cx, obj_);
    RootedId id(cx, id_);

    while (obj) {
        if (!obj->isNative()) {
            obj = obj->getProto();
            continue;
        }
        shape = obj->nativeLookup(cx, id);
        if (shape) {
            if (!obj->shadowingShapeChange(cx, *shape))
                return false;

            obj->shadowingShapeChange(cx, *shape);
            return true;
        }
        obj = obj->getProto();
    }

    return true;
}

bool
js_PurgeScopeChainHelper(JSContext *cx, JSObject *obj_, jsid id_)
{
    RootedObject obj(cx, obj_);
    RootedId id(cx, id_);

    JS_ASSERT(obj->isDelegate());
    PurgeProtoChain(cx, obj->getProto(), id);

    





    if (obj->isCall()) {
        while ((obj = obj->enclosingScope()) != NULL) {
            if (!PurgeProtoChain(cx, obj, id))
                return false;
        }
    }

    return true;
}

Shape *
js_AddNativeProperty(JSContext *cx, HandleObject obj, jsid id_,
                     PropertyOp getter, StrictPropertyOp setter, uint32_t slot,
                     unsigned attrs, unsigned flags, int shortid)
{
    RootedId id(cx, id_);

    




    if (!js_PurgeScopeChain(cx, obj, id))
        return NULL;

    return obj->putProperty(cx, id, getter, setter, slot, attrs, flags, shortid);
}

JSBool
baseops::DefineGeneric(JSContext *cx, HandleObject obj, HandleId id, const Value *value,
                        PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    return !!DefineNativeProperty(cx, obj, id, *value, getter, setter, attrs, 0, 0);
}

JSBool
baseops::DefineElement(JSContext *cx, HandleObject obj, uint32_t index, const Value *value,
                       PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    Rooted<jsid> id(cx);
    if (index <= JSID_INT_MAX) {
        id = INT_TO_JSID(index);
        return !!DefineNativeProperty(cx, obj, id, *value, getter, setter, attrs, 0, 0);
    }

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    if (!IndexToId(cx, index, id.address()))
        return false;

    return !!DefineNativeProperty(cx, obj, id, *value, getter, setter, attrs, 0, 0);
}







static inline bool
CallAddPropertyHook(JSContext *cx, Class *clasp, HandleObject obj, HandleShape shape, Value *vp)
{
    if (clasp->addProperty != JS_PropertyStub) {
        Value nominal = *vp;

        Rooted<jsid> id(cx, shape->propid());
        if (!CallJSPropertyOp(cx, clasp->addProperty, obj, id, vp))
            return false;
        if (*vp != nominal) {
            if (shape->hasSlot())
                obj->nativeSetSlotWithType(cx, shape, *vp);
        }
    }
    return true;
}

namespace js {

const Shape *
DefineNativeProperty(JSContext *cx, HandleObject obj, HandleId id, const Value &value_,
                     PropertyOp getter, StrictPropertyOp setter, unsigned attrs,
                     unsigned flags, int shortid, unsigned defineHow )
{
    JS_ASSERT((defineHow & ~(DNP_CACHE_RESULT | DNP_DONT_PURGE |
                             DNP_SKIP_TYPE)) == 0);
    JS_ASSERT(!(attrs & JSPROP_NATIVE_ACCESSORS));

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    
    RootedValue value(cx);
    value = value_;

    




    RootedShape shape(cx);
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        JSObject *pobj;
        JSProperty *prop;

        
        AddTypePropertyId(cx, obj, id, types::Type::UnknownType());
        MarkTypePropertyConfigured(cx, obj, id);

        




        if (!baseops::LookupProperty(cx, obj, id, &pobj, &prop))
            return NULL;
        if (prop && pobj == obj) {
            shape = (Shape *) prop;
            if (shape->isAccessorDescriptor()) {
                shape = obj->changeProperty(cx, shape, attrs,
                                            JSPROP_GETTER | JSPROP_SETTER,
                                            (attrs & JSPROP_GETTER)
                                            ? getter
                                            : shape->getter(),
                                            (attrs & JSPROP_SETTER)
                                            ? setter
                                            : shape->setter());
                if (!shape)
                    return NULL;
            } else {
                shape = NULL;
            }
        }
    }

    




    if (!(defineHow & DNP_DONT_PURGE)) {
        if (!js_PurgeScopeChain(cx, obj, id))
            return NULL;
    }

    
    Class *clasp = obj->getClass();
    if (!getter && !(attrs & JSPROP_GETTER))
        getter = clasp->getProperty;
    if (!setter && !(attrs & JSPROP_SETTER))
        setter = clasp->setProperty;

    if ((getter == JS_PropertyStub) && !(defineHow & DNP_SKIP_TYPE)) {
        



        AddTypePropertyId(cx, obj, id, value);
        if (attrs & JSPROP_READONLY)
            MarkTypePropertyConfigured(cx, obj, id);
    }

    if (!shape) {
        shape = obj->putProperty(cx, id, getter, setter, SHAPE_INVALID_SLOT,
                                 attrs, flags, shortid);
        if (!shape)
            return NULL;
    }

    
    if (shape->hasSlot())
        obj->nativeSetSlot(shape->slot(), value);

    if (!CallAddPropertyHook(cx, clasp, obj, shape, value.address())) {
        obj->removeProperty(cx, id);
        return NULL;
    }

    return shape;
}

} 





















static JSBool
CallResolveOp(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
              JSObject **objp, JSProperty **propp, bool *recursedp)
{
    Class *clasp = obj->getClass();
    JSResolveOp resolve = clasp->resolve;

    







    AutoResolving resolving(cx, obj, id);
    if (resolving.alreadyStarted()) {
        
        *recursedp = true;
        return true;
    }
    *recursedp = false;

    *propp = NULL;

    if (clasp->flags & JSCLASS_NEW_RESOLVE) {
        JSNewResolveOp newresolve = reinterpret_cast<JSNewResolveOp>(resolve);
        if (flags == RESOLVE_INFER)
            flags = js_InferFlags(cx, 0);

        RootedObject obj2(cx, NULL);
        if (!newresolve(cx, obj, id, flags, obj2.address()))
            return false;

        





        if (!obj2)
            return true;

        if (!obj2->isNative()) {
            
            JS_ASSERT(obj2 != obj);
            return obj2->lookupGeneric(cx, id, objp, propp);
        }
        obj = obj2;
    } else {
        if (!resolve(cx, obj, id))
            return false;
    }

    if (!obj->nativeEmpty()) {
        if (const Shape *shape = obj->nativeLookup(cx, id)) {
            *objp = obj;
            *propp = (JSProperty *) shape;
        }
    }

    return true;
}

static JS_ALWAYS_INLINE bool
LookupPropertyWithFlagsInline(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
                              JSObject **objp, JSProperty **propp)
{
    
    RootedObject current(cx, obj);
    while (true) {
        const Shape *shape = current->nativeLookup(cx, id);
        if (shape) {
            *objp = current;
            *propp = (JSProperty *) shape;
            return true;
        }

        
        if (current->getClass()->resolve != JS_ResolveStub) {
            bool recursed;
            if (!CallResolveOp(cx, current, id, flags, objp, propp, &recursed))
                return false;
            if (recursed)
                break;
            if (*propp) {
                



                return true;
            }
        }

        JSObject *proto = current->getProto();
        if (!proto)
            break;
        if (!proto->isNative()) {
            if (!proto->lookupGeneric(cx, id, objp, propp))
                return false;
#ifdef DEBUG
            








            if (*propp && (*objp)->isNative()) {
                while ((proto = proto->getProto()) != *objp)
                    JS_ASSERT(proto);
            }
#endif
            return true;
        }

        current = proto;
    }

    *objp = NULL;
    *propp = NULL;
    return true;
}

JS_FRIEND_API(JSBool)
baseops::LookupProperty(JSContext *cx, HandleObject obj, HandleId id, JSObject **objp,
                        JSProperty **propp)
{
    return LookupPropertyWithFlagsInline(cx, obj, id, cx->resolveFlags, objp, propp);
}

JS_FRIEND_API(JSBool)
baseops::LookupElement(JSContext *cx, HandleObject obj, uint32_t index, JSObject **objp, JSProperty **propp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, id.address()))
        return false;

    return LookupPropertyWithFlagsInline(cx, obj, id, cx->resolveFlags, objp, propp);
}

bool
js::LookupPropertyWithFlags(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
                            JSObject **objp, JSProperty **propp)
{
    return LookupPropertyWithFlagsInline(cx, obj, id, flags, objp, propp);
}

bool
js::FindPropertyHelper(JSContext *cx,
                       HandlePropertyName name, bool cacheResult, HandleObject scopeChain,
                       JSObject **objp, JSObject **pobjp, JSProperty **propp)
{
    RootedId id(cx, NameToId(name));

    JSObject *pobj;
    int scopeIndex;
    JSProperty *prop;

    
    RootedObject obj(cx, scopeChain);
    RootedObject parent(cx, obj->enclosingScope());
    for (scopeIndex = 0;
         parent
         ? IsCacheableNonGlobalScope(obj)
         : !obj->getOps()->lookupProperty;
         ++scopeIndex) {
        if (!LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags, &pobj, &prop))
            return false;

        if (prop) {
#ifdef DEBUG
            if (parent) {
                JS_ASSERT(pobj->isNative());
                JS_ASSERT(pobj->getClass() == obj->getClass());
                if (obj->isBlock()) {
                    




                    JS_ASSERT(pobj->isClonedBlock());
                } else {
                    
                    JS_ASSERT(!obj->getProto());
                }
                JS_ASSERT(pobj == obj);
            } else {
                JS_ASSERT(obj->isNative());
            }
#endif

            



            if (cacheResult && pobj->isNative()) {
                JS_PROPERTY_CACHE(cx).fill(cx, scopeChain, scopeIndex, pobj,
                                           (Shape *) prop);
            }

            goto out;
        }

        if (!parent) {
            pobj = NULL;
            goto out;
        }
        obj = parent;
        parent = obj->enclosingScope();
    }

    for (;;) {
        if (!obj->lookupGeneric(cx, id, &pobj, &prop))
            return false;
        if (prop)
            goto out;

        



        parent = obj->enclosingScope();
        if (!parent) {
            pobj = NULL;
            break;
        }
        obj = parent;
    }

  out:
    JS_ASSERT(!!pobj == !!prop);
    *objp = obj;
    *pobjp = pobj;
    *propp = prop;
    return true;
}





bool
js::FindProperty(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
                 JSObject **objp, JSObject **pobjp, JSProperty **propp)
{
    return !!FindPropertyHelper(cx, name, false, scopeChain, objp, pobjp, propp);
}

JSObject *
js::FindIdentifierBase(JSContext *cx, HandleObject scopeChain, HandlePropertyName name)
{
    



    JS_ASSERT(scopeChain->enclosingScope() != NULL);

    RootedObject obj(cx, scopeChain);

    








    for (int scopeIndex = 0;
         obj->isGlobal() || IsCacheableNonGlobalScope(obj);
         scopeIndex++) {
        JSObject *pobj;
        JSProperty *prop;
        if (!LookupPropertyWithFlags(cx, obj, name.value(), cx->resolveFlags, &pobj, &prop))
            return NULL;
        if (prop) {
            if (!pobj->isNative()) {
                JS_ASSERT(obj->isGlobal());
                return obj;
            }
            JS_ASSERT_IF(obj->isScope(), pobj->getClass() == obj->getClass());
            JS_PROPERTY_CACHE(cx).fill(cx, scopeChain, scopeIndex, pobj, (Shape *) prop);
            return obj;
        }

        JSObject *parent = obj->enclosingScope();
        if (!parent)
            return obj;
        obj = parent;
    }

    
    do {
        JSObject *pobj;
        JSProperty *prop;
        if (!obj->lookupProperty(cx, name, &pobj, &prop))
            return NULL;
        if (prop)
            break;

        




        JSObject *parent = obj->enclosingScope();
        if (!parent)
            break;
        obj = parent;
    } while (!obj->isGlobal());
    return obj;
}

static JS_ALWAYS_INLINE JSBool
js_NativeGetInline(JSContext *cx, Handle<JSObject*> receiver, JSObject *obj, JSObject *pobj,
                   const Shape *shape, unsigned getHow, Value *vp)
{
    JS_ASSERT(pobj->isNative());

    if (shape->hasSlot()) {
        *vp = pobj->nativeGetSlot(shape->slot());
        JS_ASSERT(!vp->isMagic());
        JS_ASSERT_IF(!pobj->hasSingletonType() && shape->hasDefaultGetter(),
                     js::types::TypeHasProperty(cx, pobj->type(), shape->propid(), *vp));
    } else {
        vp->setUndefined();
    }
    if (shape->hasDefaultGetter())
        return true;

    jsbytecode *pc;
    JSScript *script = cx->stack.currentScript(&pc);
    if (script && script->hasAnalysis()) {
        analyze::Bytecode *code = script->analysis()->maybeCode(pc);
        if (code)
            code->accessGetter = true;
    }

    Rooted<const Shape*> shapeRoot(cx, shape);
    RootedObject pobjRoot(cx, pobj);

    if (!shape->get(cx, receiver, obj, pobj, vp))
        return false;

    
    if (shapeRoot->hasSlot() && pobjRoot->nativeContains(cx, *shapeRoot))
        pobjRoot->nativeSetSlot(shapeRoot->slot(), *vp);

    return true;
}

JSBool
js_NativeGet(JSContext *cx, Handle<JSObject*> obj, Handle<JSObject*> pobj, const Shape *shape,
             unsigned getHow, Value *vp)
{
    return js_NativeGetInline(cx, obj, obj, pobj, shape, getHow, vp);
}

JSBool
js_NativeSet(JSContext *cx, Handle<JSObject*> obj, const Shape *shape, bool added, bool strict,
             Value *vp)
{
    AddTypePropertyId(cx, obj, shape->propid(), *vp);

    JS_ASSERT(obj->isNative());

    if (shape->hasSlot()) {
        uint32_t slot = shape->slot();

        
        if (shape->hasDefaultSetter()) {
            obj->nativeSetSlot(slot, *vp);
            return true;
        }
    } else {
        





        if (!shape->hasGetterValue() && shape->hasDefaultSetter())
            return js_ReportGetterOnlyAssignment(cx);
    }

    Rooted<const Shape *> shapeRoot(cx, shape);

    int32_t sample = cx->runtime->propertyRemovals;
    if (!shapeRoot->set(cx, obj, strict, vp))
        return false;

    



    if (shapeRoot->hasSlot() &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         obj->nativeContains(cx, *shapeRoot))) {
        obj->setSlot(shapeRoot->slot(), *vp);
    }

    return true;
}

static JS_ALWAYS_INLINE JSBool
js_GetPropertyHelperInline(JSContext *cx, HandleObject obj, HandleObject receiver, jsid id_,
                           uint32_t getHow, Value *vp)
{
    JSProperty *prop;

    RootedId id(cx, id_);

    
    RootedObject obj2(cx);
    if (!LookupPropertyWithFlagsInline(cx, obj, id, cx->resolveFlags, obj2.address(), &prop))
        return false;

    if (!prop) {
        vp->setUndefined();

        if (!CallJSPropertyOp(cx, obj->getClass()->getProperty, obj, id, vp))
            return JS_FALSE;

        
        if (!vp->isUndefined())
            AddTypePropertyId(cx, obj, id, *vp);

        



        jsbytecode *pc;
        if (vp->isUndefined() && ((pc = js_GetCurrentBytecodePC(cx)) != NULL)) {
            JSOp op = (JSOp) *pc;

            if (op == JSOP_GETXPROP) {
                
                JSAutoByteString printable;
                if (js_ValueToPrintable(cx, IdToValue(id), &printable))
                    js_ReportIsNotDefined(cx, printable.ptr());
                return false;
            }

            if (!cx->hasStrictOption() ||
                cx->stack.currentScript()->warnedAboutUndefinedProp ||
                (op != JSOP_GETPROP && op != JSOP_GETELEM)) {
                return JS_TRUE;
            }

            



            if (JSID_IS_ATOM(id, cx->runtime->atomState.iteratorAtom))
                return JS_TRUE;

            
            if (cx->resolveFlags == RESOLVE_INFER) {
                pc += js_CodeSpec[op].length;
                if (Detecting(cx, pc))
                    return JS_TRUE;
            } else if (cx->resolveFlags & JSRESOLVE_DETECTING) {
                return JS_TRUE;
            }

            unsigned flags = JSREPORT_WARNING | JSREPORT_STRICT;
            cx->stack.currentScript()->warnedAboutUndefinedProp = true;

            
            if (!js_ReportValueErrorFlags(cx, flags, JSMSG_UNDEFINED_PROP,
                                          JSDVG_IGNORE_STACK, IdToValue(id),
                                          NULL, NULL, NULL))
            {
                return false;
            }
        }
        return JS_TRUE;
    }

    if (!obj2->isNative()) {
        return obj2->isProxy()
               ? Proxy::get(cx, obj2, receiver, id, vp)
               : obj2->getGeneric(cx, id, vp);
    }

    Shape *shape = (Shape *) prop;

    if (getHow & JSGET_CACHE_RESULT)
        JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, obj2, shape);

    
    if (!js_NativeGetInline(cx, receiver, obj, obj2, shape, getHow, vp))
        return JS_FALSE;

    return JS_TRUE;
}

bool
js::GetPropertyHelper(JSContext *cx, HandleObject obj, jsid id, uint32_t getHow, Value *vp)
{
    return !!js_GetPropertyHelperInline(cx, obj, obj, id, getHow, vp);
}

JSBool
baseops::GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id, Value *vp)
{
    
    return js_GetPropertyHelperInline(cx, obj, receiver, id, 0, vp);
}

JSBool
baseops::GetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index, Value *vp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;

    
    return js_GetPropertyHelperInline(cx, obj, receiver, id, 0, vp);
}

JSBool
baseops::GetPropertyDefault(JSContext *cx, HandleObject obj, HandleId id, const Value &def, Value *vp)
{
    JSProperty *prop;
    RootedObject obj2(cx);
    if (!LookupPropertyWithFlags(cx, obj, id, JSRESOLVE_QUALIFIED, obj2.address(), &prop))
        return false;

    if (!prop) {
        *vp = def;
        return true;
    }

    return baseops::GetProperty(cx, obj2, id, vp);
}

JSBool
js::GetMethod(JSContext *cx, HandleObject obj, HandleId id, unsigned getHow, Value *vp)
{
    JSAutoResolveFlags rf(cx, JSRESOLVE_QUALIFIED);

    GenericIdOp op = obj->getOps()->getGeneric;
    if (!op) {
#if JS_HAS_XML_SUPPORT
        JS_ASSERT(!obj->isXML());
#endif
        return GetPropertyHelper(cx, obj, id, getHow, vp);
    }
#if JS_HAS_XML_SUPPORT
    if (obj->isXML())
        return js_GetXMLMethod(cx, obj, id, vp);
#endif
    return op(cx, obj, obj, id, vp);
}

JS_FRIEND_API(bool)
js::CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname)
{
    StackFrame *const fp = js_GetTopStackFrame(cx, FRAME_EXPAND_ALL);
    if (!fp)
        return true;

    
    if (!(fp->isScriptFrame() && fp->script()->strictModeCode) &&
        !cx->hasStrictOption()) {
        return true;
    }

    JSAutoByteString bytes(cx, propname);
    return !!bytes &&
           JS_ReportErrorFlagsAndNumber(cx,
                                        (JSREPORT_WARNING | JSREPORT_STRICT
                                         | JSREPORT_STRICT_MODE_ERROR),
                                        js_GetErrorMessage, NULL,
                                        JSMSG_UNDECLARED_VAR, bytes.ptr());
}

bool
JSObject::reportReadOnly(JSContext *cx, jsid id, unsigned report)
{
    return js_ReportValueErrorFlags(cx, report, JSMSG_READ_ONLY,
                                    JSDVG_IGNORE_STACK, IdToValue(id), NULL,
                                    NULL, NULL);
}

bool
JSObject::reportNotConfigurable(JSContext *cx, jsid id, unsigned report)
{
    return js_ReportValueErrorFlags(cx, report, JSMSG_CANT_DELETE,
                                    JSDVG_IGNORE_STACK, IdToValue(id), NULL,
                                    NULL, NULL);
}

bool
JSObject::reportNotExtensible(JSContext *cx, unsigned report)
{
    return js_ReportValueErrorFlags(cx, report, JSMSG_OBJECT_NOT_EXTENSIBLE,
                                    JSDVG_IGNORE_STACK, ObjectValue(*this),
                                    NULL, NULL, NULL);
}

bool
JSObject::callMethod(JSContext *cx, HandleId id, unsigned argc, Value *argv, Value *vp)
{
    Value fval;
    Rooted<JSObject*> obj(cx, this);
    return GetMethod(cx, obj, id, 0, &fval) &&
           Invoke(cx, ObjectValue(*obj), fval, argc, argv, vp);
}

JSBool
baseops::SetPropertyHelper(JSContext *cx, HandleObject obj, HandleId id, unsigned defineHow,
                           Value *vp, JSBool strict)
{
    JSObject *pobj;
    JSProperty *prop;
    unsigned attrs, flags;
    int shortid;
    Class *clasp;
    PropertyOp getter;
    StrictPropertyOp setter;
    bool added;

    JS_ASSERT((defineHow & ~(DNP_CACHE_RESULT | DNP_UNQUALIFIED)) == 0);

    if (JS_UNLIKELY(obj->watched())) {
        
        WatchpointMap *wpmap = cx->compartment->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, vp))
            return false;
    }

    if (!LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags, &pobj, &prop))
        return false;
    if (prop) {
        if (!pobj->isNative()) {
            if (pobj->isProxy()) {
                AutoPropertyDescriptorRooter pd(cx);
                if (!Proxy::getPropertyDescriptor(cx, pobj, id, true, &pd))
                    return false;

                if ((pd.attrs & (JSPROP_SHARED | JSPROP_SHADOWABLE)) == JSPROP_SHARED) {
                    return !pd.setter ||
                           CallSetter(cx, obj, id, pd.setter, pd.attrs, pd.shortid, strict, vp);
                }

                if (pd.attrs & JSPROP_READONLY) {
                    if (strict)
                        return JSObject::reportReadOnly(cx, id, JSREPORT_ERROR);
                    if (cx->hasStrictOption())
                        return JSObject::reportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                    return true;
                }
            }

            prop = NULL;
        }
    } else {
        
        JS_ASSERT(!obj->isBlock());

        if (obj->isGlobal() &&
            (defineHow & DNP_UNQUALIFIED) &&
            !js::CheckUndeclaredVarAssignment(cx, JSID_TO_STRING(id))) {
            return JS_FALSE;
        }
    }

    RootedShape shape(cx, (Shape *) prop);

    



    attrs = JSPROP_ENUMERATE;
    flags = 0;
    shortid = 0;
    clasp = obj->getClass();
    getter = clasp->getProperty;
    setter = clasp->setProperty;

    if (shape) {
        
        if (shape->isAccessorDescriptor()) {
            if (shape->hasDefaultSetter())
                return js_ReportGetterOnlyAssignment(cx);
        } else {
            JS_ASSERT(shape->isDataDescriptor());

            if (!shape->writable()) {
                
                if (strict)
                    return JSObject::reportReadOnly(cx, id, JSREPORT_ERROR);
                if (cx->hasStrictOption())
                    return JSObject::reportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                return JS_TRUE;
            }
        }

        attrs = shape->attributes();
        if (pobj != obj) {
            


            if (!shape->shadowable()) {
                if (defineHow & DNP_CACHE_RESULT)
                    JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, pobj, shape);

                if (shape->hasDefaultSetter() && !shape->hasGetterValue())
                    return JS_TRUE;

                return shape->set(cx, obj, strict, vp);
            }

            














            if (!shape->hasSlot()) {
                if (shape->hasShortID()) {
                    flags = Shape::HAS_SHORTID;
                    shortid = shape->shortid();
                }
                attrs &= ~JSPROP_SHARED;
                getter = shape->getter();
                setter = shape->setter();
            } else {
                
                attrs = JSPROP_ENUMERATE;
            }

            



            shape = NULL;
        }
    }

    added = false;
    if (!shape) {
        if (!obj->isExtensible()) {
            
            if (strict)
                return obj->reportNotExtensible(cx);
            if (cx->hasStrictOption())
                return obj->reportNotExtensible(cx, JSREPORT_STRICT | JSREPORT_WARNING);
            return JS_TRUE;
        }

        



        if (!js_PurgeScopeChain(cx, obj, id))
            return JS_FALSE;

        shape = obj->putProperty(cx, id, getter, setter, SHAPE_INVALID_SLOT,
                                 attrs, flags, shortid);
        if (!shape)
            return JS_FALSE;

        




        if (shape->hasSlot())
            obj->nativeSetSlot(shape->slot(), UndefinedValue());

        
        if (!CallAddPropertyHook(cx, clasp, obj, shape, vp)) {
            obj->removeProperty(cx, id);
            return JS_FALSE;
        }
        added = true;
    }

    if ((defineHow & DNP_CACHE_RESULT) && !added)
        JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, obj, shape);

    return js_NativeSet(cx, obj, shape, added, strict, vp);
}

JSBool
baseops::SetElementHelper(JSContext *cx, HandleObject obj, uint32_t index, unsigned defineHow,
                          Value *vp, JSBool strict)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, id.address()))
        return false;
    return baseops::SetPropertyHelper(cx, obj, id, defineHow, vp, strict);
}

JSBool
baseops::GetAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    JSObject *nobj;
    JSProperty *prop;
    if (!baseops::LookupProperty(cx, obj, id, &nobj, &prop))
        return false;
    if (!prop) {
        *attrsp = 0;
        return true;
    }
    if (!nobj->isNative())
        return nobj->getGenericAttributes(cx, id, attrsp);

    const Shape *shape = (Shape *)prop;
    *attrsp = shape->attributes();
    return true;
}

JSBool
baseops::GetElementAttributes(JSContext *cx, HandleObject obj, uint32_t index, unsigned *attrsp)
{
    JSObject *nobj;
    JSProperty *prop;
    if (!baseops::LookupElement(cx, obj, index, &nobj, &prop))
        return false;
    if (!prop) {
        *attrsp = 0;
        return true;
    }
    if (!nobj->isNative())
        return nobj->getElementAttributes(cx, index, attrsp);

    const Shape *shape = (Shape *)prop;
    *attrsp = shape->attributes();
    return true;
}

JSBool
baseops::SetAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    JSObject *nobj;
    JSProperty *prop;
    if (!baseops::LookupProperty(cx, obj, id, &nobj, &prop))
        return false;
    if (!prop)
        return true;
    return nobj->isNative()
           ? nobj->changePropertyAttributes(cx, (Shape *) prop, *attrsp)
           : nobj->setGenericAttributes(cx, id, attrsp);
}

JSBool
baseops::SetElementAttributes(JSContext *cx, HandleObject obj, uint32_t index, unsigned *attrsp)
{
    JSObject *nobj;
    JSProperty *prop;
    if (!baseops::LookupElement(cx, obj, index, &nobj, &prop))
        return false;
    if (!prop)
        return true;
    return nobj->isNative()
           ? nobj->changePropertyAttributes(cx, (Shape *) prop, *attrsp)
           : nobj->setElementAttributes(cx, index, attrsp);
}

JSBool
baseops::DeleteGeneric(JSContext *cx, HandleObject obj, HandleId id, Value *rval, JSBool strict)
{
    JSObject *proto;
    JSProperty *prop;
    const Shape *shape;

    rval->setBoolean(true);

    if (!baseops::LookupProperty(cx, obj, id, &proto, &prop))
        return false;
    if (!prop || proto != obj) {
        



        return CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, id, rval);
    }

    shape = (Shape *)prop;
    if (!shape->configurable()) {
        if (strict)
            return obj->reportNotConfigurable(cx, id);
        rval->setBoolean(false);
        return true;
    }

    if (shape->hasSlot()) {
        const Value &v = obj->nativeGetSlot(shape->slot());
        GCPoke(cx->runtime, v);
    }

    RootedId userid(cx);
    if (!shape->getUserId(cx, userid.address()))
        return false;

    if (!CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, userid, rval))
        return false;
    if (rval->isFalse())
        return true;

    return obj->removeProperty(cx, id) && js_SuppressDeletedProperty(cx, obj, id);
}

JSBool
baseops::DeleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, Value *rval, JSBool strict)
{
    Rooted<jsid> id(cx, NameToId(name));
    return baseops::DeleteGeneric(cx, obj, id, rval, strict);
}

JSBool
baseops::DeleteElement(JSContext *cx, HandleObject obj, uint32_t index, Value *rval, JSBool strict)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, id.address()))
        return false;
    return baseops::DeleteGeneric(cx, obj, id, rval, strict);
}

JSBool
baseops::DeleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid, Value *rval, JSBool strict)
{
    Rooted<jsid> id(cx, SPECIALID_TO_JSID(sid));
    return baseops::DeleteGeneric(cx, obj, id, rval, strict);
}

namespace js {

bool
HasDataProperty(JSContext *cx, HandleObject obj, jsid id, Value *vp)
{
    if (const Shape *shape = obj->nativeLookup(cx, id)) {
        if (shape->hasDefaultGetter() && shape->hasSlot()) {
            *vp = obj->nativeGetSlot(shape->slot());
            return true;
        }
    }

    return false;
}









static bool
MaybeCallMethod(JSContext *cx, HandleObject obj, Handle<jsid> id, Value *vp)
{
    if (!GetMethod(cx, obj, id, 0, vp))
        return false;
    if (!js_IsCallable(*vp)) {
        *vp = ObjectValue(*obj);
        return true;
    }
    return Invoke(cx, ObjectValue(*obj), *vp, 0, NULL, vp);
}

JSBool
DefaultValue(JSContext *cx, HandleObject obj, JSType hint, Value *vp)
{
    JS_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);
#if JS_HAS_XML_SUPPORT
    JS_ASSERT(!obj->isXML());
#endif

    Rooted<jsid> id(cx);

    Class *clasp = obj->getClass();
    if (hint == JSTYPE_STRING) {
        id = NameToId(cx->runtime->atomState.toStringAtom);

        
        if (clasp == &StringClass) {
            if (ClassMethodIsNative(cx, obj, &StringClass, id, js_str_toString)) {
                *vp = StringValue(obj->asString().unbox());
                return true;
            }
        }

        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp->isPrimitive())
            return true;

        id = NameToId(cx->runtime->atomState.valueOfAtom);
        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp->isPrimitive())
            return true;
    } else {

        
        if (clasp == &StringClass) {
            id = NameToId(cx->runtime->atomState.valueOfAtom);
            if (ClassMethodIsNative(cx, obj, &StringClass, id, js_str_toString)) {
                *vp = StringValue(obj->asString().unbox());
                return true;
            }
        }

        
        if (clasp == &NumberClass) {
            id = NameToId(cx->runtime->atomState.valueOfAtom);
            if (ClassMethodIsNative(cx, obj, &NumberClass, id, js_num_valueOf)) {
                *vp = NumberValue(obj->asNumber().unbox());
                return true;
            }
        }

        id = NameToId(cx->runtime->atomState.valueOfAtom);
        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp->isPrimitive())
            return true;

        id = NameToId(cx->runtime->atomState.toStringAtom);
        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp->isPrimitive())
            return true;
    }

    
    JSString *str;
    if (hint == JSTYPE_STRING) {
        str = JS_InternString(cx, clasp->name);
        if (!str)
            return false;
    } else {
        str = NULL;
    }

    js_ReportValueError2(cx, JSMSG_CANT_CONVERT_TO, JSDVG_SEARCH_STACK, ObjectValue(*obj), str,
                         (hint == JSTYPE_VOID) ? "primitive type" : JS_TYPE_STR(hint));
    return false;
}

} 

JS_FRIEND_API(JSBool)
JS_EnumerateState(JSContext *cx, JSHandleObject obj, JSIterateOp enum_op, Value *statep, jsid *idp)
{
    
    Class *clasp = obj->getClass();
    JSEnumerateOp enumerate = clasp->enumerate;
    if (clasp->flags & JSCLASS_NEW_ENUMERATE) {
        JS_ASSERT(enumerate != JS_EnumerateStub);
        return ((JSNewEnumerateOp) enumerate)(cx, obj, enum_op, statep, idp);
    }

    if (!enumerate(cx, obj))
        return false;

    
    JS_ASSERT(enum_op == JSENUMERATE_INIT || enum_op == JSENUMERATE_INIT_ALL);
    statep->setMagic(JS_NATIVE_ENUMERATE);
    return true;
}

namespace js {

JSBool
CheckAccess(JSContext *cx, JSObject *obj_, HandleId id, JSAccessMode mode,
            Value *vp, unsigned *attrsp)
{
    JSBool writing;
    JSProperty *prop;
    const Shape *shape;

    RootedObject obj(cx, obj_), pobj(cx);

    while (JS_UNLIKELY(obj->isWith()))
        obj = obj->getProto();

    writing = (mode & JSACC_WRITE) != 0;
    switch (mode & JSACC_TYPEMASK) {
      case JSACC_PROTO:
        pobj = obj;
        if (!writing)
            vp->setObjectOrNull(obj->getProto());
        *attrsp = JSPROP_PERMANENT;
        break;

      case JSACC_PARENT:
        JS_ASSERT(!writing);
        pobj = obj;
        vp->setObject(*obj->getParent());
        *attrsp = JSPROP_READONLY | JSPROP_PERMANENT;
        break;

      default:
        if (!obj->lookupGeneric(cx, id, pobj.address(), &prop))
            return JS_FALSE;
        if (!prop) {
            if (!writing)
                vp->setUndefined();
            *attrsp = 0;
            pobj = obj;
            break;
        }

        if (!pobj->isNative()) {
            if (!writing) {
                    vp->setUndefined();
                *attrsp = 0;
            }
            break;
        }

        shape = (Shape *)prop;
        *attrsp = shape->attributes();
        if (!writing) {
            if (shape->hasSlot())
                *vp = pobj->nativeGetSlot(shape->slot());
            else
                vp->setUndefined();
        }
    }

    JS_ASSERT_IF(*attrsp & JSPROP_READONLY, !(*attrsp & (JSPROP_GETTER | JSPROP_SETTER)));

    











    JSCheckAccessOp check = pobj->getClass()->checkAccess;
    if (!check)
        check = cx->runtime->securityCallbacks->checkObjectAccess;
    return !check || check(cx, pobj, id, mode, vp);
}

}

JSType
baseops::TypeOf(JSContext *cx, HandleObject obj)
{
    return obj->isCallable() ? JSTYPE_FUNCTION : JSTYPE_OBJECT;
}

bool
js_IsDelegate(JSContext *cx, JSObject *obj, const Value &v)
{
    if (v.isPrimitive())
        return false;
    JSObject *obj2 = &v.toObject();
    while ((obj2 = obj2->getProto()) != NULL) {
        if (obj2 == obj)
            return true;
    }
    return false;
}

bool
js::FindClassPrototype(JSContext *cx, JSObject *scopeobj, JSProtoKey protoKey,
                       JSObject **protop, Class *clasp)
{
    Value v;
    if (!js_FindClassObject(cx, scopeobj, protoKey, &v, clasp))
        return false;

    if (IsFunctionObject(v)) {
        JSObject *ctor = &v.toObject();
        if (!ctor->getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &v))
            return false;
    }

    *protop = v.isObject() ? &v.toObject() : NULL;
    return true;
}





JSBool
js_GetClassPrototype(JSContext *cx, JSObject *scopeobj, JSProtoKey protoKey,
                     JSObject **protop, Class *clasp)
{
    JS_ASSERT(JSProto_Null <= protoKey);
    JS_ASSERT(protoKey < JSProto_LIMIT);

    if (protoKey != JSProto_Null) {
        GlobalObject *global;
        if (scopeobj) {
            global = &scopeobj->global();
        } else {
            global = GetCurrentGlobal(cx);
            if (!global) {
                *protop = NULL;
                return true;
            }
        }
        const Value &v = global->getReservedSlot(JSProto_LIMIT + protoKey);
        if (v.isObject()) {
            *protop = &v.toObject();
            return true;
        }
    }

    return FindClassPrototype(cx, scopeobj, protoKey, protop, clasp);
}

JSObject *
PrimitiveToObject(JSContext *cx, const Value &v)
{
    if (v.isString()) {
        Rooted<JSString*> str(cx, v.toString());
        return StringObject::create(cx, str);
    }
    if (v.isNumber())
        return NumberObject::create(cx, v.toNumber());

    JS_ASSERT(v.isBoolean());
    return BooleanObject::create(cx, v.toBoolean());
}

JSBool
js_PrimitiveToObject(JSContext *cx, Value *vp)
{
    JSObject *obj = PrimitiveToObject(cx, *vp);
    if (!obj)
        return false;

    vp->setObject(*obj);
    return true;
}

JSBool
js_ValueToObjectOrNull(JSContext *cx, const Value &v, JSObject **objp)
{
    JSObject *obj;

    if (v.isObjectOrNull()) {
        obj = v.toObjectOrNull();
    } else if (v.isUndefined()) {
        obj = NULL;
    } else {
        obj = PrimitiveToObject(cx, v);
        if (!obj)
            return false;
    }
    *objp = obj;
    return true;
}

namespace js {


JSObject *
ToObjectSlow(JSContext *cx, Value *vp)
{
    JS_ASSERT(!vp->isMagic());
    JS_ASSERT(!vp->isObject());

    if (vp->isNullOrUndefined()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_CONVERT_TO,
                            vp->isNull() ? "null" : "undefined", "object");
        return NULL;
    }

    JSObject *obj = PrimitiveToObject(cx, *vp);
    if (obj)
        vp->setObject(*obj);
    return obj;
}

}

JSObject *
js_ValueToNonNullObject(JSContext *cx, const Value &v)
{
    JSObject *obj;

    if (!js_ValueToObjectOrNull(cx, v, &obj))
        return NULL;
    if (!obj)
        js_ReportIsNullOrUndefined(cx, JSDVG_SEARCH_STACK, v, NULL);
    return obj;
}

void
js_GetObjectSlotName(JSTracer *trc, char *buf, size_t bufsize)
{
    JS_ASSERT(trc->debugPrinter == js_GetObjectSlotName);

    JSObject *obj = (JSObject *)trc->debugPrintArg;
    uint32_t slot = uint32_t(trc->debugPrintIndex);

    const Shape *shape;
    if (obj->isNative()) {
        shape = obj->lastProperty();
        while (shape && (!shape->hasSlot() || shape->slot() != slot))
            shape = shape->previous();
    } else {
        shape = NULL;
    }

    if (!shape) {
        const char *slotname = NULL;
        if (obj->isGlobal()) {
#define JS_PROTO(name,code,init)                                              \
    if ((code) == slot) { slotname = js_##name##_str; goto found; }
#include "jsproto.tbl"
#undef JS_PROTO
        }
      found:
        if (slotname)
            JS_snprintf(buf, bufsize, "CLASS_OBJECT(%s)", slotname);
        else
            JS_snprintf(buf, bufsize, "**UNKNOWN SLOT %ld**", (long)slot);
    } else {
        jsid propid = shape->propid();
        if (JSID_IS_INT(propid)) {
            JS_snprintf(buf, bufsize, "%ld", (long)JSID_TO_INT(propid));
        } else if (JSID_IS_ATOM(propid)) {
            PutEscapedString(buf, bufsize, JSID_TO_ATOM(propid), 0);
        } else {
            JS_snprintf(buf, bufsize, "**FINALIZED ATOM KEY**");
        }
    }
}

static const Shape *
LastConfigurableShape(JSObject *obj)
{
    for (Shape::Range r(obj->lastProperty()->all()); !r.empty(); r.popFront()) {
        const Shape *shape = &r.front();
        if (shape->configurable())
            return shape;
    }
    return NULL;
}

bool
js_ClearNative(JSContext *cx, JSObject *obj)
{
    
    while (const Shape *shape = LastConfigurableShape(obj)) {
        if (!obj->removeProperty(cx, shape->propid()))
            return false;
    }

    
    for (Shape::Range r(obj->lastProperty()->all()); !r.empty(); r.popFront()) {
        const Shape *shape = &r.front();
        if (shape->isDataDescriptor() &&
            shape->writable() &&
            shape->hasDefaultSetter() &&
            shape->hasSlot()) {
            obj->nativeSetSlot(shape->slot(), UndefinedValue());
        }
    }
    return true;
}

JSBool
js_ReportGetterOnlyAssignment(JSContext *cx)
{
    return JS_ReportErrorFlagsAndNumber(cx,
                                        JSREPORT_WARNING | JSREPORT_STRICT |
                                        JSREPORT_STRICT_MODE_ERROR,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_GETTER_ONLY);
}

JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, jsval *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_GETTER_ONLY);
    return JS_FALSE;
}

#ifdef DEBUG







void
dumpValue(const Value &v)
{
    if (v.isNull())
        fprintf(stderr, "null");
    else if (v.isUndefined())
        fprintf(stderr, "undefined");
    else if (v.isInt32())
        fprintf(stderr, "%d", v.toInt32());
    else if (v.isDouble())
        fprintf(stderr, "%g", v.toDouble());
    else if (v.isString())
        v.toString()->dump();
    else if (v.isObject() && v.toObject().isFunction()) {
        JSFunction *fun = v.toObject().toFunction();
        if (fun->atom) {
            fputs("<function ", stderr);
            FileEscapedString(stderr, fun->atom, 0);
        } else {
            fputs("<unnamed function", stderr);
        }
        if (fun->isInterpreted()) {
            JSScript *script = fun->script();
            fprintf(stderr, " (%s:%u)",
                    script->filename ? script->filename : "", script->lineno);
        }
        fprintf(stderr, " at %p>", (void *) fun);
    } else if (v.isObject()) {
        JSObject *obj = &v.toObject();
        Class *clasp = obj->getClass();
        fprintf(stderr, "<%s%s at %p>",
                clasp->name,
                (clasp == &ObjectClass) ? "" : " object",
                (void *) obj);
    } else if (v.isBoolean()) {
        if (v.toBoolean())
            fprintf(stderr, "true");
        else
            fprintf(stderr, "false");
    } else if (v.isMagic()) {
        fprintf(stderr, "<invalid");
#ifdef DEBUG
        switch (v.whyMagic()) {
          case JS_ARRAY_HOLE:        fprintf(stderr, " array hole");         break;
          case JS_NATIVE_ENUMERATE:  fprintf(stderr, " native enumeration"); break;
          case JS_NO_ITER_VALUE:     fprintf(stderr, " no iter value");      break;
          case JS_GENERATOR_CLOSING: fprintf(stderr, " generator closing");  break;
          default:                   fprintf(stderr, " ?!");                 break;
        }
#endif
        fprintf(stderr, ">");
    } else {
        fprintf(stderr, "unexpected value");
    }
}

JS_FRIEND_API(void)
js_DumpValue(const Value &val)
{
    dumpValue(val);
    fputc('\n', stderr);
}

JS_FRIEND_API(void)
js_DumpId(jsid id)
{
    fprintf(stderr, "jsid %p = ", (void *) JSID_BITS(id));
    dumpValue(IdToValue(id));
    fputc('\n', stderr);
}

static void
DumpProperty(JSObject *obj, const Shape &shape)
{
    jsid id = shape.propid();
    uint8_t attrs = shape.attributes();

    fprintf(stderr, "    ((Shape *) %p) ", (void *) &shape);
    if (attrs & JSPROP_ENUMERATE) fprintf(stderr, "enumerate ");
    if (attrs & JSPROP_READONLY) fprintf(stderr, "readonly ");
    if (attrs & JSPROP_PERMANENT) fprintf(stderr, "permanent ");
    if (attrs & JSPROP_SHARED) fprintf(stderr, "shared ");

    if (shape.hasGetterValue())
        fprintf(stderr, "getterValue=%p ", (void *) shape.getterObject());
    else if (!shape.hasDefaultGetter())
        fprintf(stderr, "getterOp=%p ", JS_FUNC_TO_DATA_PTR(void *, shape.getterOp()));

    if (shape.hasSetterValue())
        fprintf(stderr, "setterValue=%p ", (void *) shape.setterObject());
    else if (!shape.hasDefaultSetter())
        fprintf(stderr, "setterOp=%p ", JS_FUNC_TO_DATA_PTR(void *, shape.setterOp()));

    if (JSID_IS_ATOM(id))
        JSID_TO_STRING(id)->dump();
    else if (JSID_IS_INT(id))
        fprintf(stderr, "%d", (int) JSID_TO_INT(id));
    else
        fprintf(stderr, "unknown jsid %p", (void *) JSID_BITS(id));

    uint32_t slot = shape.hasSlot() ? shape.maybeSlot() : SHAPE_INVALID_SLOT;
    fprintf(stderr, ": slot %d", slot);
    if (shape.hasSlot()) {
        fprintf(stderr, " = ");
        dumpValue(obj->getSlot(slot));
    } else if (slot != SHAPE_INVALID_SLOT) {
        fprintf(stderr, " (INVALID!)");
    }
    fprintf(stderr, "\n");
}

void
JSObject::dump()
{
    JSObject *obj = this;
    fprintf(stderr, "object %p\n", (void *) obj);
    Class *clasp = obj->getClass();
    fprintf(stderr, "class %p %s\n", (void *)clasp, clasp->name);

    fprintf(stderr, "flags:");
    if (obj->isDelegate()) fprintf(stderr, " delegate");
    if (obj->isSystem()) fprintf(stderr, " system");
    if (!obj->isExtensible()) fprintf(stderr, " not_extensible");
    if (obj->isIndexed()) fprintf(stderr, " indexed");

    if (obj->isNative()) {
        if (obj->inDictionaryMode())
            fprintf(stderr, " inDictionaryMode");
        if (obj->hasShapeTable())
            fprintf(stderr, " hasShapeTable");
    }
    fprintf(stderr, "\n");

    if (obj->isDenseArray()) {
        unsigned slots = obj->getDenseArrayInitializedLength();
        fprintf(stderr, "elements\n");
        for (unsigned i = 0; i < slots; i++) {
            fprintf(stderr, " %3d: ", i);
            dumpValue(obj->getDenseArrayElement(i));
            fprintf(stderr, "\n");
            fflush(stderr);
        }
        return;
    }

    fprintf(stderr, "proto ");
    dumpValue(ObjectOrNullValue(obj->getProto()));
    fputc('\n', stderr);

    fprintf(stderr, "parent ");
    dumpValue(ObjectOrNullValue(obj->getParent()));
    fputc('\n', stderr);

    if (clasp->flags & JSCLASS_HAS_PRIVATE)
        fprintf(stderr, "private %p\n", obj->getPrivate());

    if (!obj->isNative())
        fprintf(stderr, "not native\n");

    unsigned reservedEnd = JSCLASS_RESERVED_SLOTS(clasp);
    unsigned slots = obj->slotSpan();
    unsigned stop = obj->isNative() ? reservedEnd : slots;
    if (stop > 0)
        fprintf(stderr, obj->isNative() ? "reserved slots:\n" : "slots:\n");
    for (unsigned i = 0; i < stop; i++) {
        fprintf(stderr, " %3d ", i);
        if (i < reservedEnd)
            fprintf(stderr, "(reserved) ");
        fprintf(stderr, "= ");
        dumpValue(obj->getSlot(i));
        fputc('\n', stderr);
    }

    if (obj->isNative()) {
        fprintf(stderr, "properties:\n");
        Vector<const Shape *, 8, SystemAllocPolicy> props;
        for (Shape::Range r = obj->lastProperty()->all(); !r.empty(); r.popFront())
            props.append(&r.front());
        for (size_t i = props.length(); i-- != 0;)
            DumpProperty(obj, *props[i]);
    }
    fputc('\n', stderr);
}

static void
MaybeDumpObject(const char *name, JSObject *obj)
{
    if (obj) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(ObjectValue(*obj));
        fputc('\n', stderr);
    }
}

static void
MaybeDumpValue(const char *name, const Value &v)
{
    if (!v.isNull()) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(v);
        fputc('\n', stderr);
    }
}

JS_FRIEND_API(void)
js_DumpStackFrame(JSContext *cx, StackFrame *start)
{
    
    ScriptFrameIter i(cx, StackIter::GO_THROUGH_SAVED);
    if (!start) {
        if (i.done()) {
            fprintf(stderr, "no stack for cx = %p\n", (void*) cx);
            return;
        }
    } else {
        while (!i.done() && i.fp() != start)
            ++i;

        if (i.done()) {
            fprintf(stderr, "fp = %p not found in cx = %p\n",
                    (void *)start, (void *)cx);
            return;
        }
    }

    for (; !i.done(); ++i) {
        StackFrame *const fp = i.fp();

        fprintf(stderr, "StackFrame at %p\n", (void *) fp);
        if (fp->isFunctionFrame()) {
            fprintf(stderr, "callee fun: ");
            dumpValue(ObjectValue(fp->callee()));
        } else {
            fprintf(stderr, "global frame, no callee");
        }
        fputc('\n', stderr);

        if (fp->isScriptFrame()) {
            fprintf(stderr, "file %s line %u\n",
                    fp->script()->filename, (unsigned) fp->script()->lineno);
        }

        if (jsbytecode *pc = i.pc()) {
            if (!fp->isScriptFrame()) {
                fprintf(stderr, "*** pc && !script, skipping frame\n\n");
                continue;
            }
            fprintf(stderr, "  pc = %p\n", pc);
            fprintf(stderr, "  current op: %s\n", js_CodeName[*pc]);
        }
        Value *sp = i.sp();
        fprintf(stderr, "  slots: %p\n", (void *) fp->slots());
        fprintf(stderr, "  sp:    %p = slots + %u\n", (void *) sp, (unsigned) (sp - fp->slots()));
        if (sp - fp->slots() < 10000) { 
            for (Value *p = fp->slots(); p < sp; p++) {
                fprintf(stderr, "    %p: ", (void *) p);
                dumpValue(*p);
                fputc('\n', stderr);
            }
        }
        if (fp->hasArgs()) {
            fprintf(stderr, "  actuals: %p (%u) ", (void *) fp->actuals(), (unsigned) fp->numActualArgs());
            fprintf(stderr, "  formals: %p (%u)\n", (void *) fp->formals(), (unsigned) fp->numFormalArgs());
        }
        MaybeDumpObject("blockChain", fp->maybeBlockChain());
        if (!fp->isDummyFrame()) {
            MaybeDumpValue("this", fp->thisValue());
            fprintf(stderr, "  rval: ");
            dumpValue(fp->returnValue());
        } else {
            fprintf(stderr, "dummy frame");
        }
        fputc('\n', stderr);

        fprintf(stderr, "  flags:");
        if (fp->isConstructing())
            fprintf(stderr, " constructing");
        if (fp->isDebuggerFrame())
            fprintf(stderr, " debugger");
        if (fp->isEvalFrame())
            fprintf(stderr, " eval");
        if (fp->isYielding())
            fprintf(stderr, " yielding");
        if (fp->isGeneratorFrame())
            fprintf(stderr, " generator");
        fputc('\n', stderr);

        fprintf(stderr, "  scopeChain: (JSObject *) %p\n", (void *) fp->scopeChain());

        fputc('\n', stderr);
    }
}

#endif 

JS_FRIEND_API(void)
js_DumpBacktrace(JSContext *cx)
{
    Sprinter sprinter(cx);
    sprinter.init();
    size_t depth = 0;
    for (StackIter i(cx); !i.done(); ++i, ++depth) {
        if (i.isScript()) {
            const char *filename = JS_GetScriptFilename(cx, i.script());
            unsigned line = JS_PCToLineNumber(cx, i.script(), i.pc());
            sprinter.printf("#%d %14p   %s:%d (%p @ %d)\n",
                            depth, (i.isIon() ? 0 : i.fp()), filename, line,
                            i.script(), i.pc() - i.script()->code);
        } else {
            sprinter.printf("#%d ???\n", depth);
        }
    }
    fprintf(stdout, "%s", sprinter.string());
}
