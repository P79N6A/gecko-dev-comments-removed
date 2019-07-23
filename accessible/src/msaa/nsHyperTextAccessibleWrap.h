







































#ifndef _NSHYPERTEXTACCESSIBLEWRAP_H
#define _NSHYPERTEXTACCESSIBLEWRAP_H

#include "nsHyperTextAccessible.h"
#include "CAccessibleText.h"
#include "CAccessibleEditableText.h"
#include "CAccessibleHyperText.h"

class nsHyperTextAccessibleWrap : public nsHyperTextAccessible,
                                  public CAccessibleHypertext,
                                  public CAccessibleEditableText
{
public:
  nsHyperTextAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference* aShell) :
    nsHyperTextAccessible(aNode, aShell){}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD FireAccessibleEvent(nsIAccessibleEvent *aEvent);
protected:
  virtual nsresult GetModifiedText(PRBool aGetInsertedText, nsAString& aText,
                                   PRUint32 *aStartOffset,
                                   PRUint32 *aEndOffset);
};

#endif

