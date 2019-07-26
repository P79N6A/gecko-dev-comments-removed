





#ifndef jsapi_tests_tests_h
#define jsapi_tests_tests_h

#include "mozilla/Util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsalloc.h"
#include "jscntxt.h"
#include "jsgc.h"

#include "js/Vector.h"


class JSAPITestString {
    js::Vector<char, 0, js::SystemAllocPolicy> chars;
  public:
    JSAPITestString() {}
    JSAPITestString(const char *s) { *this += s; }
    JSAPITestString(const JSAPITestString &s) { *this += s; }

    const char *begin() const { return chars.begin(); }
    const char *end() const { return chars.end(); }
    size_t length() const { return chars.length(); }

    JSAPITestString & operator +=(const char *s) {
        if (!chars.append(s, strlen(s)))
            abort();
        return *this;
    }

    JSAPITestString & operator +=(const JSAPITestString &s) {
        if (!chars.append(s.begin(), s.length()))
            abort();
        return *this;
    }
};

inline JSAPITestString operator+(JSAPITestString a, const char *b) { return a += b; }
inline JSAPITestString operator+(JSAPITestString a, const JSAPITestString &b) { return a += b; }

class JSAPITest
{
  public:
    static JSAPITest *list;
    JSAPITest *next;

    JSRuntime *rt;
    JSContext *cx;
    JSObject *global;
    bool knownFail;
    JSAPITestString msgs;
    JSCompartment *oldCompartment;

    JSAPITest() : rt(NULL), cx(NULL), global(NULL), knownFail(false), oldCompartment(NULL) {
        next = list;
        list = this;
    }

    virtual ~JSAPITest() { uninit(); }

    virtual bool init();

    virtual void uninit() {
        if (oldCompartment) {
            JS_LeaveCompartment(cx, oldCompartment);
            oldCompartment = NULL;
        }
        if (cx) {
            JS_RemoveObjectRoot(cx, &global);
            JS_LeaveCompartment(cx, NULL);
            JS_EndRequest(cx);
            JS_DestroyContext(cx);
            cx = NULL;
        }
        if (rt) {
            destroyRuntime();
            rt = NULL;
        }
    }

    virtual const char * name() = 0;
    virtual bool run(JS::HandleObject global) = 0;

#define EXEC(s) do { if (!exec(s, __FILE__, __LINE__)) return false; } while (false)

    bool exec(const char *bytes, const char *filename, int lineno);

#define EVAL(s, vp) do { if (!evaluate(s, __FILE__, __LINE__, vp)) return false; } while (false)

    bool evaluate(const char *bytes, const char *filename, int lineno, jsval *vp);

    JSAPITestString jsvalToSource(jsval v) {
        JSString *str = JS_ValueToSource(cx, v);
        if (str) {
            JSAutoByteString bytes(cx, str);
            if (!!bytes)
                return JSAPITestString(bytes.ptr());
        }
        JS_ClearPendingException(cx);
        return JSAPITestString("<<error converting value to string>>");
    }

    JSAPITestString toSource(long v) {
        char buf[40];
        sprintf(buf, "%ld", v);
        return JSAPITestString(buf);
    }

    JSAPITestString toSource(unsigned long v) {
        char buf[40];
        sprintf(buf, "%lu", v);
        return JSAPITestString(buf);
    }

    JSAPITestString toSource(long long v) {
        char buf[40];
        sprintf(buf, "%lld", v);
        return JSAPITestString(buf);
    }

    JSAPITestString toSource(unsigned long long v) {
        char buf[40];
        sprintf(buf, "%llu", v);
        return JSAPITestString(buf);
    }

    JSAPITestString toSource(unsigned int v) {
        return toSource((unsigned long)v);
    }

    JSAPITestString toSource(int v) {
        return toSource((long)v);
    }

    JSAPITestString toSource(bool v) {
        return JSAPITestString(v ? "true" : "false");
    }

    JSAPITestString toSource(JSAtom *v) {
        return jsvalToSource(STRING_TO_JSVAL((JSString*)v));
    }

    JSAPITestString toSource(JSVersion v) {
        return JSAPITestString(JS_VersionToString(v));
    }

    template<typename T>
    bool checkEqual(const T &actual, const T &expected,
                    const char *actualExpr, const char *expectedExpr,
                    const char *filename, int lineno) {
        return (actual == expected) ||
            fail(JSAPITestString("CHECK_EQUAL failed: expected (") +
                 expectedExpr + ") = " + toSource(expected) +
                 ", got (" + actualExpr + ") = " + toSource(actual), filename, lineno);
    }

    
    
    
    
    
    template<typename T, typename U>
    bool checkEqual(const T &actual, const U &expected,
                   const char *actualExpr, const char *expectedExpr,
                   const char *filename, int lineno) {
        return checkEqual(U(actual), expected, actualExpr, expectedExpr, filename, lineno);
    }

#define CHECK_EQUAL(actual, expected) \
    do { \
        if (!checkEqual(actual, expected, #actual, #expected, __FILE__, __LINE__)) \
            return false; \
    } while (false)

    bool checkSame(jsval actualArg, jsval expectedArg,
                   const char *actualExpr, const char *expectedExpr,
                   const char *filename, int lineno) {
        bool same;
        JS::RootedValue actual(cx, actualArg), expected(cx, expectedArg);
        return (JS_SameValue(cx, actual, expected, &same) && same) ||
               fail(JSAPITestString("CHECK_SAME failed: expected JS_SameValue(cx, ") +
                    actualExpr + ", " + expectedExpr + "), got !JS_SameValue(cx, " +
                    jsvalToSource(actual) + ", " + jsvalToSource(expected) + ")", filename, lineno);
    }

#define CHECK_SAME(actual, expected) \
    do { \
        if (!checkSame(actual, expected, #actual, #expected, __FILE__, __LINE__)) \
            return false; \
    } while (false)

#define CHECK(expr) \
    do { \
        if (!(expr)) \
            return fail("CHECK failed: " #expr, __FILE__, __LINE__); \
    } while (false)

    bool fail(JSAPITestString msg = JSAPITestString(), const char *filename = "-", int lineno = 0) {
        if (JS_IsExceptionPending(cx)) {
            js::gc::AutoSuppressGC gcoff(cx);
            JS::RootedValue v(cx);
            JS_GetPendingException(cx, v.address());
            JS_ClearPendingException(cx);
            JSString *s = JS_ValueToString(cx, v);
            if (s) {
                JSAutoByteString bytes(cx, s);
                if (!!bytes)
                    msg += bytes.ptr();
            }
        }
        fprintf(stderr, "%s:%d:%.*s\n", filename, lineno, (int) msg.length(), msg.begin());
        msgs += msg;
        return false;
    }

    JSAPITestString messages() const { return msgs; }

    static JSClass * basicGlobalClass() {
        static JSClass c = {
            "global", JSCLASS_GLOBAL_FLAGS,
            JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
        };
        return &c;
    }

  protected:
    static bool
    print(JSContext *cx, unsigned argc, jsval *vp)
    {
        jsval *argv = JS_ARGV(cx, vp);
        for (unsigned i = 0; i < argc; i++) {
            JSString *str = JS_ValueToString(cx, argv[i]);
            if (!str)
                return false;
            char *bytes = JS_EncodeString(cx, str);
            if (!bytes)
                return false;
            printf("%s%s", i ? " " : "", bytes);
            JS_free(cx, bytes);
        }

        putchar('\n');
        fflush(stdout);
        JS_SET_RVAL(cx, vp, JSVAL_VOID);
        return true;
    }

    bool definePrint();

    static void setNativeStackQuota(JSRuntime *rt)
    {
        const size_t MAX_STACK_SIZE =

#if (defined(DEBUG) && defined(__SUNPRO_CC))  || defined(JS_CPU_SPARC)
            



            5000000
#else
            500000
#endif
        ;

        JS_SetNativeStackQuota(rt, MAX_STACK_SIZE);
    }

    virtual JSRuntime * createRuntime() {
        JSRuntime *rt = JS_NewRuntime(8L * 1024 * 1024, JS_USE_HELPER_THREADS);
        if (!rt)
            return NULL;
        setNativeStackQuota(rt);
        return rt;
    }

    virtual void destroyRuntime() {
        JS_ASSERT(!cx);
        JS_ASSERT(rt);
        JS_DestroyRuntime(rt);
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
        JS_SetOptions(cx, JSOPTION_VAROBJFIX);
        JS_SetErrorReporter(cx, &reportError);
        return cx;
    }

    virtual JSClass * getGlobalClass() {
        return basicGlobalClass();
    }

    virtual JSObject * createGlobal(JSPrincipals *principals = NULL);
};

#define BEGIN_TEST(testname)                                            \
    class cls_##testname : public JSAPITest {                           \
      public:                                                           \
        virtual const char * name() { return #testname; }               \
        virtual bool run(JS::HandleObject global)

#define END_TEST(testname)                                              \
    };                                                                  \
    static cls_##testname cls_##testname##_instance;









#define BEGIN_FIXTURE_TEST(fixture, testname)                           \
    class cls_##testname : public fixture {                             \
      public:                                                           \
        virtual const char * name() { return #testname; }               \
        virtual bool run(JS::HandleObject global)

#define END_FIXTURE_TEST(fixture, testname)                             \
    };                                                                  \
    static cls_##testname cls_##testname##_instance;








class TempFile {
    const char *name;
    FILE *stream;

  public:
    TempFile() : name(), stream() { }
    ~TempFile() {
        if (stream)
            close();
        if (name)
            remove();
    }

    





    FILE *open(const char *fileName)
    {
        stream = fopen(fileName, "wb+");
        if (!stream) {
            fprintf(stderr, "error opening temporary file '%s': %s\n",
                    fileName, strerror(errno));
            exit(1);
        }
        name = fileName;
        return stream;
    }

    
    void close() {
        if (fclose(stream) == EOF) {
            fprintf(stderr, "error closing temporary file '%s': %s\n",
                    name, strerror(errno));
            exit(1);
        }
        stream = NULL;
    }

    
    void remove() {
        if (::remove(name) != 0) {
            fprintf(stderr, "error deleting temporary file '%s': %s\n",
                    name, strerror(errno));
            exit(1);
        }
        name = NULL;
    }
};

#endif 
