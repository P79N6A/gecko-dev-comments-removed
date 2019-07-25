



#ifndef NSQTNETWORKMANAGER_H_
#define NSQTNETWORKMANAGER_H_

#include <QNetworkConfigurationManager>
#include <QObject>
#include <QTimer>
#include <QNetworkConfiguration>
#include <QNetworkSession>
#include "nscore.h"

class nsQtNetworkManager;



class nsQtNetworkManager : public QObject
{
  Q_OBJECT
  public:
    static void create();
    static void destroy();
    virtual ~nsQtNetworkManager();

    static nsQtNetworkManager* get() { return gQtNetworkManager; }

    static bool IsConnected();
    static bool GetLinkStatusKnown();
    static void enableInstance();
    bool openConnection(const QString&);
    bool isOnline();
  signals:
    void openConnectionSignal();

  public slots:
    void closeSession();
    void onlineStateChanged(bool);

  private slots:
    void openSession();

  private:
    explicit nsQtNetworkManager(QObject* parent = 0);

    static nsQtNetworkManager* gQtNetworkManager;
    QNetworkSession* networkSession;
    QNetworkConfiguration networkConfiguration;
    QNetworkConfigurationManager networkConfigurationManager;
    QTimer mBlockTimer;
    bool mOnline;
};

#endif 

