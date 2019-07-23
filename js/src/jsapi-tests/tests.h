







































#include "jsapi.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

class jsvalRoot
{
public:
    explicit jsvalRoot(JSContext *context, jsval value = JSVAL_NULL)
        : cx(context), v(value)
    {
        if (!JS_AddRoot(cx, &v)) {
            fprintf(stderr, "Out of memory in jsvalRoot constructor, aborting\n");
            abort();
        }
    }

    ~jsvalRoot() { JS_RemoveRoot(cx, &v); }

    operator jsval() const { return value(); }

    jsvalRoot & operator=(jsval value) {
        v = value;
        return *this;
    }

    jsval * addr() { return &v; }
    jsval value() const { return v; }

private:
    JSContext *cx;
    jsval v;
};

class JSAPITest
{
public:
    static JSAPITest *list;
    JSAPITest *next;

    JSRuntime *rt;
    JSContext *cx;
    JSObject *global;
    bool knownFail;
    std::string msgs;

    JSAPITest() : rt(NULL), cx(NULL), global(NULL), knownFail(false) {
        next = list;
        list = this;
    }

    virtual ~JSAPITest() { uninit(); }

    virtual bool init() {
        rt = createRuntime();
        if (!rt)
            return false;
        cx = createContext();
        if (!cx)
            return false;
        JS_BeginRequest(cx);
        global = createGlobal();
        return global != NULL;
    }

    virtual void uninit() {
        if (cx) {
            JS_EndRequest(cx);
            JS_DestroyContext(cx);
            cx = NULL;
        }
        if (rt) {
            JS_DestroyRuntime(rt);
            rt = NULL;
        }
    }

    virtual const char * name() = 0;
    virtual bool run() = 0;

#define EXEC(s) do { if (!exec(s, __FILE__, __LINE__)) return false; } while (false)

    bool exec(const char *bytes, const char *filename, int lineno) {
        jsvalRoot v(cx);
        return JS_EvaluateScript(cx, global, bytes, strlen(bytes), filename, lineno, v.addr()) ||
               fail(bytes, filename, lineno);
    }

#define EVAL(s, vp) do { if (!evaluate(s, __FILE__, __LINE__, vp)) return false; } while (false)

    bool evaluate(const char *bytes, const char *filename, int lineno, jsval *vp) {
        return JS_EvaluateScript(cx, global, bytes, strlen(bytes), filename, lineno, vp) ||
               fail(bytes, filename, lineno);
    }

    std::string toSource(jsval v) {
        JSString *str = JS_ValueToSource(cx, v);
        if (str)
            return std::string(JS_GetStringBytes(str));
        JS_ClearPendingException(cx);
        return std::string("<<error converting value to string>>");
    }

#define CHECK_SAME(actual, expected) \
    do { \
        if (!checkSame(actual, expected, #actual, #expected, __FILE__, __LINE__)) \
            return false; \
    } while (false)

    bool checkSame(jsval actual, jsval expected,
                   const char *actualExpr, const char *expectedExpr,
                   const char *filename, int lineno) {
        return JS_SameValue(cx, actual, expected) ||
               fail(std::string("CHECK_SAME failed: expected JS_SameValue(cx, ") +
                    actualExpr + ", " + expectedExpr + "), got !JS_SameValue(cx, " +
                    toSource(actual) + ", " + toSource(expected) + ")", filename, lineno);
    }

#define CHECK(expr) \
    do { \
        if (!(expr)) \
            return fail("CHECK failed: " #expr, __FILE__, __LINE__); \
    } while (false)

    bool fail(std::string msg = std::string(), const char *filename = "-", int lineno = 0) {
        if (JS_IsExceptionPending(cx)) {
            jsvalRoot v(cx);
            JS_GetPendingException(cx, v.addr());
            JS_ClearPendingException(cx);
            JSString *s = JS_ValueToString(cx, v);
            if (s)
                msg += JS_GetStringBytes(s);
        }
        fprintf(stderr, "%s:%d:%s\n", filename, lineno, msg.c_str());
        msgs += msg;
        return false;
    }

    std::string messages() const { return msgs; }

protected:
    virtual JSRuntime * createRuntime() {
        return JS_NewRuntime(8L * 1024 * 1024);
    }

    static void reportError(JSContext *cx, const char *message, JSErrorReport *report) {
        fprintf(stderr, "%s:%u:%s\n",
                report->filename ? report->filename : "<no filename>",
                (unsigned int) report->lineno,
                message);
    }

    virtual JSContext * createContext() {
        JSContext *cx = JS_NewContext(rt, 8192);
        if (!cx)
            return NULL;
        JS_SetOptions(cx, JSOPTION_VAROBJFIX | JSOPTION_JIT);
        JS_SetVersion(cx, JSVERSION_LATEST);
        JS_SetErrorReporter(cx, &reportError);
        return cx;
    }

    virtual JSClass * getGlobalClass() {
        static JSClass basicGlobalClass = {
            "global", JSCLASS_GLOBAL_FLAGS,
            JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
            JSCLASS_NO_OPTIONAL_MEMBERS
        };
        return &basicGlobalClass;
    }

    virtual JSObject * createGlobal() {
        
        JSObject *global = JS_NewObject(cx, getGlobalClass(), NULL, NULL);
        if (!global)
            return NULL;

        

        if (!JS_InitStandardClasses(cx, global))
            return NULL;
        return global;
    }
};

#define BEGIN_TEST(testname)                                            \
    class cls_##testname : public JSAPITest {                           \
    public:                                                             \
        virtual const char * name() { return #testname; }               \
        virtual bool run()

#define END_TEST(testname)                                              \
    };                                                                  \
    static cls_##testname cls_##testname##_instance;


