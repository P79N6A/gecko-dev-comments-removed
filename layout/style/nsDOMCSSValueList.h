






































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

private:
  PRPackedBool                mCommaDelimited;  
                                                
                                                

  PRPackedBool                mReadonly;    

  nsCOMArray<nsIDOMCSSValue>  mCSSValues;
};


#endif 
