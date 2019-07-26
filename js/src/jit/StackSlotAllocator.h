





#ifndef jit_StackSlotAllocator_h
#define jit_StackSlotAllocator_h

#include "jit/Registers.h"

namespace js {
namespace jit {

class StackSlotAllocator
{
    js::Vector<uint32_t, 4, SystemAllocPolicy> normalSlots;
    js::Vector<uint32_t, 4, SystemAllocPolicy> doubleSlots;
    uint32_t height_;

    void freeSlot(uint32_t index) {
        normalSlots.append(index);
    }
    void freeDoubleSlot(uint32_t index) {
        doubleSlots.append(index);
    }

    uint32_t allocateDoubleSlot() {
        if (!doubleSlots.empty())
            return doubleSlots.popCopy();
        if (ComputeByteAlignment(height_, DOUBLE_STACK_ALIGNMENT))
            normalSlots.append(++height_);
        height_ += (sizeof(double) / STACK_SLOT_SIZE);
        return height_;
    }
    uint32_t allocateSlot() {
        if (!normalSlots.empty())
            return normalSlots.popCopy();
        if (!doubleSlots.empty()) {
            uint32_t index = doubleSlots.popCopy();
            normalSlots.append(index - 1);
            return index;
        }
        return ++height_;
    }

  public:
    StackSlotAllocator() : height_(0)
    { }

    void freeSlot(LDefinition::Type type, uint32_t index) {
        switch (type) {
          case LDefinition::FLOAT32:
          case LDefinition::OBJECT:
          case LDefinition::SLOTS:
#ifdef JS_PUNBOX64
          case LDefinition::BOX:
#endif
          case LDefinition::GENERAL: return freeSlot(index);
#ifdef JS_NUNBOX32
          case LDefinition::TYPE:
          case LDefinition::PAYLOAD:
#endif
          case LDefinition::DOUBLE:  return freeDoubleSlot(index);
          default: MOZ_ASSUME_UNREACHABLE("Unknown slot type");
        }
    }

    uint32_t allocateSlot(LDefinition::Type type) {
        switch (type) {
          case LDefinition::FLOAT32:
          case LDefinition::OBJECT:
          case LDefinition::SLOTS:
#ifdef JS_PUNBOX64
          case LDefinition::BOX:
#endif
          case LDefinition::GENERAL: return allocateSlot();
#ifdef JS_NUNBOX32
          case LDefinition::TYPE:
          case LDefinition::PAYLOAD:
#endif
          case LDefinition::DOUBLE:  return allocateDoubleSlot();
          default: MOZ_ASSUME_UNREACHABLE("Unknown slot type");
        }
    }

    uint32_t stackHeight() const {
        return height_;
    }
};

} 
} 

#endif 
