




#ifndef _OPENSLESPROVIDER_H_
#define _OPENSLESPROVIDER_H_

#include <SLES/OpenSLES.h>
#include <mozilla/Types.h>

#ifdef __cplusplus
extern "C" {
#endif
extern MOZ_EXPORT
int mozilla_get_sles_engine(SLObjectItf * aObjectm,
                            SLuint32 aOptionCount,
                            const SLEngineOption *aOptions);
extern MOZ_EXPORT
void mozilla_destroy_sles_engine(SLObjectItf * aObjectm);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "mozilla/Mutex.h"

extern PRLogModuleInfo *gOpenSLESProviderLog;

namespace mozilla {

class OpenSLESProvider {
public:
    static int Get(SLObjectItf * aObjectm,
                   SLuint32 aOptionCount,
                   const SLEngineOption *aOptions);
    static void Destroy(SLObjectItf * aObjectm);
private:
    OpenSLESProvider();
    ~OpenSLESProvider();
    OpenSLESProvider(OpenSLESProvider const&); 
    void operator=(OpenSLESProvider const&);   
    static OpenSLESProvider& getInstance();
    int GetEngine(SLObjectItf * aObjectm,
                  SLuint32 aOptionCount,
                  const SLEngineOption *aOptions);
    int ConstructEngine(SLObjectItf * aObjectm,
                        SLuint32 aOptionCount,
                        const SLEngineOption *aOptions);
    void DestroyEngine(SLObjectItf * aObjectm);

    
    mozilla::Mutex mLock;
    SLObjectItf mSLEngine;
    int mSLEngineUsers;
    void *mOpenSLESLib;
};

} 
#endif 

#endif
