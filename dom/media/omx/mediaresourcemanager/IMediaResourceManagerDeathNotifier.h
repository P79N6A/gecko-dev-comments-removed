
















#ifndef ANDROID_IMEDIARESOURCEMANAGERDEATHNOTIFIER_H
#define ANDROID_IMEDIARESOURCEMANAGERDEATHNOTIFIER_H

#include <utils/threads.h>
#include <utils/SortedVector.h>

#include "IMediaResourceManagerService.h"

namespace android {





class IMediaResourceManagerDeathNotifier: virtual public RefBase
{
public:
    IMediaResourceManagerDeathNotifier() { addObitRecipient(this); }
    virtual ~IMediaResourceManagerDeathNotifier() { removeObitRecipient(this); }

    virtual void died() = 0;
    static const sp<IMediaResourceManagerService>& getMediaResourceManagerService();

private:
    IMediaResourceManagerDeathNotifier &operator=(const IMediaResourceManagerDeathNotifier &);
    IMediaResourceManagerDeathNotifier(const IMediaResourceManagerDeathNotifier &);

    static void addObitRecipient(const wp<IMediaResourceManagerDeathNotifier>& recipient);
    static void removeObitRecipient(const wp<IMediaResourceManagerDeathNotifier>& recipient);

    class DeathNotifier: public IBinder::DeathRecipient
    {
    public:
                DeathNotifier() {}
        virtual ~DeathNotifier();

        virtual void binderDied(const wp<IBinder>& who);
    };

    friend class DeathNotifier;

    static  Mutex                                   sServiceLock;
    static  sp<IMediaResourceManagerService>        sMediaResourceManagerService;
    static  sp<DeathNotifier>                       sDeathNotifier;
    static  SortedVector< wp<IMediaResourceManagerDeathNotifier> > sObitRecipients;
};

}; 

#endif 
