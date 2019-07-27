





#ifndef js_StructuredClone_h
#define js_StructuredClone_h

#include "mozilla/NullPtr.h"

#include <stdint.h>

#include "jstypes.h"

#include "js/RootingAPI.h"
#include "js/TypeDecls.h"
#include "js/Value.h"

struct JSRuntime;
struct JSStructuredCloneReader;
struct JSStructuredCloneWriter;



namespace JS {
enum TransferableOwnership {
    
    SCTAG_TMO_UNFILLED = 0,

    
    SCTAG_TMO_UNOWNED = 1,

    
    SCTAG_TMO_FIRST_OWNED = 2,

    
    SCTAG_TMO_ALLOC_DATA = 2,

    
    SCTAG_TMO_SHARED_BUFFER = 3,

    
    SCTAG_TMO_MAPPED_DATA = 4,

    
    
    
    SCTAG_TMO_CUSTOM = 5,

    SCTAG_TMO_USER_MIN
};
} 








typedef JSObject *(*ReadStructuredCloneOp)(JSContext *cx, JSStructuredCloneReader *r,
                                           uint32_t tag, uint32_t data, void *closure);











typedef bool (*WriteStructuredCloneOp)(JSContext *cx, JSStructuredCloneWriter *w,
                                       JS::HandleObject obj, void *closure);




typedef void (*StructuredCloneErrorOp)(JSContext *cx, uint32_t errorid);






typedef bool (*ReadTransferStructuredCloneOp)(JSContext *cx, JSStructuredCloneReader *r,
                                              uint32_t tag, void *content, uint64_t extraData,
                                              void *closure,
                                              JS::MutableHandleObject returnObject);















typedef bool (*TransferStructuredCloneOp)(JSContext *cx,
                                          JS::Handle<JSObject*> obj,
                                          void *closure,
                                          
                                          uint32_t *tag,
                                          JS::TransferableOwnership *ownership,
                                          void **content,
                                          uint64_t *extraData);




typedef void (*FreeTransferStructuredCloneOp)(uint32_t tag, JS::TransferableOwnership ownership,
                                              void *content, uint64_t extraData, void *closure);





#define JS_STRUCTURED_CLONE_VERSION 5

struct JSStructuredCloneCallbacks {
    ReadStructuredCloneOp read;
    WriteStructuredCloneOp write;
    StructuredCloneErrorOp reportError;
    ReadTransferStructuredCloneOp readTransfer;
    TransferStructuredCloneOp writeTransfer;
    FreeTransferStructuredCloneOp freeTransfer;
};


JS_PUBLIC_API(bool)
JS_ReadStructuredClone(JSContext *cx, uint64_t *data, size_t nbytes, uint32_t version,
                       JS::MutableHandleValue vp,
                       const JSStructuredCloneCallbacks *optionalCallbacks, void *closure);



JS_PUBLIC_API(bool)
JS_WriteStructuredClone(JSContext *cx, JS::HandleValue v, uint64_t **datap, size_t *nbytesp,
                        const JSStructuredCloneCallbacks *optionalCallbacks,
                        void *closure, JS::HandleValue transferable);

JS_PUBLIC_API(bool)
JS_ClearStructuredClone(uint64_t *data, size_t nbytes,
                        const JSStructuredCloneCallbacks *optionalCallbacks,
                        void *closure);

JS_PUBLIC_API(bool)
JS_StructuredCloneHasTransferables(const uint64_t *data, size_t nbytes, bool *hasTransferable);

JS_PUBLIC_API(bool)
JS_StructuredClone(JSContext *cx, JS::HandleValue v, JS::MutableHandleValue vp,
                   const JSStructuredCloneCallbacks *optionalCallbacks, void *closure);


class JS_PUBLIC_API(JSAutoStructuredCloneBuffer) {
    uint64_t *data_;
    size_t nbytes_;
    uint32_t version_;
    const JSStructuredCloneCallbacks *callbacks_;
    void *closure_;

  public:
    JSAutoStructuredCloneBuffer()
        : data_(nullptr), nbytes_(0), version_(JS_STRUCTURED_CLONE_VERSION),
          callbacks_(nullptr), closure_(nullptr)
    {}

    JSAutoStructuredCloneBuffer(const JSStructuredCloneCallbacks *callbacks, void *closure)
        : data_(nullptr), nbytes_(0), version_(JS_STRUCTURED_CLONE_VERSION),
          callbacks_(callbacks), closure_(closure)
    {}

    JSAutoStructuredCloneBuffer(JSAutoStructuredCloneBuffer &&other);
    JSAutoStructuredCloneBuffer &operator=(JSAutoStructuredCloneBuffer &&other);

    ~JSAutoStructuredCloneBuffer() { clear(); }

    uint64_t *data() const { return data_; }
    size_t nbytes() const { return nbytes_; }

    void clear();

    
    bool copy(const uint64_t *data, size_t nbytes, uint32_t version=JS_STRUCTURED_CLONE_VERSION);

    
    
    
    void adopt(uint64_t *data, size_t nbytes, uint32_t version=JS_STRUCTURED_CLONE_VERSION);

    
    
    
    void steal(uint64_t **datap, size_t *nbytesp, uint32_t *versionp=nullptr);

    bool read(JSContext *cx, JS::MutableHandleValue vp,
              const JSStructuredCloneCallbacks *optionalCallbacks=nullptr, void *closure=nullptr);

    bool write(JSContext *cx, JS::HandleValue v,
               const JSStructuredCloneCallbacks *optionalCallbacks=nullptr, void *closure=nullptr);

    bool write(JSContext *cx, JS::HandleValue v, JS::HandleValue transferable,
               const JSStructuredCloneCallbacks *optionalCallbacks=nullptr, void *closure=nullptr);

  private:
    
    JSAutoStructuredCloneBuffer(const JSAutoStructuredCloneBuffer &other) MOZ_DELETE;
    JSAutoStructuredCloneBuffer &operator=(const JSAutoStructuredCloneBuffer &other) MOZ_DELETE;
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
JS_ReadTypedArray(JSStructuredCloneReader *r, JS::MutableHandleValue vp);

JS_PUBLIC_API(bool)
JS_WriteUint32Pair(JSStructuredCloneWriter *w, uint32_t tag, uint32_t data);

JS_PUBLIC_API(bool)
JS_WriteBytes(JSStructuredCloneWriter *w, const void *p, size_t len);

JS_PUBLIC_API(bool)
JS_WriteTypedArray(JSStructuredCloneWriter *w, JS::HandleValue v);

#endif  
