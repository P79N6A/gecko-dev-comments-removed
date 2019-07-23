






































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
#include "nsIXPCScriptable.h"

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

class WebGLArray
{
    
};

class WebGLFloatArray :
    public nsIXPCScriptable,
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
    NS_DECL_NSIXPCSCRIPTABLE

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, float value);
    PRBool JSValToIndex(JSContext *cx, jsval id, PRUint32 *retval);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLByteArray :
    public nsIXPCScriptable,
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
    NS_DECL_NSIXPCSCRIPTABLE

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, char value);
    PRBool JSValToIndex(JSContext *cx, jsval id, PRUint32 *retval);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLUnsignedByteArray :
    public nsIXPCScriptable,
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
    NS_DECL_NSIXPCSCRIPTABLE

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, unsigned char value);
    PRBool JSValToIndex(JSContext *cx, jsval id, PRUint32 *retval);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLShortArray :
    public nsIXPCScriptable,
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
    NS_DECL_NSIXPCSCRIPTABLE

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, short value);
    PRBool JSValToIndex(JSContext *cx, jsval id, PRUint32 *retval);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLUnsignedShortArray :
    public nsIXPCScriptable,
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
    NS_DECL_NSIXPCSCRIPTABLE

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, unsigned short value);
    PRBool JSValToIndex(JSContext *cx, jsval id, PRUint32 *retval);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLIntArray :
    public nsIXPCScriptable,
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
    NS_DECL_NSIXPCSCRIPTABLE

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, int value);
    PRBool JSValToIndex(JSContext *cx, jsval id, PRUint32 *retval);

protected:
    nsRefPtr<WebGLArrayBuffer> mBuffer;
    PRUint32 mOffset;
    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mElementSize;
    PRUint32 mCount;
};

class WebGLUnsignedIntArray :
    public nsIXPCScriptable,
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
    NS_DECL_NSIXPCSCRIPTABLE

    NS_IMETHOD Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          PRUint32 aArgc,
                          jsval* aArgv);

    void Set(PRUint32 index, unsigned int value);
    PRBool JSValToIndex(JSContext *cx, jsval id, PRUint32 *retval);

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
