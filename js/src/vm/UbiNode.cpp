





#include "js/UbiNode.h"

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Scoped.h"

#include "jscntxt.h"

#include "js/TracingAPI.h"
#include "js/TypeDecls.h"
#include "js/Utility.h"
#include "js/Vector.h"
#include "vm/ScopeObject.h"

#include "jsobjinlines.h"

using JS::Value;
using JS::ubi::Concrete;
using JS::ubi::Edge;
using JS::ubi::EdgeRange;
using JS::ubi::Node;
using JS::ubi::TracerConcrete;


const jschar *Concrete<void>::typeName() const            { MOZ_CRASH("null ubi::Node"); }
size_t Concrete<void>::size() const                       { MOZ_CRASH("null ubi::Node"); }
EdgeRange *Concrete<void>::edges(JSContext *, bool) const { MOZ_CRASH("null ubi::Node"); }
JS::Zone *Concrete<void>::zone() const                    { MOZ_CRASH("null ubi::Node"); }
JSCompartment *Concrete<void>::compartment() const        { MOZ_CRASH("null ubi::Node"); }

Node::Node(JSGCTraceKind kind, void *ptr)
{
    switch (kind) {
      case JSTRACE_OBJECT:      construct(static_cast<JSObject *>(ptr));              break;
      case JSTRACE_STRING:      construct(static_cast<JSString *>(ptr));              break;
      case JSTRACE_SYMBOL:      construct(static_cast<JS::Symbol *>(ptr));            break;
      case JSTRACE_SCRIPT:      construct(static_cast<JSScript *>(ptr));              break;
      case JSTRACE_LAZY_SCRIPT: construct(static_cast<js::LazyScript *>(ptr));        break;
      case JSTRACE_JITCODE:     construct(static_cast<js::jit::JitCode *>(ptr));      break;
      case JSTRACE_SHAPE:       construct(static_cast<js::Shape *>(ptr));             break;
      case JSTRACE_BASE_SHAPE:  construct(static_cast<js::BaseShape *>(ptr));         break;
      case JSTRACE_TYPE_OBJECT: construct(static_cast<js::types::TypeObject *>(ptr)); break;

      default:
        MOZ_CRASH("bad JSGCTraceKind passed to JS::ubi::Node::Node");
    }
}

Node::Node(Value value)
{
    if (value.isObject())
        construct(&value.toObject());
    else if (value.isString())
        construct(value.toString());
    else if (value.isSymbol())
        construct(value.toSymbol());
    else
        construct<void>(nullptr);
}

Value
Node::exposeToJS() const
{
    Value v;

    if (is<JSObject>()) {
        JSObject &obj = *as<JSObject>();
        if (obj.is<js::ScopeObject>()) {
            v.setUndefined();
        } else if (obj.is<JSFunction>() && IsInternalFunctionObject(&obj)) {
            v.setUndefined();
        } else {
            v.setObject(obj);
        }
    } else if (is<JSString>()) {
        v.setString(as<JSString>());
    } else if (is<JS::Symbol>()) {
        v.setSymbol(as<JS::Symbol>());
    } else {
        v.setUndefined();
    }

    return v;
}



class SimpleEdge : public Edge {
    SimpleEdge(SimpleEdge &) MOZ_DELETE;
    SimpleEdge &operator=(const SimpleEdge &) MOZ_DELETE;

  public:
    SimpleEdge() : Edge() { }

    
    SimpleEdge(jschar *name, const Node &referent) {
        this->name = name;
        this->referent = referent;
    }
    ~SimpleEdge() {
        js_free(const_cast<jschar *>(name));
    }

    
    SimpleEdge(SimpleEdge &&rhs) {
        name = rhs.name;
        referent = rhs.referent;

        rhs.name = nullptr;
    }
    SimpleEdge &operator=(SimpleEdge &&rhs) {
        MOZ_ASSERT(&rhs != this);
        this->~SimpleEdge();
        new(this) SimpleEdge(mozilla::Move(rhs));
        return *this;
    }
};


typedef mozilla::Vector<SimpleEdge, 8, js::TempAllocPolicy> SimpleEdgeVector;




class SimpleEdgeVectorTracer : public JSTracer {
    
    SimpleEdgeVector *vec;

    
    bool wantNames;

    static void staticCallback(JSTracer *trc, void **thingp, JSGCTraceKind kind) {
        static_cast<SimpleEdgeVectorTracer *>(trc)->callback(thingp, kind);
    }

    void callback(void **thingp, JSGCTraceKind kind) {
        if (!okay)
            return;

        jschar *jsname = nullptr;
        if (wantNames) {
            
            char buffer[1024];
            const char *name = getTracingEdgeName(buffer, sizeof(buffer));

            
            jsname = js_pod_malloc<jschar>(strlen(name) + 1);
            if (!jsname) {
                okay = false;
                return;
            }

            size_t i;
            for (i = 0; name[i]; i++)
                jsname[i] = name[i];
            jsname[i] = '\0';
        }

        
        
        
        
        if (!vec->append(mozilla::Move(SimpleEdge(jsname, Node(kind, *thingp))))) {
            okay = false;
            return;
        }
    }

  public:
    
    bool okay;

    SimpleEdgeVectorTracer(JSContext *cx, SimpleEdgeVector *vec, bool wantNames)
      : JSTracer(JS_GetRuntime(cx), staticCallback),
        vec(vec),
        wantNames(wantNames),
        okay(true)
    { }
};




class SimpleEdgeRange : public EdgeRange {
    SimpleEdgeVector edges;
    size_t i;

    void settle() {
        front_ = i < edges.length() ? &edges[i] : nullptr;
    }

  public:
    explicit SimpleEdgeRange(JSContext *cx) : edges(cx), i(0) { }

    bool init(JSContext *cx, void *thing, JSGCTraceKind kind, bool wantNames = true) {
        SimpleEdgeVectorTracer tracer(cx, &edges, wantNames);
        JS_TraceChildren(&tracer, thing, kind);
        settle();
        return tracer.okay;
    }

    void popFront() MOZ_OVERRIDE { i++; settle(); }
};


template<typename Referent>
EdgeRange *
TracerConcrete<Referent>::edges(JSContext *cx, bool wantNames) const {
    js::ScopedJSDeletePtr<SimpleEdgeRange> r(js_new<SimpleEdgeRange>(cx));
    if (!r)
        return nullptr;

    if (!r->init(cx, ptr, ::js::gc::MapTypeToTraceKind<Referent>::kind, wantNames))
        return nullptr;

    return r.forget();
}

template<> const jschar TracerConcrete<JSObject>::concreteTypeName[] =
    MOZ_UTF16("JSObject");
template<> const jschar TracerConcrete<JSString>::concreteTypeName[] =
    MOZ_UTF16("JSString");
template<> const jschar TracerConcrete<JS::Symbol>::concreteTypeName[] =
    MOZ_UTF16("JS::Symbol");
template<> const jschar TracerConcrete<JSScript>::concreteTypeName[] =
    MOZ_UTF16("JSScript");
template<> const jschar TracerConcrete<js::LazyScript>::concreteTypeName[] =
    MOZ_UTF16("js::LazyScript");
template<> const jschar TracerConcrete<js::jit::JitCode>::concreteTypeName[] =
    MOZ_UTF16("js::jit::JitCode");
template<> const jschar TracerConcrete<js::Shape>::concreteTypeName[] =
    MOZ_UTF16("js::Shape");
template<> const jschar TracerConcrete<js::BaseShape>::concreteTypeName[] =
    MOZ_UTF16("js::BaseShape");
template<> const jschar TracerConcrete<js::types::TypeObject>::concreteTypeName[] =
    MOZ_UTF16("js::types::TypeObject");
