




































#ifndef nsHtml5PendingNotification_h__
#define nsHtml5PendingNotification_h__

#include "nsNodeUtils.h"

class nsHtml5TreeBuilder;

class nsHtml5PendingNotification {
  public:

    nsHtml5PendingNotification(nsIContent* aParent)
     : mParent(aParent),
       mChildCount(aParent->GetChildCount())
    {
      MOZ_COUNT_CTOR(nsHtml5PendingNotification);
    }

    ~nsHtml5PendingNotification() {
      MOZ_COUNT_DTOR(nsHtml5PendingNotification);
    }

    inline void Fire() {
      nsNodeUtils::ContentAppended(mParent, mChildCount);
    }

    inline PRBool Contains(nsIContent* aNode) {
      return !!(mParent == aNode);
    }
    
    inline PRBool HaveNotifiedIndex(PRUint32 index) {
      return index < mChildCount;
    }

  private:
    


    nsIContent* mParent;

    


    PRUint32    mChildCount;
};

#endif 
