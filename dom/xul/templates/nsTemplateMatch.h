




#ifndef nsTemplateMatch_h__
#define nsTemplateMatch_h__

#include "nsIContent.h"
#include "nsIXULTemplateQueryProcessor.h"
#include "nsIXULTemplateResult.h"
#include "nsRuleNetwork.h"


















class nsTemplateRule;
class nsTemplateQuerySet;

class nsTemplateMatch {
private:
    
    
    void* operator new(size_t) CPP_THROW_NEW { MOZ_ASSERT(0); return nullptr; }
    void operator delete(void*, size_t) { MOZ_ASSERT(0); }

public:
    nsTemplateMatch(uint16_t aQuerySetPriority,
                    nsIXULTemplateResult* aResult,
                    nsIContent* aContainer)
        : mRuleIndex(-1),
          mQuerySetPriority(aQuerySetPriority),
          mContainer(aContainer),
          mResult(aResult),
          mNext(nullptr)
    {
      MOZ_COUNT_CTOR(nsTemplateMatch);
    }

    ~nsTemplateMatch()
    {
      MOZ_COUNT_DTOR(nsTemplateMatch);
    }

    static nsTemplateMatch*
    Create(uint16_t aQuerySetPriority,
           nsIXULTemplateResult* aResult,
           nsIContent* aContainer) {
        return ::new nsTemplateMatch(aQuerySetPriority, aResult, aContainer);
    }

    static void Destroy(nsTemplateMatch*& aMatch, bool aRemoveResult);

    
    bool IsActive() {
        return mRuleIndex >= 0;
    }

    
    
    void SetInactive() {
        mRuleIndex = -1;
    }

    
    int16_t RuleIndex() {
        return mRuleIndex;
    }

    
    uint16_t QuerySetPriority() {
        return mQuerySetPriority;
    }

    
    nsIContent* GetContainer() {
        return mContainer;
    }

    nsresult RuleMatched(nsTemplateQuerySet* aQuerySet,
                         nsTemplateRule* aRule,
                         int16_t aRuleIndex,
                         nsIXULTemplateResult* aResult);

private:

    


    int16_t mRuleIndex;

    


    uint16_t mQuerySetPriority;

    


    nsCOMPtr<nsIContent> mContainer;

public:

    


    nsCOMPtr<nsIXULTemplateResult> mResult;

    





    nsTemplateMatch *mNext;

private:

    nsTemplateMatch(const nsTemplateMatch& aMatch); 
    void operator=(const nsTemplateMatch& aMatch); 
};

#endif 

