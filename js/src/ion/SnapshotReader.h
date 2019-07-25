








































#ifndef jsion_snapshots_h__
#define jsion_snapshots_h__

#include "Ion.h"
#include "IonCode.h"
#include "Registers.h"
#include "CompactBuffer.h"

namespace js {
namespace ion {

#ifdef TRACK_SNAPSHOTS
class LInstruction;
#endif




class SnapshotReader
{
    CompactBufferReader reader_;

    uint32 pcOffset_;           
    uint32 slotCount_;          
    uint32 frameCount_;
    BailoutKind bailoutKind_;
    bool resumeAfter_;

#ifdef DEBUG
    
    JSScript *script_;
    uint32 slotsRead_;
#endif

#ifdef TRACK_SNAPSHOTS
    uint32 pcOpcode_;
    uint32 mirOpcode_;
    uint32 mirId_;
    uint32 lirOpcode_;
    uint32 lirId_;
#endif

    void readSnapshotHeader();
    void readSnapshotBody();

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
            loc.reg_ = Register::Code(0);      
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

        bool liveInReg() const;
    };

  public:
    SnapshotReader(const uint8 *buffer, const uint8 *end);

    uint32 pcOffset() const {
        return pcOffset_;
    }
    uint32 slots() const {
        return slotCount_;
    }
    BailoutKind bailoutKind() const {
        return bailoutKind_;
    }
    bool resumeAfter() const {
        return resumeAfter_;
    }

    Slot readSlot();
    void finishReadingFrame();
    uint32 remainingFrameCount() const { return frameCount_; }
};

class FrameRecovery;

class SnapshotIterator
{
  private:
    const FrameRecovery &in_;
    Maybe<SnapshotReader> reader_;
    uint32 unreadSlots_;

    uintptr_t fromLocation(const SnapshotReader::Location &loc);
    void skipLocation(const SnapshotReader::Location &loc);
    static Value FromTypedPayload(JSValueType type, uintptr_t payload);

  public:
    SnapshotIterator(const FrameRecovery &in);

    typedef SnapshotReader::Slot Slot;

    
    Slot readSlot();
    Value slotValue(const Slot &);
    void skip(const Slot &);

    Value read() {
        return slotValue(readSlot());
    }

    bool more() {
        if (!unreadSlots_)
            reader_.ref().finishReadingFrame();
        return unreadSlots_;
    }

    
    void readFrame() {
        unreadSlots_ = slots();
    }

    bool moreFrames() const {
        return reader_.ref().remainingFrameCount() > 1;
    }

    uint32 slots() const {
        return reader_.ref().slots();
    }

    uint32 pcOffset() const {
        return reader_.ref().pcOffset();
    }

    BailoutKind bailoutKind() const {
        return reader_.ref().bailoutKind();
    }
};

}
}

#endif 

