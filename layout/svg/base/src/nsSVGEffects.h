




































#ifndef NSSVGEFFECTS_H_
#define NSSVGEFFECTS_H_

#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsReferencedElement.h"
#include "nsStubMutationObserver.h"
#include "nsSVGUtils.h"
#include "nsInterfaceHashtable.h"
#include "nsURIHashKey.h"

class nsSVGClipPathFrame;
class nsSVGFilterFrame;
class nsSVGMaskFrame;












class nsSVGRenderingObserver : public nsStubMutationObserver {
public:
  typedef mozilla::dom::Element Element;
  nsSVGRenderingObserver()
    : mInObserverList(PR_FALSE)
    {}
  virtual ~nsSVGRenderingObserver()
    {}

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  void InvalidateViaReferencedElement();

  
  
  
  void NotifyEvictedFromRenderingObserverList();

  nsIFrame* GetReferencedFrame();
  PRBool IsInObserverList() const { return mInObserverList; }

  



  nsIFrame* GetReferencedFrame(nsIAtom* aFrameType, PRBool* aOK);

  Element* GetReferencedElement();

protected:
  
  void StartListening();
  void StopListening();

  
  virtual void DoUpdate() = 0; 

  
  
  virtual Element* GetTarget() = 0;

  
  PRPackedBool mInObserverList;
};












class nsSVGIDRenderingObserver : public nsSVGRenderingObserver {
public:
  typedef mozilla::dom::Element Element;
  nsSVGIDRenderingObserver(nsIURI* aURI, nsIFrame *aFrame,
                         PRBool aReferenceImage);
  virtual ~nsSVGIDRenderingObserver();

protected:
  Element* GetTarget() { return mElement.get(); }

  
  virtual void DoUpdate();

  class SourceReference : public nsReferencedElement {
  public:
    SourceReference(nsSVGIDRenderingObserver* aContainer) : mContainer(aContainer) {}
  protected:
    virtual void ElementChanged(Element* aFrom, Element* aTo) {
      mContainer->StopListening();
      nsReferencedElement::ElementChanged(aFrom, aTo);
      mContainer->StartListening();
      mContainer->DoUpdate();
    }
    



    virtual PRBool IsPersistent() { return PR_TRUE; }
  private:
    nsSVGIDRenderingObserver* mContainer;
  };
  
  SourceReference mElement;
  
   nsIFrame *mFrame;
  
  
  
  
  
  nsIPresShell *mFramePresShell;
};

class nsSVGFilterProperty :
  public nsSVGIDRenderingObserver, public nsISVGFilterProperty {
public:
  nsSVGFilterProperty(nsIURI *aURI, nsIFrame *aFilteredFrame,
                      PRBool aReferenceImage)
    : nsSVGIDRenderingObserver(aURI, aFilteredFrame, aReferenceImage) {}

  


  nsSVGFilterFrame *GetFilterFrame();

  
  NS_DECL_ISUPPORTS

  
  virtual void Invalidate() { DoUpdate(); }

private:
  
  virtual void DoUpdate();
};

class nsSVGMarkerProperty : public nsSVGIDRenderingObserver {
public:
  nsSVGMarkerProperty(nsIURI *aURI, nsIFrame *aFrame, PRBool aReferenceImage)
    : nsSVGIDRenderingObserver(aURI, aFrame, aReferenceImage) {}

protected:
  virtual void DoUpdate();
};

class nsSVGTextPathProperty : public nsSVGIDRenderingObserver {
public:
  nsSVGTextPathProperty(nsIURI *aURI, nsIFrame *aFrame, PRBool aReferenceImage)
    : nsSVGIDRenderingObserver(aURI, aFrame, aReferenceImage) {}

protected:
  virtual void DoUpdate();
};
 
class nsSVGPaintingProperty : public nsSVGIDRenderingObserver {
public:
  nsSVGPaintingProperty(nsIURI *aURI, nsIFrame *aFrame, PRBool aReferenceImage)
    : nsSVGIDRenderingObserver(aURI, aFrame, aReferenceImage) {}

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
#ifdef DEBUG
  PRBool Contains(nsSVGRenderingObserver* aObserver)
  { return (mObservers.GetEntry(aObserver) != nsnull); }
#endif
  PRBool IsEmpty()
  { return mObservers.Count() == 0; }

  



  void InvalidateAll();

  



  void RemoveAll();

private:
  nsTHashtable<nsVoidPtrHashKey> mObservers;
};

class nsSVGEffects {
public:
  typedef mozilla::dom::Element Element;
  typedef mozilla::FramePropertyDescriptor FramePropertyDescriptor;
  typedef nsInterfaceHashtable<nsURIHashKey, nsIMutationObserver>
    URIObserverHashtable;

  static void DestroySupports(void* aPropertyValue)
  {
    (static_cast<nsISupports*>(aPropertyValue))->Release();
  }

  static void DestroyHashtable(void* aPropertyValue)
  {
    delete static_cast<URIObserverHashtable*> (aPropertyValue);
  }

  NS_DECLARE_FRAME_PROPERTY(FilterProperty, DestroySupports)
  NS_DECLARE_FRAME_PROPERTY(MaskProperty, DestroySupports)
  NS_DECLARE_FRAME_PROPERTY(ClipPathProperty, DestroySupports)
  NS_DECLARE_FRAME_PROPERTY(MarkerBeginProperty, DestroySupports)
  NS_DECLARE_FRAME_PROPERTY(MarkerMiddleProperty, DestroySupports)
  NS_DECLARE_FRAME_PROPERTY(MarkerEndProperty, DestroySupports)
  NS_DECLARE_FRAME_PROPERTY(FillProperty, DestroySupports)
  NS_DECLARE_FRAME_PROPERTY(StrokeProperty, DestroySupports)
  NS_DECLARE_FRAME_PROPERTY(HrefProperty, DestroySupports)
  NS_DECLARE_FRAME_PROPERTY(BackgroundImageProperty, DestroyHashtable)

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

  


  static void AddRenderingObserver(Element *aElement, nsSVGRenderingObserver *aObserver);
  


  static void RemoveRenderingObserver(Element *aElement, nsSVGRenderingObserver *aObserver);

  


  static void RemoveAllRenderingObservers(Element *aElement);

  














  static void InvalidateRenderingObservers(nsIFrame *aFrame);
  



  static void InvalidateDirectRenderingObservers(Element *aElement);
  static void InvalidateDirectRenderingObservers(nsIFrame *aFrame);

  


  static nsSVGMarkerProperty *
  GetMarkerProperty(nsIURI *aURI, nsIFrame *aFrame,
                    const FramePropertyDescriptor *aProperty);
  


  static nsSVGTextPathProperty *
  GetTextPathProperty(nsIURI *aURI, nsIFrame *aFrame,
                      const FramePropertyDescriptor *aProperty);
  


  static nsSVGPaintingProperty *
  GetPaintingProperty(nsIURI *aURI, nsIFrame *aFrame,
                      const FramePropertyDescriptor *aProperty);
  



  static nsSVGPaintingProperty *
  GetPaintingPropertyForURI(nsIURI *aURI, nsIFrame *aFrame,
                            const FramePropertyDescriptor *aProp);
};

#endif 
