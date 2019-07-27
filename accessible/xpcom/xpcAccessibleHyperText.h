





#ifndef mozilla_a11y_xpcAccessibleHyperText_h_
#define mozilla_a11y_xpcAccessibleHyperText_h_

#include "nsIAccessibleText.h"
#include "nsIAccessibleHyperText.h"
#include "nsIAccessibleEditableText.h"

#include "HyperTextAccessible.h"
#include "xpcAccessibleGeneric.h"

namespace mozilla {
namespace a11y {

class xpcAccessibleHyperText : public xpcAccessibleGeneric,
                               public nsIAccessibleText,
                               public nsIAccessibleEditableText,
                               public nsIAccessibleHyperText
{
public:
  explicit xpcAccessibleHyperText(Accessible* aIntl) :
    xpcAccessibleGeneric(aIntl)
  {
    if (mIntl->IsHyperText() && mIntl->AsHyperText()->IsTextRole())
      mSupportedIfaces |= eText;
  }

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIACCESSIBLETEXT
  NS_DECL_NSIACCESSIBLEHYPERTEXT
  NS_DECL_NSIACCESSIBLEEDITABLETEXT

protected:
  virtual ~xpcAccessibleHyperText() {}

private:
  HyperTextAccessible* Intl() { return mIntl->AsHyperText(); }

  xpcAccessibleHyperText(const xpcAccessibleHyperText&) MOZ_DELETE;
  xpcAccessibleHyperText& operator =(const xpcAccessibleHyperText&) MOZ_DELETE;
};

} 
} 

#endif 
