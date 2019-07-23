






































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
    public nsIWebGLArrayBuffer,
    public nsIJSNativeInitializer,
    public SimpleBuffer
{
public:

    WebGLArrayBuffer() { }
    WebGLArrayBuffer(PRUint32 length);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLARRAYBUFFER

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
    public nsIWebGLFloatArray,
    public nsIJSNativeInitializer
{
public:
    WebGLFloatArray() :
        mOffset(0), mLength(0) { }

    WebGLFloatArray(PRUint32 length);
    WebGLFloatArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLFloatArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLARRAY
    NS_DECL_NSIWEBGLFLOATARRAY
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
    public nsIWebGLByteArray,
    public nsIJSNativeInitializer
{
public:
    WebGLByteArray() :
        mOffset(0), mLength(0) { }

    WebGLByteArray(PRUint32 length);
    WebGLByteArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLByteArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLARRAY
    NS_DECL_NSIWEBGLBYTEARRAY
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
    public nsIWebGLUnsignedByteArray,
    public nsIJSNativeInitializer
{
public:
    WebGLUnsignedByteArray() :
        mOffset(0), mLength(0) { }

    WebGLUnsignedByteArray(PRUint32 length);
    WebGLUnsignedByteArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLUnsignedByteArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLARRAY
    NS_DECL_NSIWEBGLUNSIGNEDBYTEARRAY
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
    public nsIWebGLShortArray,
    public nsIJSNativeInitializer
{
public:
    WebGLShortArray() :
        mOffset(0), mLength(0) { }

    WebGLShortArray(PRUint32 length);
    WebGLShortArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLShortArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLARRAY
    NS_DECL_NSIWEBGLSHORTARRAY
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
    public nsIWebGLUnsignedShortArray,
    public nsIJSNativeInitializer
{
public:
    WebGLUnsignedShortArray() :
        mOffset(0), mLength(0) { }

    WebGLUnsignedShortArray(PRUint32 length);
    WebGLUnsignedShortArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLUnsignedShortArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLARRAY
    NS_DECL_NSIWEBGLUNSIGNEDSHORTARRAY
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
    public nsIWebGLIntArray,
    public nsIJSNativeInitializer
{
public:
    WebGLIntArray() :
        mOffset(0), mLength(0) { }

    WebGLIntArray(PRUint32 length);
    WebGLIntArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLIntArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLARRAY
    NS_DECL_NSIWEBGLINTARRAY
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
    public nsIWebGLUnsignedIntArray,
    public nsIJSNativeInitializer
{
public:
    WebGLUnsignedIntArray() :
        mOffset(0), mLength(0) { }

    WebGLUnsignedIntArray(PRUint32 length);
    WebGLUnsignedIntArray(WebGLArrayBuffer *buffer, PRUint32 offset, PRUint32 length);
    WebGLUnsignedIntArray(JSContext *cx, JSObject *arrayObj, jsuint arrayLen);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLARRAY
    NS_DECL_NSIWEBGLUNSIGNEDINTARRAY
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
