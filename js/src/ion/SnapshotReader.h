






#ifndef jsion_snapshots_h__
#define jsion_snapshots_h__

#include "IonTypes.h"
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

    uint32_t pcOffset_;           
    uint32_t slotCount_;          
    uint32_t frameCount_;
    BailoutKind bailoutKind_;
    uint32_t framesRead_;         
    uint32_t slotsRead_;          
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
        int32_t stackSlot_;

        static Location From(const Register &reg) {
            Location loc;
            loc.reg_ = reg.code();
            loc.stackSlot_ = INVALID_STACK_SLOT;
            return loc;
        }
        static Location From(int32_t stackSlot) {
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
        int32_t stackSlot() const {
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
            int32_t value_;
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
        Slot(SlotMode mode, uint32_t index)
          : mode_(mode)
        {
            JS_ASSERT(mode == CONSTANT || mode == JS_INT32);
            value_ = index;
        }

      public:
        SlotMode mode() const {
            return mode_;
        }
        uint32_t constantIndex() const {
            JS_ASSERT(mode() == CONSTANT);
            return value_;
        }
        int32_t int32Value() const {
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
        int32_t stackSlot() const {
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
    SnapshotReader(const uint8_t *buffer, const uint8_t *end);

    uint32_t pcOffset() const {
        return pcOffset_;
    }
    uint32_t slots() const {
        return slotCount_;
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
    Slot readSlot();

    Value skip() {
        readSlot();
        return UndefinedValue();
    }

    bool moreSlots() const {
        return slotsRead_ < slotCount_;
    }
    uint32_t frameCount() const {
        return frameCount_;
    }
};

}
}

#endif 

