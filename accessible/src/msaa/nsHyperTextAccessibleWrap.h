







































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
  nsHyperTextAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell) :
    nsHyperTextAccessible(aContent, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult HandleAccEvent(AccEvent* aEvent);

protected:
  virtual nsresult GetModifiedText(bool aGetInsertedText, nsAString& aText,
                                   PRUint32 *aStartOffset,
                                   PRUint32 *aEndOffset);
};

#endif

