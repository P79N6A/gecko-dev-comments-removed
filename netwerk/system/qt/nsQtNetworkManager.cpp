




































#include "nsQtNetworkManager.h"

#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsINetworkLinkService.h"
#include "nsIOService.h"
#include "nsIObserverService.h"
#include "nsIOService.h"

#include <QHostInfo>
#include <QHostAddress>
#include <QTime>

nsQtNetworkManager* nsQtNetworkManager::gQtNetworkManager = nsnull;

void nsQtNetworkManager::create()
{
    if (!gQtNetworkManager) {
        gQtNetworkManager = new nsQtNetworkManager();
        connect(gQtNetworkManager, SIGNAL(openConnectionSignal()),
                gQtNetworkManager, SLOT(openSession()),
                Qt::BlockingQueuedConnection);
        connect(&gQtNetworkManager->networkConfigurationManager,
                SIGNAL(onlineStateChanged(bool)), gQtNetworkManager,
                SLOT(onlineStateChanged(bool)));
    }
}

void nsQtNetworkManager::destroy()
{
    delete gQtNetworkManager;
    gQtNetworkManager = nsnull;
}

nsQtNetworkManager::nsQtNetworkManager(QObject* parent)
  : QObject(parent), networkSession(0)
{
    mOnline = networkConfigurationManager.isOnline();
    NS_ASSERTION(NS_IsMainThread(), "nsQtNetworkManager can only initiated in Main Thread");
}

nsQtNetworkManager::~nsQtNetworkManager()
{
    closeSession();
    networkSession->deleteLater();
}

PRBool
nsQtNetworkManager::isOnline()
{
    static PRBool sForceOnlineUSB = getenv("MOZ_MEEGO_NET_ONLINE") != 0;
    return sForceOnlineUSB || mOnline;
}

void
nsQtNetworkManager::onlineStateChanged(bool online)
{
    mOnline = online;
}














PRBool
nsQtNetworkManager::openConnection(const QString& host)
{
    
    if (isOnline()) {
        return true;
    }

    if (NS_IsMainThread()) {
        openSession();
    } else {
        
        emit openConnectionSignal();
    }

    
    
    
    if (isOnline()) {
        QHostInfo::fromName(host);
    }

    return isOnline();
}

void
nsQtNetworkManager::openSession()
{
    if (mBlockTimer.isActive()) {
        
        
        

        
        
        
        

        
        

        mBlockTimer.stop();
        mBlockTimer.setSingleShot(true);
        mBlockTimer.start(200);
        return;
    }

    if (isOnline()) {
        return;
    }

    
    
    
    if (networkSession) {
        networkSession->close();
        networkSession->deleteLater();
    }

    
    networkConfigurationManager.updateConfigurations();
    
    networkConfiguration = networkConfigurationManager.defaultConfiguration();
    networkSession = new QNetworkSession(networkConfiguration);

    networkSession->open();
    QTime current;
    current.start();
    networkSession->waitForOpened(-1);

    if (current.elapsed() < 1000) {
        NS_WARNING("Connection Creation was to fast, something is not right.");
    }

    mBlockTimer.setSingleShot(true);
    mBlockTimer.start(200);
}

void
nsQtNetworkManager::closeSession()
{
    if (networkSession) {
        networkSession->close();
    }
}
