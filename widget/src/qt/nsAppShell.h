





































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
  nsAppShell() { };

  nsresult Init();

signals:
  void activated();

private slots:
  void EventNativeCallback();

protected:
  virtual void ScheduleNativeEventCallback();
  virtual PRBool ProcessNextNativeEvent(PRBool mayWait);
  virtual ~nsAppShell();
};


#endif 

