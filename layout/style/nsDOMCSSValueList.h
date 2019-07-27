





#ifndef nsDOMCSSValueList_h___
#define nsDOMCSSValueList_h___

#include "nsIDOMCSSValueList.h"
#include "CSSValue.h"
#include "nsTArray.h"

class nsDOMCSSValueList final : public mozilla::dom::CSSValue,
                                public nsIDOMCSSValueList
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsDOMCSSValueList, mozilla::dom::CSSValue)

  
  NS_DECL_NSIDOMCSSVALUE

  
  nsDOMCSSValueList(bool aCommaDelimited, bool aReadonly);

  


  void AppendCSSValue(CSSValue* aValue);

  virtual void GetCssText(nsString& aText, mozilla::ErrorResult& aRv)
    override final;
  virtual void SetCssText(const nsAString& aText,
                          mozilla::ErrorResult& aRv) override final;
  virtual uint16_t CssValueType() const override final;

  CSSValue* IndexedGetter(uint32_t aIdx, bool& aFound) const
  {
    aFound = aIdx <= Length();
    return Item(aIdx);
  }

  CSSValue* Item(uint32_t aIndex) const
  {
    return mCSSValues.SafeElementAt(aIndex);
  }

  uint32_t Length() const
  {
    return mCSSValues.Length();
  }

  nsISupports* GetParentObject()
  {
    return nullptr;
  }

  virtual JSObject *WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

private:
  ~nsDOMCSSValueList();

  bool                        mCommaDelimited;  
                                                
                                                

  bool                        mReadonly;    

  InfallibleTArray<nsRefPtr<CSSValue> > mCSSValues;
};

#endif 
