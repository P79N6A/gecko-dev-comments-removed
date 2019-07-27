





#ifndef _nsCacheUtils_h_
#define _nsCacheUtils_h_

#include "nsThreadUtils.h"
#include "nsCOMPtr.h"
#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"

class nsIThread;




class nsShutdownThread : public nsRunnable {
public:
  explicit nsShutdownThread(nsIThread *aThread);
  ~nsShutdownThread();

  NS_IMETHOD Run();




  static nsresult Shutdown(nsIThread *aThread);





  static nsresult BlockingShutdown(nsIThread *aThread);

private:
  mozilla::Mutex      mLock;
  mozilla::CondVar    mCondVar;
  nsCOMPtr<nsIThread> mThread;
};

#endif
