





#ifndef __nsconsolemessage_h__
#define __nsconsolemessage_h__

#include "mozilla/Attributes.h"

#include "nsIConsoleMessage.h"
#include "nsString.h"

class nsConsoleMessage MOZ_FINAL : public nsIConsoleMessage
{
public:
  nsConsoleMessage();
  nsConsoleMessage(const char16_t* aMessage);

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICONSOLEMESSAGE

private:
  ~nsConsoleMessage()
  {
  }

  int64_t mTimeStamp;
  nsString mMessage;
};

#endif 
