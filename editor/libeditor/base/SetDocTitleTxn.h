




































#ifndef SetDocTitleTxn_h__
#define SetDocTitleTxn_h__

#include "EditTxn.h"
#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsITransaction.h"
#include "nsCOMPtr.h"






class SetDocTitleTxn : public EditTxn
{
public:
  



  NS_IMETHOD Init(nsIHTMLEditor  *aEditor,
                  const nsAString *aValue);
  SetDocTitleTxn();
private:
  nsresult SetDomTitle(const nsAString& aTitle);

public:
  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();
  NS_IMETHOD GetIsTransient(bool *aIsTransient);

protected:

  
  nsIHTMLEditor*  mEditor;
  
  
  nsString    mValue;

  
  nsString    mUndoValue;

  
  bool mIsTransient;
};

#endif
