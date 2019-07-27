



#ifndef nsHtml5StreamListener_h
#define nsHtml5StreamListener_h

#include "nsIStreamListener.h"
#include "nsIThreadRetargetableStreamListener.h"
#include "nsHtml5RefPtr.h"
#include "nsHtml5StreamParser.h"



















class nsHtml5StreamListener : public nsIStreamListener,
                              public nsIThreadRetargetableStreamListener
{
public:
  explicit nsHtml5StreamListener(nsHtml5StreamParser* aDelegate);

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSITHREADRETARGETABLESTREAMLISTENER

  inline nsHtml5StreamParser* GetDelegate()
  {
    return mDelegate;
  }

  void DropDelegate();

private:
  virtual ~nsHtml5StreamListener();

  nsHtml5RefPtr<nsHtml5StreamParser> mDelegate;
};

#endif 
