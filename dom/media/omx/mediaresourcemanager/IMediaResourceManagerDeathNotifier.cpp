

















#define LOG_TAG "IMediaResourceManagerDeathNotifier"
#include <utils/Log.h>

#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include "IMediaResourceManagerDeathNotifier.h"

#define DN_LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define DN_LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define DN_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define DN_LOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )

namespace android {


Mutex IMediaResourceManagerDeathNotifier::sServiceLock;
sp<IMediaResourceManagerService> IMediaResourceManagerDeathNotifier::sMediaResourceManagerService;
sp<IMediaResourceManagerDeathNotifier::DeathNotifier> IMediaResourceManagerDeathNotifier::sDeathNotifier;
SortedVector< wp<IMediaResourceManagerDeathNotifier> > IMediaResourceManagerDeathNotifier::sObitRecipients;


const sp<IMediaResourceManagerService>&
IMediaResourceManagerDeathNotifier::getMediaResourceManagerService()
{
    DN_LOGV("getMediaResourceManagerService");
    Mutex::Autolock _l(sServiceLock);
    if (sMediaResourceManagerService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("media.resource_manager"));
            if (binder != 0) {
                break;
             }
             DN_LOGW("Media resource manager service not published, waiting...");
             usleep(500000); 
        } while(true);

        if (sDeathNotifier == NULL) {
        sDeathNotifier = new DeathNotifier();
    }
    binder->linkToDeath(sDeathNotifier);
    sMediaResourceManagerService = interface_cast<IMediaResourceManagerService>(binder);
    }
    DN_LOGE_IF(sMediaResourceManagerService == 0, "no media player service!?");
    return sMediaResourceManagerService;
}

 void
IMediaResourceManagerDeathNotifier::addObitRecipient(const wp<IMediaResourceManagerDeathNotifier>& recipient)
{
    Mutex::Autolock _l(sServiceLock);
    sObitRecipients.add(recipient);
}

 void
IMediaResourceManagerDeathNotifier::removeObitRecipient(const wp<IMediaResourceManagerDeathNotifier>& recipient)
{
    Mutex::Autolock _l(sServiceLock);
    sObitRecipients.remove(recipient);
}

void
IMediaResourceManagerDeathNotifier::DeathNotifier::binderDied(const wp<IBinder>& who)
{
    DN_LOGW("media resource manager service died");
    
    SortedVector< wp<IMediaResourceManagerDeathNotifier> > list;
    {
        Mutex::Autolock _l(sServiceLock);
        sMediaResourceManagerService.clear();
        list = sObitRecipients;
    }

    
    
    
    size_t count = list.size();
    for (size_t iter = 0; iter < count; ++iter) {
        sp<IMediaResourceManagerDeathNotifier> notifier = list[iter].promote();
        if (notifier != 0) {
            notifier->died();
        }
    }
}

IMediaResourceManagerDeathNotifier::DeathNotifier::~DeathNotifier()
{
    Mutex::Autolock _l(sServiceLock);
    sObitRecipients.clear();
    if (sMediaResourceManagerService != 0) {
        sMediaResourceManagerService->asBinder()->unlinkToDeath(this);
    }
}

}; 
