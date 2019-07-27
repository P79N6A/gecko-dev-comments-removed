





#include "shell/jsheaptools.h"

#include "mozilla/Move.h"

#include <string.h>

#include "jsalloc.h"
#include "jsapi.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsobj.h"
#include "jsprf.h"

#include "jsobjinlines.h"

using namespace js;

using mozilla::Move;

#ifdef DEBUG

































class HeapReverser : public JSTracer, public JS::CustomAutoRooter
{
  public:
    struct Edge;

    
    class Node {
      public:
        Node() { }
        explicit Node(JSGCTraceKind kind)
          : kind(kind), incoming(), marked(false) { }

        




        Node(Node &&rhs)
          : kind(rhs.kind), incoming(Move(rhs.incoming)), marked(rhs.marked) { }
        Node &operator=(Node &&rhs) {
            MOZ_ASSERT(this != &rhs, "self-move assignment is prohibited");
            this->~Node();
            new(this) Node(Move(rhs));
            return *this;
        }

        void trace(JSTracer *trc) {
            for (Edge *e = incoming.begin(); e != incoming.end(); e++)
                e->trace(trc);
        }

        
        JSGCTraceKind kind;

        




        Vector<Edge, 0, SystemAllocPolicy> incoming;

        
        bool marked;

      private:
        Node(const Node &) MOZ_DELETE;
        Node &operator=(const Node &) MOZ_DELETE;
    };

    
    struct Edge {
      public:
        Edge(char *name, void *origin) : name(name), origin(origin) { }
        ~Edge() { js_free(name); }

        




        Edge(Edge &&rhs) : name(rhs.name), origin(rhs.origin) {
            rhs.name = nullptr;
        }
        Edge &operator=(Edge &&rhs) {
            MOZ_ASSERT(this != &rhs, "self-move assignment is prohibited");
            this->~Edge();
            new(this) Edge(Move(rhs));
            return *this;
        }

        void trace(JSTracer *trc) {
            if (origin)
                gc::MarkGCThingRoot(trc, &origin, "HeapReverser::Edge");
        }

        
        char *name;

        






        void *origin;
    };

    



    typedef HashMap<void *, Node, DefaultHasher<void *>, SystemAllocPolicy> Map;
    Map map;

    
    explicit HeapReverser(JSContext *cx)
      : JSTracer(cx->runtime(), traverseEdgeWithThis),
        JS::CustomAutoRooter(cx),
        noggc(JS_GetRuntime(cx)),
        nocgc(JS_GetRuntime(cx)),
        runtime(JS_GetRuntime(cx)),
        parent(nullptr)
    {
    }

    bool init() { return map.init(); }

    
    bool reverseHeap();

  private:
    JS::AutoDisableGenerationalGC noggc;
    js::AutoDisableCompactingGC nocgc;

    
    JSRuntime *runtime;

    







    char *getEdgeDescription();

    
    class AutoParent {
      public:
        AutoParent(HeapReverser *reverser, void *newParent) : reverser(reverser) {
            savedParent = reverser->parent;
            reverser->parent = newParent;
        }
        ~AutoParent() {
            reverser->parent = savedParent;
        }
      private:
        HeapReverser *reverser;
        void *savedParent;
    };

    
    struct Child {
        Child(void *cell, JSGCTraceKind kind) : cell(cell), kind(kind) { }
        void *cell;
        JSGCTraceKind kind;
    };

    



    Vector<Child, 0, SystemAllocPolicy> work;

    
    void *parent;

    
    bool traverseEdge(void *cell, JSGCTraceKind kind);

    





    bool traversalStatus;

    
    static void traverseEdgeWithThis(JSTracer *tracer, void **thingp, JSGCTraceKind kind) {
        HeapReverser *reverser = static_cast<HeapReverser *>(tracer);
        if (!reverser->traverseEdge(*thingp, kind))
            reverser->traversalStatus = false;
    }

    
    jsval nodeToValue(void *cell, int kind) {
        if (kind != JSTRACE_OBJECT)
            return JSVAL_VOID;
        JSObject *object = static_cast<JSObject *>(cell);
        return OBJECT_TO_JSVAL(object);
    }

    
    virtual void trace(JSTracer *trc) MOZ_OVERRIDE {
        if (!map.initialized())
            return;
        for (Map::Enum e(map); !e.empty(); e.popFront()) {
            gc::MarkGCThingRoot(trc, const_cast<void **>(&e.front().key()), "HeapReverser::map::key");
            e.front().value().trace(trc);
        }
        for (Child *c = work.begin(); c != work.end(); ++c)
            gc::MarkGCThingRoot(trc, &c->cell, "HeapReverser::Child");
    }
};

bool
HeapReverser::traverseEdge(void *cell, JSGCTraceKind kind)
{
    
    char *edgeDescription = getEdgeDescription();
    if (!edgeDescription)
        return false;
    Edge e(edgeDescription, parent);

    Map::AddPtr a = map.lookupForAdd(cell);
    if (!a) {
        




        Node n(kind);
        uint32_t generation = map.generation();
        if (!map.add(a, cell, Move(n)) ||
            !work.append(Child(cell, kind)))
            return false;
        
        if (map.generation() != generation)
            a = map.lookupForAdd(cell);
    }

    
    return a->value().incoming.append(Move(e));
}

bool
HeapReverser::reverseHeap()
{
    traversalStatus = true;

    
    JS_TraceRuntime(this);
    if (!traversalStatus)
        return false;

    
    while (!work.empty()) {
        const Child child = work.popCopy();
        AutoParent autoParent(this, child.cell);
        JS_TraceChildren(this, child.cell, child.kind);
        if (!traversalStatus)
            return false;
    }

    return true;
}

char *
HeapReverser::getEdgeDescription()
{
    if (!debugPrinter() && debugPrintIndex() == (size_t) -1) {
        const char *arg = static_cast<const char *>(debugPrintArg());
        char *name = js_pod_malloc<char>(strlen(arg) + 1);
        if (!name)
            return nullptr;
        strcpy(name, arg);
        return name;
    }

    
    static const int nameSize = 200;
    char *name = js_pod_malloc<char>(nameSize);
    if (!name)
        return nullptr;
    if (debugPrinter())
        debugPrinter()(this, name, nameSize);
    else
        JS_snprintf(name, nameSize, "%s[%lu]",
                    static_cast<const char *>(debugPrintArg()), debugPrintIndex());

    
    return static_cast<char *>(js_realloc(name, strlen(name) + 1));
}





class ReferenceFinder {
  public:
    ReferenceFinder(JSContext *cx, const HeapReverser &reverser)
      : context(cx), reverser(reverser), result(cx) { }

    
    JSObject *findReferences(HandleObject target);

  private:
    
    JSContext *context;

    
    const HeapReverser &reverser;

    
    RootedObject result;

    
    class Path {
      public:
        Path(const HeapReverser::Edge &edge, Path *next) : edge(edge), next(next) { }

        



        char *computeName(JSContext *cx);

      private:
        const HeapReverser::Edge &edge;
        Path *next;
    };

    struct AutoNodeMarker {
        explicit AutoNodeMarker(HeapReverser::Node *node) : node(node) { node->marked = true; }
        ~AutoNodeMarker() { node->marked = false; }
      private:
        HeapReverser::Node *node;
    };

    




    bool visit(void *cell, Path *path);

    



    jsval representable(void *cell, int kind) {
        if (kind == JSTRACE_OBJECT) {
            JSObject *object = static_cast<JSObject *>(cell);

            
            if (object->is<BlockObject>() ||
                object->is<CallObject>() ||
                object->is<StaticWithObject>() ||
                object->is<DynamicWithObject>() ||
                object->is<DeclEnvObject>()) {
                return JSVAL_VOID;
            }

            
            if (JS_ObjectIsFunction(context, object) && IsInternalFunctionObject(object))
                return JSVAL_VOID;

            return OBJECT_TO_JSVAL(object);
        }

        return JSVAL_VOID;
    }

    
    bool addReferrer(jsval referrer, Path *path);
};

bool
ReferenceFinder::visit(void *cell, Path *path)
{
    
    JS_CHECK_RECURSION(context, return false);

    
    if (!cell)
        return addReferrer(JSVAL_NULL, path);

    HeapReverser::Map::Ptr p = reverser.map.lookup(cell);
    JS_ASSERT(p);
    HeapReverser::Node *node = &p->value();

    
    if (path != nullptr) {
        jsval representation = representable(cell, node->kind);
        if (!representation.isUndefined())
            return addReferrer(representation, path);
    }

    




    if (node->marked)
        return true;
    AutoNodeMarker marker(node);

    
    for (size_t i = 0; i < node->incoming.length(); i++) {
        const HeapReverser::Edge &edge = node->incoming[i];
        Path extendedPath(edge, path);
        if (!visit(edge.origin, &extendedPath))
            return false;
    }

    return true;
}

char *
ReferenceFinder::Path::computeName(JSContext *cx)
{
    
    size_t size = 6;
    for (Path *l = this; l; l = l->next)
        size += strlen(l->edge.name) + (l->next ? 2 : 0);
    size += 1;

    char *path = cx->pod_malloc<char>(size);
    if (!path)
        return nullptr;

    





    strcpy(path, "edge: ");
    char *next = path + 6;
    for (Path *l = this; l; l = l->next) {
        strcpy(next, l->edge.name);
        next += strlen(next);
        if (l->next) {
            strcpy(next, "; ");
            next += 2;
        }
    }
    JS_ASSERT(next + 1 == path + size);

    return path;
}

bool
ReferenceFinder::addReferrer(jsval referrerArg, Path *path)
{
    RootedValue referrer(context, referrerArg);

    if (!context->compartment()->wrap(context, &referrer))
        return false;

    ScopedJSFreePtr<char> pathName(path->computeName(context));
    if (!pathName)
        return false;

    
    RootedValue v(context);

    if (!JS_GetProperty(context, result, pathName, &v))
        return false;
    if (v.isUndefined()) {
        
        JSObject *array = JS_NewArrayObject(context, HandleValueArray(referrer));
        if (!array)
            return false;
        v.setObject(*array);
        return !!JS_SetProperty(context, result, pathName, v);
    }

    
    RootedObject array(context, &v.toObject());
    JS_ASSERT(JS_IsArrayObject(context, array));

    
    uint32_t length;
    return JS_GetArrayLength(context, array, &length) &&
           JS_SetElement(context, array, length, referrer);
}

JSObject *
ReferenceFinder::findReferences(HandleObject target)
{
    result = JS_NewObject(context, nullptr, JS::NullPtr(), JS::NullPtr());
    if (!result)
        return nullptr;
    if (!visit(target, nullptr))
        return nullptr;

    return result;
}


bool
FindReferences(JSContext *cx, unsigned argc, jsval *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             "findReferences", "0", "s");
        return false;
    }

    RootedValue target(cx, args[0]);
    if (!target.isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                             "argument", "not an object");
        return false;
    }

    
    HeapReverser reverser(cx);
    if (!reverser.init() || !reverser.reverseHeap())
        return false;

    
    ReferenceFinder finder(cx, reverser);
    Rooted<JSObject*> targetObj(cx, &target.toObject());
    JSObject *references = finder.findReferences(targetObj);
    if (!references)
        return false;

    args.rval().setObject(*references);
    return true;
}

#endif 
