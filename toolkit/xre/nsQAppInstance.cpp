





#include "nsQAppInstance.h"
#include <QApplication>
#ifdef MOZ_ENABLE_MEEGOTOUCH
#include <MComponentData>
#include <MApplicationService>
#endif
#include "prenv.h"
#include <stdlib.h>

QApplication *nsQAppInstance::sQAppInstance = nullptr;
#ifdef MOZ_ENABLE_MEEGOTOUCH
MComponentData* nsQAppInstance::sMComponentData = nullptr;
#endif
int nsQAppInstance::sQAppRefCount = 0;

void nsQAppInstance::AddRef(int& aArgc, char** aArgv, bool aDefaultProcess) {
  if (qApp)
    return;
  if (!sQAppInstance) {
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    const char *graphicsSystem = getenv("MOZ_QT_GRAPHICSSYSTEM");
    if (graphicsSystem) {
      QApplication::setGraphicsSystem(QString(graphicsSystem));
    }
#endif
    sQAppInstance = new QApplication(aArgc, aArgv);
  }
  sQAppRefCount++;
}

void nsQAppInstance::Release(void) {
  if (sQAppInstance && !--sQAppRefCount) {
#ifdef MOZ_ENABLE_MEEGOTOUCH
    delete sMComponentData;
    sMComponentData = nullptr;
#endif
    delete sQAppInstance;
    sQAppInstance = nullptr;
  }
}
