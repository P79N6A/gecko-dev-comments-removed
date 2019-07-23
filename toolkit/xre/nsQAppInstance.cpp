





































#include "nsQAppInstance.h"
#include <QApplication>


extern int    gArgc;
extern char **gArgv;

nsQAppInstance *nsQAppInstance::sQAppInstance = NULL;
int nsQAppInstance::sQAppRefCount = 0;

nsQAppInstance::nsQAppInstance(int gArgc, char** gArgv)
  : QApplication(gArgc, gArgv)
{
}

void nsQAppInstance::AddRef(void) {
  if (qApp) return;
  if (!sQAppInstance)
    sQAppInstance = new nsQAppInstance(gArgc, gArgv);
  sQAppRefCount++;
}

void nsQAppInstance::Release(void) {
  if (sQAppInstance && !--sQAppRefCount) {
    delete sQAppInstance;
    sQAppInstance = NULL;
  }
}
