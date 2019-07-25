





































#include "nsQAppInstance.h"
#include <QApplication>
#include "prenv.h"


extern int    gArgc;
extern char **gArgv;

QApplication *nsQAppInstance::sQAppInstance = NULL;
int nsQAppInstance::sQAppRefCount = 0;

void nsQAppInstance::AddRef(void) {
  if (qApp) return;
  if (!sQAppInstance) {
    const char *graphicsSystem = PR_GetEnv("MOZ_QT_GRAPHICSSYSTEM");
    if (graphicsSystem)
      QApplication::setGraphicsSystem(QString(graphicsSystem));
#if (MOZ_PLATFORM_MAEMO == 6)
    QApplication::setStyle(QLatin1String("windows"));
#endif
    sQAppInstance = new QApplication(gArgc, gArgv);
  }
  sQAppRefCount++;
}

void nsQAppInstance::Release(void) {
  if (sQAppInstance && !--sQAppRefCount) {
    delete sQAppInstance;
    sQAppInstance = NULL;
  }
}
