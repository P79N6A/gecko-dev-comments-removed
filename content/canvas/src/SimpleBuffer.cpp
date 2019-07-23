





































#include "SimpleBuffer.h"

using namespace mozilla;

#define FOO(_x,_y) JSBool _x (JSContext *cx, JSObject *obj, jsuint offset, jsuint count, _y *dest) { return 0; }

FOO(js_ArrayToJSUint8Buffer, JSUint8)
FOO(js_ArrayToJSUint16Buffer, JSUint16)
FOO(js_ArrayToJSUint32Buffer, JSUint32)
FOO(js_ArrayToJSInt8Buffer, JSInt8)
FOO(js_ArrayToJSInt16Buffer, JSInt16)
FOO(js_ArrayToJSInt32Buffer, JSInt32)
FOO(js_ArrayToJSDoubleBuffer, jsdouble)

PRBool
SimpleBuffer::InitFromJSArray(PRUint32 typeParam,
                              PRUint32 sizeParam,
                              JSContext *ctx,
                              JSObject *arrayObj,
                              jsuint arrayLen)
{
    if (typeParam == LOCAL_GL_SHORT) {
        Prepare(typeParam, sizeParam, arrayLen);
        short *ptr = (short*) data;

        if (!js_ArrayToJSInt16Buffer(ctx, arrayObj, 0, arrayLen, ptr)) {
            for (PRUint32 i = 0; i < arrayLen; i++) {
                jsval jv;
                int32 iv;
                ::JS_GetElement(ctx, arrayObj, i, &jv);
                ::JS_ValueToECMAInt32(ctx, jv, &iv);
                *ptr++ = (short) iv;
            }
        }
    } else if (typeParam == LOCAL_GL_FLOAT) {
        Prepare(typeParam, sizeParam, arrayLen);
        float *ptr = (float*) data;
        double *tmpd = new double[arrayLen];
        if (js_ArrayToJSDoubleBuffer(ctx, arrayObj, 0, arrayLen, tmpd)) {
            for (PRUint32 i = 0; i < arrayLen; i++)
                ptr[i] = (float) tmpd[i];
        } else {
            for (PRUint32 i = 0; i < arrayLen; i++) {
                jsval jv;
                jsdouble dv;
                ::JS_GetElement(ctx, arrayObj, i, &jv);
                ::JS_ValueToNumber(ctx, jv, &dv);
                *ptr++ = (float) dv;
            }
        }
        delete [] tmpd;
    } else if (typeParam == LOCAL_GL_UNSIGNED_BYTE) {
        Prepare(typeParam, sizeParam, arrayLen);
        unsigned char *ptr = (unsigned char*) data;
        if (!js_ArrayToJSUint8Buffer(ctx, arrayObj, 0, arrayLen, ptr)) {
            for (PRUint32 i = 0; i < arrayLen; i++) {
                jsval jv;
                uint32 iv;
                ::JS_GetElement(ctx, arrayObj, i, &jv);
                ::JS_ValueToECMAUint32(ctx, jv, &iv);
                *ptr++ = (unsigned char) iv;
            }
        }
    } else if (typeParam == LOCAL_GL_UNSIGNED_SHORT) {
        Prepare(typeParam, sizeParam, arrayLen);
        PRUint16 *ptr = (PRUint16*) data;
        if (!js_ArrayToJSUint16Buffer(ctx, arrayObj, 0, arrayLen, ptr)) {
            for (PRUint32 i = 0; i < arrayLen; i++) {
                jsval jv;
                uint32 iv;
                ::JS_GetElement(ctx, arrayObj, i, &jv);
                ::JS_ValueToECMAUint32(ctx, jv, &iv);
                *ptr++ = (unsigned short) iv;
            }
        }
    } else if (typeParam == LOCAL_GL_UNSIGNED_INT) {
        Prepare(typeParam, sizeParam, arrayLen);
        PRUint32 *ptr = (PRUint32*) data;
        if (!js_ArrayToJSUint32Buffer(ctx, arrayObj, 0, arrayLen, ptr)) {
            for (PRUint32 i = 0; i < arrayLen; i++) {
                jsval jv;
                uint32 iv;
                ::JS_GetElement(ctx, arrayObj, i, &jv);
                ::JS_ValueToECMAUint32(ctx, jv, &iv);
                *ptr++ = iv;
            }
        }
    } else if (typeParam == LOCAL_GL_INT) {
        Prepare(typeParam, sizeParam, arrayLen);
        PRInt32 *ptr = (PRInt32*) data;
        if (!js_ArrayToJSInt32Buffer(ctx, arrayObj, 0, arrayLen, ptr)) {
            for (PRUint32 i = 0; i < arrayLen; i++) {
                jsval jv;
                int32 iv;
                ::JS_GetElement(ctx, arrayObj, i, &jv);
                ::JS_ValueToECMAInt32(ctx, jv, &iv);
                *ptr++ = iv;
            }
        }
    } else {
        return PR_FALSE;
    }

    return PR_TRUE;
}

