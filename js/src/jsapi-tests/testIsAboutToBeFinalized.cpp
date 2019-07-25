



#include "tests.h"
#include "jsstr.h"

static JSGCCallback oldGCCallback;

static void **checkPointers;
static jsuint checkPointersLength;
static size_t checkPointersStaticStrings;

static JSBool
TestAboutToBeFinalizedCallback(JSContext *cx, JSGCStatus status)
{
    if (status == JSGC_MARK_END && checkPointers) {
        for (jsuint i = 0; i != checkPointersLength; ++i) {
            void *p = checkPointers[i];
            JS_ASSERT(p);
            if (JS_IsAboutToBeFinalized(cx, p))
                checkPointers[i] = NULL;
        }
    }

    return !oldGCCallback || oldGCCallback(cx, status);
}





volatile void *ptrSink;

static JS_NEVER_INLINE void
NativeFrameCleaner()
{
    char buffer[1 << 16];
    memset(buffer, 0, sizeof buffer);
    ptrSink = buffer; 
}

BEGIN_TEST(testIsAboutToBeFinalized_bug528645)
{
    



    createAndTestRooted();
    NativeFrameCleaner();

    JS_GC(cx);

    
    for (jsuint i = 0; i != checkPointersLength; ++i) {
        void *p = checkPointers[i];
        if (p) {
            CHECK(JSString::isStatic(p));
            CHECK(checkPointersStaticStrings != 0);
            --checkPointersStaticStrings;
        }
    }
    CHECK(checkPointersStaticStrings == 0);

    free(checkPointers);
    checkPointers = NULL;
    JS_SetGCCallback(cx, oldGCCallback);

    return true;
}

JS_NEVER_INLINE bool createAndTestRooted();

END_TEST(testIsAboutToBeFinalized_bug528645)

JS_NEVER_INLINE bool
cls_testIsAboutToBeFinalized_bug528645::createAndTestRooted()
{
    jsvalRoot root(cx);

    



    EVAL("var x = 1.1; "
         "[x + 0.1, ''+x, 'a', '42', 'something'.substring(1), "
         "{}, [], new Function('return 10;'), <xml/>];",
         root.addr());

    JSObject *array = JSVAL_TO_OBJECT(root.value());
    JS_ASSERT(JS_IsArrayObject(cx, array));

    JSBool ok = JS_GetArrayLength(cx, array, &checkPointersLength);
    CHECK(ok);

    checkPointers = (void **) malloc(sizeof(void *) * checkPointersLength);
    CHECK(checkPointers);

    checkPointersStaticStrings = 0;
    for (jsuint i = 0; i != checkPointersLength; ++i) {
        jsval v;
        ok = JS_GetElement(cx, array, i, &v);
        CHECK(ok);
        JS_ASSERT(JSVAL_IS_GCTHING(v));
        JS_ASSERT(!JSVAL_IS_NULL(v));
        checkPointers[i] = JSVAL_TO_GCTHING(v);
        if (JSString::isStatic(checkPointers[i]))
            ++checkPointersStaticStrings;
    }

    oldGCCallback = JS_SetGCCallback(cx, TestAboutToBeFinalizedCallback);
    JS_GC(cx);

    



    for (jsuint i = 0; i != checkPointersLength; ++i)
        CHECK(checkPointers[i]);
}

