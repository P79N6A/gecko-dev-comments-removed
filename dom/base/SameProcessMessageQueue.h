





#ifndef mozilla_dom_SameProcessMessageQueue_h
#define mozilla_dom_SameProcessMessageQueue_h

#include "nsIRunnable.h"
#include "nsRefPtr.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {

class CancelableRunnable;

class SameProcessMessageQueue
{
public:
  SameProcessMessageQueue();
  virtual ~SameProcessMessageQueue();

  class Runnable : public nsIRunnable
  {
  public:
    explicit Runnable();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    virtual nsresult HandleMessage() = 0;

  protected:
    virtual ~Runnable() {}

  private:
    bool mDispatched;
  };

  void Push(Runnable* aRunnable);
  void Flush();

  static SameProcessMessageQueue* Get();

private:
  friend class CancelableRunnable;

  nsTArray<nsRefPtr<Runnable>> mQueue;
  static SameProcessMessageQueue* sSingleton;
};

} 
} 

#endif 
