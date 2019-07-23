



































#include "nsBasePointerService.h"

class nsPointerService : public nsBasePointerService {
 public:

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD WidgetUnderPointer(nsIWidget **_retval,
				PRUint32 *aXOffset, PRUint32 *aYOffset);
};


