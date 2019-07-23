







































#include <string.h>

#include "nscore.h"
#include "plstr.h"
#include "prlink.h"

#include "nsSound.h"

#include "nsIURL.h"
#include "nsIFileURL.h"
#include "nsNetUtil.h"
#include "nsCOMPtr.h"

#include <qapplication.h>
#include <stdio.h>
#include <unistd.h>

NS_IMPL_ISUPPORTS1(nsSound, nsISound)


nsSound::nsSound()
{
}

nsSound::~nsSound()
{
}

NS_IMETHODIMP
nsSound::Init()
{
    return NS_OK;
}

NS_METHOD nsSound::Beep()
{
    QApplication::beep();

    return NS_OK;
}

NS_METHOD nsSound::Play(nsIURL *aURL)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
    if (aSoundAlias.EqualsLiteral("_moz_mailbeep")) {
        QApplication::beep();
        return NS_OK;
    }

    return NS_ERROR_FAILURE;

















}
