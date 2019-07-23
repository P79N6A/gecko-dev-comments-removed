




































#ifndef nsStylesheetTxns_h__
#define nsStylesheetTxns_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsIEditor.h"
#include "nsICSSStyleSheet.h"

#define ADD_STYLESHEET_TXN_CID \
{/* d05e2980-2fbe-11d3-9ce4-e8393835307c */ \
0xd05e2980, 0x2fbe, 0x11d3, { 0x9c, 0xe4, 0xe8, 0x39, 0x38, 0x35, 0x30, 0x7c } }

#define REMOVE_STYLESHEET_TXN_CID \
{/* d05e2981-2fbe-11d3-9ce4-e8393835307c */ \
0xd05e2981, 0x2fbe, 0x11d3, { 0x9c, 0xe4, 0xe8, 0x39, 0x38, 0x35, 0x30, 0x7c } }


class AddStyleSheetTxn : public EditTxn
{
  friend class TransactionFactory;

public:

  static const nsIID& GetCID() { static const nsIID iid = ADD_STYLESHEET_TXN_CID; return iid; }

  



  NS_IMETHOD Init(nsIEditor         *aEditor,
                  nsICSSStyleSheet  *aSheet);

private:
  AddStyleSheetTxn();

public:
  NS_DECL_EDITTXN

protected:

  nsIEditor*  mEditor;									
  nsCOMPtr<nsICSSStyleSheet>	mSheet;		
  
};


class RemoveStyleSheetTxn : public EditTxn
{
  friend class TransactionFactory;

public:

  static const nsIID& GetCID() { static const nsIID iid = REMOVE_STYLESHEET_TXN_CID; return iid; }

  



  NS_IMETHOD Init(nsIEditor         *aEditor,
                  nsICSSStyleSheet  *aSheet);
	
private:
  RemoveStyleSheetTxn();

public:
  NS_DECL_EDITTXN

protected:

  nsIEditor*  mEditor;									
  nsCOMPtr<nsICSSStyleSheet>	mSheet;		
  
};


#endif 
