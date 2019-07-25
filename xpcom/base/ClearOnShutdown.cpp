






#include "mozilla/ClearOnShutdown.h"

namespace mozilla {
namespace ClearOnShutdown_Internal {

bool sHasShutDown = false;
LinkedList<ShutdownObserver> sShutdownObservers;

} 
} 
