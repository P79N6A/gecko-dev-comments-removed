






































#ifndef nsMime_h__
#define nsMime_h__

#include "nsITransferable.h"
#include "nsCOMPtr.h"

#include <qcstring.h>
#include <qmime.h>
#include <qwidget.h>
#include <qdragobject.h>

class nsMimeStoreData
{
public:
   nsMimeStoreData(const QCString &name, const QByteArray &data);
   nsMimeStoreData(const char *name, void *rawdata, PRInt32 rawlen);

   QCString    flavorName;
   QByteArray  flavorData;
};

class nsMimeStore: public QMimeSource
{
public:
    nsMimeStore();
    virtual ~nsMimeStore();

    virtual const char* format(int n = 0) const ;
    virtual QByteArray encodedData(const char*) const;

    PRBool AddFlavorData(const char* name, const QByteArray &data);
    PRBool ContainsFlavor(const char* name);
    PRUint32  count();

protected:
    mutable QPtrList<nsMimeStoreData> mMimeStore;
    nsMimeStoreData*       at(int n);
};

inline PRUint32 nsMimeStore::count() { return mMimeStore.count(); }


class nsDragObject : public QDragObject
{
    Q_OBJECT
public:
    nsDragObject(nsMimeStore* mimeStore,QWidget* dragSource = 0,
                 const char* name = 0);
    ~nsDragObject();

    const char* format(int i) const;
    virtual QByteArray encodedData(const char*) const;

protected:
    nsMimeStore* mMimeStore;
};

#endif
