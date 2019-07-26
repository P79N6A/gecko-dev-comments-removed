





#ifndef jit_Snapshot_h
#define jit_Snapshot_h

#include "jsalloc.h"
#include "jsbytecode.h"

#include "jit/CompactBuffer.h"
#include "jit/IonTypes.h"
#include "jit/Registers.h"

#include "js/HashTable.h"

namespace js {
namespace jit {

class RValueAllocation;










class RValueAllocation
{
  public:

    
    enum Mode
    {
        CONSTANT            = 0x00,
        CST_UNDEFINED       = 0x01,
        CST_NULL            = 0x02,
        DOUBLE_REG          = 0x03,
        FLOAT32_REG         = 0x04,
        FLOAT32_STACK       = 0x05,
#if defined(JS_NUNBOX32)
        UNTYPED_REG_REG     = 0x06,
        UNTYPED_REG_STACK   = 0x07,
        UNTYPED_STACK_REG   = 0x08,
        UNTYPED_STACK_STACK = 0x09,
#elif defined(JS_PUNBOX64)
        UNTYPED_REG         = 0x06,
        UNTYPED_STACK       = 0x07,
#endif
        
        TYPED_REG_MIN       = 0x10,
        TYPED_REG_MAX       = 0x17,
        TYPED_REG = TYPED_REG_MIN,

        
        TYPED_STACK_MIN     = 0x18,
        TYPED_STACK_MAX     = 0x1f,
        TYPED_STACK = TYPED_STACK_MIN,

        INVALID = 0x100,
    };

    
    enum PayloadType {
        PAYLOAD_NONE,
        PAYLOAD_INDEX,
        PAYLOAD_STACK_OFFSET,
        PAYLOAD_GPR,
        PAYLOAD_FPU,
        PAYLOAD_PACKED_TAG
    };

    struct Layout {
        PayloadType type1;
        PayloadType type2;
        const char *name;
    };

  private:
    Mode mode_;

    
    union Payload {
        uint32_t index;
        int32_t stackOffset;
        Register gpr;
        FloatRegister fpu;
        JSValueType type;
    };

    Payload arg1_;
    Payload arg2_;

    static Payload payloadOfIndex(uint32_t index) {
        Payload p;
        p.index = index;
        return p;
    }
    static Payload payloadOfStackOffset(int32_t offset) {
        Payload p;
        p.stackOffset = offset;
        return p;
    }
    static Payload payloadOfRegister(Register reg) {
        Payload p;
        p.gpr = reg;
        return p;
    }
    static Payload payloadOfFloatRegister(FloatRegister reg) {
        Payload p;
        p.fpu = reg;
        return p;
    }
    static Payload payloadOfValueType(JSValueType type) {
        Payload p;
        p.type = type;
        return p;
    }

    static const Layout &layoutFromMode(Mode mode);

    static void readPayload(CompactBufferReader &reader, PayloadType t,
                            uint8_t *mode, Payload *p);
    static void writePayload(CompactBufferWriter &writer, PayloadType t,
                             Payload p);
    static void writePadding(CompactBufferWriter &writer);
    static void dumpPayload(FILE *fp, PayloadType t, Payload p);
    static bool equalPayloads(PayloadType t, Payload lhs, Payload rhs);

    RValueAllocation(Mode mode, Payload a1, Payload a2)
      : mode_(mode),
        arg1_(a1),
        arg2_(a2)
    {
    }

    RValueAllocation(Mode mode, Payload a1)
      : mode_(mode),
        arg1_(a1)
    {
    }

    RValueAllocation(Mode mode)
      : mode_(mode)
    {
    }

  public:
    RValueAllocation()
      : mode_(INVALID)
    { }

    
    static RValueAllocation Double(const FloatRegister &reg) {
        return RValueAllocation(DOUBLE_REG, payloadOfFloatRegister(reg));
    }

    
    static RValueAllocation Float32(const FloatRegister &reg) {
        return RValueAllocation(FLOAT32_REG, payloadOfFloatRegister(reg));
    }
    static RValueAllocation Float32(int32_t offset) {
        return RValueAllocation(FLOAT32_STACK, payloadOfStackOffset(offset));
    }

    
    static RValueAllocation Typed(JSValueType type, const Register &reg) {
        JS_ASSERT(type != JSVAL_TYPE_DOUBLE &&
                  type != JSVAL_TYPE_MAGIC &&
                  type != JSVAL_TYPE_NULL &&
                  type != JSVAL_TYPE_UNDEFINED);
        return RValueAllocation(TYPED_REG, payloadOfValueType(type),
                                payloadOfRegister(reg));
    }
    static RValueAllocation Typed(JSValueType type, int32_t offset) {
        JS_ASSERT(type != JSVAL_TYPE_MAGIC &&
                  type != JSVAL_TYPE_NULL &&
                  type != JSVAL_TYPE_UNDEFINED);
        return RValueAllocation(TYPED_STACK, payloadOfValueType(type),
                                payloadOfStackOffset(offset));
    }

    
#if defined(JS_NUNBOX32)
    static RValueAllocation Untyped(const Register &type, const Register &payload) {
        return RValueAllocation(UNTYPED_REG_REG,
                                payloadOfRegister(type),
                                payloadOfRegister(payload));
    }

    static RValueAllocation Untyped(const Register &type, int32_t payloadStackOffset) {
        return RValueAllocation(UNTYPED_REG_STACK,
                                payloadOfRegister(type),
                                payloadOfStackOffset(payloadStackOffset));
    }

    static RValueAllocation Untyped(int32_t typeStackOffset, const Register &payload) {
        return RValueAllocation(UNTYPED_STACK_REG,
                                payloadOfStackOffset(typeStackOffset),
                                payloadOfRegister(payload));
    }

    static RValueAllocation Untyped(int32_t typeStackOffset, int32_t payloadStackOffset) {
        return RValueAllocation(UNTYPED_STACK_STACK,
                                payloadOfStackOffset(typeStackOffset),
                                payloadOfStackOffset(payloadStackOffset));
    }

#elif defined(JS_PUNBOX64)
    static RValueAllocation Untyped(const Register &reg) {
        return RValueAllocation(UNTYPED_REG, payloadOfRegister(reg));
    }

    static RValueAllocation Untyped(int32_t stackOffset) {
        return RValueAllocation(UNTYPED_STACK, payloadOfStackOffset(stackOffset));
    }
#endif

    
    static RValueAllocation Undefined() {
        return RValueAllocation(CST_UNDEFINED);
    }
    static RValueAllocation Null() {
        return RValueAllocation(CST_NULL);
    }

    
    static RValueAllocation ConstantPool(uint32_t index) {
        return RValueAllocation(CONSTANT, payloadOfIndex(index));
    }

    void writeHeader(CompactBufferWriter &writer, JSValueType type, uint32_t regCode) const;
  public:
    static RValueAllocation read(CompactBufferReader &reader);
    void write(CompactBufferWriter &writer) const;

  public:
    Mode mode() const {
        return mode_;
    }

    uint32_t index() const {
        JS_ASSERT(layoutFromMode(mode()).type1 == PAYLOAD_INDEX);
        return arg1_.index;
    }
    int32_t stackOffset() const {
        JS_ASSERT(layoutFromMode(mode()).type1 == PAYLOAD_STACK_OFFSET);
        return arg1_.stackOffset;
    }
    Register reg() const {
        JS_ASSERT(layoutFromMode(mode()).type1 == PAYLOAD_GPR);
        return arg1_.gpr;
    }
    FloatRegister fpuReg() const {
        JS_ASSERT(layoutFromMode(mode()).type1 == PAYLOAD_FPU);
        return arg1_.fpu;
    }
    JSValueType knownType() const {
        JS_ASSERT(layoutFromMode(mode()).type1 == PAYLOAD_PACKED_TAG);
        return arg1_.type;
    }

    int32_t stackOffset2() const {
        JS_ASSERT(layoutFromMode(mode()).type2 == PAYLOAD_STACK_OFFSET);
        return arg2_.stackOffset;
    }
    Register reg2() const {
        JS_ASSERT(layoutFromMode(mode()).type2 == PAYLOAD_GPR);
        return arg2_.gpr;
    }

  public:
    void dump(FILE *fp) const;

  public:
    bool operator==(const RValueAllocation &rhs) const {
        if (mode_ != rhs.mode_)
            return false;

        const Layout &layout = layoutFromMode(mode());
        return equalPayloads(layout.type1, arg1_, rhs.arg1_) &&
            equalPayloads(layout.type2, arg2_, rhs.arg2_);
    }

    HashNumber hash() const;

    struct Hasher
    {
        typedef RValueAllocation Key;
        typedef Key Lookup;
        static HashNumber hash(const Lookup &v) {
            return v.hash();
        }
        static bool match(const Key &k, const Lookup &l) {
            return k == l;
        }
    };
};



class SnapshotWriter
{
    CompactBufferWriter writer_;
    CompactBufferWriter allocWriter_;

    
    
    typedef RValueAllocation RVA;
    typedef HashMap<RVA, uint32_t, RVA::Hasher, SystemAllocPolicy> RValueAllocMap;
    RValueAllocMap allocMap_;

    
    uint32_t nallocs_;
    uint32_t allocWritten_;
    uint32_t nframes_;
    uint32_t framesWritten_;
    SnapshotOffset lastStart_;

  public:
    bool init();

    SnapshotOffset startSnapshot(uint32_t frameCount, BailoutKind kind, bool resumeAfter);
#ifdef TRACK_SNAPSHOTS
    void trackSnapshot(uint32_t pcOpcode, uint32_t mirOpcode, uint32_t mirId,
                       uint32_t lirOpcode, uint32_t lirId);
#endif
    void startFrame(JSFunction *fun, JSScript *script, jsbytecode *pc, uint32_t exprStack);
    void endFrame();

    bool add(const RValueAllocation &slot);

    void endSnapshot();

    bool oom() const {
        return writer_.oom() || writer_.length() >= MAX_BUFFER_SIZE;
    }

    size_t listSize() const {
        return writer_.length();
    }
    const uint8_t *listBuffer() const {
        return writer_.buffer();
    }

    size_t RVATableSize() const {
        return allocWriter_.length();
    }
    const uint8_t *RVATableBuffer() const {
        return allocWriter_.buffer();
    }
};





class SnapshotReader
{
    CompactBufferReader reader_;
    CompactBufferReader allocReader_;
    const uint8_t* allocTable_;

    uint32_t pcOffset_;           
    uint32_t allocCount_;         
    uint32_t frameCount_;
    BailoutKind bailoutKind_;
    uint32_t framesRead_;         
    uint32_t allocRead_;          
    bool resumeAfter_;

#ifdef TRACK_SNAPSHOTS
  private:
    uint32_t pcOpcode_;
    uint32_t mirOpcode_;
    uint32_t mirId_;
    uint32_t lirOpcode_;
    uint32_t lirId_;

  public:
    void spewBailingFrom() const;
#endif

  private:
    void readSnapshotHeader();
    void readFrameHeader();

  public:
    SnapshotReader(const uint8_t *snapshots, uint32_t offset,
                   uint32_t RVATableSize, uint32_t listSize);

    uint32_t allocations() const {
        return allocCount_;
    }
    RValueAllocation readAllocation();

    bool moreAllocations() const {
        return allocRead_ < allocCount_;
    }

    uint32_t pcOffset() const {
        return pcOffset_;
    }
    BailoutKind bailoutKind() const {
        return bailoutKind_;
    }
    bool resumeAfter() const {
        if (moreFrames())
            return false;
        return resumeAfter_;
    }

    bool moreFrames() const {
        return framesRead_ < frameCount_;
    }
    void nextFrame() {
        readFrameHeader();
    }
    uint32_t frameCount() const {
        return frameCount_;
    }
};

}
}

#endif 
