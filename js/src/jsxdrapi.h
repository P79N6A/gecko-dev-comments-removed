







































#ifndef jsxdrapi_h___
#define jsxdrapi_h___





















#include "jspubtd.h"
#include "jsprvtd.h"

JS_BEGIN_EXTERN_C



#if defined IS_LITTLE_ENDIAN
#define JSXDR_SWAB32(x) x
#define JSXDR_SWAB16(x) x
#elif defined IS_BIG_ENDIAN
#define JSXDR_SWAB32(x) (((uint32)(x) >> 24) |                                \
                         (((uint32)(x) >> 8) & 0xff00) |                      \
                         (((uint32)(x) << 8) & 0xff0000) |                    \
                         ((uint32)(x) << 24))
#define JSXDR_SWAB16(x) (((uint16)(x) >> 8) | ((uint16)(x) << 8))
#else
#error "unknown byte order"
#endif

#define JSXDR_ALIGN     4

typedef enum JSXDRMode {
    JSXDR_ENCODE,
    JSXDR_DECODE,
    JSXDR_FREE
} JSXDRMode;

typedef enum JSXDRWhence {
    JSXDR_SEEK_SET,
    JSXDR_SEEK_CUR,
    JSXDR_SEEK_END
} JSXDRWhence;

typedef struct JSXDROps {
    JSBool      (*get32)(JSXDRState *, uint32 *);
    JSBool      (*set32)(JSXDRState *, uint32 *);
    JSBool      (*getbytes)(JSXDRState *, char *, uint32);
    JSBool      (*setbytes)(JSXDRState *, char *, uint32);
    void *      (*raw)(JSXDRState *, uint32);
    JSBool      (*seek)(JSXDRState *, int32, JSXDRWhence);
    uint32      (*tell)(JSXDRState *);
    void        (*finalize)(JSXDRState *);
} JSXDROps;

struct JSXDRState {
    JSXDRMode   mode;
    JSXDROps    *ops;
    JSContext   *cx;
    JSClass     **registry;
    uintN       numclasses;
    uintN       maxclasses;
    void        *reghash;
    void        *userdata;
    JSScript    *script;
};

extern JS_PUBLIC_API(void)
JS_XDRInitBase(JSXDRState *xdr, JSXDRMode mode, JSContext *cx);

extern JS_PUBLIC_API(JSXDRState *)
JS_XDRNewMem(JSContext *cx, JSXDRMode mode);

extern JS_PUBLIC_API(void *)
JS_XDRMemGetData(JSXDRState *xdr, uint32 *lp);

extern JS_PUBLIC_API(void)
JS_XDRMemSetData(JSXDRState *xdr, void *data, uint32 len);

extern JS_PUBLIC_API(uint32)
JS_XDRMemDataLeft(JSXDRState *xdr);

extern JS_PUBLIC_API(void)
JS_XDRMemResetData(JSXDRState *xdr);

extern JS_PUBLIC_API(void)
JS_XDRDestroy(JSXDRState *xdr);

extern JS_PUBLIC_API(JSBool)
JS_XDRUint8(JSXDRState *xdr, uint8 *b);

extern JS_PUBLIC_API(JSBool)
JS_XDRUint16(JSXDRState *xdr, uint16 *s);

extern JS_PUBLIC_API(JSBool)
JS_XDRUint32(JSXDRState *xdr, uint32 *lp);

extern JS_PUBLIC_API(JSBool)
JS_XDRBytes(JSXDRState *xdr, char *bytes, uint32 len);

extern JS_PUBLIC_API(JSBool)
JS_XDRCString(JSXDRState *xdr, char **sp);

extern JS_PUBLIC_API(JSBool)
JS_XDRCStringOrNull(JSXDRState *xdr, char **sp);

extern JS_PUBLIC_API(JSBool)
JS_XDRString(JSXDRState *xdr, JSString **strp);

extern JS_PUBLIC_API(JSBool)
JS_XDRStringOrNull(JSXDRState *xdr, JSString **strp);

extern JS_PUBLIC_API(JSBool)
JS_XDRDouble(JSXDRState *xdr, jsdouble **dp);

extern JS_PUBLIC_API(JSBool)
JS_XDRValue(JSXDRState *xdr, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_XDRScript(JSXDRState *xdr, JSScript **scriptp);

extern JS_PUBLIC_API(JSBool)
JS_XDRRegisterClass(JSXDRState *xdr, JSClass *clasp, uint32 *lp);

extern JS_PUBLIC_API(uint32)
JS_XDRFindClassIdByName(JSXDRState *xdr, const char *name);

extern JS_PUBLIC_API(JSClass *)
JS_XDRFindClassById(JSXDRState *xdr, uint32 id);




#define JSXDR_MAGIC_SCRIPT_1        0xdead0001
#define JSXDR_MAGIC_SCRIPT_2        0xdead0002
#define JSXDR_MAGIC_SCRIPT_3        0xdead0003
#define JSXDR_MAGIC_SCRIPT_4        0xdead0004
#define JSXDR_MAGIC_SCRIPT_5        0xdead0005
#define JSXDR_MAGIC_SCRIPT_6        0xdead0006
#define JSXDR_MAGIC_SCRIPT_7        0xdead0007
#define JSXDR_MAGIC_SCRIPT_8        0xdead0008
#define JSXDR_MAGIC_SCRIPT_9        0xdead0009
#define JSXDR_MAGIC_SCRIPT_CURRENT  JSXDR_MAGIC_SCRIPT_9










#define JSXDR_BYTECODE_VERSION      (0xb973c0de - 53)




extern JSBool
js_XDRAtom(JSXDRState *xdr, JSAtom **atomp);

extern JSBool
js_XDRStringAtom(JSXDRState *xdr, JSAtom **atomp);

JS_END_EXTERN_C

#endif 
