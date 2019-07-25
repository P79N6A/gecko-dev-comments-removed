




#ifndef nsTemplateMatch_h__
#define nsTemplateMatch_h__

#include "nsFixedSizeAllocator.h"
#include "nsIContent.h"
#include "nsIXULTemplateQueryProcessor.h"
#include "nsIXULTemplateResult.h"
#include "nsRuleNetwork.h"
#include NEW_H


















class nsTemplateRule;
class nsTemplateQuerySet;

class nsTemplateMatch {
private:
    
    
    void* operator new(size_t) CPP_THROW_NEW { return 0; }
    void operator delete(void*, size_t) {}

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
    Create(nsFixedSizeAllocator& aPool,
           uint16_t aQuerySetPriority,
           nsIXULTemplateResult* aResult,
           nsIContent* aContainer) {
        void* place = aPool.Alloc(sizeof(nsTemplateMatch));
        return place ? ::new (place) nsTemplateMatch(aQuerySetPriority,
                                                     aResult, aContainer)
                     : nullptr; }

    static void Destroy(nsFixedSizeAllocator& aPool,
                        nsTemplateMatch*& aMatch,
                        bool aRemoveResult);

    
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

