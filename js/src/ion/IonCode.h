








































#ifndef jsion_coderef_h__
#define jsion_coderef_h__

#include "jscell.h"

namespace JSC {
    class ExecutablePool;
}

class JSScript;

namespace js {
namespace ion {




static const uint32 MAX_BUFFER_SIZE = (1 << 30) - 1;

typedef uint32 SnapshotOffset;

class MacroAssembler;

class IonCode : public gc::Cell
{
    uint8 *code_;
    JSC::ExecutablePool *pool_;
    uint32 bufferSize_;             
    uint32 insnSize_;               
    uint32 dataSize_;               
    uint32 relocTableSize_;         

    IonCode()
      : code_(NULL),
        pool_(NULL)
    { }
    IonCode(uint8 *code, uint32 bufferSize, JSC::ExecutablePool *pool)
      : code_(code),
        pool_(pool),
        bufferSize_(bufferSize),
        insnSize_(0),
        dataSize_(0),
        relocTableSize_(0)
    { }

    uint32 dataOffset() const {
        return insnSize_;
    }
    uint32 relocTableOffset() const {
        return dataOffset() + relocTableSize_;
    }

  public:
    uint8 *raw() const {
        return code_;
    }
    size_t instructionsSize() const {
        return insnSize_;
    }
    void trace(JSTracer *trc);
    void finalize(JSContext *cx);

    template <typename T> T as() const {
        return JS_DATA_TO_FUNC_PTR(T, raw());
    }

    void copyFrom(MacroAssembler &masm);

    static IonCode *FromExecutable(uint8 *buffer) {
        IonCode *code = *(IonCode **)(buffer - sizeof(IonCode *));
        JS_ASSERT(code->raw() == buffer);
        return code;
    }

    
    
    
    static IonCode *New(JSContext *cx, uint8 *code, uint32 bufferSize, JSC::ExecutablePool *pool);
};

#define ION_DISABLED_SCRIPT ((IonScript *)0x1)

class SnapshotWriter;


struct IonScript
{
    
    IonCode *method_;

    
    IonCode *deoptTable_;

    
    uint32 snapshots_;
    uint32 snapshotsSize_;

    
    uint32 bailoutTable_;
    uint32 bailoutEntries_;

    SnapshotOffset *bailoutTable() {
        return (SnapshotOffset *)(reinterpret_cast<uint8 *>(this) + bailoutTable_);
    }

  private:
    void trace(JSTracer *trc, JSScript *script);

  public:
    
    IonScript();

    static IonScript *New(JSContext *cx, size_t snapshotsSize, size_t snapshotEntries);
    static void Trace(JSTracer *trc, JSScript *script);
    static void Destroy(JSContext *cx, JSScript *script);

  public:
    IonCode *method() const {
        return method_;
    }
    void setMethod(IonCode *code) {
        method_ = code;
    }
    void setDeoptTable(IonCode *code) {
        deoptTable_ = code;
    }
    const uint8 *snapshots() const {
        return reinterpret_cast<const uint8 *>(this) + snapshots_;
    }
    size_t snapshotsSize() const {
        return snapshotsSize_;
    }
    SnapshotOffset bailoutToSnapshot(uint32 bailoutId) {
        JS_ASSERT(bailoutId < bailoutEntries_);
        return bailoutTable()[bailoutId];
    }
    void copySnapshots(const SnapshotWriter *writer);
    void copyBailoutTable(const SnapshotOffset *table);
};

}
}

#endif 

