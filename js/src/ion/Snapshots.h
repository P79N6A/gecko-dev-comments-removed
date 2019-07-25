








































#ifndef jsion_snapshots_h__
#define jsion_snapshots_h__

#include "IonCode.h"
#include "IonRegisters.h"
#include "CompactBuffer.h"

namespace js {
namespace ion {

static const uint32 SNAPSHOT_MAX_NARGS = 127;
static const uint32 SNAPSHOT_MAX_STACK = 127;




class SnapshotReader
{
    CompactBufferReader reader_;

    uint32 pcOffset_;       
    uint32 slotCount_;      

#ifdef DEBUG
    
    JSScript *script_;
    uint32 slotsRead_;
#endif

    template <typename T> inline T readVariableLength();

  public:
    enum SlotMode
    {
        CONSTANT,           
        DOUBLE_REG,         
        TYPED_REG,          
        TYPED_STACK,        
        UNTYPED,            
        JS_UNDEFINED,       
        JS_NULL,            
        JS_INT32            
    };

    class Location
    {
        friend class SnapshotReader;

        Register::Code reg_;
        int32 stackSlot_;

        static Location From(const Register &reg) {
            Location loc;
            loc.reg_ = reg.code();
            loc.stackSlot_ = INVALID_STACK_SLOT;
            return loc;
        }
        static Location From(int32 stackSlot) {
            Location loc;
            loc.reg = InvalidReg.code();
            loc.stackSlot_ = stackSlot;
            return loc;
        }

      public:
        Register reg() const {
            JS_ASSERT(!isStackSlot());
            return Register::FromCode(reg_);
        }
        int32 stackSlot() const {
            JS_ASSERT(isStackSlot());
            return stackSlot_;
        }
        bool isStackSlot() const {
            return stackSlot_ != INVALID_STACK_SLOT;
        }
    };

    class Slot
    {
        friend class SnapshotReader;

        SlotMode mode_;

        union {
            FloatRegister::Code fpu_;
            struct {
                JSValueType type;
                Location payload;
            } known_type_;
#if defined(JS_NUNBOX32)
            struct {
                Location type;
                Location payload;
            } unknown_type_;
#elif defined(JS_PUNBOX64)
            struct {
                Location value;
            } unknown_type_;
#endif
            int32 value_;
        };

        Slot(SlotMode mode, JSValueType type, const Location &loc)
          : mode_(mode)
        {
            known_type_.type = type;
            known_type_.payload = loc;
        }
        Slot(const FloatRegister &reg)
          : mode_(DOUBLE_REG)
        {
            fpu_ = reg.code();
        }
        Slot(SlotMode mode)
          : mode_(mode)
        { }
        Slot(SlotMode mode, uint32 index)
          : mode_(mode)
        {
            JS_ASSERT(mode == CONSTANT || mode == JS_INT32);
            value_ = index;
        }

      public:
        SlotMode mode() const {
            return mode_;
        }
        uint32 constantIndex() const {
            JS_ASSERT(mode() == CONSTANT);
            return value_;
        }
        int32 int32Value() const {
            JS_ASSERT(mode() == JS_INT32);
            return value_;
        }
        JSValueType knownType() const {
            JS_ASSERT(mode() == TYPED_REG || mode() == TYPED_STACK);
            return known_type_.type;
        }
        Register reg() const {
            JS_ASSERT(mode() == TYPED_REG && knownType() != JSVAL_TYPE_DOUBLE);
            return known_type_.payload.reg();
        }
        FloatRegister floatReg() const {
            JS_ASSERT(mode() == DOUBLE_REG);
            return FloatRegister::FromCode(fpu_);
        }
        int32 stackSlot() const {
            JS_ASSERT(mode() == TYPED_STACK);
            return known_type_.payload.stackSlot();
        }
#if defined(JS_NUNBOX32)
        Location payload() const {
            JS_ASSERT(mode() == UNTYPED);
            return unknown_type_.payload;
        }
        Location type() const {
            JS_ASSERT(mode() == UNTYPED);
            return unknown_type_.type;
        }
#elif defined(JS_PUNBOX64)
        Location value() const {
            JS_ASSERT(mode() == UNTYPED);
            return unknown_type_.value;
        }
#endif
    };

  public:
    SnapshotReader(const uint8 *buffer, const uint8 *end);

    uint32 pcOffset() const {
        return pcOffset_;
    }
    uint32 slots() const {
        return slotCount_;
    }

    Slot readSlot();
    void finishReading();
};

static const SnapshotOffset INVALID_SNAPSHOT_OFFSET = uint32(-1);



class SnapshotWriter
{
    CompactBufferWriter writer_;

    
    uint32 nslots_;
    uint32 slotsWritten_;
    SnapshotOffset lastStart_;

    void writeSlotHeader(JSValueType type, uint32 regCode);

  public:
    SnapshotWriter();

    SnapshotOffset start(JSFunction *fun, JSScript *script, jsbytecode *pc,
                         uint32 frameSize, uint32 exprStack);
    void addSlot(const FloatRegister &reg);
    void addSlot(JSValueType type, const Register &reg);
    void addSlot(JSValueType type, int32 stackOffset);
    void addUndefinedSlot();
    void addNullSlot();
    void addInt32Slot(int32 value);
    void addConstantPoolSlot(uint32 index);
#if defined(JS_NUNBOX32)
    void addSlot(const Register &type, const Register &payload);
    void addSlot(const Register &type, int32 payloadStackOffset);
    void addSlot(int32 typeStackOffset, const Register &payload);
    void addSlot(int32 typeStackOffset, int32 payloadStackOffset);
#elif defined(JS_PUNBOX64)
    void addSlot(const Register &value);
    void addSlot(int32 valueStackSlot);
#endif
    void endSnapshot();

    bool oom() const {
        return writer_.oom() || writer_.length() >= MAX_BUFFER_SIZE;
    }

    size_t length() const {
        return writer_.length();
    }
    const uint8 *buffer() const {
        return writer_.buffer();
    }
};

}
}

#endif 

