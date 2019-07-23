




































#ifndef NSSVGEFFECTS_H_
#define NSSVGEFFECTS_H_

#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsReferencedElement.h"
#include "nsStubMutationObserver.h"
#include "nsSVGUtils.h"
#include "nsSVGFilterFrame.h"
#include "nsSVGClipPathFrame.h"
#include "nsSVGMaskFrame.h"

class nsSVGPropertyBase : public nsStubMutationObserver {
public:
  nsSVGPropertyBase(nsIURI* aURI, nsIFrame *aFrame);
  virtual ~nsSVGPropertyBase();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

protected:
  
  virtual void DoUpdate() = 0;

  class SourceReference : public nsReferencedElement {
  public:
    SourceReference(nsSVGPropertyBase* aContainer) : mContainer(aContainer) {}
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
    nsSVGPropertyBase* mContainer;
  };
  
  



  nsIFrame* GetReferencedFrame(nsIAtom* aFrameType, PRBool* aOK);

  SourceReference mElement;
  nsIFrame *mFrame;
};

class nsSVGFilterProperty :
  public nsSVGPropertyBase, public nsISVGFilterProperty {
public:
  nsSVGFilterProperty(nsIURI *aURI, nsIFrame *aFilteredFrame);

  nsSVGFilterFrame *GetFilterFrame(PRBool *aOK) {
    return static_cast<nsSVGFilterFrame *>
      (GetReferencedFrame(nsGkAtoms::svgFilterFrame, aOK));
  }
  void UpdateRect();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIMUTATIONOBSERVER_PARENTCHAINCHANGED

  
  virtual void Invalidate() { DoUpdate(); }

private:
  
  virtual void DoUpdate();
  
  
  
  
  
  nsRect mFilterRect;
};

class nsSVGClipPathProperty : public nsSVGPropertyBase {
public:
  nsSVGClipPathProperty(nsIURI *aURI, nsIFrame *aClippedFrame)
    : nsSVGPropertyBase(aURI, aClippedFrame) {}

  nsSVGClipPathFrame *GetClipPathFrame(PRBool *aOK) {
    return static_cast<nsSVGClipPathFrame *>
      (GetReferencedFrame(nsGkAtoms::svgClipPathFrame, aOK));
  }

  
  NS_DECL_NSIMUTATIONOBSERVER_PARENTCHAINCHANGED

private:
  virtual void DoUpdate();
};

class nsSVGMaskProperty : public nsSVGPropertyBase {
public:
  nsSVGMaskProperty(nsIURI *aURI, nsIFrame *aMaskedFrame)
    : nsSVGPropertyBase(aURI, aMaskedFrame) {}

  nsSVGMaskFrame *GetMaskFrame(PRBool *aOK) {
    return static_cast<nsSVGMaskFrame *>
      (GetReferencedFrame(nsGkAtoms::svgMaskFrame, aOK));
  }

  
  NS_DECL_NSIMUTATIONOBSERVER_PARENTCHAINCHANGED

private:
  virtual void DoUpdate();
};

class nsSVGEffects {
public:
  struct EffectProperties {
    nsSVGFilterProperty*   mFilter;
    nsSVGMaskProperty*     mMask;
    nsSVGClipPathProperty* mClipPath;
  };

  


  static EffectProperties GetEffectProperties(nsIFrame *aFrame);

  



  static void UpdateEffects(nsIFrame *aFrame);

  


  static nsSVGFilterProperty *GetFilterProperty(nsIFrame *aFrame);
  
  static nsSVGFilterFrame *GetFilterFrame(nsIFrame *aFrame) {
    nsSVGFilterProperty *prop = GetFilterProperty(aFrame);
    PRBool ok;
    return prop ? prop->GetFilterFrame(&ok) : nsnull;
  }
};

#endif 
