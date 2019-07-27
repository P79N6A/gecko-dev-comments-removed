




#ifndef _LOADMANAGERFACTORY_H_
#define _LOADMANAGERFACTORY_H_

namespace mozilla {

class LoadManager;

mozilla::LoadManager* LoadManagerBuild();
void LoadManagerDestroy(mozilla::LoadManager* aLoadManager);

} 

#endif 
