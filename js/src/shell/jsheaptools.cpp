







































#include <string.h>

#include "jsapi.h"

#include "jsalloc.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsfun.h"
#include "jsobj.h"
#include "jsprf.h"
#include "jsutil.h"

using namespace js;

#ifdef DEBUG























class HeapReverser : public JSTracer {
  public:
    struct Edge;

    
    class Node {
      public:
        Node() { }
        Node(JSGCTraceKind kind)
          : kind(kind), incoming(), marked(false) { }

        




        Node(MoveRef<Node> rhs)
          : kind(rhs->kind), incoming(Move(rhs->incoming)), marked(rhs->marked) { }
        Node &operator=(MoveRef<Node> rhs) {
            this->~Node();
            new(this) Node(rhs);
            return *this;
        }

        
        JSGCTraceKind kind;

        




        Vector<Edge, 0, SystemAllocPolicy> incoming;

        
        bool marked;

      private:
        Node(const Node &);
        Node &operator=(const Node &);
    };

    
    struct Edge {
      public:
        Edge(char *name, void *origin) : name(name), origin(origin) { }
        ~Edge() { free(name); }

        




        Edge(MoveRef<Edge> rhs) : name(rhs->name), origin(rhs->origin) {
            rhs->name = NULL;
        }
        Edge &operator=(MoveRef<Edge> rhs) {
            this->~Edge();
            new(this) Edge(rhs);
            return *this;
        }

        
        char *name;

        






        void *origin;
    };

    



    typedef HashMap<void *, Node> Map;
    Map map;

    
    HeapReverser(JSContext *cx) : map(cx), work(cx), parent(NULL) {
        context = cx;
        callback = traverseEdgeWithThis;
    }

    bool init() { return map.init(); }

    
    bool reverseHeap();

  private:    
    







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

    



    Vector<Child> work; 

    
    void *parent;

    
    bool traverseEdge(void *cell, JSGCTraceKind kind);

    





    bool traversalStatus;

    
    static void traverseEdgeWithThis(JSTracer *tracer, void *cell, JSGCTraceKind kind) {
        HeapReverser *reverser = static_cast<HeapReverser *>(tracer);
        reverser->traversalStatus = reverser->traverseEdge(cell, kind);
    }
};

bool
HeapReverser::traverseEdge(void *cell, JSGCTraceKind kind) {
    
    char *edgeDescription = getEdgeDescription();
    if (!edgeDescription)
        return false;
    Edge e(edgeDescription, parent);

    Map::AddPtr a = map.lookupForAdd(cell);
    if (!a) {
        




        Node n(kind);
        uint32 generation = map.generation();
        if (!map.add(a, cell, Move(n)) ||
            !work.append(Child(cell, kind)))
            return false;
        
        if (map.generation() != generation)
            a = map.lookupForAdd(cell);
    }

    
    return a->value.incoming.append(Move(e));
}

bool
HeapReverser::reverseHeap() {
    
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
    if (!debugPrinter && debugPrintIndex == (size_t) -1) {
        const char *arg = static_cast<const char *>(debugPrintArg);
        char *name = static_cast<char *>(context->malloc_(strlen(arg) + 1));
        if (!name)
            return NULL;
        strcpy(name, arg);
        return name;
    }

    
    static const int nameSize = 200;
    char *name = static_cast<char *>(context->malloc_(nameSize));
    if (!name)
        return NULL;
    if (debugPrinter)
        debugPrinter(this, name, nameSize);
    else
        JS_snprintf(name, nameSize, "%s[%lu]",
                    static_cast<const char *>(debugPrintArg), debugPrintIndex);

    
    return static_cast<char *>(context->realloc_(name, strlen(name) + 1));
}





class ReferenceFinder {
  public:
    ReferenceFinder(JSContext *cx, const HeapReverser &reverser) 
      : context(cx), reverser(reverser) { }

    
    JSObject *findReferences(JSObject *target);

  private:
    
    JSContext *context;

    
    const HeapReverser &reverser;

    
    JSObject *result;

    
    class Path {
      public:
        Path(const HeapReverser::Edge &edge, Path *next) : edge(edge), next(next) { }
        
        



        char *computeName(JSContext *cx);

      private:
        const HeapReverser::Edge &edge;
        Path *next;
    };

    struct AutoNodeMarker {
        AutoNodeMarker(HeapReverser::Node *node) : node(node) { node->marked = true; }
        ~AutoNodeMarker() { node->marked = false; }
      private:
        HeapReverser::Node *node;
    };

    




    bool visit(void *cell, Path *path);

    



    jsval representable(void *cell, int kind) {
        if (kind == JSTRACE_OBJECT) {
            JSObject *object = static_cast<JSObject *>(cell);

            
            if (object->isBlock() ||
                object->isCall() ||
                object->isWith() ||
                object->isDeclEnv()) {
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
    HeapReverser::Node *node = &p->value;

    
    if (path != NULL) {
        jsval representation = representable(cell, node->kind);
        if (!JSVAL_IS_VOID(representation))
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

    char *path = static_cast<char *>(cx->malloc_(size));
    if (!path)
        return NULL;

    





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
ReferenceFinder::addReferrer(jsval referrer, Path *path)
{
    if (!context->compartment->wrap(context, &referrer))
        return NULL;

    char *pathName = path->computeName(context);
    if (!pathName)
        return false;
    AutoReleasePtr releasePathName(context, pathName);

    
    jsval v;
    if (!JS_GetProperty(context, result, pathName, &v))
        return false;
    if (JSVAL_IS_VOID(v)) {
        
        JSObject *array = JS_NewArrayObject(context, 1, &referrer);
        if (!array)
            return false;
        v = OBJECT_TO_JSVAL(array);
        return !!JS_SetProperty(context, result, pathName, &v);
    }

    
    JS_ASSERT(JSVAL_IS_OBJECT(v) && !JSVAL_IS_NULL(v));
    JSObject *array = JSVAL_TO_OBJECT(v);
    JS_ASSERT(JS_IsArrayObject(context, array));

    
    jsuint length;
    return JS_GetArrayLength(context, array, &length) &&
           JS_SetElement(context, array, length, &referrer);
}

JSObject *
ReferenceFinder::findReferences(JSObject *target)
{
    result = JS_NewObject(context, NULL, NULL, NULL);
    if (!result)
        return NULL;
    if (!visit(target, NULL))
        return NULL;

    return result;
}



























JSBool
FindReferences(JSContext *cx, uintN argc, jsval *vp)
{
    if (argc < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "findReferences", "0", "s");
        return false;
    }

    jsval target = JS_ARGV(cx, vp)[0];
    if (!JSVAL_IS_OBJECT(target) || JSVAL_IS_NULL(target)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_UNEXPECTED_TYPE,
                             "argument", "not an object");
        return false;
    }

    
    HeapReverser reverser(cx);
    if (!reverser.init() || !reverser.reverseHeap())
        return false;

    
    ReferenceFinder finder(cx, reverser);
    JSObject *references = finder.findReferences(JSVAL_TO_OBJECT(target));
    if (!references)
        return false;
    
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(references));
    return true;
}

#endif 
