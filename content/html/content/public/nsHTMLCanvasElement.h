




































#if !defined(nsHTMLCanvasElement_h__)
#define nsHTMLCanvasElement_h__

#include "nsIDOMHTMLCanvasElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsSize.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsDOMError.h"
#include "nsNodeInfoManager.h"

#include "nsICanvasRenderingContextInternal.h"
#include "nsICanvasElementExternal.h"
#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsLayoutUtils.h"

#include "Layers.h"

class nsIDOMFile;

class nsHTMLCanvasElement : public nsGenericHTMLElement,
                            public nsICanvasElementExternal,
                            public nsIDOMHTMLCanvasElement
{
  typedef mozilla::layers::CanvasLayer CanvasLayer;
  typedef mozilla::layers::LayerManager LayerManager;

public:
  nsHTMLCanvasElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLCanvasElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLCANVASELEMENT

  
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLCanvasElement,
                                                     nsGenericHTMLElement)

  


  nsIFrame *GetPrimaryCanvasFrame();

  


  nsIntSize GetSize();

  


  PRBool IsWriteOnly();

  


  void SetWriteOnly();

  



  void InvalidateCanvasContent(const gfxRect* aDamageRect);
  



  void InvalidateCanvas();

  



  PRInt32 CountContexts ();
  nsICanvasRenderingContextInternal *GetContextAtIndex (PRInt32 index);

  



  PRBool GetIsOpaque();

  


  NS_IMETHOD_(nsIntSize) GetSizeExternal();
  NS_IMETHOD RenderContextsExternal(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter);

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute, PRInt32 aModType) const;

  
  
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  



  already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                               CanvasLayer *aOldLayer,
                                               LayerManager *aManager);

  
  
  
  
  void MarkContextClean();

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  nsIntSize GetWidthHeight();

  nsresult UpdateContext(nsIPropertyBag *aNewContextOptions = nsnull);
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
                            nsICanvasRenderingContextInternal **aContext);

  nsString mCurrentContextId;
  nsCOMPtr<nsICanvasRenderingContextInternal> mCurrentContext;
  
public:
  
  
  
  
  PRPackedBool             mWriteOnly;
};

#endif 
