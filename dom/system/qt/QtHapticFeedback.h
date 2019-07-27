




#include "nsIHapticFeedback.h"

class QtHapticFeedback : public nsIHapticFeedback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIHAPTICFEEDBACK
protected:
  virtual ~QtHapticFeedback() {}
};
