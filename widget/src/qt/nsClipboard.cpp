








































#include <QApplication>
#include <QMimeData>
#include <QString>
#include <QStringList>

#include "nsClipboard.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"

NS_IMPL_ISUPPORTS1(nsClipboard, nsIClipboard)






nsClipboard::nsClipboard() : nsIClipboard(),
                             mSelectionOwner(nsnull),
                             mGlobalOwner(nsnull),
                             mSelectionTransferable(nsnull),
                             mGlobalTransferable(nsnull)
{
    
}






nsClipboard::~nsClipboard()
{
}




NS_IMETHODIMP
nsClipboard::SetNativeClipboardData( nsITransferable *aTransferable,
                                     QClipboard::Mode clipboardMode )
{
    if (nsnull == aTransferable)
    {
        qDebug("nsClipboard::SetNativeClipboardData(): no transferable!");
        return NS_ERROR_FAILURE;
    }

    
    
    nsCOMPtr<nsISupportsArray> flavorList;
    nsresult errCode = aTransferable->FlavorsTransferableCanExport( getter_AddRefs(flavorList) );

    if (NS_FAILED(errCode))
    {
        qDebug("nsClipboard::SetNativeClipboardData(): no FlavorsTransferable !");
        return NS_ERROR_FAILURE;
    }

    QClipboard *cb = QApplication::clipboard();
    PRUint32 flavorCount = 0;
    flavorList->Count(&flavorCount);

    for (PRUint32 i = 0; i < flavorCount; ++i)
    {
        nsCOMPtr<nsISupports> genericFlavor;
        flavorList->GetElementAt(i,getter_AddRefs(genericFlavor));
        nsCOMPtr<nsISupportsCString> currentFlavor(do_QueryInterface(genericFlavor));
        
        if (currentFlavor)
        {
            
            nsXPIDLCString flavorStr;
            currentFlavor->ToString(getter_Copies(flavorStr));

            
            nsCOMPtr<nsISupports> clip;
            
            PRUint32 len;

            
            if (!strcmp(flavorStr.get(), kUnicodeMime))
            {
                aTransferable->GetTransferData(flavorStr,getter_AddRefs(clip),&len);
                nsCOMPtr<nsISupportsString> wideString;
                wideString = do_QueryInterface(clip);
                if (!wideString)
                {
                    return NS_ERROR_FAILURE;
                }

                nsAutoString utf16string;
                wideString->GetData(utf16string);
                QString str = QString::fromUtf16(utf16string.get());

                
                cb->setText(str, clipboardMode);
                break;
            }

            if ( !strcmp(flavorStr.get(), kNativeImageMime)
              || !strcmp(flavorStr.get(), kPNGImageMime)
              || !strcmp(flavorStr.get(), kJPEGImageMime)
              || !strcmp(flavorStr.get(), kGIFImageMime))
            {
                qDebug("nsClipboard::SetNativeClipboardData(): Copying image data not implemented!");
            }
        }
    }

    return NS_OK;
}



NS_IMETHODIMP
nsClipboard::GetNativeClipboardData(nsITransferable *aTransferable,
                                    QClipboard::Mode clipboardMode)
{
    if (nsnull == aTransferable)
    {
        qDebug("  GetNativeClipboardData: Transferable is null!");
        return NS_ERROR_FAILURE;
    }

    
    
    nsCOMPtr<nsISupportsArray> flavorList;
    nsresult errCode = aTransferable->FlavorsTransferableCanImport(
        getter_AddRefs(flavorList));

    if (NS_FAILED(errCode))
    {
        qDebug("nsClipboard::GetNativeClipboardData(): no FlavorsTransferable %i !",
               errCode);
        return NS_ERROR_FAILURE;
    }

    QClipboard *cb = QApplication::clipboard();
    const QMimeData *mimeData = cb->mimeData(clipboardMode);

    
    PRUint32 flavorCount;
    flavorList->Count(&flavorCount);
    nsCAutoString foundFlavor;

    for (PRUint32 i = 0; i < flavorCount; ++i)
    {
        nsCOMPtr<nsISupports> genericFlavor;
        flavorList->GetElementAt(i,getter_AddRefs(genericFlavor));
        nsCOMPtr<nsISupportsCString> currentFlavor(do_QueryInterface( genericFlavor) );

        if (currentFlavor)
        {
            nsXPIDLCString flavorStr;
            currentFlavor->ToString(getter_Copies(flavorStr));

            
            
            if (!strcmp(flavorStr.get(), kUnicodeMime)) 
            {
                if (mimeData->hasText())
                {
                    
                    
                    foundFlavor = nsCAutoString(flavorStr);

                    
                    QString text = mimeData->text();
                    const QChar *unicode = text.unicode();
                    
                    PRUint32 len = (PRUint32) 2*text.size();

                    
                    nsCOMPtr<nsISupports> genericDataWrapper;
                    nsPrimitiveHelpers::CreatePrimitiveForData(
                        foundFlavor.get(),
                        (void*)unicode,
                        len,
                        getter_AddRefs(genericDataWrapper));

                    
                    aTransferable->SetTransferData(foundFlavor.get(),
                                                   genericDataWrapper,len);
                    
                    break;
                }
            }

            
            if (!strcmp(flavorStr.get(), kJPEGImageMime)
             || !strcmp(flavorStr.get(), kPNGImageMime)
             || !strcmp(flavorStr.get(), kGIFImageMime))
            {
                qDebug("nsClipboard::GetNativeClipboardData(): Pasting image data not implemented!");
                break;
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(const char** aFlavorList, PRUint32 aLength,
                                    PRInt32 aWhichClipboard, PRBool *_retval)
{
    *_retval = PR_FALSE;
    if (aWhichClipboard != kGlobalClipboard)
        return NS_OK;

    
    QClipboard *cb = QApplication::clipboard();
    const QMimeData *mimeData = cb->mimeData();
    const char *flavor=NULL;
    QStringList formats = mimeData->formats();
    
    QString utf8text("text/plain;charset=utf-8");
    
    for (PRUint32 i = 0; i < aLength; ++i)
    {
        flavor = aFlavorList[i];
        if (flavor)
        {
            QString qflavor(flavor);

            if (strcmp(flavor,kTextMime) == 0)
            {
                NS_WARNING("DO NOT USE THE text/plain DATA FLAVOR ANY MORE. USE text/unicode INSTEAD");
            }

            
            
            if (formats.contains(qflavor) ||
                ((strcmp(flavor, kUnicodeMime) == 0) && formats.contains(utf8text)))
            {
                
                *_retval = PR_TRUE;
                break;
            }
        }
    }
    return NS_OK;
}




NS_IMETHODIMP
nsClipboard::SetData(nsITransferable *aTransferable,
                     nsIClipboardOwner *aOwner,
                     PRInt32 aWhichClipboard)
{
    
    if (
        (aWhichClipboard == kGlobalClipboard
           && aTransferable == mGlobalTransferable.get()
           && aOwner == mGlobalOwner.get()
        )
       ||
        (aWhichClipboard == kSelectionClipboard
         && aTransferable == mSelectionTransferable.get()
         && aOwner == mSelectionOwner.get()
        )
       )
    {
        return NS_OK;
    }

    nsresult rv;
    if (!mPrivacyHandler) {
      rv = NS_NewClipboardPrivacyHandler(getter_AddRefs(mPrivacyHandler));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = mPrivacyHandler->PrepareDataForClipboard(aTransferable);
    NS_ENSURE_SUCCESS(rv, rv);

    EmptyClipboard(aWhichClipboard);

    QClipboard::Mode mode;

    if (kGlobalClipboard == aWhichClipboard)
    {
        mGlobalOwner = aOwner;
        mGlobalTransferable = aTransferable;

        mode = QClipboard::Clipboard;
    }
    else
    {
        mSelectionOwner = aOwner;
        mSelectionTransferable = aTransferable;

        mode = QClipboard::Selection;
    }
    return SetNativeClipboardData( aTransferable, mode );
}




NS_IMETHODIMP
nsClipboard::GetData(nsITransferable *aTransferable, PRInt32 aWhichClipboard)
{
    if (nsnull != aTransferable)
    {
        QClipboard::Mode mode;
        if (kGlobalClipboard == aWhichClipboard)
        {
            mode = QClipboard::Clipboard;
        }
        else
        {
            mode = QClipboard::Selection;
        }
        return GetNativeClipboardData(aTransferable, mode);
    }
    else
    {
        qDebug("  nsClipboard::GetData(), aTransferable is NULL.");
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsClipboard::EmptyClipboard(PRInt32 aWhichClipboard)
{
    if (aWhichClipboard == kSelectionClipboard)
    {
        if (mSelectionOwner)
        {
            mSelectionOwner->LosingOwnership(mSelectionTransferable);
            mSelectionOwner = nsnull;
        }
        mSelectionTransferable = nsnull;
    }
    else
    {
        if (mGlobalOwner)
        {
            mGlobalOwner->LosingOwnership(mGlobalTransferable);
            mGlobalOwner = nsnull;
        }
        mGlobalTransferable = nsnull;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsSelectionClipboard(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    QClipboard *cb = QApplication::clipboard();
    if (cb->supportsSelection())
    {
        *_retval = PR_TRUE; 
    }
    else
    {
        *_retval = PR_FALSE;
    }

    return NS_OK;
}
