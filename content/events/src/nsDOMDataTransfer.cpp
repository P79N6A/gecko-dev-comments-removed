




































#include "nsDOMDataTransfer.h"

#include "prlog.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsIVariant.h"
#include "nsISupportsPrimitives.h"
#include "nsDOMClassInfo.h"
#include "nsDOMLists.h"
#include "nsGUIEvent.h"
#include "nsDOMError.h"
#include "nsIDragService.h"
#include "nsIScriptableRegion.h"
#include "nsContentUtils.h"

NS_IMPL_CYCLE_COLLECTION_2(nsDOMDataTransfer, mDragTarget, mDragImage)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMDataTransfer)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMDataTransfer)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMDataTransfer)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDataTransfer)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSDataTransfer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMDataTransfer)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DataTransfer)
NS_INTERFACE_MAP_END


const char nsDOMDataTransfer::sEffects[8][9] = {
  "none", "copy", "move", "copyMove", "link", "copyLink", "linkMove", "all"
};

nsDOMDataTransfer::nsDOMDataTransfer()
  : mEventType(NS_DRAGDROP_START),
    mDropEffect(nsIDragService::DRAGDROP_ACTION_NONE),
    mEffectAllowed(nsIDragService::DRAGDROP_ACTION_UNINITIALIZED),
    mCursorState(PR_FALSE),
    mReadOnly(PR_FALSE),
    mIsExternal(PR_FALSE),
    mUserCancelled(PR_FALSE),
    mDragImageX(0),
    mDragImageY(0)
{
}

nsDOMDataTransfer::nsDOMDataTransfer(PRUint32 aEventType, PRUint32 aAction)
  : mEventType(aEventType),
    mDropEffect(nsIDragService::DRAGDROP_ACTION_NONE),
    mReadOnly(PR_TRUE),
    mIsExternal(PR_TRUE),
    mUserCancelled(PR_FALSE),
    mDragImageX(0),
    mDragImageY(0)
{
  mEffectAllowed = aAction &
                   (nsIDragService::DRAGDROP_ACTION_COPY |
                    nsIDragService::DRAGDROP_ACTION_LINK |
                    nsIDragService::DRAGDROP_ACTION_MOVE);

  CacheExternalFormats();
}

nsDOMDataTransfer::nsDOMDataTransfer(PRUint32 aEventType,
                                     const PRUint32 aEffectAllowed,
                                     PRBool aIsExternal,
                                     PRBool aUserCancelled,
                                     nsTArray<nsTArray<TransferItem> >& aItems,
                                     nsIDOMElement* aDragImage,
                                     PRUint32 aDragImageX,
                                     PRUint32 aDragImageY)
  : mEventType(aEventType),
    mDropEffect(nsIDragService::DRAGDROP_ACTION_NONE),
    mEffectAllowed(aEffectAllowed),
    mReadOnly(PR_TRUE),
    mIsExternal(aIsExternal),
    mUserCancelled(aUserCancelled),
    mItems(aItems),
    mDragImage(aDragImage),
    mDragImageX(aDragImageX),
    mDragImageY(aDragImageY)
{
  
  
  
  
  
  NS_ASSERTION(aEventType != NS_DRAGDROP_GESTURE &&
               aEventType != NS_DRAGDROP_START,
               "invalid event type for nsDOMDataTransfer constructor");
}

NS_IMETHODIMP
nsDOMDataTransfer::GetDropEffect(nsAString& aDropEffect)
{
  aDropEffect.AssignASCII(sEffects[mDropEffect]);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::SetDropEffect(const nsAString& aDropEffect)
{
  
  for (PRUint32 e = 0; e <= nsIDragService::DRAGDROP_ACTION_LINK; e++) {
    if (aDropEffect.EqualsASCII(sEffects[e])) {
      
      if (e != (nsIDragService::DRAGDROP_ACTION_COPY |
                nsIDragService::DRAGDROP_ACTION_MOVE))
        mDropEffect = e;
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::GetEffectAllowed(nsAString& aEffectAllowed)
{
  if (mEffectAllowed == nsIDragService::DRAGDROP_ACTION_UNINITIALIZED)
    aEffectAllowed.AssignLiteral("uninitialized");
  else
    aEffectAllowed.AssignASCII(sEffects[mEffectAllowed]);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::SetEffectAllowed(const nsAString& aEffectAllowed)
{
  if (aEffectAllowed.EqualsLiteral("uninitialized")) {
    mEffectAllowed = nsIDragService::DRAGDROP_ACTION_UNINITIALIZED;
    return NS_OK;
  }

  PR_STATIC_ASSERT(nsIDragService::DRAGDROP_ACTION_NONE == 0);
  PR_STATIC_ASSERT(nsIDragService::DRAGDROP_ACTION_COPY == 1);
  PR_STATIC_ASSERT(nsIDragService::DRAGDROP_ACTION_MOVE == 2);
  PR_STATIC_ASSERT(nsIDragService::DRAGDROP_ACTION_LINK == 4);

  for (PRUint32 e = 0; e < NS_ARRAY_LENGTH(sEffects); e++) {
    if (aEffectAllowed.EqualsASCII(sEffects[e])) {
      mEffectAllowed = e;
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::GetDropEffectInt(PRUint32* aDropEffect)
{
  *aDropEffect = mDropEffect;
  return  NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::SetDropEffectInt(PRUint32 aDropEffect)
{
  mDropEffect = aDropEffect;
  return  NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::GetEffectAllowedInt(PRUint32* aEffectAllowed)
{
  *aEffectAllowed = mEffectAllowed;
  return  NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::SetEffectAllowedInt(PRUint32 aEffectAllowed)
{
  mEffectAllowed = aEffectAllowed;
  return  NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::GetMozUserCancelled(PRBool* aUserCancelled)
{
  *aUserCancelled = mUserCancelled;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::GetFiles(nsIDOMFileList** aFileList)
{
  *aFileList = nsnull;

  if (mEventType != NS_DRAGDROP_DROP && mEventType != NS_DRAGDROP_DRAGDROP)
    return NS_OK;

  if (!mFiles) {
    mFiles = new nsDOMFileList();
    NS_ENSURE_TRUE(mFiles, NS_ERROR_OUT_OF_MEMORY);

    PRUint32 count = mItems.Length();

    for (PRUint32 i = 0; i < count; i++) {
      nsCOMPtr<nsIVariant> variant;
      nsresult rv = MozGetDataAt(NS_ConvertUTF8toUTF16(kFileMime), i, getter_AddRefs(variant));
      NS_ENSURE_SUCCESS(rv, rv);

      if (!variant)
        continue;

      nsCOMPtr<nsISupports> supports;
      rv = variant->GetAsISupports(getter_AddRefs(supports));

      if (NS_FAILED(rv))
        continue;

      nsCOMPtr<nsIFile> file = do_QueryInterface(supports);

      if (!file)
        continue;

      nsRefPtr<nsDOMFile> domFile = new nsDOMFile(file);
      NS_ENSURE_TRUE(domFile, NS_ERROR_OUT_OF_MEMORY);

      if (!mFiles->Append(domFile))
        return NS_ERROR_FAILURE;
    }
  }

  *aFileList = mFiles;
  NS_ADDREF(*aFileList);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::GetTypes(nsIDOMDOMStringList** aTypes)
{
  *aTypes = nsnull;

  nsRefPtr<nsDOMStringList> types = new nsDOMStringList();
  NS_ENSURE_TRUE(types, NS_ERROR_OUT_OF_MEMORY);

  if (mItems.Length()) {
    nsTArray<TransferItem>& item = mItems[0];
    for (PRUint32 i = 0; i < item.Length(); i++)
      types->Add(item[i].mFormat);

    PRBool filePresent, filePromisePresent;
    types->Contains(NS_LITERAL_STRING(kFileMime), &filePresent);
    types->Contains(NS_LITERAL_STRING("application/x-moz-file-promise"), &filePromisePresent);
    if (filePresent || filePromisePresent)
      types->Add(NS_LITERAL_STRING("Files"));
  }

  *aTypes = types;
  NS_ADDREF(*aTypes);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::GetData(const nsAString& aFormat, nsAString& aData)
{
  
  aData.Truncate();

  nsCOMPtr<nsIVariant> data;
  nsresult rv = MozGetDataAt(aFormat, 0, getter_AddRefs(data));
  if (rv == NS_ERROR_DOM_INDEX_SIZE_ERR)
    return NS_OK;

  NS_ENSURE_SUCCESS(rv, rv);

  if (data) {
    nsAutoString stringdata;
    data->GetAsAString(stringdata);

    
    
    if (aFormat.EqualsLiteral("URL")) {
      PRInt32 lastidx = 0, idx;
      PRInt32 length = stringdata.Length();
      while (lastidx < length) {
        idx = stringdata.FindChar('\n', lastidx);
        
        if (stringdata[lastidx] == '#') {
          if (idx == -1)
            break;
        }
        else {
          if (idx == -1)
            aData.Assign(Substring(stringdata, lastidx));
          else
            aData.Assign(Substring(stringdata, lastidx, idx - lastidx));
          aData = nsContentUtils::TrimWhitespace(aData, PR_TRUE);
          return NS_OK;
        }
        lastidx = idx + 1;
      }
    }
    else {
      aData = stringdata;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::SetData(const nsAString& aFormat, const nsAString& aData)
{
  nsCOMPtr<nsIWritableVariant> variant = do_CreateInstance(NS_VARIANT_CONTRACTID);
  NS_ENSURE_TRUE(variant, NS_ERROR_OUT_OF_MEMORY);

  variant->SetAsAString(aData);

  return MozSetDataAt(aFormat, variant, 0);
}

NS_IMETHODIMP
nsDOMDataTransfer::ClearData(const nsAString& aFormat)
{
  nsresult rv = MozClearDataAt(aFormat, 0);
  return (rv == NS_ERROR_DOM_INDEX_SIZE_ERR) ? NS_OK : rv;
}

NS_IMETHODIMP
nsDOMDataTransfer::GetMozItemCount(PRUint32* aCount)
{
  *aCount = mItems.Length();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::GetMozCursor(nsAString& aCursorState)
{
  if (mCursorState) {
    aCursorState.AssignLiteral("default");
  } else {
    aCursorState.AssignLiteral("auto");
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::SetMozCursor(const nsAString& aCursorState)
{
  
  mCursorState = aCursorState.EqualsLiteral("default");

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::MozTypesAt(PRUint32 aIndex, nsIDOMDOMStringList** aTypes)
{
  *aTypes = nsnull;

  nsRefPtr<nsDOMStringList> types = new nsDOMStringList();
  NS_ENSURE_TRUE(types, NS_ERROR_OUT_OF_MEMORY);

  if (aIndex < mItems.Length()) {
    
    nsTArray<TransferItem>& item = mItems[aIndex];
    for (PRUint32 i = 0; i < item.Length(); i++)
      types->Add(item[i].mFormat);
  }

  *aTypes = types;
  NS_ADDREF(*aTypes);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::MozGetDataAt(const nsAString& aFormat,
                                PRUint32 aIndex,
                                nsIVariant** aData)
{
  *aData = nsnull;

  if (aFormat.IsEmpty())
    return NS_OK;

  if (aIndex >= mItems.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  nsAutoString format;
  GetRealFormat(aFormat, format);

  nsTArray<TransferItem>& item = mItems[aIndex];

  
  
  
  nsIPrincipal* principal = nsnull;
  if (mEventType != NS_DRAGDROP_DROP && mEventType != NS_DRAGDROP_DRAGDROP &&
      !nsContentUtils::IsCallerTrustedForCapability("UniversalBrowserRead"))
    principal = GetCurrentPrincipal();

  PRUint32 count = item.Length();
  for (PRUint32 i = 0; i < count; i++) {
    TransferItem& formatitem = item[i];
    if (formatitem.mFormat.Equals(format)) {
      PRBool subsumes;
      if (formatitem.mPrincipal && principal &&
          (NS_FAILED(principal->Subsumes(formatitem.mPrincipal, &subsumes)) || !subsumes))
        return NS_ERROR_DOM_SECURITY_ERR;

      if (!formatitem.mData)
        FillInExternalDragData(formatitem, aIndex);
      *aData = formatitem.mData;
      NS_IF_ADDREF(*aData);
      return NS_OK;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::MozSetDataAt(const nsAString& aFormat,
                                nsIVariant* aData,
                                PRUint32 aIndex)
{
  NS_ENSURE_TRUE(aData, NS_ERROR_NULL_POINTER);

  if (aFormat.IsEmpty())
    return NS_OK;

  if (mReadOnly)
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;

  
  
  if (aIndex > mItems.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  
  
  if ((aFormat.EqualsLiteral("application/x-moz-file-promise") ||
       aFormat.EqualsLiteral("application/x-moz-file")) &&
       !nsContentUtils::IsCallerTrustedForCapability("UniversalXPConnect")) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  return SetDataWithPrincipal(aFormat, aData, aIndex, GetCurrentPrincipal());
}

NS_IMETHODIMP
nsDOMDataTransfer::MozClearDataAt(const nsAString& aFormat, PRUint32 aIndex)
{
  if (mReadOnly)
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;

  if (aIndex >= mItems.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  nsAutoString format;
  GetRealFormat(aFormat, format);

  nsIPrincipal* principal = GetCurrentPrincipal();

  
  PRBool clearall = format.IsEmpty();

  nsTArray<TransferItem>& item = mItems[aIndex];
  
  
  for (PRInt32 i = item.Length() - 1; i >= 0; i--) {
    TransferItem& formatitem = item[i];
    if (clearall || formatitem.mFormat.Equals(format)) {
      
      PRBool subsumes;
      if (formatitem.mPrincipal && principal &&
          (NS_FAILED(principal->Subsumes(formatitem.mPrincipal, &subsumes)) || !subsumes))
        return NS_ERROR_DOM_SECURITY_ERR;

      item.RemoveElementAt(i);

      
      
      if (!clearall)
        break;
    }
  }

  
  if (!item.Length())
     mItems.RemoveElementAt(aIndex);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::SetDragImage(nsIDOMElement* aImage, PRInt32 aX, PRInt32 aY)
{
  if (mReadOnly)
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;

  mDragImage = aImage;
  mDragImageX = aX;
  mDragImageY = aY;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMDataTransfer::AddElement(nsIDOMElement* aElement)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);

  if (mReadOnly)
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;

  mDragTarget = do_QueryInterface(aElement);

  return NS_OK;
}

nsresult
nsDOMDataTransfer::Clone(PRUint32 aEventType, PRBool aUserCancelled,
                         nsIDOMDataTransfer** aNewDataTransfer)
{
  nsDOMDataTransfer* newDataTransfer =
    new nsDOMDataTransfer(aEventType, mEffectAllowed, mIsExternal, aUserCancelled,
                          mItems, mDragImage, mDragImageX, mDragImageY);
  NS_ENSURE_TRUE(newDataTransfer, NS_ERROR_OUT_OF_MEMORY);

  *aNewDataTransfer = newDataTransfer;
  NS_ADDREF(*aNewDataTransfer);
  return NS_OK;
}

void
nsDOMDataTransfer::GetTransferables(nsISupportsArray** aArray)
{
  *aArray = nsnull;

  nsCOMPtr<nsISupportsArray> transArray =
    do_CreateInstance("@mozilla.org/supports-array;1");
  if (!transArray)
    return;

  PRBool added = PR_FALSE;
  PRUint32 count = mItems.Length();
  for (PRUint32 i = 0; i < count; i++) {

    nsTArray<TransferItem>& item = mItems[i];
    PRUint32 count = item.Length();
    if (!count)
      continue;

    nsCOMPtr<nsITransferable> transferable =
      do_CreateInstance("@mozilla.org/widget/transferable;1");
    if (!transferable)
      return;

    for (PRUint32 f = 0; f < count; f++) {
      TransferItem& formatitem = item[f];
      if (!formatitem.mData) 
        continue;

      PRUint32 length;
      nsCOMPtr<nsISupports> convertedData;
      if (!ConvertFromVariant(formatitem.mData, getter_AddRefs(convertedData), &length))
        continue;

      
      const char* format;
      NS_ConvertUTF16toUTF8 utf8format(formatitem.mFormat);
      if (utf8format.EqualsLiteral("text/plain"))
        format = kUnicodeMime;
      else
        format = utf8format.get();

      
      
      nsCOMPtr<nsIFormatConverter> converter = do_QueryInterface(convertedData);
      if (converter) {
        transferable->AddDataFlavor(format);
        transferable->SetConverter(converter);
        continue;
      }

      nsresult rv = transferable->SetTransferData(format, convertedData, length);
      if (NS_FAILED(rv))
        return;

      added = PR_TRUE;
    }

    
    if (added)
      transArray->AppendElement(transferable);
  }

  NS_ADDREF(*aArray = transArray);
}

PRBool
nsDOMDataTransfer::ConvertFromVariant(nsIVariant* aVariant,
                                      nsISupports** aSupports,
                                      PRUint32* aLength)
{
  *aSupports = nsnull;
  *aLength = 0;

  PRUint16 type;
  aVariant->GetDataType(&type);
  if (type == nsIDataType::VTYPE_INTERFACE ||
      type == nsIDataType::VTYPE_INTERFACE_IS) {
    nsCOMPtr<nsISupports> data;
    if (NS_FAILED(aVariant->GetAsISupports(getter_AddRefs(data))))
       return PR_FALSE;
 
    nsCOMPtr<nsIFlavorDataProvider> fdp = do_QueryInterface(data);
    if (fdp) {
      
      
      NS_ADDREF(*aSupports = fdp);
      *aLength = nsITransferable::kFlavorHasDataProvider;
    }
    else {
      
      nsCOMPtr<nsISupportsInterfacePointer> ptrSupports =
        do_CreateInstance(NS_SUPPORTS_INTERFACE_POINTER_CONTRACTID);
      if (!ptrSupports)
        return PR_FALSE;

      ptrSupports->SetData(data);
      NS_ADDREF(*aSupports = ptrSupports);

      *aLength = sizeof(nsISupportsInterfacePointer *);
    }

    return PR_TRUE;
  }

  PRUnichar* chrs;
  nsresult rv = aVariant->GetAsWString(&chrs);
  if (NS_FAILED(rv))
    return PR_FALSE;

  nsCOMPtr<nsISupportsString>
    strSupports(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
  if (!strSupports)
    return PR_FALSE;

  nsAutoString str(chrs);
  strSupports->SetData(str);

  *aSupports = strSupports;
  NS_ADDREF(*aSupports);

  
  *aLength = str.Length() << 1;

  return PR_TRUE;
}

void
nsDOMDataTransfer::ClearAll()
{
  mItems.Clear();
}

nsresult
nsDOMDataTransfer::SetDataWithPrincipal(const nsAString& aFormat,
                                        nsIVariant* aData,
                                        PRUint32 aIndex,
                                        nsIPrincipal* aPrincipal)
{
  nsAutoString format;
  GetRealFormat(aFormat, format);

  
  
  TransferItem* formatitem;
  if (aIndex < mItems.Length()) {
    nsTArray<TransferItem>& item = mItems[aIndex];
    PRUint32 count = item.Length();
    for (PRUint32 i = 0; i < count; i++) {
      TransferItem& itemformat = item[i];
      if (itemformat.mFormat.Equals(format)) {
        
        PRBool subsumes;
        if (itemformat.mPrincipal && aPrincipal &&
            (NS_FAILED(aPrincipal->Subsumes(itemformat.mPrincipal, &subsumes)) || !subsumes))
          return NS_ERROR_DOM_SECURITY_ERR;

        itemformat.mPrincipal = aPrincipal;
        itemformat.mData = aData;
        return NS_OK;
      }
    }

    
    formatitem = item.AppendElement();
  }
  else {
    NS_ASSERTION(aIndex == mItems.Length(), "Index out of range");

    
    nsTArray<TransferItem>* item = mItems.AppendElement();
    NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

    formatitem = item->AppendElement();
  }

  NS_ENSURE_TRUE(formatitem, NS_ERROR_OUT_OF_MEMORY);

  formatitem->mFormat = format;
  formatitem->mPrincipal = aPrincipal;
  formatitem->mData = aData;

  return NS_OK;
}

nsIPrincipal*
nsDOMDataTransfer::GetCurrentPrincipal()
{
  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();

  nsCOMPtr<nsIPrincipal> currentPrincipal;
  ssm->GetSubjectPrincipal(getter_AddRefs(currentPrincipal));
  if (!currentPrincipal)
    ssm->GetSystemPrincipal(getter_AddRefs(currentPrincipal));

  return currentPrincipal.get();
}

void
nsDOMDataTransfer::GetRealFormat(const nsAString& aInFormat, nsAString& aOutFormat)
{
  
  if (aInFormat.EqualsLiteral("Text") || aInFormat.EqualsLiteral("text/unicode"))
    aOutFormat.AssignLiteral("text/plain");
  else if (aInFormat.EqualsLiteral("URL"))
    aOutFormat.AssignLiteral("text/uri-list");
  else
    aOutFormat.Assign(aInFormat);
}

void
nsDOMDataTransfer::CacheExternalFormats()
{
  
  
  
  
  

  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");
  if (!dragService)
    return;

  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (!dragSession)
    return;

  
  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIPrincipal> sysPrincipal;
  ssm->GetSystemPrincipal(getter_AddRefs(sysPrincipal));

  
  
  
  const char* formats[] = { kFileMime, kHTMLMime, kURLMime, kURLDataMime, kUnicodeMime };

  PRUint32 count;
  dragSession->GetNumDropItems(&count);
  for (PRUint32 c = 0; c < count; c++) {
    for (PRUint32 f = 0; f < NS_ARRAY_LENGTH(formats); f++) {
      
      
      
      
      PRBool supported;
      dragSession->IsDataFlavorSupported(formats[f], &supported);
      
      
      if (supported) {
        if (strcmp(formats[f], kUnicodeMime) == 0) {
          SetDataWithPrincipal(NS_LITERAL_STRING("text/plain"), nsnull, c, sysPrincipal);
        }
        else {
          if (strcmp(formats[f], kURLDataMime) == 0)
            SetDataWithPrincipal(NS_LITERAL_STRING("text/uri-list"), nsnull, c, sysPrincipal);
          SetDataWithPrincipal(NS_ConvertUTF8toUTF16(formats[f]), nsnull, c, sysPrincipal);
        }
      }
    }
  }
}

void
nsDOMDataTransfer::FillInExternalDragData(TransferItem& aItem, PRUint32 aIndex)
{
  NS_PRECONDITION(mIsExternal, "Not an external drag");

  if (!aItem.mData) {
    nsCOMPtr<nsITransferable> trans =
      do_CreateInstance("@mozilla.org/widget/transferable;1");
    if (!trans)
      return;

    NS_ConvertUTF16toUTF8 utf8format(aItem.mFormat);
    const char* format = utf8format.get();
    if (strcmp(format, "text/plain") == 0)
      format = kUnicodeMime;
    else if (strcmp(format, "text/uri-list") == 0)
      format = kURLDataMime;

    nsCOMPtr<nsIDragService> dragService =
      do_GetService("@mozilla.org/widget/dragservice;1");
    if (!dragService)
      return;

    nsCOMPtr<nsIDragSession> dragSession;
    dragService->GetCurrentSession(getter_AddRefs(dragSession));
    if (!dragSession)
      return;

    trans->AddDataFlavor(format);
    dragSession->GetData(trans, aIndex);

    PRUint32 length = 0;
    nsCOMPtr<nsISupports> data;
    trans->GetTransferData(format, getter_AddRefs(data), &length);
    if (!data)
      return;

    nsCOMPtr<nsIWritableVariant> variant = do_CreateInstance(NS_VARIANT_CONTRACTID);
    if (!variant)
      return;

    nsCOMPtr<nsISupportsString> supportsstr = do_QueryInterface(data);
    if (supportsstr) {
      nsAutoString str;
      supportsstr->GetData(str);
      variant->SetAsAString(str);
    }
    else {
      variant->SetAsISupports(data);
    }
    aItem.mData = variant;
  }
}
