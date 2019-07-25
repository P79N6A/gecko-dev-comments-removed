





































#include <QNetworkConfigurationManager>
#include <QNetworkConfiguration>
#include <QNetworkSession>

#include "nsQtNetworkManager.h"

#include "nsCOMPtr.h"
#include "nsThreadUtils.h"

#include "nsINetworkLinkService.h"

#include "nsIOService.h"
#include "nsIObserverService.h"
#include "nsIOService.h"

#include "nsINetworkLinkService.h"

static QNetworkConfigurationManager* sNetworkConfig = 0;

PRBool
nsQtNetworkManager::OpenConnectionSync()
{
    if (!sNetworkConfig)
        return PR_FALSE;

    
    if (sNetworkConfig->isOnline())
        return PR_FALSE;

    if (!(sNetworkConfig->capabilities() & QNetworkConfigurationManager::CanStartAndStopInterfaces))
        return PR_FALSE;

    
    QNetworkConfiguration default_cfg = sNetworkConfig->defaultConfiguration();

    if (!default_cfg.isValid())
    {
        NS_WARNING("default configuration is not valid. Looking for any other:");
        foreach (QNetworkConfiguration cfg, sNetworkConfig->allConfigurations())
        {
            if (cfg.isValid())
                default_cfg = cfg;
        }

        if (!default_cfg.isValid())
        {
            NS_WARNING("No valid configuration found. Giving up.");
            return PR_FALSE;
        }
    }

    
    
    QNetworkSession* session = new QNetworkSession(default_cfg);
    QObject::connect(session, SIGNAL(opened()),
                     session, SLOT(deleteLater()));
    QObject::connect(session, SIGNAL(error(QNetworkSession::SessionError)),
                     session, SLOT(deleteLater()));
    session->open();
    return session->waitForOpened(-1);
}

void
nsQtNetworkManager::CloseConnection()
{
    NS_WARNING("nsQtNetworkManager::CloseConnection() Not implemented by QtNetwork.");
}

PRBool
nsQtNetworkManager::IsConnected()
{
    NS_ASSERTION(sNetworkConfig, "Network not initialized");
    return sNetworkConfig->isOnline();
}

PRBool
nsQtNetworkManager::GetLinkStatusKnown()
{
    return IsConnected();
}

PRBool
nsQtNetworkManager::Startup()
{
    
    if (sNetworkConfig)
        return PR_FALSE;

    sNetworkConfig = new QNetworkConfigurationManager();

    return PR_TRUE;
}

void
nsQtNetworkManager::Shutdown()
{
    delete sNetworkConfig;
    sNetworkConfig = nsnull;
}
