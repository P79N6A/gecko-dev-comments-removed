





#ifndef jit_ValueNumbering_h
#define jit_ValueNumbering_h

#include "jit/MIR.h"

namespace js {
namespace jit {

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
            
            
            
            if (k->dependency() != l->dependency())
                return false;
            return k->congruentTo(l);
        }
    };

    typedef HashMap<MDefinition *,
                    uint32_t,
                    ValueHasher,
                    IonAllocPolicy> ValueMap;

    struct DominatingValue
    {
        MDefinition *def;
        uint32_t validEnd;
    };

    typedef HashMap<uint32_t,
                    DominatingValue,
                    DefaultHasher<uint32_t>,
                    IonAllocPolicy> InstructionMap;

  protected:
    TempAllocator &alloc() const;
    uint32_t lookupValue(MDefinition *ins);
    MDefinition *findDominatingDef(InstructionMap &defs, MDefinition *ins, size_t index);

    MDefinition *simplify(MDefinition *def, bool useValueNumbers);
    MControlInstruction *simplifyControlInstruction(MControlInstruction *def);
    bool eliminateRedundancies();

    bool computeValueNumbers();

    inline bool isMarked(MDefinition *def) {
        return pessimisticPass_ || def->isInWorklist();
    }

    void markDefinition(MDefinition *def);
    void unmarkDefinition(MDefinition *def);

    void markConsumers(MDefinition *def);
    void markBlock(MBasicBlock *block);
    void setClass(MDefinition *toSet, MDefinition *representative);

  public:
    static MDefinition *findSplit(MDefinition *);
    void breakClass(MDefinition*);

  protected:
    MIRGenerator *mir;
    MIRGraph &graph_;
    ValueMap values;
    bool pessimisticPass_;
    size_t count_;

  public:
    ValueNumberer(MIRGenerator *mir, MIRGraph &graph, bool optimistic);
    bool analyze();
    bool clear();
};

class ValueNumberData : public TempObject {

    friend void ValueNumberer::breakClass(MDefinition*);
    friend MDefinition *ValueNumberer::findSplit(MDefinition*);
    uint32_t number;
    MDefinition *classNext;
    MDefinition *classPrev;

  public:
    ValueNumberData() : number(0), classNext(nullptr), classPrev(nullptr) {}

    void setValueNumber(uint32_t number_) {
        number = number_;
    }

    uint32_t valueNumber() {
        return number;
    }
    
    void setClass(MDefinition *thisDef, MDefinition *rep) {
        JS_ASSERT(thisDef->valueNumberData() == this);
        
        
        
        
        
        
        if (this == rep->valueNumberData())
            return;

        if (classNext)
            classNext->valueNumberData()->classPrev = classPrev;
        if (classPrev)
            classPrev->valueNumberData()->classNext = classNext;


        classPrev = rep;
        classNext = rep->valueNumberData()->classNext;

        if (rep->valueNumberData()->classNext)
            rep->valueNumberData()->classNext->valueNumberData()->classPrev = thisDef;
        rep->valueNumberData()->classNext = thisDef;
    }
};
} 
} 

#endif 
