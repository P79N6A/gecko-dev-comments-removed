





#ifndef mozilla_a11y_xpcAccessibleHyperText_h_
#define mozilla_a11y_xpcAccessibleHyperText_h_

#include "nsIAccessibleText.h"
#include "nsIAccessibleHyperText.h"
#include "nsIAccessibleEditableText.h"

namespace mozilla {
namespace a11y {

class xpcAccessibleHyperText : public nsIAccessibleText,
                               public nsIAccessibleEditableText,
                               public nsIAccessibleHyperText
{
public:
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_NSIACCESSIBLETEXT
  NS_DECL_NSIACCESSIBLEHYPERTEXT
  NS_DECL_NSIACCESSIBLEEDITABLETEXT

private:
  xpcAccessibleHyperText() { }
  friend class HyperTextAccessible;

  xpcAccessibleHyperText(const xpcAccessibleHyperText&) MOZ_DELETE;
  xpcAccessibleHyperText& operator =(const xpcAccessibleHyperText&) MOZ_DELETE;
};

} 
} 

#endif 
