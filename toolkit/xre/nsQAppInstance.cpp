





































#include "nsQAppInstance.h"
#include <QApplication>
#ifdef MOZ_ENABLE_MEEGOTOUCH
#include <MComponentData>
#include <MApplicationService>
#endif
#include "prenv.h"
#include <stdlib.h>

QApplication *nsQAppInstance::sQAppInstance = NULL;
#ifdef MOZ_ENABLE_MEEGOTOUCH
MComponentData* nsQAppInstance::sMComponentData = NULL;
#endif
int nsQAppInstance::sQAppRefCount = 0;

void nsQAppInstance::AddRef(int& aArgc, char** aArgv, bool aDefaultProcess) {
  if (qApp)
    return;
  if (!sQAppInstance) {
    const char *graphicsSystem = getenv("MOZ_QT_GRAPHICSSYSTEM");
    if (graphicsSystem) {
      QApplication::setGraphicsSystem(QString(graphicsSystem));
    }
#if (MOZ_PLATFORM_MAEMO == 6)
    
    
    
    if (!aDefaultProcess) {
      QApplication::setStyle(QLatin1String("windows"));
    }
    if (!aArgc) {
      aArgv[aArgc] = strdup("nsQAppInstance");
      aArgc++;
    }
#endif
    sQAppInstance = new QApplication(aArgc, aArgv);
#ifdef MOZ_ENABLE_MEEGOTOUCH
    if (aDefaultProcess) {
      
      
      
      
      
      
      gArgv[gArgc] = strdup("-software");
      gArgc++;
      sMComponentData = new MComponentData(aArgc, aArgv, "", new MApplicationService(""));
    }
#endif
  }
  sQAppRefCount++;
}

void nsQAppInstance::Release(void) {
  if (sQAppInstance && !--sQAppRefCount) {
#ifdef MOZ_ENABLE_MEEGOTOUCH
    delete sMComponentData;
    sMComponentData = NULL;
#endif
    delete sQAppInstance;
    sQAppInstance = NULL;
  }
}
