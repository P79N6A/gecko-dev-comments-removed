




#include "nsIQueryContentEventResult.h"
#include "nsString.h"
#include "nsRect.h"
#include "mozilla/Attributes.h"

class nsQueryContentEvent;
class nsIWidget;

class nsQueryContentEventResult MOZ_FINAL : public nsIQueryContentEventResult
{
public:
  nsQueryContentEventResult();
  ~nsQueryContentEventResult();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIQUERYCONTENTEVENTRESULT

  void SetEventResult(nsIWidget* aWidget, const nsQueryContentEvent &aEvent);

protected:
  PRUint32 mEventID;

  PRUint32 mOffset;
  nsString mString;
  nsIntRect mRect;

  bool mSucceeded;
  bool mReversed;
};
