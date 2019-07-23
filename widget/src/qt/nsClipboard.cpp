








































#include "nsClipboard.h"
#include "nsMime.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsISupportsArray.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"
#include "nsIComponentManager.h"
#include "nsWidgetsCID.h"

#include <qapplication.h>
#include <qclipboard.h>
#include <qdragobject.h>

NS_IMPL_ISUPPORTS1(nsClipboard, nsIClipboard)






nsClipboard::nsClipboard()
    : nsIClipboard(),
      mIgnoreEmptyNotification(PR_FALSE),
      mSelectionOwner(nsnull),
      mGlobalOwner(nsnull),
      mSelectionTransferable(nsnull),
      mGlobalTransferable(nsnull)
{
    qDebug("###############################################################s");
}






nsClipboard::~nsClipboard()
{
}


NS_IMETHODIMP
nsClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
    qDebug("SetNativeClipboardData");
    mIgnoreEmptyNotification = PR_TRUE;

    nsCOMPtr<nsITransferable> transferable(
        getter_AddRefs(GetTransferable(aWhichClipboard)));

    
    if (nsnull == transferable) {
        qDebug("nsClipboard::SetNativeClipboardData(): no transferable!\n");
        return NS_ERROR_FAILURE;
    }
    
    
    nsCOMPtr<nsISupportsArray> flavorList;
    nsresult errCode = transferable->FlavorsTransferableCanExport(
        getter_AddRefs(flavorList));

    if (NS_FAILED(errCode)) {
        qDebug("nsClipboard::SetNativeClipboardData(): no FlavorsTransferable !\n");
        return NS_ERROR_FAILURE;
    }
    QClipboard *cb = QApplication::clipboard();
    nsMimeStore *mimeStore =  new nsMimeStore();
    PRUint32 cnt;

    flavorList->Count(&cnt);
    for (PRUint32 i = 0; i < cnt; ++i) {
        nsCOMPtr<nsISupports> genericFlavor;

        flavorList->GetElementAt(i,getter_AddRefs(genericFlavor));

        nsCOMPtr<nsISupportsCString> currentFlavor(do_QueryInterface(genericFlavor));

        if (currentFlavor) {
            nsXPIDLCString flavorStr;

            currentFlavor->ToString(getter_Copies(flavorStr));

            
            PRUint32   len;
            nsCOMPtr<nsISupports> clip;

            transferable->GetTransferData(flavorStr,getter_AddRefs(clip),&len);

            nsCOMPtr<nsISupportsString> wideString;
            wideString = do_QueryInterface(clip);
            if (!wideString)
                return NS_ERROR_FAILURE;

            nsAutoString ucs2string;
            wideString->GetData(ucs2string);
            QString str = QString::fromUcs2(ucs2string.get());
            qDebug("HERE %s '%s'", flavorStr.get(), str.latin1());

            mimeStore->AddFlavorData(flavorStr,str.utf8());
        }
    }
    cb->setData(mimeStore);
    mIgnoreEmptyNotification = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::GetNativeClipboardData(nsITransferable *aTransferable,
                                    PRInt32 aWhichClipboard)
{
    qDebug("GetNativeClipboardData");
    
    if (nsnull == aTransferable) {
        qDebug("  GetNativeClipboardData: Transferable is null!\n");
        return NS_ERROR_FAILURE;
    }
    
    
    nsCOMPtr<nsISupportsArray> flavorList;
    nsresult errCode = aTransferable->FlavorsTransferableCanImport(
        getter_AddRefs(flavorList));

    if (NS_FAILED(errCode)) {
        qDebug("nsClipboard::GetNativeClipboardData(): no FlavorsTransferable %i !\n",
               errCode);
        return NS_ERROR_FAILURE;
    }
    QClipboard *cb = QApplication::clipboard();
    QMimeSource *ms = cb->data();

    
    PRUint32 cnt;

    flavorList->Count(&cnt);

    nsCAutoString foundFlavor;
    for (PRUint32 i = 0; i < cnt; ++i) {
        nsCOMPtr<nsISupports> genericFlavor;

        flavorList->GetElementAt(i,getter_AddRefs(genericFlavor));
        nsCOMPtr<nsISupportsCString> currentFlavor(do_QueryInterface(
                                                       genericFlavor));

        if (currentFlavor) {
            nsXPIDLCString flavorStr;

            currentFlavor->ToString(getter_Copies(flavorStr));
            foundFlavor = nsCAutoString(flavorStr);

            if (ms->provides((const char*)flavorStr)) {
                QByteArray ba = ms->encodedData((const char*)flavorStr);
                nsCOMPtr<nsISupports> genericDataWrapper;
                PRUint32 len = (PRUint32)ba.count();

                nsPrimitiveHelpers::CreatePrimitiveForData(
                    foundFlavor.get(),
                    (void*)ba.data(),len,
                    getter_AddRefs(genericDataWrapper));

                aTransferable->SetTransferData(foundFlavor.get(),
                                               genericDataWrapper,len);
            }
        }
    }
    return NS_OK;
}


nsITransferable *
nsClipboard::GetTransferable(PRInt32 aWhichClipboard)
{
    qDebug("GetTransferable");
    nsITransferable *retval;

    if (aWhichClipboard == kSelectionClipboard)
        retval = mSelectionTransferable.get();
    else
        retval = mGlobalTransferable.get();

    return retval;
}


NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(nsISupportsArray *aFlavorList,
                                    PRInt32 aWhichClipboard,
                                    PRBool           *_retval)
{
    qDebug("HasDataMatchingFlavors");
    *_retval = PR_FALSE;
    if (aWhichClipboard != kGlobalClipboard)
        return NS_OK;

    QClipboard *cb = QApplication::clipboard();
    QMimeSource *ms = cb->data();
    PRUint32 cnt;

    aFlavorList->Count(&cnt);
    for (PRUint32 i = 0;i < cnt; ++i) {
        nsCOMPtr<nsISupports> genericFlavor;

        aFlavorList->GetElementAt(i,getter_AddRefs(genericFlavor));

        nsCOMPtr<nsISupportsCString> currentFlavor(do_QueryInterface(genericFlavor));
        if (currentFlavor) {
            nsXPIDLCString flavorStr;

            currentFlavor->ToString(getter_Copies(flavorStr));

            if (strcmp(flavorStr,kTextMime) == 0)
                NS_WARNING("DO NOT USE THE text/plain DATA FLAVOR ANY MORE. USE text/unicode INSTEAD");

            if (ms->provides((const char*)flavorStr)) {
                *_retval = PR_TRUE;
                qDebug("GetFormat %s\n",(const char*)flavorStr);
                break;
            }
        }
    }
    return NS_OK;
}




NS_IMETHODIMP
nsClipboard::SetData(nsITransferable *aTransferable,
                     nsIClipboardOwner *anOwner,
                     PRInt32 aWhichClipboard)
{
    qDebug("SetData");
    if ((aTransferable == mGlobalTransferable.get()
         && anOwner == mGlobalOwner.get()
         && aWhichClipboard == kGlobalClipboard)
        || (aTransferable == mSelectionTransferable.get()
            && anOwner == mSelectionOwner.get()
            && aWhichClipboard == kSelectionClipboard)) {
        return NS_OK;
    }
    EmptyClipboard(aWhichClipboard);

    switch (aWhichClipboard) {
    case kSelectionClipboard:
        mSelectionOwner = anOwner;
        mSelectionTransferable = aTransferable;
        break;

    case kGlobalClipboard:
        mGlobalOwner = anOwner;
        mGlobalTransferable = aTransferable;
        break;
    }
    QApplication::clipboard()->clear();
    return SetNativeClipboardData(aWhichClipboard);
}




NS_IMETHODIMP
nsClipboard::GetData(nsITransferable *aTransferable,PRInt32 aWhichClipboard)
{
    qDebug("GetData");
    if (nsnull != aTransferable) {
        return GetNativeClipboardData(aTransferable,aWhichClipboard);
    } else {
        qDebug("  nsClipboard::GetData(), aTransferable is NULL.\n");
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsClipboard::EmptyClipboard(PRInt32 aWhichClipboard)
{
    qDebug("EmptyClipoard");
    if (mIgnoreEmptyNotification) {
        return NS_OK;
    }
    switch(aWhichClipboard) {
    case kSelectionClipboard:
        if (mSelectionOwner) {
            mSelectionOwner->LosingOwnership(mSelectionTransferable);
            mSelectionOwner = nsnull;
        }
        mSelectionTransferable = nsnull;
        break;

    case kGlobalClipboard:
        if (mGlobalOwner) {
            mGlobalOwner->LosingOwnership(mGlobalTransferable);
            mGlobalOwner = nsnull;
        }
        mGlobalTransferable = nsnull;
        break;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsSelectionClipboard(PRBool *_retval)
{
    qDebug("SuppotsSelectionClipboard");
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = PR_TRUE; 
    return NS_OK;
}
