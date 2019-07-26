




#ifndef jsclone_h___
#define jsclone_h___

#include "jsapi.h"
#include "jscntxt.h"

#include "js/HashTable.h"
#include "js/Vector.h"

namespace js {

bool
WriteStructuredClone(JSContext *cx, HandleValue v, uint64_t **bufp, size_t *nbytesp,
                     const JSStructuredCloneCallbacks *cb, void *cbClosure,
                     jsval transferable);

bool
ReadStructuredClone(JSContext *cx, uint64_t *data, size_t nbytes, Value *vp,
                    const JSStructuredCloneCallbacks *cb, void *cbClosure);

bool
ClearStructuredClone(const uint64_t *data, size_t nbytes);

bool
StructuredCloneHasTransferObjects(const uint64_t *data, size_t nbytes,
                                  bool *hasTransferable);

struct SCOutput {
  public:
    explicit SCOutput(JSContext *cx);

    JSContext *context() const { return cx; }

    bool write(uint64_t u);
    bool writePair(uint32_t tag, uint32_t data);
    bool writeDouble(double d);
    bool writeBytes(const void *p, size_t nbytes);
    bool writeChars(const jschar *p, size_t nchars);
    bool writePtr(const void *);

    template <class T>
    bool writeArray(const T *p, size_t nbytes);

    bool extractBuffer(uint64_t **datap, size_t *sizep);

    uint64_t count() { return buf.length(); }

  private:
    JSContext *cx;
    js::Vector<uint64_t> buf;
};

struct SCInput {
  public:
    SCInput(JSContext *cx, uint64_t *data, size_t nbytes);

    JSContext *context() const { return cx; }

    bool read(uint64_t *p);
    bool readPair(uint32_t *tagp, uint32_t *datap);
    bool readDouble(double *p);
    bool readBytes(void *p, size_t nbytes);
    bool readChars(jschar *p, size_t nchars);
    bool readPtr(void **);

    bool get(uint64_t *p);
    bool getPair(uint32_t *tagp, uint32_t *datap);

    bool replace(uint64_t u);
    bool replacePair(uint32_t tag, uint32_t data);

    template <class T>
    bool readArray(T *p, size_t nelems);

  private:
    bool eof();

    void staticAssertions() {
        JS_STATIC_ASSERT(sizeof(jschar) == 2);
        JS_STATIC_ASSERT(sizeof(uint32_t) == 4);
        JS_STATIC_ASSERT(sizeof(double) == 8);
    }

    JSContext *cx;
    uint64_t *point;
    uint64_t *end;
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

    bool readTransferMap();

    bool checkDouble(double d);
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

    friend JSBool JS_ReadTypedArray(JSStructuredCloneReader *r, jsval *vp);
};

struct JSStructuredCloneWriter {
  public:
    explicit JSStructuredCloneWriter(js::SCOutput &out,
                                     const JSStructuredCloneCallbacks *cb,
                                     void *cbClosure,
                                     jsval tVal)
        : out(out), objs(out.context()),
          counts(out.context()), ids(out.context()),
          memory(out.context()), callbacks(cb), closure(cbClosure),
          transferable(out.context(), tVal), transferableObjects(out.context()) { }

    bool init() { return transferableObjects.init() && parseTransferable() &&
                         memory.init() && writeTransferMap(); }

    bool write(const js::Value &v);

    js::SCOutput &output() { return out; }

  private:
    JSContext *context() { return out.context(); }

    bool writeTransferMap();

    bool writeString(uint32_t tag, JSString *str);
    bool writeId(jsid id);
    bool writeArrayBuffer(JSHandleObject obj);
    bool writeTypedArray(JSHandleObject obj);
    bool startObject(JSHandleObject obj, bool *backref);
    bool startWrite(const js::Value &v);
    bool traverseObject(JSHandleObject obj);

    bool parseTransferable();
    void reportErrorTransferable();

    inline void checkStack();

    js::SCOutput &out;

    
    
    
    
    js::AutoValueVector objs;

    
    
    js::Vector<size_t> counts;

    
    js::AutoIdVector ids;

    
    
    
    typedef js::AutoObjectUnsigned32HashMap CloneMemory;
    CloneMemory memory;

    
    const JSStructuredCloneCallbacks *callbacks;

    
    void *closure;

    
    JS::RootedValue transferable;
    js::AutoObjectHashSet transferableObjects;

    friend JSBool JS_WriteTypedArray(JSStructuredCloneWriter *w, jsval v);
};

#endif 
