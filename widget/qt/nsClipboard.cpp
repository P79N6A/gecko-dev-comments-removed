



#include <QApplication>
#include <QMimeData>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QImage>
#include <QImageWriter>
#include <QBuffer>

#include "mozilla/Util.h"

#include "nsClipboard.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"
#include "nsIInputStream.h"
#include "nsReadableUtils.h"
#include "nsStringStream.h"
#include "nsComponentManagerUtils.h"

#include "imgIContainer.h"
#include "gfxImageSurface.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS1(nsClipboard, nsIClipboard)






nsClipboard::nsClipboard() : nsIClipboard(),
                             mSelectionOwner(nullptr),
                             mGlobalOwner(nullptr),
                             mSelectionTransferable(nullptr),
                             mGlobalTransferable(nullptr)
{
    
}






nsClipboard::~nsClipboard()
{
}

static inline QImage::Format
_gfximage_to_qformat(gfxASurface::gfxImageFormat aFormat)
{
    switch (aFormat) {
    case gfxASurface::ImageFormatARGB32:
        return QImage::Format_ARGB32_Premultiplied;
    case gfxASurface::ImageFormatRGB24:
        return QImage::Format_ARGB32;
    case gfxASurface::ImageFormatRGB16_565:
        return QImage::Format_RGB16;
    default:
        return QImage::Format_Invalid;
    }
}



NS_IMETHODIMP
nsClipboard::SetNativeClipboardData( nsITransferable *aTransferable,
                                     QClipboard::Mode clipboardMode )
{
    if (nullptr == aTransferable)
    {
        NS_WARNING("nsClipboard::SetNativeClipboardData(): no transferable!");
        return NS_ERROR_FAILURE;
    }

    
    
    nsCOMPtr<nsISupportsArray> flavorList;
    nsresult rv = aTransferable->FlavorsTransferableCanExport( getter_AddRefs(flavorList) );

    if (NS_FAILED(rv))
    {
        NS_WARNING("nsClipboard::SetNativeClipboardData(): no FlavorsTransferable !");
        return NS_ERROR_FAILURE;
    }

    QClipboard *cb = QApplication::clipboard();
    QMimeData *mimeData = new QMimeData;

    uint32_t flavorCount = 0;
    flavorList->Count(&flavorCount);
    bool imageAdded = false;

    for (uint32_t i = 0; i < flavorCount; ++i)
    {
        nsCOMPtr<nsISupports> genericFlavor;
        flavorList->GetElementAt(i,getter_AddRefs(genericFlavor));
        nsCOMPtr<nsISupportsCString> currentFlavor(do_QueryInterface(genericFlavor));
        
        if (currentFlavor)
        {
            
            nsXPIDLCString flavorStr;
            currentFlavor->ToString(getter_Copies(flavorStr));

            
            nsCOMPtr<nsISupports> clip;
            
            uint32_t len;

            
            if (!strcmp(flavorStr.get(), kUnicodeMime))
            {
                rv = aTransferable->GetTransferData(flavorStr,getter_AddRefs(clip),&len);
                nsCOMPtr<nsISupportsString> wideString;
                wideString = do_QueryInterface(clip);
                if (!wideString || NS_FAILED(rv))
                    continue;

                nsAutoString utf16string;
                wideString->GetData(utf16string);
                QString str = QString::fromUtf16(utf16string.get());

                
                mimeData->setText(str);
            }

            
            else if (!strcmp(flavorStr.get(), kHTMLMime))
            {
                rv = aTransferable->GetTransferData(flavorStr,getter_AddRefs(clip),&len);
                nsCOMPtr<nsISupportsString> wideString;
                wideString = do_QueryInterface(clip);
                if (!wideString || NS_FAILED(rv))
                    continue;

                nsAutoString utf16string;
                wideString->GetData(utf16string);
                QString str = QString::fromUtf16(utf16string.get());

                
                mimeData->setHtml(str);
            }

            
            else if (!imageAdded 
                     && (!strcmp(flavorStr.get(), kNativeImageMime)
                     ||  !strcmp(flavorStr.get(), kPNGImageMime)
                     ||  !strcmp(flavorStr.get(), kJPEGImageMime)
                     ||  !strcmp(flavorStr.get(), kJPGImageMime)
                     ||  !strcmp(flavorStr.get(), kGIFImageMime))
                    )
            {
                
                static const char* const imageMimeTypes[] = {
                    kNativeImageMime, kPNGImageMime, kJPEGImageMime, kJPGImageMime, kGIFImageMime };
                nsCOMPtr<nsISupportsInterfacePointer> ptrPrimitive;
                for (uint32_t i = 0; !ptrPrimitive && i < ArrayLength(imageMimeTypes); i++)
                {
                    aTransferable->GetTransferData(imageMimeTypes[i], getter_AddRefs(clip), &len);
                    ptrPrimitive = do_QueryInterface(clip);
                }

                if (!ptrPrimitive)
                    continue;

                nsCOMPtr<nsISupports> primitiveData;
                ptrPrimitive->GetData(getter_AddRefs(primitiveData));
                nsCOMPtr<imgIContainer> image(do_QueryInterface(primitiveData));
                if (!image)  
                   continue;

                nsRefPtr<gfxASurface> surface;
                image->GetFrame(imgIContainer::FRAME_CURRENT,
                                imgIContainer::FLAG_SYNC_DECODE,
                                getter_AddRefs(surface));
                if (!surface)
                  continue;

                nsRefPtr<gfxImageSurface> frame(surface->GetAsReadableARGB32ImageSurface());
                if (!frame)
                  continue;

                QImage qImage(frame->Data(),
                              frame->Width(),
                              frame->Height(),
                              frame->Stride(),
                              _gfximage_to_qformat(frame->Format()));

                
                mimeData->setImageData(qImage);
                imageAdded = true;
            }

            
            else
            {
                rv = aTransferable->GetTransferData(flavorStr.get(), getter_AddRefs(clip), &len);
                
                if (!clip || NS_FAILED(rv))
                    continue;

                void *primitive_data = nullptr;
                nsPrimitiveHelpers::CreateDataFromPrimitive(flavorStr.get(), clip,
                                                            &primitive_data, len);

                if (primitive_data)
                {
                    QByteArray data ((const char *)primitive_data, len);
                    
                    mimeData->setData(flavorStr.get(), data);
                    nsMemory::Free(primitive_data);
                }
            }
        }
    }

    
    if(!mimeData->formats().isEmpty())
        cb->setMimeData(mimeData, clipboardMode);
    else
        delete mimeData;

    return NS_OK;
}



NS_IMETHODIMP
nsClipboard::GetNativeClipboardData(nsITransferable *aTransferable,
                                    QClipboard::Mode clipboardMode)
{
    if (nullptr == aTransferable)
    {
        NS_WARNING("GetNativeClipboardData: Transferable is null!");
        return NS_ERROR_FAILURE;
    }

    
    
    nsCOMPtr<nsISupportsArray> flavorList;
    nsresult errCode = aTransferable->FlavorsTransferableCanImport(
        getter_AddRefs(flavorList));

    if (NS_FAILED(errCode))
    {
        NS_WARNING("nsClipboard::GetNativeClipboardData(): no FlavorsTransferable!");
        return NS_ERROR_FAILURE;
    }

    QClipboard *cb = QApplication::clipboard();
    const QMimeData *mimeData = cb->mimeData(clipboardMode);

    
    uint32_t flavorCount;
    flavorList->Count(&flavorCount);
    nsAutoCString foundFlavor;

    for (uint32_t i = 0; i < flavorCount; ++i)
    {
        nsCOMPtr<nsISupports> genericFlavor;
        flavorList->GetElementAt(i,getter_AddRefs(genericFlavor));
        nsCOMPtr<nsISupportsCString> currentFlavor(do_QueryInterface( genericFlavor) );

        if (currentFlavor)
        {
            nsXPIDLCString flavorStr;
            currentFlavor->ToString(getter_Copies(flavorStr));

            
            
            if (!strcmp(flavorStr.get(), kUnicodeMime) && mimeData->hasText())
            {
                
                
                foundFlavor = nsAutoCString(flavorStr);

                
                QString text = mimeData->text();
                const QChar *unicode = text.unicode();
                
                uint32_t len = (uint32_t) 2*text.size();

                
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

            
            if (!strcmp(flavorStr.get(), kHTMLMime) && mimeData->hasHtml())
            {
                
                
                foundFlavor = nsAutoCString(flavorStr);

                
                QString html = mimeData->html();
                const QChar *unicode = html.unicode();
                
                uint32_t len = (uint32_t) 2*html.size();

                
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

            
            if ((  !strcmp(flavorStr.get(), kJPEGImageMime)
                || !strcmp(flavorStr.get(), kJPGImageMime)
                || !strcmp(flavorStr.get(), kPNGImageMime)
                || !strcmp(flavorStr.get(), kGIFImageMime))
                && mimeData->hasImage())
            {
                
                QImage image = cb->image();
                if(image.isNull())
                    continue;

                
                QByteArray imageFormat;
                if (!strcmp(flavorStr.get(), kJPEGImageMime) || !strcmp(flavorStr.get(), kJPGImageMime))
                    imageFormat = "jpeg";
                else if (!strcmp(flavorStr.get(), kPNGImageMime))
                    imageFormat = "png";
                else if (!strcmp(flavorStr.get(), kGIFImageMime))
                    imageFormat = "gif";
                else
                    continue;

                
                QByteArray imageData;
                QBuffer imageBuffer(&imageData);
                QImageWriter imageWriter(&imageBuffer, imageFormat);
                if(!imageWriter.write(image))
                    continue;

                
                nsCOMPtr<nsIInputStream> byteStream;
                NS_NewByteInputStream(getter_AddRefs(byteStream), imageData.constData(),
                                      imageData.size(), NS_ASSIGNMENT_COPY);
                
                aTransferable->SetTransferData(flavorStr, byteStream, sizeof(nsIInputStream*));

                imageBuffer.close();

                
                break;
            }

            
            
            if(mimeData->hasFormat(flavorStr.get()))
            {
                
                QByteArray clipboardData = mimeData->data(flavorStr.get());
                
                nsCOMPtr<nsISupports> genericDataWrapper;
                nsPrimitiveHelpers::CreatePrimitiveForData(
                        foundFlavor.get(),
                        (void*) clipboardData.data(),
                        clipboardData.size(),
                        getter_AddRefs(genericDataWrapper));

                
                aTransferable->SetTransferData(foundFlavor.get(),
                                               genericDataWrapper,clipboardData.size());
                
                break;
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(const char** aFlavorList, uint32_t aLength,
                                    int32_t aWhichClipboard, bool *_retval)
{
    *_retval = false;
    if (aWhichClipboard != kGlobalClipboard)
        return NS_OK;

    
    QClipboard *cb = QApplication::clipboard();
    const QMimeData *mimeData = cb->mimeData();
    const char *flavor=NULL;
    QStringList formats = mimeData->formats();
    for (uint32_t i = 0; i < aLength; ++i)
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
                strcmp(flavor, kUnicodeMime) == 0)
            {
                
                *_retval = true;
                break;
            }
        }
    }
    return NS_OK;
}




NS_IMETHODIMP
nsClipboard::SetData(nsITransferable *aTransferable,
                     nsIClipboardOwner *aOwner,
                     int32_t aWhichClipboard)
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
nsClipboard::GetData(nsITransferable *aTransferable, int32_t aWhichClipboard)
{
    if (nullptr != aTransferable)
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
        NS_WARNING("nsClipboard::GetData(), aTransferable is NULL.");
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsClipboard::EmptyClipboard(int32_t aWhichClipboard)
{
    if (aWhichClipboard == kSelectionClipboard)
    {
        if (mSelectionOwner)
        {
            mSelectionOwner->LosingOwnership(mSelectionTransferable);
            mSelectionOwner = nullptr;
        }
        mSelectionTransferable = nullptr;
    }
    else
    {
        if (mGlobalOwner)
        {
            mGlobalOwner->LosingOwnership(mGlobalTransferable);
            mGlobalOwner = nullptr;
        }
        mGlobalTransferable = nullptr;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsSelectionClipboard(bool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    QClipboard *cb = QApplication::clipboard();
    if (cb->supportsSelection())
    {
        *_retval = true; 
    }
    else
    {
        *_retval = false;
    }

    return NS_OK;
}
