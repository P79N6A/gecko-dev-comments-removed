






































#include "nsSystemSoundService.h"

#include <QApplication>
#include <QSound>





NS_IMPL_ISUPPORTS1(nsSystemSoundService, nsISystemSoundService)

NS_IMPL_ISYSTEMSOUNDSERVICE_GETINSTANCE(nsSystemSoundService)

nsSystemSoundService::nsSystemSoundService() :
    nsSystemSoundServiceBase()
{
}

nsSystemSoundService::~nsSystemSoundService()
{
}

NS_IMETHODIMP
nsSystemSoundService::Beep()
{
    nsresult rv = nsSystemSoundServiceBase::Beep();
    NS_ENSURE_SUCCESS(rv, rv);

    QApplication::beep();
    return NS_OK;
}

NS_IMETHODIMP
nsSystemSoundService::PlayEventSound(PRUint32 aEventID)
{
    nsresult rv = nsSystemSoundServiceBase::PlayEventSound(aEventID);
    NS_ENSURE_SUCCESS(rv, rv);

    if (aEventID == EVENT_NEW_MAIL_RECEIVED) {
        StopSoundPlayer();
        return Beep();
    }
    return NS_OK;
}
