




































#ifndef NSQTNETWORKMANAGER_H_
#define NSQTNETWORKMANAGER_H_

#include <QNetworkConfigurationManager>
#include <QObject>
#include <QTimer>
#include <QNetworkConfiguration>
#include <QNetworkSession>
#include "nscore.h"

class nsQtNetworkManager;

static nsQtNetworkManager* gQtNetworkManager = nsnull;

class nsQtNetworkManager : public QObject
{
  Q_OBJECT
  public:
    explicit nsQtNetworkManager(QObject* parent = 0);

    virtual ~nsQtNetworkManager();

    static PRBool IsConnected();
    static PRBool GetLinkStatusKnown();
    static void enableInstance();
    PRBool openConnection(const QString&);
    PRBool isOnline();
  signals:
    void openConnectionSignal();

  public slots:
    void closeSession();
    void onlineStateChanged(bool);

  private slots:
    void openSession();

  private:
    QNetworkSession* networkSession;
    QNetworkConfiguration networkConfiguration;
    QNetworkConfigurationManager networkConfigurationManager;
    QTimer mBlockTimer;
    bool mOnline;
};

#endif 

