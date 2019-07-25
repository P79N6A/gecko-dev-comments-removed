




































#ifndef nsStylesheetTxns_h__
#define nsStylesheetTxns_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsIEditor.h"

class AddStyleSheetTxn : public EditTxn
{
public:
  



  NS_IMETHOD Init(nsIEditor         *aEditor,
                  nsCSSStyleSheet   *aSheet);

  AddStyleSheetTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AddStyleSheetTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

protected:

  nsIEditor*  mEditor;                  
  nsRefPtr<nsCSSStyleSheet>  mSheet;    

};


class RemoveStyleSheetTxn : public EditTxn
{
public:
  



  NS_IMETHOD Init(nsIEditor         *aEditor,
                  nsCSSStyleSheet   *aSheet);

  RemoveStyleSheetTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(RemoveStyleSheetTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

protected:

  nsIEditor*  mEditor;                  
  nsRefPtr<nsCSSStyleSheet>  mSheet;    

};


#endif 
