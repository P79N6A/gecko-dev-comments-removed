





#include "nsQAppInstance.h"
#include <QGuiApplication>
#include "prenv.h"
#include "nsXPCOMPrivate.h"
#include <stdlib.h>

QGuiApplication *nsQAppInstance::sQAppInstance = nullptr;
int nsQAppInstance::sQAppRefCount = 0;

void nsQAppInstance::AddRef(int& aArgc, char** aArgv, bool aDefaultProcess) {
  if (qApp)
    return;
  if (!sQAppInstance) {
    mozilla::SetICUMemoryFunctions();
    sQAppInstance = new QGuiApplication(aArgc, aArgv);
  }
  sQAppRefCount++;
}

void nsQAppInstance::Release(void) {
  if (sQAppInstance && !--sQAppRefCount) {
    delete sQAppInstance;
    sQAppInstance = nullptr;
  }
}
