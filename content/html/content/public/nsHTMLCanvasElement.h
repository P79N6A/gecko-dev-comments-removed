




#if !defined(nsHTMLCanvasElement_h__)
#define nsHTMLCanvasElement_h__

#include "nsIDOMHTMLCanvasElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsSize.h"
#include "nsError.h"
#include "nsNodeInfoManager.h"

#include "nsICanvasElementExternal.h"
#include "nsLayoutUtils.h"

class nsICanvasRenderingContextInternal;
class nsIDOMFile;
class nsHTMLCanvasPrintState;
class nsITimerCallback;
class nsIPropertyBag;

namespace mozilla {
namespace layers {
class CanvasLayer;
class LayerManager;
}
}

class nsHTMLCanvasElement : public nsGenericHTMLElement,
                            public nsICanvasElementExternal,
                            public nsIDOMHTMLCanvasElement
{
  typedef mozilla::layers::CanvasLayer CanvasLayer;
  typedef mozilla::layers::LayerManager LayerManager;

public:
  nsHTMLCanvasElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLCanvasElement();

  static nsHTMLCanvasElement* FromContent(nsIContent* aPossibleCanvas)
  {
    if (!aPossibleCanvas || !aPossibleCanvas->IsHTML(nsGkAtoms::canvas)) {
      return nullptr;
    }
    return static_cast<nsHTMLCanvasElement*>(aPossibleCanvas);
  }
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLCANVASELEMENT

  
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLCanvasElement,
                                           nsGenericHTMLElement)

  


  nsIntSize GetSize();

  


  bool IsWriteOnly();

  


  void SetWriteOnly();

  



  void InvalidateCanvasContent(const gfxRect* aDamageRect);
  



  void InvalidateCanvas();

  



  int32_t CountContexts ();
  nsICanvasRenderingContextInternal *GetContextAtIndex (int32_t index);

  



  bool GetIsOpaque();

  


  NS_IMETHOD_(nsIntSize) GetSizeExternal();
  NS_IMETHOD RenderContextsExternal(gfxContext *aContext,
                                    gfxPattern::GraphicsFilter aFilter,
                                    uint32_t aFlags = RenderFlagPremultAlpha);

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute, int32_t aModType) const;

  
  
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  nsresult CopyInnerTo(nsGenericElement* aDest);

  



  already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                               CanvasLayer *aOldLayer,
                                               LayerManager *aManager);
  
  
  
  bool ShouldForceInactiveLayer(LayerManager *aManager);

  
  
  
  
  void MarkContextClean();

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  nsIntSize GetWidthHeight();

  nsresult UpdateContext(nsIPropertyBag *aNewContextOptions = nullptr);
  nsresult ExtractData(const nsAString& aType,
                       const nsAString& aOptions,
                       nsIInputStream** aStream,
                       bool& aFellBackToPNG);
  nsresult ToDataURLImpl(const nsAString& aMimeType,
                         nsIVariant* aEncoderOptions,
                         nsAString& aDataURL);
  nsresult MozGetAsFileImpl(const nsAString& aName,
                            const nsAString& aType,
                            nsIDOMFile** aResult);
  nsresult GetContextHelper(const nsAString& aContextId,
                            bool aForceThebes,
                            nsICanvasRenderingContextInternal **aContext);
  void CallPrintCallback();

  nsString mCurrentContextId;
  nsCOMPtr<nsIDOMHTMLCanvasElement> mOriginalCanvas;
  nsCOMPtr<nsIPrintCallback> mPrintCallback;
  nsCOMPtr<nsICanvasRenderingContextInternal> mCurrentContext;
  nsCOMPtr<nsHTMLCanvasPrintState> mPrintState;
  
public:
  
  
  
  
  bool                     mWriteOnly;

  bool IsPrintCallbackDone();

  void HandlePrintCallback(nsPresContext::nsPresContextType aType);

  nsresult DispatchPrintCallback(nsITimerCallback* aCallback);

  void ResetPrintCallback();

  nsIDOMHTMLCanvasElement* GetOriginalCanvas();
};

inline nsISupports*
GetISupports(nsHTMLCanvasElement* p)
{
  return static_cast<nsGenericElement*>(p);
}

#endif 
