








































#ifndef jsion_value_numbering_h__
#define jsion_value_numbering_h__

#include "MIR.h"
#include "MIRGraph.h"

namespace js {
namespace ion {

class ValueNumberer
{
  private:
    struct ValueHasher
    {
        typedef MDefinition * Lookup;
        typedef MDefinition * Key;
        static HashNumber hash(const Lookup &ins) {
            return ins->valueHash();
        }

        static bool match(const Key &k, const Lookup &l) {
            return k->congruentTo(l);
        }
    };

    typedef HashMap<MDefinition *,
                    uint32,
                    ValueHasher,
                    IonAllocPolicy> ValueMap;

    struct DominatingValue
    {
        MDefinition *def;
        uint32 validUntil;
    };

    typedef HashMap<uint32,
                    DominatingValue,
                    DefaultHasher<uint32>,
                    IonAllocPolicy> InstructionMap;

    MIRGraph &graph_;
    bool pessimisticPass_;

    uint32 lookupValue(ValueMap &values, MDefinition *ins);
    MDefinition *findDominatingDef(InstructionMap &defs, MDefinition *ins, size_t index);

    MDefinition *simplify(MDefinition *def, bool useValueNumbers);
    bool eliminateRedundancies();

    bool computeValueNumbers();

  public:
    ValueNumberer(MIRGraph &graph, bool optimistic);
    bool analyze();
};

} 
} 

#endif 

