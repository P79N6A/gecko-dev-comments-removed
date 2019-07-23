




































#ifndef _INC_NSSShutDown_H
#define _INC_NSSShutDown_H

#include "nscore.h"
#include "nspr.h"
#include "pldhash.h"

class nsNSSShutDownObject;
class nsOnPK11LogoutCancelObject;


class nsNSSActivityState
{
public:
  nsNSSActivityState();
  ~nsNSSActivityState();

  
  
  void enter();
  void leave();
  
  
  
  void enterBlockingUIState();
  void leaveBlockingUIState();
  
  
  PRBool isBlockingUIActive();

  
  PRBool isUIForbidden();
  
  
  
  
  
  enum RealOrTesting {test_only, do_it_for_real};
  PRBool ifPossibleDisallowUI(RealOrTesting rot);

  
  
  
  void allowUI();

  
  
  PRStatus restrictActivityToCurrentThread();
  
  
  void releaseCurrentThreadActivityRestriction();

private:
  
  PRLock *mNSSActivityStateLock;

  
  
  
  PRCondVar *mNSSActivityChanged;

  
  int mNSSActivityCounter;

  
  
  int mBlockingUICounter;

  
  PRBool mIsUIForbidden;

  
  
  PRThread* mNSSRestrictedThread;
};


class nsNSSShutDownPreventionLock
{
public:
  nsNSSShutDownPreventionLock();
  ~nsNSSShutDownPreventionLock();
};


class nsPSMUITracker
{
public:
  nsPSMUITracker();
  ~nsPSMUITracker();
  
  PRBool isUIForbidden();
};



class nsNSSShutDownList
{
public:
  ~nsNSSShutDownList();

  static nsNSSShutDownList *construct();
  
  
  static void remember(nsNSSShutDownObject *o);
  static void forget(nsNSSShutDownObject *o);

  
  
  static void remember(nsOnPK11LogoutCancelObject *o);
  static void forget(nsOnPK11LogoutCancelObject *o);

  
  
  static void trackSSLSocketCreate();
  static void trackSSLSocketClose();
  static PRBool areSSLSocketsActive();
  
  
  
  PRBool isUIActive();

  
  
  PRBool ifPossibleDisallowUI();

  
  void allowUI();
  
  
  nsresult evaporateAllNSSResources();

  
  
  nsresult doPK11Logout();
  
  static nsNSSActivityState *getActivityState()
  {
    return singleton ? &singleton->mActivityState : nsnull;
  }
  
private:
  nsNSSShutDownList();
  static PLDHashOperator PR_CALLBACK
  evaporateAllNSSResourcesHelper(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                                        PRUint32 number, void *arg);

  static PLDHashOperator PR_CALLBACK
  doPK11LogoutHelper(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                                    PRUint32 number, void *arg);
protected:
  PRLock* mListLock;
  static nsNSSShutDownList *singleton;
  PLDHashTable mObjects;
  PRUint32 mActiveSSLSockets;
  PLDHashTable mPK11LogoutCancelObjects;
  nsNSSActivityState mActivityState;
};






































































class nsNSSShutDownObject
{
public:

  enum CalledFromType {calledFromList, calledFromObject};

  nsNSSShutDownObject()
  {
    mAlreadyShutDown = PR_FALSE;
    nsNSSShutDownList::remember(this);
  }
  
  virtual ~nsNSSShutDownObject()
  {
    
    
    
  }
  
  void shutdown(CalledFromType calledFrom)
  {
    if (!mAlreadyShutDown) {
      if (calledFromObject == calledFrom) {
        nsNSSShutDownList::forget(this);
      }
      if (calledFromList == calledFrom) {
        virtualDestroyNSSReference();
      }
      mAlreadyShutDown = PR_TRUE;
    }
  }
  
  PRBool isAlreadyShutDown() { return mAlreadyShutDown; }

protected:
  virtual void virtualDestroyNSSReference() = 0;
private:
  volatile PRBool mAlreadyShutDown;
};

class nsOnPK11LogoutCancelObject
{
public:
  nsOnPK11LogoutCancelObject()
  :mIsLoggedOut(PR_FALSE)
  {
    nsNSSShutDownList::remember(this);
  }
  
  virtual ~nsOnPK11LogoutCancelObject()
  {
    nsNSSShutDownList::forget(this);
  }
  
  void logout()
  {
    
    
    
    
    
    mIsLoggedOut = PR_TRUE;
  }
  
  PRBool isPK11LoggedOut()
  {
    return mIsLoggedOut;
  }

private:
  volatile PRBool mIsLoggedOut;
};

#endif
