





































#ifndef __NSDOMWORKERBASE_H__
#define __NSDOMWORKERBASE_H__

#include "nsCOMPtr.h"
#include "nsIDOMThreads.h"
#include "nsStringGlue.h"

class nsIRunnable;
class nsDOMWorkerPool;

class nsDOMWorkerBase
{
  friend class nsDOMPostMessageRunnable;

public:
  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) = 0;
  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
  NS_IMETHOD_(nsrefcnt) Release(void) = 0;

  PRBool IsCanceled() {
    return !!mCanceled;
  }

  PRBool IsSuspended() {
    return !!mSuspended;
  }

protected:
  nsDOMWorkerBase() : mCanceled(0), mSuspended(0) { }
  virtual ~nsDOMWorkerBase() { }

  void SetMessageListener(nsIDOMWorkerMessageListener* aListener) {
    mMessageListener = aListener;
  }

  nsIDOMWorkerMessageListener* GetMessageListener() {
    return mMessageListener;
  }

  nsresult PostMessageInternal(const nsAString& aMessage,
                               nsDOMWorkerBase* aSource);

  nsresult PostMessageInternal(const nsAString& aMessage);


  virtual void Cancel();
  virtual void Suspend();
  virtual void Resume();

  
  virtual nsresult HandleMessage(const nsAString& aMessage,
                                 nsDOMWorkerBase* aSource) = 0;
  virtual nsresult DispatchMessage(nsIRunnable* aMessage) = 0;
  virtual nsDOMWorkerPool* Pool() = 0;

private:
  nsCOMPtr<nsIDOMWorkerMessageListener> mMessageListener;
  PRInt32 mCanceled;
  PRInt32 mSuspended;
};

#endif 
