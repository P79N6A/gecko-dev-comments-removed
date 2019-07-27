




#ifndef HTMLFrameSetElement_h
#define HTMLFrameSetElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLFrameSetElement.h"
#include "nsGenericHTMLElement.h"





enum nsFramesetUnit {
  eFramesetUnit_Fixed = 0,
  eFramesetUnit_Percent,
  eFramesetUnit_Relative
};





struct nsFramesetSpec {
  nsFramesetUnit mUnit;
  nscoord        mValue;
};





#define NS_MAX_FRAMESET_SPEC_COUNT 16000



namespace mozilla {
namespace dom {

class OnBeforeUnloadEventHandlerNonNull;

class HTMLFrameSetElement MOZ_FINAL : public nsGenericHTMLElement,
                                      public nsIDOMHTMLFrameSetElement
{
public:
  explicit HTMLFrameSetElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo),
      mNumRows(0),
      mNumCols(0),
      mCurrentRowColHint(NS_STYLE_HINT_REFLOW)
  {
    SetHasWeirdParserInsertionMode();
  }

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLFrameSetElement, frameset)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLFRAMESETELEMENT

  void GetCols(nsString& aCols)
  {
    GetHTMLAttr(nsGkAtoms::cols, aCols);
  }
  void SetCols(const nsAString& aCols, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::cols, aCols, aError);
  }
  void GetRows(nsString& aRows)
  {
    GetHTMLAttr(nsGkAtoms::rows, aRows);
  }
  void SetRows(const nsAString& aRows, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::rows, aRows, aError);
  }

  virtual bool IsEventAttributeName(nsIAtom *aName) MOZ_OVERRIDE;

  
  
#define EVENT(name_, id_, type_, struct_)
#define WINDOW_EVENT_HELPER(name_, type_)                               \
  type_* GetOn##name_();                                                \
  void SetOn##name_(type_* handler);
#define WINDOW_EVENT(name_, id_, type_, struct_)                        \
  WINDOW_EVENT_HELPER(name_, EventHandlerNonNull)
#define BEFOREUNLOAD_EVENT(name_, id_, type_, struct_)                  \
  WINDOW_EVENT_HELPER(name_, OnBeforeUnloadEventHandlerNonNull)
#include "mozilla/EventNameList.h" 
#undef BEFOREUNLOAD_EVENT
#undef WINDOW_EVENT
#undef WINDOW_EVENT_HELPER
#undef EVENT

  
  
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) MOZ_OVERRIDE;

   






  nsresult GetRowSpec(int32_t *aNumValues, const nsFramesetSpec** aSpecs);
   






  nsresult GetColSpec(int32_t *aNumValues, const nsFramesetSpec** aSpecs);


  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) MOZ_OVERRIDE;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

protected:
  virtual ~HTMLFrameSetElement();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

private:
  nsresult ParseRowCol(const nsAString& aValue,
                       int32_t&         aNumSpecs,
                       nsFramesetSpec** aSpecs);

  


  int32_t          mNumRows;
  


  int32_t          mNumCols;
  



  nsChangeHint      mCurrentRowColHint;
  


  nsAutoArrayPtr<nsFramesetSpec>  mRowSpecs; 
  


  nsAutoArrayPtr<nsFramesetSpec>  mColSpecs; 
};

} 
} 

#endif 
