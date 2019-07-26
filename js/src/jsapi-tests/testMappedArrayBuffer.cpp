



#ifdef XP_UNIX
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "jsfriendapi.h"
#include "js/StructuredClone.h"
#include "jsapi-tests/tests.h"
#include "vm/ArrayBufferObject.h"

const char test_data[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const char test_filename[] = "temp-bug945152_MappedArrayBuffer";

BEGIN_TEST(testMappedArrayBuffer_bug945152)
{
    TempFile test_file;
    FILE *test_stream = test_file.open(test_filename);
    CHECK(fputs(test_data, test_stream) != EOF);
    test_file.close();

    
    CHECK(TestCreateObject(0, 12));

    
    CHECK(TestCreateObject(8, 12));

    
    CHECK(CreateNewObject(11, 12) == nullptr);

    
    CHECK(CreateNewObject(8, sizeof(test_data) - 7) == nullptr);

    
    CHECK(TestReleaseContents());

    
    CHECK(TestNeuterObject());

    
    CHECK(TestCloneObject());

    
    CHECK(TestStealContents());

    
    CHECK(TestTransferObject());

    test_file.remove();

    return true;
}

JSObject *CreateNewObject(const int offset, const int length)
{
    int fd = open(test_filename, O_RDONLY);
    void *ptr = JS_CreateMappedArrayBufferContents(fd, offset, length);
    close(fd);
    if (!ptr)
        return nullptr;
    JSObject *obj = JS_NewMappedArrayBufferWithContents(cx, length, ptr);
    if (!obj) {
        JS_ReleaseMappedArrayBufferContents(ptr, length);
        return nullptr;
    }
    return obj;
}

bool VerifyObject(JS::HandleObject obj, const int offset, const int length, const bool mapped)
{
    CHECK(obj);
    CHECK(JS_IsArrayBufferObject(obj));
    CHECK_EQUAL(JS_GetArrayBufferByteLength(obj), length);
    if (mapped)
        CHECK(JS_IsMappedArrayBufferObject(obj));
    else
        CHECK(!JS_IsMappedArrayBufferObject(obj));
    const char *data = reinterpret_cast<const char *>(JS_GetArrayBufferData(obj));
    CHECK(data);
    CHECK(memcmp(data, test_data + offset, length) == 0);

    return true;
}

bool TestCreateObject(const int offset, const int length)
{
    JS::RootedObject obj(cx, CreateNewObject(offset, length));
    CHECK(VerifyObject(obj, offset, length, true));

    return true;
}

bool TestReleaseContents()
{
    int fd = open(test_filename, O_RDONLY);
    void *ptr = JS_CreateMappedArrayBufferContents(fd, 0, 12);
    close(fd);
    if (!ptr)
        return false;
    JS_ReleaseMappedArrayBufferContents(ptr, 12);

    return true;
}

bool TestNeuterObject()
{
    JS::RootedObject obj(cx, CreateNewObject(8, 12));
    CHECK(obj);
    JS_NeuterArrayBuffer(cx, obj, ChangeData);
    CHECK(isNeutered(obj));

    return true;
}

bool TestCloneObject()
{
    JS::RootedObject obj1(cx, CreateNewObject(8, 12));
    CHECK(obj1);
    JSAutoStructuredCloneBuffer cloned_buffer;
    JS::RootedValue v1(cx, OBJECT_TO_JSVAL(obj1));
    const JSStructuredCloneCallbacks *callbacks = js::GetContextStructuredCloneCallbacks(cx);
    CHECK(cloned_buffer.write(cx, v1, callbacks, nullptr));
    JS::RootedValue v2(cx);
    CHECK(cloned_buffer.read(cx, &v2, callbacks, nullptr));
    JS::RootedObject obj2(cx, v2.toObjectOrNull());
    CHECK(VerifyObject(obj2, 8, 12, false));

    return true;
}

bool TestStealContents()
{
    JS::RootedObject obj(cx, CreateNewObject(8, 12));
    CHECK(obj);
    void *contents = JS_StealArrayBufferContents(cx, obj);
    CHECK(contents);
    CHECK(memcmp(contents, test_data + 8, 12) == 0);
    CHECK(isNeutered(obj));

    return true;
}

bool TestTransferObject()
{
    JS::RootedObject obj1(cx, CreateNewObject(8, 12));
    CHECK(obj1);
    JS::RootedValue v1(cx, OBJECT_TO_JSVAL(obj1));

    
    JS::AutoValueVector argv(cx);
    argv.append(v1);
    JS::RootedObject obj(cx, JS_NewArrayObject(cx, JS::HandleValueArray::subarray(argv, 0, 1)));
    CHECK(obj);
    JS::RootedValue transferable(cx, OBJECT_TO_JSVAL(obj));

    JSAutoStructuredCloneBuffer cloned_buffer;
    const JSStructuredCloneCallbacks *callbacks = js::GetContextStructuredCloneCallbacks(cx);
    CHECK(cloned_buffer.write(cx, v1, transferable, callbacks, nullptr));
    JS::RootedValue v2(cx);
    CHECK(cloned_buffer.read(cx, &v2, callbacks, nullptr));
    JS::RootedObject obj2(cx, v2.toObjectOrNull());
    CHECK(VerifyObject(obj2, 8, 12, true));
    CHECK(isNeutered(obj1));

    return true;
}

bool isNeutered(JS::HandleObject obj)
{
    JS::RootedValue v(cx);
    return JS_GetProperty(cx, obj, "byteLength", &v) && v.toInt32() == 0;
}

static void GC(JSContext *cx)
{
    JS_GC(JS_GetRuntime(cx));
    
    JS_GC(JS_GetRuntime(cx));
}

END_TEST(testMappedArrayBuffer_bug945152)
#endif
