





































#ifndef NSREFERENCEDELEMENT_H_
#define NSREFERENCEDELEMENT_H_

#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsIDocument.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"

class nsIURI;
class nsCycleCollectionCallback;


















class nsReferencedElement {
public:
  nsReferencedElement() {}
  ~nsReferencedElement() {
    Unlink();
    if (mPendingNotification) {
      mPendingNotification->Clear();
    }
  }

  


  nsIContent* get() { return mContent; }

  









  void Reset(nsIContent* aFrom, nsIURI* aURI, PRBool aWatch = PR_TRUE);
  



  void Unlink();

  void Traverse(nsCycleCollectionTraversalCallback* aCB);
  
protected:
  




  virtual void ContentChanged(nsIContent* aFrom, nsIContent* aTo) {
    mContent = aTo;
  }

  



  virtual PRBool IsPersistent() { return PR_FALSE; }
  
private:
  static PRBool Observe(nsIContent* aOldContent,
                        nsIContent* aNewContent, void* aData);

  class Notification : public nsRunnable {
  public:
    Notification(nsReferencedElement* aTarget, nsIContent* aFrom, nsIContent* aTo)
      : mTarget(aTarget), mFrom(aFrom), mTo(aTo) {}
    NS_IMETHOD Run() {
      if (mTarget) {
        mTarget->mPendingNotification = nsnull;
        mTarget->ContentChanged(mFrom, mTo);
      }
      return NS_OK;
    }
    void SetTo(nsIContent* aTo) { mTo = aTo; }
    void Clear() { mTarget = nsnull; mFrom = nsnull; mTo = nsnull; }
  private:
    nsReferencedElement* mTarget;
    nsCOMPtr<nsIContent> mFrom;
    nsCOMPtr<nsIContent> mTo;
  };
  friend class Notification;

  nsCOMPtr<nsIAtom>      mWatchID;
  nsCOMPtr<nsIDocument>  mWatchDocument;
  nsCOMPtr<nsIContent>   mContent;
  nsRefPtr<Notification> mPendingNotification;
};

#endif 
