





































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
    nsTemplateMatch(PRUint16 aQuerySetPriority,
                    nsIXULTemplateResult* aResult,
                    nsIContent* aContainer)
        : mRuleIndex(-1),
          mQuerySetPriority(aQuerySetPriority),
          mContainer(aContainer),
          mResult(aResult),
          mNext(nsnull) {}

    ~nsTemplateMatch() {}

    static nsTemplateMatch*
    Create(nsFixedSizeAllocator& aPool,
           PRUint16 aQuerySetPriority,
           nsIXULTemplateResult* aResult,
           nsIContent* aContainer) {
        void* place = aPool.Alloc(sizeof(nsTemplateMatch));
        return place ? ::new (place) nsTemplateMatch(aQuerySetPriority,
                                                     aResult, aContainer)
                     : nsnull; }

    static void Destroy(nsFixedSizeAllocator& aPool,
                        nsTemplateMatch*& aMatch,
                        PRBool aRemoveResult);

    
    PRBool IsActive() {
        return mRuleIndex >= 0;
    }

    
    
    void SetInactive() {
        mRuleIndex = -1;
    }

    
    PRInt16 RuleIndex() {
        return mRuleIndex;
    }

    
    PRUint16 QuerySetPriority() {
        return mQuerySetPriority;
    }

    
    nsIContent* GetContainer() {
        return mContainer;
    }

    nsresult RuleMatched(nsTemplateQuerySet* aQuerySet,
                         nsTemplateRule* aRule,
                         PRInt16 aRuleIndex,
                         nsIXULTemplateResult* aResult);

private:

    


    PRInt16 mRuleIndex;

    


    PRUint16 mQuerySetPriority;

    


    nsCOMPtr<nsIContent> mContainer;

public:

    


    nsCOMPtr<nsIXULTemplateResult> mResult;

    





    nsTemplateMatch *mNext;

private:

    nsTemplateMatch(const nsTemplateMatch& aMatch); 
    void operator=(const nsTemplateMatch& aMatch); 
};

#endif 

