





































#ifndef NSREFERENCEDELEMENT_H_
#define NSREFERENCEDELEMENT_H_

#include "mozilla/dom/Element.h"
#include "nsIAtom.h"
#include "nsIDocument.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"

class nsIURI;
class nsCycleCollectionCallback;


















class nsReferencedElement {
public:
  typedef mozilla::dom::Element Element;

  nsReferencedElement() {}
  ~nsReferencedElement() {
    Unlink();
  }

  


  Element* get() { return mElement; }

  











  void Reset(nsIContent* aFrom, nsIURI* aURI, bool aWatch = true,
             bool aReferenceImage = false);

  








  void ResetWithID(nsIContent* aFrom, const nsString& aID,
                   bool aWatch = true);

  



  void Unlink();

  void Traverse(nsCycleCollectionTraversalCallback* aCB);
  
protected:
  




  virtual void ElementChanged(Element* aFrom, Element* aTo) {
    mElement = aTo;
  }

  



  virtual bool IsPersistent() { return false; }

  



  void HaveNewDocument(nsIDocument* aDocument, bool aWatch,
                       const nsString& aRef);
  
private:
  static bool Observe(Element* aOldElement,
                        Element* aNewElement, void* aData);

  class Notification : public nsISupports {
  public:
    virtual void SetTo(Element* aTo) = 0;
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
    ChangeNotification(nsReferencedElement* aTarget,
                       Element* aFrom, Element* aTo)
      : Notification(aTarget), mFrom(aFrom), mTo(aTo)
    {}
    virtual ~ChangeNotification() {}

    NS_DECL_ISUPPORTS_INHERITED
    NS_IMETHOD Run() {
      if (mTarget) {
        mTarget->mPendingNotification = nsnull;
        mTarget->ElementChanged(mFrom, mTo);
      }
      return NS_OK;
    }
    virtual void SetTo(Element* aTo) { mTo = aTo; }
    virtual void Clear()
    {
      Notification::Clear(); mFrom = nsnull; mTo = nsnull;
    }
  protected:
    nsRefPtr<Element> mFrom;
    nsRefPtr<Element> mTo;
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
    virtual void SetTo(Element* aTo) { }

    nsString mRef;
  };
  friend class DocumentLoadNotification;
  
  nsCOMPtr<nsIAtom>      mWatchID;
  nsCOMPtr<nsIDocument>  mWatchDocument;
  nsRefPtr<Element> mElement;
  nsRefPtr<Notification> mPendingNotification;
  bool                   mReferencingImage;
};

#endif 
