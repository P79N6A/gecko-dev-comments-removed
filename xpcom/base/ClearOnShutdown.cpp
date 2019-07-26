






#include "mozilla/ClearOnShutdown.h"

namespace mozilla {
namespace ClearOnShutdown_Internal {

bool sHasShutDown = false;
StaticAutoPtr<LinkedList<ShutdownObserver>> sShutdownObservers;

} 
} 
