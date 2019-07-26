





#include "xpcprivate.h"

#ifdef XP_WIN
#include <windows.h>
#endif

static void DebugDump(const char* fmt, ...)
{
  char buffer[2048];
  va_list ap;
  va_start(ap, fmt);
#ifdef XPWIN
  _vsnprintf(buffer, sizeof(buffer), fmt, ap);
#else
  vsnprintf(buffer, sizeof(buffer), fmt, ap);
#endif
  buffer[sizeof(buffer)-1] = '\0';
  va_end(ap);
#ifdef XP_WIN
  if (IsDebuggerPresent()) {
    OutputDebugStringA(buffer);
  }
#endif
  printf("%s", buffer);
}

JSBool
xpc_DumpJSStack(JSContext* cx, JSBool showArgs, JSBool showLocals, JSBool showThisProps)
{
    if (char* buf = xpc_PrintJSStack(cx, showArgs, showLocals, showThisProps)) {
        DebugDump("%s\n", buf);
        JS_smprintf_free(buf);
    }
    return true;
}

char*
xpc_PrintJSStack(JSContext* cx, JSBool showArgs, JSBool showLocals,
                 JSBool showThisProps)
{
    char* buf;
    JSExceptionState *state = JS_SaveExceptionState(cx);
    if (!state)
        DebugDump("%s", "Call to a debug function modifying state!\n");

    JS_ClearPendingException(cx);

    buf = JS::FormatStackDump(cx, nullptr, showArgs, showLocals, showThisProps);
    if (!buf)
        DebugDump("%s", "Failed to format JavaScript stack for dump\n");

    JS_RestoreExceptionState(cx, state);
    return buf;
}



static void
xpcDumpEvalErrorReporter(JSContext *cx, const char *message,
                         JSErrorReport *report)
{
    DebugDump("Error: %s\n", message);
}

JSBool
xpc_DumpEvalInJSStackFrame(JSContext* cx, uint32_t frameno, const char* text)
{
    JSStackFrame* fp;
    JSStackFrame* iter = nullptr;
    uint32_t num = 0;

    if (!cx || !text) {
        DebugDump("%s", "invalid params passed to xpc_DumpEvalInJSStackFrame!\n");
        return false;
    }

    DebugDump("js[%d]> %s\n", frameno, text);

    while (nullptr != (fp = JS_BrokenFrameIterator(cx, &iter))) {
        if (num == frameno)
            break;
        num++;
    }

    if (!fp) {
        DebugDump("%s", "invalid frame number!\n");
        return false;
    }

    JSAutoRequest ar(cx);

    JSExceptionState* exceptionState = JS_SaveExceptionState(cx);
    JSErrorReporter older = JS_SetErrorReporter(cx, xpcDumpEvalErrorReporter);

    jsval rval;
    JSString* str;
    JSAutoByteString bytes;
    if (JS_EvaluateInStackFrame(cx, fp, text, strlen(text), "eval", 1, &rval) &&
        nullptr != (str = JS_ValueToString(cx, rval)) &&
        bytes.encode(cx, str)) {
        DebugDump("%s\n", bytes.ptr());
    } else
        DebugDump("%s", "eval failed!\n");
    JS_SetErrorReporter(cx, older);
    JS_RestoreExceptionState(cx, exceptionState);
    return true;
}



JSTrapStatus
xpc_DebuggerKeywordHandler(JSContext *cx, JSScript *script, jsbytecode *pc,
                           jsval *rval, void *closure)
{
    static const char line[] =
    "------------------------------------------------------------------------\n";
    DebugDump("%s", line);
    DebugDump("%s", "Hit JavaScript \"debugger\" keyword. JS call stack...\n");
    xpc_DumpJSStack(cx, true, true, false);
    DebugDump("%s", line);
    return JSTRAP_CONTINUE;
}

JSBool xpc_InstallJSDebuggerKeywordHandler(JSRuntime* rt)
{
    return JS_SetDebuggerHandler(rt, xpc_DebuggerKeywordHandler, nullptr);
}









class ObjectPile
{
public:
    enum result {primary, seen, overflow};

    result Visit(JSObject* obj)
    {
        if (member_count == max_count)
            return overflow;
        for (int i = 0; i < member_count; i++)
            if (array[i] == obj)
                return seen;
        array[member_count++] = obj;
        return primary;
    }

    ObjectPile() : member_count(0){}

private:
    enum {max_count = 50};
    JSObject* array[max_count];
    int member_count;
};


static const int tab_width = 2;
#define INDENT(_d) (_d)*tab_width, " "

static void PrintObjectBasics(JSObject* obj)
{
    if (JS_IsNative(obj))
        DebugDump("%p 'native' <%s>",
                  (void *)obj, js::GetObjectClass(obj)->name);
    else
        DebugDump("%p 'host'", (void *)obj);
}

static void PrintObject(JSObject* obj, int depth, ObjectPile* pile)
{
    PrintObjectBasics(obj);

    switch (pile->Visit(obj)) {
    case ObjectPile::primary:
        DebugDump("%s", "\n");
        break;
    case ObjectPile::seen:
        DebugDump("%s", " (SEE ABOVE)\n");
        return;
    case ObjectPile::overflow:
        DebugDump("%s", " (TOO MANY OBJECTS)\n");
        return;
    }

    if (!JS_IsNative(obj))
        return;

    JSObject* parent = js::GetObjectParent(obj);
    JSObject* proto  = js::GetObjectProto(obj);

    DebugDump("%*sparent: ", INDENT(depth+1));
    if (parent)
        PrintObject(parent, depth+1, pile);
    else
        DebugDump("%s", "null\n");
    DebugDump("%*sproto: ", INDENT(depth+1));
    if (proto)
        PrintObject(proto, depth+1, pile);
    else
        DebugDump("%s", "null\n");
}

JSBool
xpc_DumpJSObject(JSObject* obj)
{
    ObjectPile pile;

    DebugDump("%s", "Debugging reminders...\n");
    DebugDump("%s", "  class:  (JSClass*)(obj->fslots[2]-1)\n");
    DebugDump("%s", "  parent: (JSObject*)(obj->fslots[1])\n");
    DebugDump("%s", "  proto:  (JSObject*)(obj->fslots[0])\n");
    DebugDump("%s", "\n");

    if (obj)
        PrintObject(obj, 0, &pile);
    else
        DebugDump("%s", "xpc_DumpJSObject passed null!\n");

    return true;
}

#ifdef DEBUG
void
xpc_PrintAllReferencesTo(void *p)
{
    
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    JS_DumpHeap(rt->GetJSRuntime(), stdout, nullptr, JSTRACE_OBJECT, p, 0x7fffffff, nullptr);
}
#endif
