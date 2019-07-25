






#ifndef _NSHYPERTEXTACCESSIBLEWRAP_H
#define _NSHYPERTEXTACCESSIBLEWRAP_H

#include "nsHyperTextAccessible.h"
#include "CAccessibleText.h"
#include "CAccessibleEditableText.h"
#include "ia2AccessibleHyperText.h"

class nsHyperTextAccessibleWrap : public nsHyperTextAccessible,
                                  public ia2AccessibleHypertext,
                                  public CAccessibleEditableText
{
public:
  nsHyperTextAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc) :
    nsHyperTextAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult HandleAccEvent(AccEvent* aEvent);

protected:
  virtual nsresult GetModifiedText(bool aGetInsertedText, nsAString& aText,
                                   PRUint32 *aStartOffset,
                                   PRUint32 *aEndOffset);
};

#endif

