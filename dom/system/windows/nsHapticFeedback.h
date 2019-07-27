




#include "nsIHapticFeedback.h"

class nsHapticFeedback final : public nsIHapticFeedback
{
  ~nsHapticFeedback() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIHAPTICFEEDBACK
};
