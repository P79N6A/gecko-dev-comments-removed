




































#ifndef nsStylesheetTxns_h__
#define nsStylesheetTxns_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsIEditor.h"
#include "nsICSSStyleSheet.h"

class AddStyleSheetTxn : public EditTxn
{
public:
  



  NS_IMETHOD Init(nsIEditor         *aEditor,
                  nsICSSStyleSheet  *aSheet);

  AddStyleSheetTxn();

  NS_DECL_EDITTXN

protected:

  nsIEditor*  mEditor;									
  nsCOMPtr<nsICSSStyleSheet>	mSheet;		
  
};


class RemoveStyleSheetTxn : public EditTxn
{
public:
  



  NS_IMETHOD Init(nsIEditor         *aEditor,
                  nsICSSStyleSheet  *aSheet);
	
  RemoveStyleSheetTxn();

  NS_DECL_EDITTXN

protected:

  nsIEditor*  mEditor;									
  nsCOMPtr<nsICSSStyleSheet>	mSheet;		
  
};


#endif 
