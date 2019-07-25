





































#ifndef jsclone_h___
#define jsclone_h___

#include "jsapi.h"
#include "jscntxt.h"
#include "jshashtable.h"
#include "jsstdint.h"
#include "jsvector.h"
#include "jsvalue.h"

namespace js {

bool
WriteStructuredClone(JSContext *cx, const Value &v, uint64_t **bufp, size_t *nbytesp,
                     const JSStructuredCloneCallbacks *cb, void *cbClosure);

bool
ReadStructuredClone(JSContext *cx, const uint64_t *data, size_t nbytes, Value *vp,
                    const JSStructuredCloneCallbacks *cb, void *cbClosure);

struct SCOutput {
  public:
    explicit SCOutput(JSContext *cx);

    JSContext *context() const { return cx; }

    bool write(uint64_t u);
    bool writePair(uint32_t tag, uint32_t data);
    bool writeDouble(jsdouble d);
    bool writeBytes(const void *p, size_t nbytes);
    bool writeChars(const jschar *p, size_t nchars);

    template <class T>
    bool writeArray(const T *p, size_t nbytes);

    bool extractBuffer(uint64_t **datap, size_t *sizep);

  private:
    JSContext *cx;
    js::Vector<uint64_t> buf;
};

struct SCInput {
  public:
    SCInput(JSContext *cx, const uint64_t *data, size_t nbytes);

    JSContext *context() const { return cx; }

    bool read(uint64_t *p);
    bool readPair(uint32_t *tagp, uint32_t *datap);
    bool readDouble(jsdouble *p);
    bool readBytes(void *p, size_t nbytes);
    bool readChars(jschar *p, size_t nchars);

    template <class T>
    bool readArray(T *p, size_t nelems);

  private:
    bool eof();

    void staticAssertions() {
        JS_STATIC_ASSERT(sizeof(jschar) == 2);
        JS_STATIC_ASSERT(sizeof(uint32_t) == 4);
        JS_STATIC_ASSERT(sizeof(jsdouble) == 8);
    }

    JSContext *cx;
    const uint64_t *point;
    const uint64_t *end;
};

}

struct JSStructuredCloneReader {
  public:
    explicit JSStructuredCloneReader(js::SCInput &in, const JSStructuredCloneCallbacks *cb,
                                     void *cbClosure)
        : in(in), objs(in.context()), allObjs(in.context()),
          callbacks(cb), closure(cbClosure) { }

    js::SCInput &input() { return in; }
    bool read(js::Value *vp);

  private:
    JSContext *context() { return in.context(); }

    bool checkDouble(jsdouble d);
    JSString *readString(uint32_t nchars);
    bool readTypedArray(uint32_t tag, uint32_t nelems, js::Value *vp);
    bool readArrayBuffer(uint32_t nbytes, js::Value *vp);
    bool readId(jsid *idp);
    bool startRead(js::Value *vp);

    js::SCInput &in;

    
    js::AutoValueVector objs;

    
    js::AutoValueVector allObjs;

    
    const JSStructuredCloneCallbacks *callbacks;

    
    void *closure;
};

struct JSStructuredCloneWriter {
  public:
    explicit JSStructuredCloneWriter(js::SCOutput &out, const JSStructuredCloneCallbacks *cb,
                                     void *cbClosure)
        : out(out), objs(out.context()), counts(out.context()), ids(out.context()),
          memory(out.context()), callbacks(cb), closure(cbClosure) { }

    bool init() { return memory.init(); }

    bool write(const js::Value &v);

    js::SCOutput &output() { return out; }

  private:
    JSContext *context() { return out.context(); }

    bool writeString(uint32_t tag, JSString *str);
    bool writeId(jsid id);
    bool writeArrayBuffer(JSObject *obj);
    bool writeTypedArray(JSObject *obj);
    bool startObject(JSObject *obj);
    bool startWrite(const js::Value &v);

    inline void checkStack();

    js::SCOutput &out;

    
    js::AutoValueVector objs;

    
    
    js::Vector<size_t> counts;

    
    js::AutoIdVector ids;

    
    
    
    typedef js::HashMap<JSObject *, uint32> CloneMemory;
    CloneMemory memory;

    
    const JSStructuredCloneCallbacks *callbacks;

    
    void *closure;
};

#endif 
