



































#define MOZILLA_INTERNAL_API

#include <ole2.h>
#include <shlobj.h>

#include "TestHarness.h"
#include "nsIArray.h"
#include "nsILocalFile.h"
#include "nsNetUtil.h"
#include "nsISupportsPrimitives.h"
#include "nsIFileURL.h"
#include "nsITransferable.h"
#include "nsClipboard.h"
#include "nsDataObjCollection.h"

nsIFile* xferFile;

nsresult CheckValidHDROP(STGMEDIUM* pSTG)
{
  if (pSTG->tymed != TYMED_HGLOBAL) {
    fail("Received data is not in an HGLOBAL");
    return NS_ERROR_UNEXPECTED;
  }

  HGLOBAL hGlobal = pSTG->hGlobal;
  DROPFILES* pDropFiles;
  pDropFiles = (DROPFILES*)GlobalLock(hGlobal);
  if (!pDropFiles) {
    fail("There is no data at the given HGLOBAL");
    return NS_ERROR_UNEXPECTED;
  }

  if (pDropFiles->pFiles != sizeof(DROPFILES))
    fail("DROPFILES struct has wrong size");

  if (pDropFiles->fWide != true) {
    fail("Received data is not Unicode");
    return NS_ERROR_UNEXPECTED;
  }

  nsString s;
  s = (PRUnichar*)((char*)pDropFiles + pDropFiles->pFiles);
  nsresult rv;
  nsCOMPtr<nsILocalFile> localFile(
             do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
  rv = localFile->InitWithPath(s);
  if (NS_FAILED(rv)) {
    fail("File could not be opened");
    return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}

nsresult CheckValidTEXT(STGMEDIUM* pSTG)
{
  if (pSTG->tymed != TYMED_HGLOBAL) {
    fail("Received data is not in an HGLOBAL");
    return NS_ERROR_UNEXPECTED;
  }

  HGLOBAL hGlobal = pSTG->hGlobal;
  char* pText;
  pText = (char*)GlobalLock(hGlobal);
  if (!pText) {
    fail("There is no data at the given HGLOBAL");
    return NS_ERROR_UNEXPECTED;
  }

  nsCString string;
  string = pText;

  if (!string.Equals(NS_LITERAL_CSTRING("Mozilla can drag and drop"))) {
    fail("Text passed through drop object wrong");
    return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}

nsresult CheckValidUNICODE(STGMEDIUM* pSTG)
{
  if (pSTG->tymed != TYMED_HGLOBAL) {
    fail("Received data is not in an HGLOBAL");
    return NS_ERROR_UNEXPECTED;
  }

  HGLOBAL hGlobal = pSTG->hGlobal;
  PRUnichar* pText;
  pText = (PRUnichar*)GlobalLock(hGlobal);
  if (!pText) {
    fail("There is no data at the given HGLOBAL");
    return NS_ERROR_UNEXPECTED;
  }

  nsString string;
  string = pText;

  if (!string.Equals(NS_LITERAL_STRING("Mozilla can drag and drop"))) {
    fail("Text passed through drop object wrong");
    return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}

nsresult GetTransferableFile(nsCOMPtr<nsITransferable>& pTransferable)
{
  nsresult rv;

  nsCOMPtr<nsISupports> genericWrapper = do_QueryInterface(xferFile);

  pTransferable = do_CreateInstance("@mozilla.org/widget/transferable;1");
  rv = pTransferable->SetTransferData("application/x-moz-file", genericWrapper,
                                      0);
  return rv;
}

nsresult GetTransferableText(nsCOMPtr<nsITransferable>& pTransferable)
{
  nsresult rv;
  NS_NAMED_LITERAL_STRING(mozString, "Mozilla can drag and drop");
  nsCOMPtr<nsISupportsString> xferString =
                               do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
  rv = xferString->SetData(mozString);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> genericWrapper = do_QueryInterface(xferString);

  pTransferable = do_CreateInstance("@mozilla.org/widget/transferable;1");
  rv = pTransferable->SetTransferData("text/unicode", genericWrapper,
                                      mozString.Length() * sizeof(PRUnichar));
  return rv;
}

nsresult GetTransferableURI(nsCOMPtr<nsITransferable>& pTransferable)
{
  nsresult rv;

  nsCOMPtr<nsIURI> xferURI;

  rv = NS_NewURI(getter_AddRefs(xferURI), "http://www.mozilla.org");
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> genericWrapper = do_QueryInterface(xferURI);

  pTransferable = do_CreateInstance("@mozilla.org/widget/transferable;1");
  rv = pTransferable->SetTransferData("text/x-moz-url", genericWrapper, 0);
  return rv;
}

nsresult MakeDataObject(nsISupportsArray* transferableArray,
                        nsRefPtr<IDataObject>& itemToDrag)
{
  nsresult rv;
  PRUint32 itemCount = 0;

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), "http://www.mozilla.org");
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transferableArray->Count(&itemCount);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (itemCount > 1) {
    nsDataObjCollection * dataObjCollection = new nsDataObjCollection();
    if (!dataObjCollection)
      return NS_ERROR_OUT_OF_MEMORY;
    itemToDrag = dataObjCollection;
    for (PRUint32 i=0; i<itemCount; ++i) {
      nsCOMPtr<nsISupports> supports;
      transferableArray->GetElementAt(i, getter_AddRefs(supports));
      nsCOMPtr<nsITransferable> trans(do_QueryInterface(supports));
      if (trans) {
        nsRefPtr<IDataObject> dataObj;
        rv = nsClipboard::CreateNativeDataObject(trans,
                                                 getter_AddRefs(dataObj), uri);
        NS_ENSURE_SUCCESS(rv, rv);
        
        rv = nsClipboard::SetupNativeDataObject(trans, dataObjCollection);
        NS_ENSURE_SUCCESS(rv, rv);

        dataObjCollection->AddDataObject(dataObj);
      }
    }
  } 
  else {
    nsCOMPtr<nsISupports> supports;
    transferableArray->GetElementAt(0, getter_AddRefs(supports));
    nsCOMPtr<nsITransferable> trans(do_QueryInterface(supports));
    if (trans) {
      rv = nsClipboard::CreateNativeDataObject(trans,
                                               getter_AddRefs(itemToDrag),
                                               uri);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } 
  return rv;
}

nsresult Do_CheckOneFile()
{
  nsresult rv;
  nsCOMPtr<nsITransferable> transferable;
  nsCOMPtr<nsISupportsArray> transferableArray;
  nsCOMPtr<nsISupports> genericWrapper;
  nsRefPtr<IDataObject> dataObj;
  rv = NS_NewISupportsArray(getter_AddRefs(transferableArray));
  if (NS_FAILED(rv)) {
    fail("Could not create the necessary nsISupportsArray");
    return rv;
  }

  rv = GetTransferableFile(transferable);
  if (NS_FAILED(rv)) {
    fail("Could not create the proper nsITransferable!");
    return rv;
  }
  genericWrapper = do_QueryInterface(transferable);
  rv = transferableArray->AppendElement(genericWrapper);
  if (NS_FAILED(rv)) {
    fail("Could not append element to transferable array");
    return rv;
  }

  rv = MakeDataObject(transferableArray, dataObj);
  if (NS_FAILED(rv)) {
    fail("Could not create data object");
    return rv;
  }

  FORMATETC fe;
  SET_FORMATETC(fe, CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
  if (dataObj->QueryGetData(&fe) != S_OK) {
    fail("File data object does not support the file data type!");
    return NS_ERROR_UNEXPECTED;
  }

  STGMEDIUM* stg;
  stg = (STGMEDIUM*)CoTaskMemAlloc(sizeof(STGMEDIUM));
  if (dataObj->GetData(&fe, stg) != S_OK) {
    fail("File data object did not provide data on request");
    return NS_ERROR_UNEXPECTED;
  }

  rv = CheckValidHDROP(stg);
  if (NS_FAILED(rv)) {
    fail("HDROP was invalid");
    return rv;
  }

  ReleaseStgMedium(stg);

  return S_OK;
}

nsresult Do_CheckOneString()
{
  nsresult rv;
  nsCOMPtr<nsITransferable> transferable;
  nsCOMPtr<nsISupportsArray> transferableArray;
  nsCOMPtr<nsISupports> genericWrapper;
  nsRefPtr<IDataObject> dataObj;
  rv = NS_NewISupportsArray(getter_AddRefs(transferableArray));
  if (NS_FAILED(rv)) {
    fail("Could not create the necessary nsISupportsArray");
    return rv;
  }

  rv = GetTransferableText(transferable);
  if (NS_FAILED(rv)) {
    fail("Could not create the proper nsITransferable!");
    return rv;
  }
  genericWrapper = do_QueryInterface(transferable);
  rv = transferableArray->AppendElement(genericWrapper);
  if (NS_FAILED(rv)) {
    fail("Could not append element to transferable array");
    return rv;
  }

  rv = MakeDataObject(transferableArray, dataObj);
  if (NS_FAILED(rv)) {
    fail("Could not create data object");
    return rv;
  }

  FORMATETC fe;
  SET_FORMATETC(fe, CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
  if (dataObj->QueryGetData(&fe) != S_OK) {
    fail("String data object does not support the ASCII text data type!");
    return NS_ERROR_UNEXPECTED;
  }

  STGMEDIUM* stg;
  stg = (STGMEDIUM*)CoTaskMemAlloc(sizeof(STGMEDIUM));
  HRESULT hr;
  if ((hr = dataObj->GetData(&fe, stg)) != S_OK) {
    fail("String data object did not provide ASCII data on request");
    return NS_ERROR_UNEXPECTED;
  }

  rv = CheckValidTEXT(stg);
  if (NS_FAILED(rv)) {
    fail("TEXT was invalid");
    return rv;
  }

  ReleaseStgMedium(stg);

  SET_FORMATETC(fe, CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
  if (dataObj->QueryGetData(&fe) != S_OK) {
    fail("String data object does not support the wide text data type!");
    return NS_ERROR_UNEXPECTED;
  }

  if (dataObj->GetData(&fe, stg) != S_OK) {
    fail("String data object did not provide wide data on request");
    return NS_ERROR_UNEXPECTED;
  }
  
  rv = CheckValidUNICODE(stg);
  if (NS_FAILED(rv)) {
    fail("UNICODE was invalid");
    return rv;
  }
  
  return S_OK;
}



nsresult Do_Test1()
{
  nsresult rv = NS_OK;
  nsresult workingrv;

  workingrv = Do_CheckOneFile();
  if (NS_FAILED(workingrv)) {
    fail("Drag object tests failed on a single file");
    rv = NS_ERROR_UNEXPECTED;
  } else {
    passed("Successfully created a working file drag object!");
  }

  workingrv = Do_CheckOneString();
  if (NS_FAILED(workingrv)) {
    fail("Drag object tests failed on a single string");
    rv = NS_ERROR_UNEXPECTED;
  } else {
    passed("Successfully created a working string drag object!");
  }

  return rv;
}

int main(int argc, char** argv)
{
  ScopedXPCOM xpcom("Test Windows Drag and Drop");

  nsCOMPtr<nsIFile> file;
  file = xpcom.GetProfileDirectory();
  xferFile = file;

  if (NS_SUCCEEDED(Do_Test1()))
    passed("Basic Drag and Drop data type tests succeeded!");

  return gFailCount;
}
