




































#ifndef NSSVGEFFECTS_H_
#define NSSVGEFFECTS_H_

#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsReferencedElement.h"
#include "nsStubMutationObserver.h"
#include "nsSVGUtils.h"
#include "nsTHashtable.h"

class nsSVGClipPathFrame;
class nsSVGFilterFrame;
class nsSVGMaskFrame;











class nsSVGRenderingObserver : public nsStubMutationObserver {
public:
  nsSVGRenderingObserver(nsIURI* aURI, nsIFrame *aFrame);
  virtual ~nsSVGRenderingObserver();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  void InvalidateViaReferencedFrame();

  nsIFrame* GetReferencedFrame();
  



  nsIFrame* GetReferencedFrame(nsIAtom* aFrameType, PRBool* aOK);

protected:
  
  virtual void DoUpdate();

  class SourceReference : public nsReferencedElement {
  public:
    SourceReference(nsSVGRenderingObserver* aContainer) : mContainer(aContainer) {}
  protected:
    virtual void ContentChanged(nsIContent* aFrom, nsIContent* aTo) {
      if (aFrom) {
        aFrom->RemoveMutationObserver(mContainer);
      }
      nsReferencedElement::ContentChanged(aFrom, aTo);
      if (aTo) {
        aTo->AddMutationObserver(mContainer);
      }
      mContainer->DoUpdate();
    }
    



    virtual PRBool IsPersistent() { return PR_TRUE; }
  private:
    nsSVGRenderingObserver* mContainer;
  };
  
  SourceReference mElement;
  
   nsIFrame *mFrame;
  
  
  
  
  
  nsIPresShell *mFramePresShell;
  nsIFrame *mReferencedFrame;
  nsIPresShell *mReferencedFramePresShell;
};

class nsSVGFilterProperty :
  public nsSVGRenderingObserver, public nsISVGFilterProperty {
public:
  nsSVGFilterProperty(nsIURI *aURI, nsIFrame *aFilteredFrame)
    : nsSVGRenderingObserver(aURI, aFilteredFrame) {}

  


  nsSVGFilterFrame *GetFilterFrame();

  
  NS_DECL_ISUPPORTS

  
  virtual void Invalidate() { DoUpdate(); }

private:
  
  virtual void DoUpdate();
};

class nsSVGMarkerProperty : public nsSVGRenderingObserver {
public:
  nsSVGMarkerProperty(nsIURI *aURI, nsIFrame *aFrame)
    : nsSVGRenderingObserver(aURI, aFrame) {}

protected:
  virtual void DoUpdate();
};

class nsSVGTextPathProperty : public nsSVGRenderingObserver {
public:
  nsSVGTextPathProperty(nsIURI *aURI, nsIFrame *aFrame)
    : nsSVGRenderingObserver(aURI, aFrame) {}

protected:
  virtual void DoUpdate();
};
 
class nsSVGPaintingProperty : public nsSVGRenderingObserver {
public:
  nsSVGPaintingProperty(nsIURI *aURI, nsIFrame *aFrame)
    : nsSVGRenderingObserver(aURI, aFrame) {}

protected:
  virtual void DoUpdate();
};
















class nsSVGRenderingObserverList {
public:
  nsSVGRenderingObserverList() {
    MOZ_COUNT_CTOR(nsSVGRenderingObserverList);
    mObservers.Init(5);
  }

  ~nsSVGRenderingObserverList() {
    InvalidateAll();
    MOZ_COUNT_DTOR(nsSVGRenderingObserverList);
  }

  void Add(nsSVGRenderingObserver* aObserver)
  { mObservers.PutEntry(aObserver); }
  void Remove(nsSVGRenderingObserver* aObserver)
  { mObservers.RemoveEntry(aObserver); }
  void InvalidateAll();

private:
  nsTHashtable<nsVoidPtrHashKey> mObservers;
};

class nsSVGEffects {
public:
  struct EffectProperties {
    nsSVGFilterProperty*   mFilter;
    nsSVGPaintingProperty* mMask;
    nsSVGPaintingProperty* mClipPath;

    





    nsSVGClipPathFrame *GetClipPathFrame(PRBool *aOK);
    





    nsSVGMaskFrame *GetMaskFrame(PRBool *aOK);
    





    nsSVGFilterFrame *GetFilterFrame(PRBool *aOK) {
      if (!mFilter)
        return nsnull;
      nsSVGFilterFrame *filter = mFilter->GetFilterFrame();
      if (!filter) {
        *aOK = PR_FALSE;
      }
      return filter;
    }
  };

  


  static EffectProperties GetEffectProperties(nsIFrame *aFrame);
  



  static void UpdateEffects(nsIFrame *aFrame);
  


  static nsSVGFilterProperty *GetFilterProperty(nsIFrame *aFrame);
  static nsSVGFilterFrame *GetFilterFrame(nsIFrame *aFrame) {
    nsSVGFilterProperty *prop = GetFilterProperty(aFrame);
    return prop ? prop->GetFilterFrame() : nsnull;
  }

  


  static void AddRenderingObserver(nsIFrame *aFrame, nsSVGRenderingObserver *aObserver);
  


  static void RemoveRenderingObserver(nsIFrame *aFrame, nsSVGRenderingObserver *aObserver);
  



  static void InvalidateRenderingObservers(nsIFrame *aFrame);
  



  static void InvalidateDirectRenderingObservers(nsIFrame *aFrame);

  


  static nsSVGMarkerProperty *
  GetMarkerProperty(nsIURI *aURI, nsIFrame *aFrame, nsIAtom *aProp);
  


  static nsSVGTextPathProperty *
  GetTextPathProperty(nsIURI *aURI, nsIFrame *aFrame, nsIAtom *aProp);
  


  static nsSVGPaintingProperty *
  GetPaintingProperty(nsIURI *aURI, nsIFrame *aFrame, nsIAtom *aProp);
};

#endif 
