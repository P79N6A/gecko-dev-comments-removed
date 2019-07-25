




































#ifndef nsAutoCopyListener_h_
#define nsAutoCopyListener_h_

#include "nsISelectionListener.h"
#include "nsISelectionPrivate.h"

class nsAutoCopyListener : public nsISelectionListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISELECTIONLISTENER

  void Listen(nsISelectionPrivate *aSelection)
  {
      NS_ASSERTION(aSelection, "Null selection passed to Listen()");
      aSelection->AddSelectionListener(this);
  }

  static nsAutoCopyListener* GetInstance()
  {
    if (!sInstance) {
      sInstance = new nsAutoCopyListener();

      NS_ADDREF(sInstance);
    }

    return sInstance;
  }

  static void Shutdown()
  {
    NS_IF_RELEASE(sInstance);
  }

private:
  static nsAutoCopyListener* sInstance;
};

#endif
