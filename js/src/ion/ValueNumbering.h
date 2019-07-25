








































#ifndef jsion_value_numbering_h__
#define jsion_value_numbering_h__

#include "MIR.h"
#include "MIRGraph.h"
#include "CompileInfo.h"

namespace js {
namespace ion {

class ValueNumberer
{
  protected:
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

  protected:
    uint32 lookupValue(MDefinition *ins);
    MDefinition *findDominatingDef(InstructionMap &defs, MDefinition *ins, size_t index);

    MDefinition *simplify(MDefinition *def, bool useValueNumbers);
    bool eliminateRedundancies();

    bool computeValueNumbers();

    inline bool isMarked(MDefinition *def) {
        return pessimisticPass_ || def->isInWorklist();
    }

    void markDefinition(MDefinition *def);
    void unmarkDefinition(MDefinition *def);

    void markConsumers(MDefinition *def);
    void markBlock(MBasicBlock *block);

  protected:
    MIRGraph &graph_;
    ValueMap values;
    bool pessimisticPass_;
    size_t count_;

  public:
    ValueNumberer(MIRGraph &graph, bool optimistic);
    bool analyze();
};

} 
} 

#endif 

