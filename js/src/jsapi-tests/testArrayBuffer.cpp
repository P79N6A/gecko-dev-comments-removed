



#include "jsfriendapi.h"

#include "jsapi-tests/tests.h"

#define NUM_TEST_BUFFERS 2
#define MAGIC_VALUE_1 3
#define MAGIC_VALUE_2 17

BEGIN_TEST(testArrayBuffer_bug720949_steal)
{
    JS::RootedObject buf_len1(cx), buf_len200(cx);
    JS::RootedObject tarray_len1(cx), tarray_len200(cx);

    uint32_t sizes[NUM_TEST_BUFFERS] = { sizeof(uint32_t), 200 * sizeof(uint32_t) };
    JS::HandleObject testBuf[NUM_TEST_BUFFERS] = { buf_len1, buf_len200 };
    JS::HandleObject testArray[NUM_TEST_BUFFERS] = { tarray_len1, tarray_len200 };

    
    CHECK(buf_len1 = JS_NewArrayBuffer(cx, sizes[0]));
    CHECK(tarray_len1 = JS_NewInt32ArrayWithBuffer(cx, testBuf[0], 0, -1));

    jsval dummy = INT_TO_JSVAL(MAGIC_VALUE_1);
    JS_SetElement(cx, testArray[0], 0, &dummy);

    
    CHECK(buf_len200 = JS_NewArrayBuffer(cx, 200 * sizeof(uint32_t)));
    CHECK(tarray_len200 = JS_NewInt32ArrayWithBuffer(cx, testBuf[1], 0, -1));

    for (unsigned i = 0; i < NUM_TEST_BUFFERS; i++) {
        JS::HandleObject obj = testBuf[i];
        JS::HandleObject view = testArray[i];
        uint32_t size = sizes[i];
        JS::RootedValue v(cx);

        
        CHECK(JS_IsArrayBufferObject(obj));
        CHECK_EQUAL(JS_GetArrayBufferByteLength(obj), size);
        JS_GetProperty(cx, obj, "byteLength", &v);
        CHECK_SAME(v, INT_TO_JSVAL(size));
        JS_GetProperty(cx, view, "byteLength", &v);
        CHECK_SAME(v, INT_TO_JSVAL(size));

        
        uint8_t *data = JS_GetArrayBufferData(obj);
        CHECK(data != NULL);
        *reinterpret_cast<uint32_t*>(data) = MAGIC_VALUE_2;
        CHECK(JS_GetElement(cx, view, 0, v.address()));
        CHECK_SAME(v, INT_TO_JSVAL(MAGIC_VALUE_2));

        
        void *contents;
        CHECK(JS_StealArrayBufferContents(cx, obj, &contents, &data));
        CHECK(contents != NULL);
        CHECK(data != NULL);

        
        CHECK_EQUAL(JS_GetArrayBufferByteLength(obj), 0);
        CHECK(JS_GetProperty(cx, obj, "byteLength", &v));
        CHECK_SAME(v, INT_TO_JSVAL(0));
        CHECK(JS_GetProperty(cx, view, "byteLength", &v));
        CHECK_SAME(v, INT_TO_JSVAL(0));
        CHECK(JS_GetProperty(cx, view, "byteOffset", &v));
        CHECK_SAME(v, INT_TO_JSVAL(0));
        CHECK(JS_GetProperty(cx, view, "length", &v));
        CHECK_SAME(v, INT_TO_JSVAL(0));
        CHECK_EQUAL(JS_GetArrayBufferByteLength(obj), 0);
        v = JSVAL_VOID;
        JS_GetElement(cx, obj, 0, v.address());
        CHECK_SAME(v, JSVAL_VOID);

        
        JS::RootedObject dst(cx, JS_NewArrayBufferWithContents(cx, contents));
        CHECK(JS_IsArrayBufferObject(dst));
        data = JS_GetArrayBufferData(obj);

        JS::RootedObject dstview(cx, JS_NewInt32ArrayWithBuffer(cx, dst, 0, -1));
        CHECK(dstview != NULL);

        CHECK_EQUAL(JS_GetArrayBufferByteLength(dst), size);
        data = JS_GetArrayBufferData(dst);
        CHECK(data != NULL);
        CHECK_EQUAL(*reinterpret_cast<uint32_t*>(data), MAGIC_VALUE_2);
        CHECK(JS_GetElement(cx, dstview, 0, v.address()));
        CHECK_SAME(v, INT_TO_JSVAL(MAGIC_VALUE_2));
    }

    return true;
}
END_TEST(testArrayBuffer_bug720949_steal)

static void GC(JSContext *cx)
{
    JS_GC(JS_GetRuntime(cx));
    JS_GC(JS_GetRuntime(cx)); 
}


BEGIN_TEST(testArrayBuffer_bug720949_viewList)
{
    JS::RootedObject buffer(cx);

    
    buffer = JS_NewArrayBuffer(cx, 2000);
    buffer = NULL;
    GC(cx);

    
    {
        buffer = JS_NewArrayBuffer(cx, 2000);
        JS::RootedObject view(cx, JS_NewUint8ArrayWithBuffer(cx, buffer, 0, -1));
        void *contents;
        uint8_t *data;
        CHECK(JS_StealArrayBufferContents(cx, buffer, &contents, &data));
        CHECK(contents != NULL);
        CHECK(data != NULL);
        JS_free(NULL, contents);
        GC(cx);
        CHECK(isNeutered(view));
        CHECK(isNeutered(buffer));
        view = NULL;
        GC(cx);
        buffer = NULL;
        GC(cx);
    }

    
    {
        buffer = JS_NewArrayBuffer(cx, 2000);

        JS::RootedObject view1(cx, JS_NewUint8ArrayWithBuffer(cx, buffer, 0, -1));
        JS::RootedObject view2(cx, JS_NewUint8ArrayWithBuffer(cx, buffer, 1, 200));

        
        view2 = NULL;
        GC(cx);
        view2 = JS_NewUint8ArrayWithBuffer(cx, buffer, 1, 200);

        
        void *contents;
        uint8_t *data;
        CHECK(JS_StealArrayBufferContents(cx, buffer, &contents, &data));
        CHECK(contents != NULL);
        CHECK(data != NULL);
        JS_free(NULL, contents);

        CHECK(isNeutered(view1));
        CHECK(isNeutered(view2));
        CHECK(isNeutered(buffer));

        view1 = NULL;
        GC(cx);
        view2 = NULL;
        GC(cx);
        buffer = NULL;
        GC(cx);
    }

    return true;
}

bool isNeutered(JS::HandleObject obj) {
    JS::RootedValue v(cx);
    return JS_GetProperty(cx, obj, "byteLength", &v) && v.toInt32() == 0;
}

END_TEST(testArrayBuffer_bug720949_viewList)
