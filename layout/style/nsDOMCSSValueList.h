





#ifndef nsDOMCSSValueList_h___
#define nsDOMCSSValueList_h___

#include "nsIDOMCSSValueList.h"
#include "CSSValue.h"
#include "nsTArray.h"

class nsDOMCSSValueList MOZ_FINAL : public mozilla::dom::CSSValue,
  public nsIDOMCSSValueList
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsDOMCSSValueList, mozilla::dom::CSSValue)

  
  NS_DECL_NSIDOMCSSVALUE

  
  nsDOMCSSValueList(bool aCommaDelimited, bool aReadonly);
  ~nsDOMCSSValueList();

  


  void AppendCSSValue(CSSValue* aValue);

  virtual void GetCssText(nsString& aText, mozilla::ErrorResult& aRv)
    MOZ_OVERRIDE MOZ_FINAL;
  virtual void SetCssText(const nsAString& aText,
                          mozilla::ErrorResult& aRv) MOZ_OVERRIDE MOZ_FINAL;
  virtual uint16_t CssValueType() const MOZ_OVERRIDE MOZ_FINAL;

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

  virtual JSObject *WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap);

private:
  bool                        mCommaDelimited;  
                                                
                                                

  bool                        mReadonly;    

  InfallibleTArray<nsRefPtr<CSSValue> > mCSSValues;
};

#endif 
