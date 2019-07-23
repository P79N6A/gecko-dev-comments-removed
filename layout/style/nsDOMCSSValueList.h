






































#ifndef nsDOMCSSValueList_h___
#define nsDOMCSSValueList_h___

#include "nsIDOMCSSValue.h"
#include "nsIDOMCSSValueList.h"
#include "nsCOMArray.h"

#include "nsCOMPtr.h"

class nsDOMCSSValueList : public nsIDOMCSSValueList
{
public:
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMCSSVALUELIST

  
  NS_DECL_NSIDOMCSSVALUE

  
  nsDOMCSSValueList(PRBool aCommaDelimited, PRBool aReadonly);
  virtual ~nsDOMCSSValueList();

  




  PRBool AppendCSSValue(nsIDOMCSSValue* aValue);

  nsIDOMCSSValue* GetItemAt(PRUint32 aIndex)
  {
    return mCSSValues.SafeObjectAt(aIndex);
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
  PRPackedBool                mCommaDelimited;  
                                                
                                                

  PRPackedBool                mReadonly;    

  nsCOMArray<nsIDOMCSSValue>  mCSSValues;
};


#endif 
