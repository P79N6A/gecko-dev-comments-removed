




#if !defined(mozilla_dom_HTMLCanvasElement_h)
#define mozilla_dom_HTMLCanvasElement_h

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
class nsITimerCallback;
class nsIPropertyBag;

namespace mozilla {

namespace layers {
class CanvasLayer;
class LayerManager;
}

namespace gfx {
struct Rect;
}

namespace dom {

class HTMLCanvasPrintState;

class HTMLCanvasElement MOZ_FINAL : public nsGenericHTMLElement,
                                    public nsICanvasElementExternal,
                                    public nsIDOMHTMLCanvasElement
{
  enum {
    DEFAULT_CANVAS_WIDTH = 300,
    DEFAULT_CANVAS_HEIGHT = 150
  };

  typedef layers::CanvasLayer CanvasLayer;
  typedef layers::LayerManager LayerManager;

public:
  HTMLCanvasElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLCanvasElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLCanvasElement, canvas)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLCANVASELEMENT

  
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLCanvasElement,
                                           nsGenericHTMLElement)

  
  uint32_t Height()
  {
    return GetUnsignedIntAttr(nsGkAtoms::height, DEFAULT_CANVAS_HEIGHT);
  }
  void SetHeight(uint32_t aHeight, ErrorResult& aRv)
  {
    SetUnsignedIntAttr(nsGkAtoms::height, aHeight, aRv);
  }
  uint32_t Width()
  {
    return GetUnsignedIntAttr(nsGkAtoms::width, DEFAULT_CANVAS_WIDTH);
  }
  void SetWidth(uint32_t aWidth, ErrorResult& aRv)
  {
    SetUnsignedIntAttr(nsGkAtoms::width, aWidth, aRv);
  }
  already_AddRefed<nsISupports>
  GetContext(JSContext* aCx, const nsAString& aContextId,
             const Optional<LazyRootedValue>& aContextOptions, ErrorResult& aRv)
  {
    JS::Value contextOptions = aContextOptions.WasPassed()
                             ? aContextOptions.Value()
                             : JS::UndefinedValue();
    nsCOMPtr<nsISupports> context;
    aRv = GetContext(aContextId, contextOptions, aCx, getter_AddRefs(context));
    return context.forget();
  }
  void ToDataURL(JSContext* aCx, const nsAString& aType,
                 const Optional<LazyRootedValue>& aParams, nsAString& aDataURL,
                 ErrorResult& aRv)
  {
    JS::Value params = aParams.WasPassed()
                     ? aParams.Value()
                     : JS::UndefinedValue();
    aRv = ToDataURL(aType, params, aCx, aDataURL);
  }
  void ToBlob(nsIFileCallback* aCallback, const nsAString& aType,
              ErrorResult& aRv)
  {
    aRv = ToBlob(aCallback, aType);
  }

  bool MozOpaque() const
  {
    return GetBoolAttr(nsGkAtoms::moz_opaque);
  }
  void SetMozOpaque(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::moz_opaque, aValue, aRv);
  }
  already_AddRefed<nsIDOMFile> MozGetAsFile(const nsAString& aName,
                                            const nsAString& aType,
                                            ErrorResult& aRv);
  already_AddRefed<nsISupports> MozGetIPCContext(const nsAString& aContextId,
                                                 ErrorResult& aRv)
  {
    nsCOMPtr<nsISupports> context;
    aRv = MozGetIPCContext(aContextId, getter_AddRefs(context));
    return context.forget();
  }
  void MozFetchAsStream(nsIInputStreamCallback* aCallback,
                        const nsAString& aType, ErrorResult& aRv)
  {
    aRv = MozFetchAsStream(aCallback, aType);
  }
  nsIPrintCallback* GetMozPrintCallback() const;
  

  


  nsIntSize GetSize();

  


  bool IsWriteOnly();

  


  void SetWriteOnly();

  



  void InvalidateCanvasContent(const mozilla::gfx::Rect* aDamageRect);
  



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
  nsresult CopyInnerTo(mozilla::dom::Element* aDest);

  



  already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                               CanvasLayer *aOldLayer,
                                               LayerManager *aManager);
  
  
  
  bool ShouldForceInactiveLayer(LayerManager *aManager);

  
  
  
  
  void MarkContextClean();

  nsresult GetContext(const nsAString& aContextId, nsISupports** aContext);

  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsIntSize GetWidthHeight();

  nsresult UpdateContext(nsIPropertyBag *aNewContextOptions = nullptr);
  nsresult ExtractData(const nsAString& aType,
                       const nsAString& aOptions,
                       nsIInputStream** aStream,
                       bool& aFellBackToPNG);
  nsresult ToDataURLImpl(JSContext* aCx,
                         const nsAString& aMimeType,
                         const JS::Value& aEncoderOptions,
                         nsAString& aDataURL);
  nsresult MozGetAsFileImpl(const nsAString& aName,
                            const nsAString& aType,
                            nsIDOMFile** aResult);
  nsresult GetContextHelper(const nsAString& aContextId,
                            nsICanvasRenderingContextInternal **aContext);
  void CallPrintCallback();

  nsString mCurrentContextId;
  nsRefPtr<HTMLCanvasElement> mOriginalCanvas;
  nsCOMPtr<nsIPrintCallback> mPrintCallback;
  nsCOMPtr<nsICanvasRenderingContextInternal> mCurrentContext;
  nsCOMPtr<HTMLCanvasPrintState> mPrintState;

public:
  
  
  
  
  bool                     mWriteOnly;

  bool IsPrintCallbackDone();

  void HandlePrintCallback(nsPresContext::nsPresContextType aType);

  nsresult DispatchPrintCallback(nsITimerCallback* aCallback);

  void ResetPrintCallback();

  HTMLCanvasElement* GetOriginalCanvas();
};

} 
} 

#endif 
