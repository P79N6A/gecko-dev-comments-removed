





































#include "nsQAppInstance.h"
#include <QApplication>
#include "prenv.h"


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
  if (!sQAppInstance) {
    const char *graphicsSystem = PR_GetEnv("MOZ_QT_GRAPHICSSYSTEM");
    if (graphicsSystem)
      QApplication::setGraphicsSystem(QString(graphicsSystem));
    sQAppInstance = new nsQAppInstance(gArgc, gArgv);
  }
  sQAppRefCount++;
}

void nsQAppInstance::Release(void) {
  if (sQAppInstance && !--sQAppRefCount) {
    delete sQAppInstance;
    sQAppInstance = NULL;
  }
}
