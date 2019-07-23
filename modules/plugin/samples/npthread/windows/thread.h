




































#ifndef __THREAD_H__
#define __THREAD_H__

typedef enum {
  ts_dead = 0,
  ts_ready,
  ts_busy

} ThreadState;

class CThread {
private:
  HANDLE mThread;
  DWORD mID;
  HANDLE mEventThreadInitializationFinished;
  HANDLE mEventThreadShutdownFinished;
  DWORD mInitTimeOut;
  DWORD mShutTimeOut;
  HANDLE mEventDo;
  ThreadState mState;

protected:
  int mAction;

public:
  BOOL setInitEvent();
  BOOL setShutEvent();
  BOOL isAlive();
  BOOL isBusy();
  HANDLE getHandle();

  void run();

  
  CThread(DWORD aInitTimeOut = INFINITE, DWORD aShutTimeOut = INFINITE);
  virtual ~CThread();

  BOOL open(CThread * aThread);
  void close(CThread * aThread);

  BOOL tryAction(int aAction);
  BOOL doAction(int aAction);

  
  virtual BOOL init() = 0;
  virtual void shut() = 0; 
  virtual void dispatch() = 0;
};

BOOL InitThreadLib(HINSTANCE aInstance);
void ShutThreadLib(HINSTANCE aInstance);

#endif 
