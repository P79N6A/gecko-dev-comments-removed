





































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include <qsocketnotifier.h>
#include "nsBaseAppShell.h"
#include "nsCOMPtr.h"





class nsAppShell : public QObject,
                   public nsBaseAppShell
{
  Q_OBJECT

public:
  nsAppShell() { };

  nsresult Init();

  virtual bool event (QEvent *e);

protected:
  virtual void ScheduleNativeEventCallback();
  virtual PRBool ProcessNextNativeEvent(PRBool mayWait);
  virtual ~nsAppShell();
};


#endif 

