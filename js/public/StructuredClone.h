





#ifndef js_StructuredClone_h
#define js_StructuredClone_h

#include "mozilla/NullPtr.h"

#include <stdint.h>

#include "jstypes.h"

#include "js/TypeDecls.h"

struct JSRuntime;
struct JSStructuredCloneReader;
struct JSStructuredCloneWriter;










typedef JSObject *(*ReadStructuredCloneOp)(JSContext *cx, JSStructuredCloneReader *r,
                                           uint32_t tag, uint32_t data, void *closure);










typedef bool (*WriteStructuredCloneOp)(JSContext *cx, JSStructuredCloneWriter *w,
                                         JS::Handle<JSObject*> obj, void *closure);




typedef void (*StructuredCloneErrorOp)(JSContext *cx, uint32_t errorid);


#define JS_STRUCTURED_CLONE_VERSION 2

struct JSStructuredCloneCallbacks {
    ReadStructuredCloneOp read;
    WriteStructuredCloneOp write;
    StructuredCloneErrorOp reportError;
};


JS_PUBLIC_API(bool)
JS_ReadStructuredClone(JSContext *cx, uint64_t *data, size_t nbytes, uint32_t version,
                       JS::Value *vp, const JSStructuredCloneCallbacks *optionalCallbacks,
                       void *closure);



JS_PUBLIC_API(bool)
JS_WriteStructuredClone(JSContext *cx, JS::Value v, uint64_t **datap, size_t *nbytesp,
                        const JSStructuredCloneCallbacks *optionalCallbacks,
                        void *closure, JS::Value transferable);

JS_PUBLIC_API(bool)
JS_ClearStructuredClone(const uint64_t *data, size_t nbytes);

JS_PUBLIC_API(bool)
JS_StructuredCloneHasTransferables(const uint64_t *data, size_t nbytes, bool *hasTransferable);

JS_PUBLIC_API(bool)
JS_StructuredClone(JSContext *cx, JS::Value v, JS::Value *vp,
                   const JSStructuredCloneCallbacks *optionalCallbacks, void *closure);


class JS_PUBLIC_API(JSAutoStructuredCloneBuffer) {
    uint64_t *data_;
    size_t nbytes_;
    uint32_t version_;

  public:
    JSAutoStructuredCloneBuffer()
        : data_(nullptr), nbytes_(0), version_(JS_STRUCTURED_CLONE_VERSION) {}

    ~JSAutoStructuredCloneBuffer() { clear(); }

    uint64_t *data() const { return data_; }
    size_t nbytes() const { return nbytes_; }

    void clear();

    
    bool copy(const uint64_t *data, size_t nbytes, uint32_t version=JS_STRUCTURED_CLONE_VERSION);

    
    
    
    void adopt(uint64_t *data, size_t nbytes, uint32_t version=JS_STRUCTURED_CLONE_VERSION);

    
    
    
    void steal(uint64_t **datap, size_t *nbytesp, uint32_t *versionp=nullptr);

    bool read(JSContext *cx, JS::Value *vp,
              const JSStructuredCloneCallbacks *optionalCallbacks=nullptr, void *closure=nullptr);

    bool write(JSContext *cx, JS::Value v,
               const JSStructuredCloneCallbacks *optionalCallbacks=nullptr, void *closure=nullptr);

    bool write(JSContext *cx, JS::Value v, JS::Value transferable,
               const JSStructuredCloneCallbacks *optionalCallbacks=nullptr, void *closure=nullptr);

    
    void swap(JSAutoStructuredCloneBuffer &other);

  private:
    
    JSAutoStructuredCloneBuffer(const JSAutoStructuredCloneBuffer &other);
    JSAutoStructuredCloneBuffer &operator=(const JSAutoStructuredCloneBuffer &other);
};


#define JS_SCTAG_USER_MIN  ((uint32_t) 0xFFFF8000)
#define JS_SCTAG_USER_MAX  ((uint32_t) 0xFFFFFFFF)

#define JS_SCERR_RECURSION 0
#define JS_SCERR_TRANSFERABLE 1

JS_PUBLIC_API(void)
JS_SetStructuredCloneCallbacks(JSRuntime *rt, const JSStructuredCloneCallbacks *callbacks);

JS_PUBLIC_API(bool)
JS_ReadUint32Pair(JSStructuredCloneReader *r, uint32_t *p1, uint32_t *p2);

JS_PUBLIC_API(bool)
JS_ReadBytes(JSStructuredCloneReader *r, void *p, size_t len);

JS_PUBLIC_API(bool)
JS_ReadTypedArray(JSStructuredCloneReader *r, JS::Value *vp);

JS_PUBLIC_API(bool)
JS_WriteUint32Pair(JSStructuredCloneWriter *w, uint32_t tag, uint32_t data);

JS_PUBLIC_API(bool)
JS_WriteBytes(JSStructuredCloneWriter *w, const void *p, size_t len);

JS_PUBLIC_API(bool)
JS_WriteTypedArray(JSStructuredCloneWriter *w, JS::Value v);

#endif  
