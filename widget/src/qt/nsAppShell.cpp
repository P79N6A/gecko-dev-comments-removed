





































#include "nsAppShell.h"
#include <qapplication.h>

#define NOTIFY_TOKEN 0xFA

void nsAppShell::EventNativeCallback(qint64 numBytes)
{
    char c;
    mBuff.read(&c, 1);
    

    NativeEventCallback();
    return;
}

nsAppShell::~nsAppShell()
{
    mBuff.close();
}

nsresult
nsAppShell::Init()
{
    while (!tcpServer.isListening() && !tcpServer.listen()) {}
    mBuff.connectToHost(QHostAddress::LocalHost, tcpServer.serverPort());
    connect(&mBuff, SIGNAL(bytesWritten(qint64)), this, SLOT(EventNativeCallback(qint64)));

    return nsBaseAppShell::Init();
}

void
nsAppShell::ScheduleNativeEventCallback()
{
  char buf [] = { NOTIFY_TOKEN };
  mBuff.write(buf, 1);
}

PRBool
nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
  qApp->processEvents();
  return PR_TRUE;
}
