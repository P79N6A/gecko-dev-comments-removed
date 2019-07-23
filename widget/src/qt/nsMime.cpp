







































#include "nsDragService.h"
#include "nsIServiceManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"
#include "nsReadableUtils.h"
#include "nsClipboard.h"
#include "nsMime.h"
#include "nsCRT.h"

#include <qapplication.h>
#include <qclipboard.h>


nsMimeStoreData::nsMimeStoreData(const QCString &name, const QByteArray &data)
{
    flavorName = name;
    flavorData = data;
}

nsMimeStoreData::nsMimeStoreData(const char *name, void *rawdata, PRInt32 rawlen)
{
    flavorName = name;
    flavorData.assign((char*)rawdata,(unsigned int)rawlen);
}


nsMimeStore::nsMimeStore()
{
    mMimeStore.setAutoDelete(TRUE);
}

nsMimeStore::~nsMimeStore()
{
}

const char* nsMimeStore::format(int n) const
{
    if (n >= (int)mMimeStore.count())
        return 0;

    

    const nsMimeStoreData* msd;
    msd = mMimeStore.at(n);

    return msd->flavorName;
}

QByteArray nsMimeStore::encodedData(const char* name) const
{
    QByteArray qba;

    QString mime(name);

    



    int n = mime.findRev(";");
    mime = mime.remove(n, mime.length() - n);

    
    nsMimeStoreData* msd;
    QPtrList<nsMimeStoreData>::const_iterator it = mMimeStore.begin();
    while ((msd = *it)) {
        if (mime.utf8() == msd->flavorName) {
            qba = msd->flavorData;
            return qba;
        }
        ++it;
    }
#ifdef NS_DEBUG
    printf("nsMimeStore::encodedData requested unknown %s\n", name);
#endif
    return qba;
}

PRBool nsMimeStore::ContainsFlavor(const char* name)
{
    for (nsMimeStoreData *msd = mMimeStore.first(); msd; msd = mMimeStore.next()) {
        if (!strcmp(name, msd->flavorName))
            return PR_TRUE;
    }
    return PR_FALSE;
}

PRBool nsMimeStore::AddFlavorData(const char* name, const QByteArray &data)
{
    if (ContainsFlavor(name))
        return PR_FALSE;
    mMimeStore.append(new nsMimeStoreData(name, data));

    
    
    if (strcmp(name, kUnicodeMime) || ContainsFlavor(kTextMime))
        return PR_TRUE;
    
    

    
    
    mMimeStore.insert(0,new nsMimeStoreData(kTextMime,data.data(),data.count() + 1));
    return PR_TRUE;
}


nsDragObject::nsDragObject(nsMimeStore* mimeStore,QWidget* dragSource,
                           const char* name)
    : QDragObject(dragSource, name)
{
    if (!mimeStore)
        NS_ASSERTION(PR_TRUE, "Invalid  pointer.");

    mMimeStore = mimeStore;
}

nsDragObject::~nsDragObject()
{
    delete mMimeStore;
}

const char* nsDragObject::format(int i) const
{
    if (i >= (int)mMimeStore->count())
        return 0;

    const char* frm = mMimeStore->format(i);
#ifdef NS_DEBUG
    printf("nsDragObject::format i=%i %s\n",i, frm);
#endif
    return frm;
}

QByteArray nsDragObject::encodedData(const char* frm) const
{
#ifdef NS_DEBUG
    printf("nsDragObject::encodedData %s\n",frm);
#endif
    return mMimeStore->encodedData(frm);
}
