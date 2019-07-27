





#ifndef NSREFERENCEDELEMENT_H_
#define NSREFERENCEDELEMENT_H_

#include "mozilla/Attributes.h"
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
    virtual void Clear() { mTarget = nullptr; }
    virtual ~Notification() {}
  protected:
    explicit Notification(nsReferencedElement* aTarget)
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

    NS_DECL_ISUPPORTS_INHERITED
    NS_IMETHOD Run() MOZ_OVERRIDE {
      if (mTarget) {
        mTarget->mPendingNotification = nullptr;
        mTarget->ElementChanged(mFrom, mTo);
      }
      return NS_OK;
    }
    virtual void SetTo(Element* aTo) { mTo = aTo; }
    virtual void Clear()
    {
      Notification::Clear(); mFrom = nullptr; mTo = nullptr;
    }
  protected:
    virtual ~ChangeNotification() {}

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

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
  private:
    virtual ~DocumentLoadNotification() {}

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
