




#ifndef NSSVGEFFECTS_H_
#define NSSVGEFFECTS_H_

#include "mozilla/Attributes.h"
#include "FramePropertyTable.h"
#include "mozilla/dom/Element.h"
#include "nsHashKeys.h"
#include "nsID.h"
#include "nsIFrame.h"
#include "nsIMutationObserver.h"
#include "nsInterfaceHashtable.h"
#include "nsISupportsBase.h"
#include "nsISupportsImpl.h"
#include "nsReferencedElement.h"
#include "nsStubMutationObserver.h"
#include "nsSVGUtils.h"
#include "nsTHashtable.h"
#include "nsURIHashKey.h"

class nsIAtom;
class nsIPresShell;
class nsIURI;
class nsSVGClipPathFrame;
class nsSVGPaintServerFrame;
class nsSVGFilterFrame;
class nsSVGMaskFrame;
class nsSVGFilterChainObserver;












class nsSVGRenderingObserver : public nsStubMutationObserver {

protected:
  virtual ~nsSVGRenderingObserver()
    {}

public:
  typedef mozilla::dom::Element Element;
  nsSVGRenderingObserver()
    : mInObserverList(false)
    {}

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  void InvalidateViaReferencedElement();

  
  
  
  void NotifyEvictedFromRenderingObserverList();

  bool IsInObserverList() const { return mInObserverList; }

  nsIFrame* GetReferencedFrame();
  



  nsIFrame* GetReferencedFrame(nsIAtom* aFrameType, bool* aOK);

  Element* GetReferencedElement();

  virtual bool ObservesReflow() { return true; }

protected:
  
  void StartListening();
  void StopListening();

  
  virtual void DoUpdate() = 0; 

  
  
  virtual Element* GetTarget() = 0;

  
  bool mInObserverList;
};












class nsSVGIDRenderingObserver : public nsSVGRenderingObserver {
public:
  typedef mozilla::dom::Element Element;
  nsSVGIDRenderingObserver(nsIURI* aURI, nsIContent* aObservingContent,
                         bool aReferenceImage);
  virtual ~nsSVGIDRenderingObserver();

protected:
  Element* GetTarget() MOZ_OVERRIDE { return mElement.get(); }

  
  virtual void DoUpdate() MOZ_OVERRIDE;

  class SourceReference : public nsReferencedElement {
  public:
    explicit SourceReference(nsSVGIDRenderingObserver* aContainer) : mContainer(aContainer) {}
  protected:
    virtual void ElementChanged(Element* aFrom, Element* aTo) MOZ_OVERRIDE {
      mContainer->StopListening();
      nsReferencedElement::ElementChanged(aFrom, aTo);
      mContainer->StartListening();
      mContainer->DoUpdate();
    }
    



    virtual bool IsPersistent() MOZ_OVERRIDE { return true; }
  private:
    nsSVGIDRenderingObserver* mContainer;
  };

  SourceReference mElement;
};

struct nsSVGFrameReferenceFromProperty
{
  nsSVGFrameReferenceFromProperty(nsIFrame* aFrame)
    : mFrame(aFrame)
    , mFramePresShell(aFrame->PresContext()->PresShell())
  {}

  
  void Detach();

  
  nsIFrame* Get();

private:
  
  nsIFrame *mFrame;
  
  
  
  
  
  
  
  nsIPresShell *mFramePresShell;
};

class nsSVGRenderingObserverProperty : public nsSVGIDRenderingObserver {
public:
  nsSVGRenderingObserverProperty(nsIURI* aURI, nsIFrame *aFrame,
                                 bool aReferenceImage)
    : nsSVGIDRenderingObserver(aURI, aFrame->GetContent(), aReferenceImage)
    , mFrameReference(aFrame)
  {}

protected:
  virtual void DoUpdate() MOZ_OVERRIDE;

  nsSVGFrameReferenceFromProperty mFrameReference;
};













class nsSVGFilterReference MOZ_FINAL :
  public nsSVGIDRenderingObserver, public nsISVGFilterReference {
public:
  nsSVGFilterReference(nsIURI* aURI,
                       nsIContent* aObservingContent,
                       nsSVGFilterChainObserver* aFilterChainObserver)
    : nsSVGIDRenderingObserver(aURI, aObservingContent, false)
    , mFilterChainObserver(aFilterChainObserver)
  {
  }

  bool ReferencesValidResource() { return GetFilterFrame(); }

  void DetachFromChainObserver() { mFilterChainObserver = nullptr; }

  


  nsSVGFilterFrame *GetFilterFrame();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Invalidate() MOZ_OVERRIDE { DoUpdate(); };

protected:
  virtual ~nsSVGFilterReference() {}

  
  virtual void DoUpdate() MOZ_OVERRIDE;

private:
  nsSVGFilterChainObserver* mFilterChainObserver;
};











class nsSVGFilterChainObserver : public nsISupports {
public:
  nsSVGFilterChainObserver(const nsTArray<nsStyleFilter>& aFilters,
                           nsIContent* aFilteredElement);

  bool ReferencesValidResources();
  bool IsInObserverLists() const;
  void Invalidate() { DoUpdate(); }

  
  NS_DECL_ISUPPORTS

protected:
  virtual ~nsSVGFilterChainObserver();

  virtual void DoUpdate() = 0;

private:
  nsTArray<nsRefPtr<nsSVGFilterReference>> mReferences;
};

class nsSVGFilterProperty : public nsSVGFilterChainObserver {
public:
  nsSVGFilterProperty(const nsTArray<nsStyleFilter> &aFilters,
                      nsIFrame *aFilteredFrame)
    : nsSVGFilterChainObserver(aFilters, aFilteredFrame->GetContent())
    , mFrameReference(aFilteredFrame)
  {}

protected:
  virtual void DoUpdate() MOZ_OVERRIDE;

  nsSVGFrameReferenceFromProperty mFrameReference;
};

class nsSVGMarkerProperty : public nsSVGRenderingObserverProperty {
public:
  nsSVGMarkerProperty(nsIURI *aURI, nsIFrame *aFrame, bool aReferenceImage)
    : nsSVGRenderingObserverProperty(aURI, aFrame, aReferenceImage) {}

protected:
  virtual void DoUpdate() MOZ_OVERRIDE;
};

class nsSVGTextPathProperty : public nsSVGRenderingObserverProperty {
public:
  nsSVGTextPathProperty(nsIURI *aURI, nsIFrame *aFrame, bool aReferenceImage)
    : nsSVGRenderingObserverProperty(aURI, aFrame, aReferenceImage)
    , mValid(true) {}

  virtual bool ObservesReflow() MOZ_OVERRIDE { return false; }

protected:
  virtual void DoUpdate() MOZ_OVERRIDE;

private:
  


  bool TargetIsValid();

  bool mValid;
};

class nsSVGPaintingProperty : public nsSVGRenderingObserverProperty {
public:
  nsSVGPaintingProperty(nsIURI *aURI, nsIFrame *aFrame, bool aReferenceImage)
    : nsSVGRenderingObserverProperty(aURI, aFrame, aReferenceImage) {}

protected:
  virtual void DoUpdate() MOZ_OVERRIDE;
};

















class nsSVGRenderingObserverList {
public:
  nsSVGRenderingObserverList()
    : mObservers(4)
  {
    MOZ_COUNT_CTOR(nsSVGRenderingObserverList);
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
  bool Contains(nsSVGRenderingObserver* aObserver)
  { return (mObservers.GetEntry(aObserver) != nullptr); }
#endif
  bool IsEmpty()
  { return mObservers.Count() == 0; }

  



  void InvalidateAll();

  



  void InvalidateAllForReflow();

  



  void RemoveAll();

private:
  nsTHashtable<nsPtrHashKey<nsSVGRenderingObserver> > mObservers;
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

  


  static nsSVGPaintServerFrame *GetPaintServer(nsIFrame *aTargetFrame,
                                               const nsStyleSVGPaint *aPaint,
                                               const FramePropertyDescriptor *aProperty);

  struct EffectProperties {
    nsSVGFilterProperty*   mFilter;
    nsSVGPaintingProperty* mMask;
    nsSVGPaintingProperty* mClipPath;

    





    nsSVGClipPathFrame *GetClipPathFrame(bool *aOK);
    





    nsSVGMaskFrame *GetMaskFrame(bool *aOK);

    bool HasValidFilter() {
      return mFilter && mFilter->ReferencesValidResources();
    }

    bool HasNoFilterOrHasValidFilter() {
      return !mFilter || mFilter->ReferencesValidResources();
    }
  };

  


  static EffectProperties GetEffectProperties(nsIFrame *aFrame);

  












  static void UpdateEffects(nsIFrame *aFrame);

  


  static nsSVGFilterProperty *GetFilterProperty(nsIFrame *aFrame);

  


  static void AddRenderingObserver(Element *aElement, nsSVGRenderingObserver *aObserver);
  


  static void RemoveRenderingObserver(Element *aElement, nsSVGRenderingObserver *aObserver);

  


  static void RemoveAllRenderingObservers(Element *aElement);

  














  static void InvalidateRenderingObservers(nsIFrame *aFrame);

  enum {
    INVALIDATE_REFLOW = 1
  };

  



  static void InvalidateDirectRenderingObservers(Element *aElement, uint32_t aFlags = 0);
  static void InvalidateDirectRenderingObservers(nsIFrame *aFrame, uint32_t aFlags = 0);

  


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
