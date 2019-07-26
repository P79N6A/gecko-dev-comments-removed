




#include "nsIQueryContentEventResult.h"
#include "nsString.h"
#include "nsRect.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"

class nsIWidget;

class nsQueryContentEventResult MOZ_FINAL : public nsIQueryContentEventResult
{
public:
  nsQueryContentEventResult();
  ~nsQueryContentEventResult();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIQUERYCONTENTEVENTRESULT

  void SetEventResult(nsIWidget* aWidget,
                      const mozilla::WidgetQueryContentEvent &aEvent);

protected:
  uint32_t mEventID;

  uint32_t mOffset;
  nsString mString;
  nsIntRect mRect;

  bool mSucceeded;
  bool mReversed;
};
