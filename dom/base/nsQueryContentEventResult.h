




#ifndef mozilla_dom_nsQueryContentEventResult_h
#define mozilla_dom_nsQueryContentEventResult_h

#include "nsIQueryContentEventResult.h"
#include "nsString.h"
#include "nsRect.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"

class nsIWidget;

class nsQueryContentEventResult final : public nsIQueryContentEventResult
{
public:
  nsQueryContentEventResult();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIQUERYCONTENTEVENTRESULT

  void SetEventResult(nsIWidget* aWidget,
                      const mozilla::WidgetQueryContentEvent &aEvent);

protected:
  ~nsQueryContentEventResult();

  uint32_t mEventID;

  uint32_t mOffset;
  uint32_t mTentativeCaretOffset;
  nsString mString;
  mozilla::LayoutDeviceIntRect mRect;

  bool mSucceeded;
  bool mReversed;
};

#endif 
