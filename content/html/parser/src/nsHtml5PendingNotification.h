




































#ifndef nsHtml5PendingNotification_h__
#define nsHtml5PendingNotification_h__

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
    void Fire(nsHtml5TreeBuilder* aBuilder);
    inline PRBool Contains(nsIContent* aNode) {
      return !!(mParent == aNode);
    }
  private:
    nsIContent* mParent;
    PRUint32    mChildCount;    
};

#endif 