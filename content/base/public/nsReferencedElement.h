





































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

  



  void HaveNewDocument(nsIDocument* aDocument, PRBool aWatch,
                       const nsString& aRef);
  
private:
  static PRBool Observe(nsIContent* aOldContent,
                        nsIContent* aNewContent, void* aData);

  class Notification : public nsISupports {
  public:
    virtual void SetTo(nsIContent* aTo) = 0;
    virtual void Clear() { mTarget = nsnull; }
    virtual ~Notification() {}
  protected:
    Notification(nsReferencedElement* aTarget)
      : mTarget(aTarget)
    {
      NS_PRECONDITION(aTarget, "Must have a target");
    }
    nsReferencedElement* mTarget;
  };

  class ChangeNotification : public nsRunnable,
                             public Notification
  {
  public:
    ChangeNotification(nsReferencedElement* aTarget, nsIContent* aFrom, nsIContent* aTo)
      : Notification(aTarget), mFrom(aFrom), mTo(aTo)
    {}
    virtual ~ChangeNotification() {}

    NS_DECL_ISUPPORTS_INHERITED
    NS_IMETHOD Run() {
      if (mTarget) {
        mTarget->mPendingNotification = nsnull;
        mTarget->ContentChanged(mFrom, mTo);
      }
      return NS_OK;
    }
    virtual void SetTo(nsIContent* aTo) { mTo = aTo; }
    virtual void Clear()
    {
      Notification::Clear(); mFrom = nsnull; mTo = nsnull;
    }
  protected:
    nsCOMPtr<nsIContent> mFrom;
    nsCOMPtr<nsIContent> mTo;
  };
  friend class ChangeNotification;

  class DocumentLoadNotification : public Notification,
                                   public nsIObserver
  {
  public:
    DocumentLoadNotification(nsReferencedElement* aTarget,
                             const nsString& aRef) :
      Notification(aTarget)
    {
      if (!mTarget->IsPersistent()) {
        mRef = aRef;
      }
    }
    virtual ~DocumentLoadNotification() {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
  private:
    virtual void SetTo(nsIContent* aTo) { }

    nsString mRef;
  };
  friend class DocumentLoadNotification;
  
  nsCOMPtr<nsIAtom>      mWatchID;
  nsCOMPtr<nsIDocument>  mWatchDocument;
  nsCOMPtr<nsIContent>   mContent;
  nsRefPtr<Notification> mPendingNotification;
};

#endif 
