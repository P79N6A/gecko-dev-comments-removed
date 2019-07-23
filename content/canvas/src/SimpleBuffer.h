





































#ifndef SIMPLEBUFFER_H_
#define SIMPLEBUFFER_H_

#include "nsICanvasRenderingContextWebGL.h"
#include "localgl.h"

#include "jsapi.h"

namespace mozilla {

class SimpleBuffer {
public:
    SimpleBuffer()
      : type(LOCAL_GL_FLOAT), data(nsnull), length(0), capacity(0), sizePerVertex(0)
    { }

    SimpleBuffer(PRUint32 typeParam,
                 PRUint32 sizeParam,
                 JSContext *ctx,
                 JSObject *arrayObj,
                 jsuint arrayLen)
      : type(LOCAL_GL_FLOAT), data(nsnull), length(0), capacity(0), sizePerVertex(0)
    {
        InitFromJSArray(typeParam, sizeParam, ctx, arrayObj, arrayLen);
    }

    PRBool InitFromJSArray(PRUint32 typeParam,
                           PRUint32 sizeParam,
                           JSContext *ctx,
                           JSObject *arrayObj,
                           jsuint arrayLen);

    ~SimpleBuffer() {
        Release();
    }

    inline PRBool Valid() {
        return data != nsnull;
    }

    inline PRUint32 ElementSize() {
        if (type == LOCAL_GL_FLOAT) return sizeof(float);
        if (type == LOCAL_GL_SHORT) return sizeof(short);
        if (type == LOCAL_GL_UNSIGNED_SHORT) return sizeof(unsigned short);
        if (type == LOCAL_GL_BYTE) return 1;
        if (type == LOCAL_GL_UNSIGNED_BYTE) return 1;
        if (type == LOCAL_GL_INT) return sizeof(int);
        if (type == LOCAL_GL_UNSIGNED_INT) return sizeof(unsigned int);
        if (type == LOCAL_GL_DOUBLE) return sizeof(double);
        return 1;
    }

    void Clear() {
        Release();
    }

    void Set(PRUint32 t, PRUint32 spv, PRUint32 count, void* vals) {
        Prepare(t, spv, count);

        if (count)
            memcpy(data, vals, count*ElementSize());
    }

    void Prepare(PRUint32 t, PRUint32 spv, PRUint32 count) {
        if (count == 0) {
            Release();
        } else {
            type = t;
            EnsureCapacity(PR_FALSE, count*ElementSize());
            length = count;
            sizePerVertex = spv;
        }
    }

    void Release() {
        if (data)
            free(data);
        length = 0;
        capacity = 0;
        data = nsnull;
    }

    void EnsureCapacity(PRBool preserve, PRUint32 cap) {
        if (capacity >= cap)
            return;

        void* newdata = malloc(cap);
        if (preserve && length)
            memcpy(newdata, data, length*ElementSize());
        free(data);
        data = newdata;
        capacity = cap;
    }

    PRUint32 type;
    void* data;
    PRUint32 length;        
    PRUint32 capacity;      
    PRUint32 sizePerVertex; 
};

}

#endif
