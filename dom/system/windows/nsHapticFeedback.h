




#include "nsIHapticFeedback.h"

class nsHapticFeedback MOZ_FINAL : public nsIHapticFeedback
{
  ~nsHapticFeedback() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIHAPTICFEEDBACK
};
