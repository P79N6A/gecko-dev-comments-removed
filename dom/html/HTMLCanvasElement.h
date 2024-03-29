




#if !defined(mozilla_dom_HTMLCanvasElement_h)
#define mozilla_dom_HTMLCanvasElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLCanvasElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsSize.h"
#include "nsError.h"

#include "mozilla/gfx/Rect.h"

class nsICanvasRenderingContextInternal;
class nsITimerCallback;

namespace mozilla {

namespace layers {
class CanvasLayer;
class LayerManager;
} 
namespace gfx {
class SourceSurface;
} 

namespace dom {
class CanvasCaptureMediaStream;
class File;
class FileCallback;
class HTMLCanvasPrintState;
class PrintCallback;

enum class CanvasContextType : uint8_t {
  Canvas2D,
  WebGL1,
  WebGL2
};

class HTMLCanvasElement final : public nsGenericHTMLElement,
                                public nsIDOMHTMLCanvasElement
{
  enum {
    DEFAULT_CANVAS_WIDTH = 300,
    DEFAULT_CANVAS_HEIGHT = 150
  };

  typedef layers::CanvasLayer CanvasLayer;
  typedef layers::LayerManager LayerManager;

public:
  explicit HTMLCanvasElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLCanvasElement, canvas)

  
  NS_DECL_ISUPPORTS_INHERITED

  
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
             JS::Handle<JS::Value> aContextOptions,
             ErrorResult& aRv);
  void ToDataURL(JSContext* aCx, const nsAString& aType,
                 JS::Handle<JS::Value> aParams,
                 nsAString& aDataURL, ErrorResult& aRv)
  {
    aRv = ToDataURL(aType, aParams, aCx, aDataURL);
  }
  void ToBlob(JSContext* aCx,
              FileCallback& aCallback,
              const nsAString& aType,
              JS::Handle<JS::Value> aParams,
              ErrorResult& aRv);

  bool MozOpaque() const
  {
    return GetBoolAttr(nsGkAtoms::moz_opaque);
  }
  void SetMozOpaque(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::moz_opaque, aValue, aRv);
  }
  already_AddRefed<File> MozGetAsFile(const nsAString& aName,
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
  PrintCallback* GetMozPrintCallback() const;
  void SetMozPrintCallback(PrintCallback* aCallback);

  already_AddRefed<CanvasCaptureMediaStream>
  CaptureStream(const Optional<double>& aFrameRate, ErrorResult& aRv);

  


  nsIntSize GetSize();

  


  bool IsWriteOnly();

  


  void SetWriteOnly();

  



  void InvalidateCanvasContent(const mozilla::gfx::Rect* aDamageRect);
  



  void InvalidateCanvas();

  



  int32_t CountContexts ();
  nsICanvasRenderingContextInternal *GetContextAtIndex (int32_t index);

  



  bool GetIsOpaque();

  virtual already_AddRefed<gfx::SourceSurface> GetSurfaceSnapshot(bool* aPremultAlpha = nullptr);

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) override;
  nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute, int32_t aModType) const override;

  
  
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) override;

  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                             bool aNotify) override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;
  nsresult CopyInnerTo(mozilla::dom::Element* aDest);

  virtual nsresult PreHandleEvent(mozilla::EventChainPreVisitor& aVisitor) override;

  



  already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                               CanvasLayer *aOldLayer,
                                               LayerManager *aManager);
  
  
  
  bool ShouldForceInactiveLayer(LayerManager *aManager);

  
  
  
  
  void MarkContextClean();

  nsresult GetContext(const nsAString& aContextId, nsISupports** aContext);

protected:
  virtual ~HTMLCanvasElement();

  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  nsIntSize GetWidthHeight();

  nsresult UpdateContext(JSContext* aCx, JS::Handle<JS::Value> options);
  nsresult ParseParams(JSContext* aCx,
                       const nsAString& aType,
                       const JS::Value& aEncoderOptions,
                       nsAString& aParams,
                       bool* usingCustomParseOptions);
  nsresult ExtractData(nsAString& aType,
                       const nsAString& aOptions,
                       nsIInputStream** aStream);
  nsresult ToDataURLImpl(JSContext* aCx,
                         const nsAString& aMimeType,
                         const JS::Value& aEncoderOptions,
                         nsAString& aDataURL);
  nsresult MozGetAsBlobImpl(const nsAString& aName,
                            const nsAString& aType,
                            nsISupports** aResult);
  void CallPrintCallback();

  CanvasContextType mCurrentContextType;
  nsRefPtr<HTMLCanvasElement> mOriginalCanvas;
  nsRefPtr<PrintCallback> mPrintCallback;
  nsCOMPtr<nsICanvasRenderingContextInternal> mCurrentContext;
  nsRefPtr<HTMLCanvasPrintState> mPrintState;

public:
  
  
  
  
  bool                     mWriteOnly;

  bool IsPrintCallbackDone();

  void HandlePrintCallback(nsPresContext::nsPresContextType aType);

  nsresult DispatchPrintCallback(nsITimerCallback* aCallback);

  void ResetPrintCallback();

  HTMLCanvasElement* GetOriginalCanvas();
};

class HTMLCanvasPrintState final : public nsWrapperCache
{
public:
  HTMLCanvasPrintState(HTMLCanvasElement* aCanvas,
                       nsICanvasRenderingContextInternal* aContext,
                       nsITimerCallback* aCallback);

  nsISupports* Context() const;

  void Done();

  void NotifyDone();

  bool mIsDone;

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(HTMLCanvasPrintState)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(HTMLCanvasPrintState)

  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

  HTMLCanvasElement* GetParentObject()
  {
    return mCanvas;
  }

private:
  ~HTMLCanvasPrintState();
  bool mPendingNotify;

protected:
  nsRefPtr<HTMLCanvasElement> mCanvas;
  nsCOMPtr<nsICanvasRenderingContextInternal> mContext;
  nsCOMPtr<nsITimerCallback> mCallback;
};

} 
} 

#endif
