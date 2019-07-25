








































#ifndef jsion_value_numbering_h__
#define jsion_value_numbering_h__

#include "MIR.h"
#include "MIRGraph.h"


#define ION_GVN_PESSIMISTIC false

namespace js {
namespace ion {

class ValueNumberer
{
  private:
    struct ValueHasher
    {
        typedef MInstruction * Lookup;
        typedef MInstruction * Key;
        static HashNumber hash(const Lookup &ins) {
            return ins->valueHash();
        }

        static bool match(const Key &k, const Lookup &l) {
            return k->congruentTo(l);
        }
    };

    typedef HashMap<MInstruction *,
                    uint32,
                    ValueHasher,
                    IonAllocPolicy> ValueMap;

    struct DominatingValue
    {
        MInstruction *def;
        uint32 validUntil;
    };

    typedef HashMap<uint32,
                    DominatingValue,
                    DefaultHasher<uint32>,
                    IonAllocPolicy> InstructionMap;

    MIRGraph &graph_;
    bool pessimisticPass_;

    uint32 lookupValue(ValueMap &values, MInstruction *ins);
    MInstruction *findDominatingInstruction(InstructionMap &defs, MInstruction *ins, size_t index);
    bool eliminateRedundancies();

    bool computeValueNumbers();

  public:
    ValueNumberer(MIRGraph &graph, bool pessimistic);
    bool analyze();
};

} 
} 

#endif 

