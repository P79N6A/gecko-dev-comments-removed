




































#ifndef SetDocTitleTxn_h__
#define SetDocTitleTxn_h__

#include "EditTxn.h"
#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsITransaction.h"
#include "nsCOMPtr.h"

#define SET_DOC_TITLE_TXN_CID \
{ /*7FC508B5-ED8F-11d4-AF02-0050040AE132 */ \
0x7fc508b5, 0xed8f, 0x11d4, \
{ 0xaf, 0x2, 0x0, 0x50, 0x4, 0xa, 0xe1, 0x32 } }






class SetDocTitleTxn : public EditTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = SET_DOC_TITLE_TXN_CID; return iid; }

  



  NS_IMETHOD Init(nsIHTMLEditor  *aEditor,
                  const nsAString *aValue);
private:
  SetDocTitleTxn();
  nsresult SetDocTitle(const nsAString& aTitle);
  nsresult SetDomTitle(const nsAString& aTitle);

public:
  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();
  NS_IMETHOD GetIsTransient(PRBool *aIsTransient);

protected:

  
  nsIHTMLEditor*  mEditor;
  
  
  nsString    mValue;

  
  nsString    mUndoValue;

  
  PRPackedBool mIsTransient;

  friend class TransactionFactory;
};

#endif
