




#ifndef nsStylesheetTxns_h__
#define nsStylesheetTxns_h__

#include "EditTxn.h"                    
#include "nsAutoPtr.h"                  
#include "mozilla/CSSStyleSheet.h"      
#include "nsCycleCollectionParticipant.h"
#include "nsID.h"                       
#include "nsISupportsImpl.h"            
#include "nscore.h"                     

class nsIEditor;

class AddStyleSheetTxn : public EditTxn
{
public:
  



  NS_IMETHOD Init(nsIEditor* aEditor,
                  mozilla::CSSStyleSheet* aSheet);

  AddStyleSheetTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AddStyleSheetTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

protected:

  nsIEditor* mEditor;                      
  nsRefPtr<mozilla::CSSStyleSheet> mSheet; 

};


class RemoveStyleSheetTxn : public EditTxn
{
public:
  



  NS_IMETHOD Init(nsIEditor* aEditor,
                  mozilla::CSSStyleSheet* aSheet);

  RemoveStyleSheetTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(RemoveStyleSheetTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

protected:

  nsIEditor* mEditor;                      
  nsRefPtr<mozilla::CSSStyleSheet> mSheet; 

};


#endif 
