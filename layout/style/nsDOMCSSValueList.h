






































#ifndef nsDOMCSSValueList_h___
#define nsDOMCSSValueList_h___

#include "nsIDOMCSSValue.h"
#include "nsIDOMCSSValueList.h"
#include "nsTArray.h"


#include "nsCOMPtr.h"

class nsDOMCSSValueList : public nsIDOMCSSValueList
{
public:
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMCSSVALUELIST

  
  NS_DECL_NSIDOMCSSVALUE

  
  nsDOMCSSValueList(bool aCommaDelimited, bool aReadonly);
  virtual ~nsDOMCSSValueList();

  


  void AppendCSSValue(nsIDOMCSSValue* aValue);

  nsIDOMCSSValue* GetItemAt(PRUint32 aIndex)
  {
    return mCSSValues.SafeElementAt(aIndex, nsnull);
  }

  static nsDOMCSSValueList* FromSupports(nsISupports* aSupports)
  {
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMCSSValueList> list_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(list_qi == static_cast<nsIDOMCSSValueList*>(aSupports),
                   "Uh, fix QI!");
    }
#endif

    return static_cast<nsDOMCSSValueList*>(aSupports);
  }

private:
  bool                        mCommaDelimited;  
                                                
                                                

  bool                        mReadonly;    

  InfallibleTArray<nsCOMPtr<nsIDOMCSSValue> > mCSSValues;
};


#endif 
