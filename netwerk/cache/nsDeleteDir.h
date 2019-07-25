






































#ifndef nsDeleteDir_h__
#define nsDeleteDir_h__

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"

class nsIFile;
class nsIThread;
class nsITimer;


class nsDeleteDir {
public:
  nsDeleteDir();
  ~nsDeleteDir();

  static nsresult Init();
  static nsresult Shutdown(bool finishDeleting);

  

















  static nsresult DeleteDir(nsIFile *dir, bool moveToTrash, PRUint32 delay = 0);

  


  static nsresult GetTrashDir(nsIFile *dir, nsCOMPtr<nsIFile> *result);

  



  static nsresult RemoveOldTrashes(nsIFile *cacheDir);

  static void TimerCallback(nsITimer *aTimer, void *arg);

private:
  friend class nsBlockOnBackgroundThreadEvent;
  friend class nsDestroyThreadEvent;

  nsresult InitThread();
  void     DestroyThread();
  nsresult PostTimer(void *arg, PRUint32 delay);
  nsresult RemoveDir(nsIFile *file, bool *stopDeleting);

  static nsDeleteDir * gInstance;
  mozilla::Mutex       mLock;
  mozilla::CondVar     mCondVar;
  nsCOMArray<nsITimer> mTimers;
  nsCOMPtr<nsIThread>  mThread;
  bool                 mShutdownPending;
  bool                 mStopDeleting;
};

#endif  
