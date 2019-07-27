





#include "compiler/translator/RegenerateStructNames.h"
#include "compiler/translator/compilerdebug.h"

void RegenerateStructNames::visitSymbol(TIntermSymbol *symbol)
{
    ASSERT(symbol);
    TType *type = symbol->getTypePointer();
    ASSERT(type);
    TStructure *userType = type->getStruct();
    if (!userType)
        return;

    if (mSymbolTable.findBuiltIn(userType->name(), mShaderVersion))
    {
        
        return;
    }

    int uniqueId = userType->uniqueId();

    ASSERT(mScopeDepth > 0);
    if (mScopeDepth == 1)
    {
        
        
        
        
        
        
        
        
        
        
        
        
        
        mDeclaredGlobalStructs.insert(uniqueId);
        return;
    }
    if (mDeclaredGlobalStructs.count(uniqueId) > 0)
        return;
    
    const char kPrefix[] = "_webgl_struct_";
    if (userType->name().find(kPrefix) == 0)
    {
        
        return;
    }
    std::string id = Str(uniqueId);
    TString tmp = kPrefix + TString(id.c_str());
    tmp += "_" + userType->name();
    userType->setName(tmp);
}

bool RegenerateStructNames::visitAggregate(Visit, TIntermAggregate *aggregate)
{
    ASSERT(aggregate);
    switch (aggregate->getOp())
    {
      case EOpSequence:
        ++mScopeDepth;
        {
            TIntermSequence &sequence = *(aggregate->getSequence());
            for (size_t ii = 0; ii < sequence.size(); ++ii)
            {
                TIntermNode *node = sequence[ii];
                ASSERT(node != NULL);
                node->traverse(this);
            }
        }
        --mScopeDepth;
        return false;
      default:
        return true;
    }
}
