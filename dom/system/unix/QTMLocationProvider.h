



#ifndef QTMLocationProvider_h
#define QTMLocationProvider_h

#include <QGeoPositionInfoSource>
#include "nsGeolocation.h"
#include "nsIGeolocationProvider.h"
#include "nsCOMPtr.h"


using namespace QtMobility;

class QTMLocationProvider : public QObject,
                            public nsIGeolocationProvider
{
    Q_OBJECT

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIGEOLOCATIONPROVIDER

    QTMLocationProvider();

public Q_SLOTS:
    
    void positionUpdated(const QGeoPositionInfo&);

private:
    ~QTMLocationProvider();

    QGeoPositionInfoSource* mLocation;
    nsCOMPtr<nsIGeolocationUpdate> mCallback;
};

#endif 
