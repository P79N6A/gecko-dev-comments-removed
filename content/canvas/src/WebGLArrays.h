






































#ifndef WEBGLARRAYS_H_
#define WEBGLARRAYS_H_

#include <stdarg.h>

#include "nsTArray.h"
#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"

#include "nsICanvasRenderingContextWebGL.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsIJSNativeInitializer.h"

#include "SimpleBuffer.h"

namespace mozilla {






class WebGLArrayBuffer :
    public nsICanvasArrayBuffer,
    public nsIJSNativeInitializer,
    public SimpleBuffer
{
public:

    WebGLArrayBuffer() { }
    WebGLArrayBuffer(PRUint32 length);

    NS_DECL_ISUPPORTS
    NS_DECL_NSICANVASARRAYBUFFER

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);
};

class WebGLFloatArray :
    public nsICanvasFloatArray,
    public nsIJSNativeInitializer
{
public:
    WebGLFloatArray() :
        mOffset(0), mLength(0) { }

    WebGLFloatArray(PRUint32 length);
    WebGLFloatArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLFloatArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSICANVASARRAY
    NS_DECL_NSICANVASFLOATARRAY

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, float value);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLByteArray :
    public nsICanvasByteArray,
    public nsIJSNativeInitializer
{
public:
    WebGLByteArray() :
        mOffset(0), mLength(0) { }

    WebGLByteArray(PRUint32 length);
    WebGLByteArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLByteArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSICANVASARRAY
    NS_DECL_NSICANVASBYTEARRAY

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, char value);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLUnsignedByteArray :
    public nsICanvasUnsignedByteArray,
    public nsIJSNativeInitializer
{
public:
    WebGLUnsignedByteArray() :
        mOffset(0), mLength(0) { }

    WebGLUnsignedByteArray(PRUint32 length);
    WebGLUnsignedByteArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLUnsignedByteArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSICANVASARRAY
    NS_DECL_NSICANVASUNSIGNEDBYTEARRAY

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, unsigned char value);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLShortArray :
    public nsICanvasShortArray,
    public nsIJSNativeInitializer
{
public:
    WebGLShortArray() :
        mOffset(0), mLength(0) { }

    WebGLShortArray(PRUint32 length);
    WebGLShortArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLShortArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSICANVASARRAY
    NS_DECL_NSICANVASSHORTARRAY

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, short value);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLUnsignedShortArray :
    public nsICanvasUnsignedShortArray,
    public nsIJSNativeInitializer
{
public:
    WebGLUnsignedShortArray() :
        mOffset(0), mLength(0) { }

    WebGLUnsignedShortArray(PRUint32 length);
    WebGLUnsignedShortArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLUnsignedShortArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSICANVASARRAY
    NS_DECL_NSICANVASUNSIGNEDSHORTARRAY

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, unsigned short value);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLIntArray :
    public nsICanvasIntArray,
    public nsIJSNativeInitializer
{
public:
    WebGLIntArray() :
        mOffset(0), mLength(0) { }

    WebGLIntArray(PRUint32 length);
    WebGLIntArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLIntArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSICANVASARRAY
    NS_DECL_NSICANVASINTARRAY

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, int value);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLUnsignedIntArray :
    public nsICanvasUnsignedIntArray,
    public nsIJSNativeInitializer
{
public:
    WebGLUnsignedIntArray() :
        mOffset(0), mLength(0) { }

    WebGLUnsignedIntArray(PRUint32 length);
    WebGLUnsignedIntArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLUnsignedIntArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSICANVASARRAY
    NS_DECL_NSICANVASUNSIGNEDINTARRAY

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, unsigned int value);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

} 


#endif 
