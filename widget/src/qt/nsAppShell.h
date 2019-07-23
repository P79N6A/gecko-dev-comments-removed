





































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsBaseAppShell.h"
#include "nsCOMPtr.h"
#include <qsocketnotifier.h>






class nsAppShell : public QObject,
                   public nsBaseAppShell
{
  Q_OBJECT

public:
  nsAppShell() : mTag(0) {
      mPipeFDs[0] = mPipeFDs[1] = 0;
  };

  nsresult Init();

private slots:
  void EventNativeCallback(int fd);

protected:
  virtual void ScheduleNativeEventCallback();
  virtual PRBool ProcessNextNativeEvent(PRBool mayWait);
  virtual ~nsAppShell();

private:
  int           mPipeFDs[2];
  QSocketNotifier *mTag;
};


#endif 

