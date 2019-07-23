





































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsBaseAppShell.h"
#include "nsCOMPtr.h"
#include <QtNetwork>





#define SOCK_IMPL 1

class nsAppShell : public QObject, public nsBaseAppShell
{
  Q_OBJECT
public:
  nsAppShell() { };

  nsresult Init();

private slots:
  void EventNativeCallback(qint64 numBytes);

protected:
  virtual void ScheduleNativeEventCallback();
  virtual PRBool ProcessNextNativeEvent(PRBool mayWait);
  virtual ~nsAppShell();

private:
  QTcpSocket mBuff;
  QTcpServer tcpServer;
};


#endif 

