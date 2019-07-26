






#ifndef mozilla_a11y_HyperTextAccessibleWrap_h__
#define mozilla_a11y_HyperTextAccessibleWrap_h__

#include "HyperTextAccessible.h"
#include "CAccessibleText.h"
#include "CAccessibleEditableText.h"
#include "ia2AccessibleHyperText.h"

class HyperTextAccessibleWrap : public HyperTextAccessible,
                                public ia2AccessibleHypertext,
                                public CAccessibleEditableText
{
public:
  HyperTextAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    HyperTextAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult HandleAccEvent(AccEvent* aEvent);

protected:
  virtual nsresult GetModifiedText(bool aGetInsertedText, nsAString& aText,
                                   PRUint32 *aStartOffset,
                                   PRUint32 *aEndOffset);
};

#endif

