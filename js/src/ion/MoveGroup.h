








































#ifndef js_ion_movegroup_h__
#define js_ion_movegroup_h__

#include "IonLIR.h"

namespace js {
namespace ion {


class MoveGroup : public TempObject
{
    RegisterSet freeRegs;

  public:
    struct Entry {
        LAllocation *from;
        LAllocation *to;

        Entry () { }
        Entry(LAllocation *from, LAllocation *to)
          : from(from),
            to(to)
        { }
    };

  private:
    Vector<Entry, 1, IonAllocPolicy> entries_;

  public:
    bool add(LAllocation *from, LAllocation *to) {
        return entries_.append(Entry(from, to));
    }
    bool add(const Entry &ent) {
        return entries_.append(ent);
    }
    size_t numEntries() {
        return entries_.length();
    }
    Entry *getEntry(size_t i) {
        return &entries_[i];
    }
    void setEntry(size_t i, Entry ent) {
        entries_[i] = ent;
    }
    void setFreeRegisters(const RegisterSet &freeRegs) {
        this->freeRegs = freeRegs;
    }
    bool toInstructionsBefore(LBlock *block, LInstruction *ins, uint32 stack);
    bool toInstructionsAfter(LBlock *block, LInstruction *ins, uint32 stack);
#ifdef DEBUG
    void spewWorkStack(const Vector<Entry, 0, IonAllocPolicy>& workStack);
#else
    void spewWorkStack(const Vector<Entry, 0, IonAllocPolicy>& workStack) { };
#endif
};

}
}

#endif
