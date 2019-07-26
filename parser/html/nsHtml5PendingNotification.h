



#ifndef nsHtml5PendingNotification_h__
#define nsHtml5PendingNotification_h__

#include "nsNodeUtils.h"

class nsHtml5TreeBuilder;

class nsHtml5PendingNotification {
  public:

    nsHtml5PendingNotification(nsIContent* aParent)
     : mParent(aParent),
       mChildCount(aParent->GetChildCount() - 1)
    {
      MOZ_COUNT_CTOR(nsHtml5PendingNotification);
    }

    ~nsHtml5PendingNotification() {
      MOZ_COUNT_DTOR(nsHtml5PendingNotification);
    }

    inline void Fire() {
      nsNodeUtils::ContentAppended(mParent, mParent->GetChildAt(mChildCount),
                                   mChildCount);
    }

    inline bool Contains(nsIContent* aNode) {
      return !!(mParent == aNode);
    }
    
    inline bool HaveNotifiedIndex(uint32_t index) {
      return index < mChildCount;
    }

  private:
    


    nsIContent* mParent;

    


    uint32_t    mChildCount;
};

#endif 
