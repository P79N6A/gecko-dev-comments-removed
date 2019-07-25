







































#ifndef jsxdrapi_h___
#define jsxdrapi_h___





















#include "jspubtd.h"
#include "jsprvtd.h"

JS_BEGIN_EXTERN_C



#if defined IS_LITTLE_ENDIAN
#define JSXDR_SWAB32(x) x
#define JSXDR_SWAB16(x) x
#elif defined IS_BIG_ENDIAN
#define JSXDR_SWAB32(x) (((uint32_t)(x) >> 24) |                              \
                         (((uint32_t)(x) >> 8) & 0xff00) |                    \
                         (((uint32_t)(x) << 8) & 0xff0000) |                  \
                         ((uint32_t)(x) << 24))
#define JSXDR_SWAB16(x) (((uint16_t)(x) >> 8) | ((uint16_t)(x) << 8))
#else
#error "unknown byte order"
#endif

#define JSXDR_ALIGN     4

typedef enum JSXDRMode {
    JSXDR_ENCODE,
    JSXDR_DECODE
} JSXDRMode;

typedef enum JSXDRWhence {
    JSXDR_SEEK_SET,
    JSXDR_SEEK_CUR,
    JSXDR_SEEK_END
} JSXDRWhence;

typedef struct JSXDROps {
    JSBool      (*get32)(JSXDRState *, uint32_t *);
    JSBool      (*set32)(JSXDRState *, uint32_t *);
    JSBool      (*getbytes)(JSXDRState *, char *, uint32_t);
    JSBool      (*setbytes)(JSXDRState *, char *, uint32_t);
    void *      (*raw)(JSXDRState *, uint32_t);
    JSBool      (*seek)(JSXDRState *, int32_t, JSXDRWhence);
    uint32_t    (*tell)(JSXDRState *);
    void        (*finalize)(JSXDRState *);
} JSXDROps;

struct JSXDRState;

namespace js {

class XDRScriptState {
public:
    XDRScriptState(JSXDRState *x);
    ~XDRScriptState();

    JSXDRState      *xdr;
    const char      *filename;
    bool             filenameSaved;
};

} 

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
    js::XDRScriptState *state;
};

extern JS_PUBLIC_API(void)
JS_XDRInitBase(JSXDRState *xdr, JSXDRMode mode, JSContext *cx);

extern JS_PUBLIC_API(JSXDRState *)
JS_XDRNewMem(JSContext *cx, JSXDRMode mode);

extern JS_PUBLIC_API(void *)
JS_XDRMemGetData(JSXDRState *xdr, uint32_t *lp);

extern JS_PUBLIC_API(void)
JS_XDRMemSetData(JSXDRState *xdr, void *data, uint32_t len);

extern JS_PUBLIC_API(uint32_t)
JS_XDRMemDataLeft(JSXDRState *xdr);

extern JS_PUBLIC_API(void)
JS_XDRMemResetData(JSXDRState *xdr);

extern JS_PUBLIC_API(void)
JS_XDRDestroy(JSXDRState *xdr);

extern JS_PUBLIC_API(JSBool)
JS_XDRUint8(JSXDRState *xdr, uint8_t *b);

extern JS_PUBLIC_API(JSBool)
JS_XDRUint16(JSXDRState *xdr, uint16_t *s);

extern JS_PUBLIC_API(JSBool)
JS_XDRUint32(JSXDRState *xdr, uint32_t *lp);

extern JS_PUBLIC_API(JSBool)
JS_XDRBytes(JSXDRState *xdr, char *bytes, uint32_t len);

extern JS_PUBLIC_API(JSBool)
JS_XDRCString(JSXDRState *xdr, char **sp);

extern JS_PUBLIC_API(JSBool)
JS_XDRCStringOrNull(JSXDRState *xdr, char **sp);

extern JS_PUBLIC_API(JSBool)
JS_XDRString(JSXDRState *xdr, JSString **strp);

extern JS_PUBLIC_API(JSBool)
JS_XDRStringOrNull(JSXDRState *xdr, JSString **strp);

extern JS_PUBLIC_API(JSBool)
JS_XDRDouble(JSXDRState *xdr, jsdouble *dp);

extern JS_PUBLIC_API(JSBool)
JS_XDRValue(JSXDRState *xdr, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_XDRFunctionObject(JSXDRState *xdr, JSObject **objp);

extern JS_PUBLIC_API(JSBool)
JS_XDRScript(JSXDRState *xdr, JSScript **scriptp);

extern JS_PUBLIC_API(JSBool)
JS_XDRRegisterClass(JSXDRState *xdr, JSClass *clasp, uint32_t *lp);

extern JS_PUBLIC_API(uint32_t)
JS_XDRFindClassIdByName(JSXDRState *xdr, const char *name);

extern JS_PUBLIC_API(JSClass *)
JS_XDRFindClassById(JSXDRState *xdr, uint32_t id);




#define JSXDR_MAGIC_SCRIPT_1        0xdead0001
#define JSXDR_MAGIC_SCRIPT_2        0xdead0002
#define JSXDR_MAGIC_SCRIPT_3        0xdead0003
#define JSXDR_MAGIC_SCRIPT_4        0xdead0004
#define JSXDR_MAGIC_SCRIPT_5        0xdead0005
#define JSXDR_MAGIC_SCRIPT_6        0xdead0006
#define JSXDR_MAGIC_SCRIPT_7        0xdead0007
#define JSXDR_MAGIC_SCRIPT_8        0xdead0008
#define JSXDR_MAGIC_SCRIPT_9        0xdead0009
#define JSXDR_MAGIC_SCRIPT_10       0xdead000a
#define JSXDR_MAGIC_SCRIPT_11       0xdead000b
#define JSXDR_MAGIC_SCRIPT_12       0xdead000c
#define JSXDR_MAGIC_SCRIPT_CURRENT  JSXDR_MAGIC_SCRIPT_12










#define JSXDR_BYTECODE_VERSION      (0xb973c0de - 100)




extern JSBool
js_XDRAtom(JSXDRState *xdr, JSAtom **atomp);

JS_END_EXTERN_C

#endif 
