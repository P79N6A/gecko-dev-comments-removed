






































#ifndef _mozStorageBackground_h_
#define _mozStorageBackground_h_

#include "nsClassHashtable.h"
class mozStorageConnection;
class nsIThreadPool;
class nsIEventTarget;
class nsIObserver;






class mozStorageBackground
{
public:

  



  nsIEventTarget *target();

  


  nsresult initialize();

  




  static mozStorageBackground *getService();

  mozStorageBackground();
  ~mozStorageBackground();

private:

  nsCOMPtr<nsIThreadPool> mThreadPool;

  static mozStorageBackground *mSingleton;

  nsCOMPtr<nsIObserver> mObserver;
};

#endif 
