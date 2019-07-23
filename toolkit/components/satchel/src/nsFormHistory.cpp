





































#include "nsFormHistory.h"

#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsICategoryManager.h"
#include "nsIDirectoryService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsMorkCID.h"
#include "nsIMdbFactoryFactory.h"
#include "nsQuickSort.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsIDOMNode.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsVoidArray.h"
#include "nsCOMArray.h"

static void SwapBytes(PRUnichar* aDest, const PRUnichar* aSrc, PRUint32 aLen)
{
  for(PRUint32 i = 0; i < aLen; i++)
  {
    PRUnichar aChar = *aSrc++;
    *aDest++ = (0xff & (aChar >> 8)) | (aChar << 8);
  }
}

#define PREF_FORMFILL_BRANCH "browser.formfill."
#define PREF_FORMFILL_ENABLE "enable"


#define FORMFILL_NAME_MAX_LEN  1000
#define FORMFILL_VALUE_MAX_LEN 4000

static const char *kFormHistoryFileName = "formhistory.dat";

NS_INTERFACE_MAP_BEGIN(nsFormHistory)
  NS_INTERFACE_MAP_ENTRY(nsIFormHistory2)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIFormSubmitObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsFormHistory)
NS_IMPL_THREADSAFE_RELEASE(nsFormHistory)

mdb_column nsFormHistory::kToken_ValueColumn = 0;
mdb_column nsFormHistory::kToken_NameColumn = 0;

PRBool nsFormHistory::gFormHistoryEnabled = PR_FALSE;
PRBool nsFormHistory::gPrefsInitialized = PR_FALSE;

nsFormHistory::nsFormHistory() :
  mEnv(nsnull),
  mStore(nsnull),
  mTable(nsnull),
  mReverseByteOrder(PR_FALSE)
{
  NS_ASSERTION(!gFormHistory, "nsFormHistory must be used as a service");
  gFormHistory = this;
}

nsFormHistory::~nsFormHistory()
{
  NS_ASSERTION(gFormHistory == this,
               "nsFormHistory must be used as a service");
  CloseDatabase();
  gFormHistory = nsnull;
}

nsresult
nsFormHistory::Init()
{
  nsCOMPtr<nsIObserverService> service = do_GetService("@mozilla.org/observer-service;1");
  if (service)
    service->AddObserver(this, NS_EARLYFORMSUBMIT_SUBJECT, PR_TRUE);
  
  return NS_OK;
}

nsFormHistory *nsFormHistory::gFormHistory = nsnull;

 PRBool
nsFormHistory::FormHistoryEnabled()
{
  if (!gPrefsInitialized) {
    nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);

    prefService->GetBranch(PREF_FORMFILL_BRANCH,
                           getter_AddRefs(gFormHistory->mPrefBranch));
    gFormHistory->mPrefBranch->GetBoolPref(PREF_FORMFILL_ENABLE,
                                           &gFormHistoryEnabled);

    nsCOMPtr<nsIPrefBranch2> branchInternal =
      do_QueryInterface(gFormHistory->mPrefBranch);
    branchInternal->AddObserver(PREF_FORMFILL_ENABLE, gFormHistory, PR_TRUE);

    gPrefsInitialized = PR_TRUE;
  }

  return gFormHistoryEnabled;
}





NS_IMETHODIMP
nsFormHistory::GetHasEntries(PRBool *aHasEntries)
{
  nsresult rv = OpenDatabase(); 
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  mdb_err err = mTable->GetCount(mEnv, &rowCount);
  NS_ENSURE_TRUE(!err, NS_ERROR_FAILURE);

  *aHasEntries = rowCount != 0;
  return NS_OK;
}

NS_IMETHODIMP
nsFormHistory::AddEntry(const nsAString &aName, const nsAString &aValue)
{
  if (!FormHistoryEnabled())
    return NS_OK;

  nsresult rv = OpenDatabase(); 
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMdbRow> row;
  AppendRow(aName, aValue, getter_AddRefs(row));
  return NS_OK;
}

NS_IMETHODIMP
nsFormHistory::EntryExists(const nsAString &aName, const nsAString &aValue, PRBool *_retval)
{
  return EntriesExistInternal(&aName, &aValue, _retval);
}

NS_IMETHODIMP
nsFormHistory::NameExists(const nsAString &aName, PRBool *_retval)
{
  return EntriesExistInternal(&aName, nsnull, _retval);
}

NS_IMETHODIMP
nsFormHistory::RemoveEntry(const nsAString &aName, const nsAString &aValue)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFormHistory::RemoveEntriesForName(const nsAString &aName)
{
  return RemoveEntriesInternal(&aName);
}

NS_IMETHODIMP
nsFormHistory::RemoveAllEntries()
{
  nsresult rv = RemoveEntriesInternal(nsnull);

  if (NS_SUCCEEDED(rv))
    rv = InitByteOrder(PR_TRUE);
  
  rv |= Flush();
  
  return rv;
}




NS_IMETHODIMP
nsFormHistory::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData) 
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    mPrefBranch->GetBoolPref(PREF_FORMFILL_ENABLE, &gFormHistoryEnabled);
  }

  return NS_OK;
}




NS_IMETHODIMP
nsFormHistory::Notify(nsIDOMHTMLFormElement* formElt, nsIDOMWindowInternal* aWindow, nsIURI* aActionURL, PRBool* aCancelSubmit)
{
  if (!FormHistoryEnabled())
    return NS_OK;

  nsresult rv = OpenDatabase(); 
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMHTMLCollection> elts;
  formElt->GetElements(getter_AddRefs(elts));
  
  const char *textString = "text";
  
  PRUint32 length;
  elts->GetLength(&length);
  for (PRUint32 i = 0; i < length; ++i) {
    nsCOMPtr<nsIDOMNode> node;
    elts->Item(i, getter_AddRefs(node));
    nsCOMPtr<nsIDOMHTMLInputElement> inputElt = do_QueryInterface(node);
    if (inputElt) {
      
      nsAutoString type;
      inputElt->GetType(type);
      if (type.EqualsIgnoreCase(textString)) {
        
        nsAutoString value;
        inputElt->GetValue(value);
        if (!value.IsEmpty()) {
          nsAutoString name;
          inputElt->GetName(name);
          if (name.IsEmpty())
            inputElt->GetId(name);
          
          if (!name.IsEmpty())
            AppendRow(name, value, nsnull);
        }
      }
    }
  }

  return NS_OK;
}




class SatchelErrorHook : public nsIMdbErrorHook
{
public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD OnErrorString(nsIMdbEnv* ev, const char* inAscii);
  NS_IMETHOD OnErrorYarn(nsIMdbEnv* ev, const mdbYarn* inYarn);
  NS_IMETHOD OnWarningString(nsIMdbEnv* ev, const char* inAscii);
  NS_IMETHOD OnWarningYarn(nsIMdbEnv* ev, const mdbYarn* inYarn);
  NS_IMETHOD OnAbortHintString(nsIMdbEnv* ev, const char* inAscii);
  NS_IMETHOD OnAbortHintYarn(nsIMdbEnv* ev, const mdbYarn* inYarn);
};


NS_IMPL_ISUPPORTS0(SatchelErrorHook)

NS_IMETHODIMP
SatchelErrorHook::OnErrorString(nsIMdbEnv *ev, const char *inAscii)
{
  printf("mork error: %s\n", inAscii);
  return NS_OK;
}

NS_IMETHODIMP
SatchelErrorHook::OnErrorYarn(nsIMdbEnv *ev, const mdbYarn* inYarn)
{
  printf("mork error yarn: %p\n", (void*)inYarn);
  return NS_OK;
}

NS_IMETHODIMP
SatchelErrorHook::OnWarningString(nsIMdbEnv *ev, const char *inAscii)
{
  printf("mork warning: %s\n", inAscii);
  return NS_OK;
}

NS_IMETHODIMP
SatchelErrorHook::OnWarningYarn(nsIMdbEnv *ev, const mdbYarn *inYarn)
{
  printf("mork warning yarn: %p\n", (void*)inYarn);
  return NS_OK;
}

NS_IMETHODIMP
SatchelErrorHook::OnAbortHintString(nsIMdbEnv *ev, const char *inAscii)
{
  printf("mork abort: %s\n", inAscii);
  return NS_OK;
}

NS_IMETHODIMP
SatchelErrorHook::OnAbortHintYarn(nsIMdbEnv *ev, const mdbYarn *inYarn)
{
  printf("mork abort yarn: %p\n", (void*)inYarn);
  return NS_OK;
}

nsresult
nsFormHistory::OpenDatabase()
{
  if (mStore)
    return NS_OK;
  
  
  nsCOMPtr <nsIFile> historyFile;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(historyFile));
  NS_ENSURE_SUCCESS(rv, rv);
  historyFile->Append(NS_ConvertUTF8toUTF16(kFormHistoryFileName));

  
  nsCOMPtr <nsIMdbFactoryService> mdbFactory = do_GetService(NS_MORK_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mdbFactory->GetMdbFactory(getter_AddRefs(mMdbFactory));
  NS_ENSURE_SUCCESS(rv, rv);

  
  mdb_err err = mMdbFactory->MakeEnv(nsnull, &mEnv);
  NS_ASSERTION(err == 0, "ERROR: Unable to create Form History mdb");
  mEnv->SetAutoClear(PR_TRUE);
  NS_ENSURE_TRUE(!err, NS_ERROR_FAILURE);
  mEnv->SetErrorHook(new SatchelErrorHook());

  nsCAutoString filePath;
  historyFile->GetNativePath(filePath);
  PRBool exists = PR_TRUE;
  historyFile->Exists(&exists);

  PRBool createdNew = PR_FALSE;
  
  if (!exists || NS_FAILED(rv = OpenExistingFile(filePath.get()))) {
    
    
    historyFile->Remove(PR_FALSE);
    rv = CreateNewFile(filePath.get());
    createdNew = PR_TRUE;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  historyFile->GetFileSize(&mFileSizeOnDisk);

  rv = InitByteOrder(createdNew);

  






  
  















  return rv;
}

nsresult
nsFormHistory::OpenExistingFile(const char *aPath)
{
  nsCOMPtr<nsIMdbFile> oldFile;
  nsIMdbHeap* dbHeap = 0;
  mdb_err err = mMdbFactory->OpenOldFile(mEnv, dbHeap, aPath, mdbBool_kFalse, getter_AddRefs(oldFile));
  NS_ENSURE_TRUE(!err && oldFile, NS_ERROR_FAILURE);

  mdb_bool canOpen = 0;
  mdbYarn outFormat = {nsnull, 0, 0, 0, 0, nsnull};
  err = mMdbFactory->CanOpenFilePort(mEnv, oldFile, &canOpen, &outFormat);
  NS_ENSURE_TRUE(!err && canOpen, NS_ERROR_FAILURE);

  nsCOMPtr<nsIMdbThumb> thumb;
  mdbOpenPolicy policy = {{0, 0}, 0, 0};
  err = mMdbFactory->OpenFileStore(mEnv, dbHeap, oldFile, &policy, getter_AddRefs(thumb));
  NS_ENSURE_TRUE(!err && thumb, NS_ERROR_FAILURE);

  PRBool done;
  mdb_err thumbErr = UseThumb(thumb, &done);

  if (err == 0 && done)
    err = mMdbFactory->ThumbToOpenStore(mEnv, thumb, &mStore);
  NS_ENSURE_TRUE(!err, NS_ERROR_FAILURE);

  nsresult rv = CreateTokens();
  NS_ENSURE_SUCCESS(rv, rv);

  mdbOid oid = {kToken_RowScope, 1};
  err = mStore->GetTable(mEnv, &oid, &mTable);
  NS_ENSURE_TRUE(!err, NS_ERROR_FAILURE);
  if (!mTable) {
    NS_WARNING("ERROR: Form history file is corrupt, now deleting it.");
    return NS_ERROR_FAILURE;
  }

  err = mTable->GetMetaRow(mEnv, &oid, nsnull, getter_AddRefs(mMetaRow));
  if (err)
    NS_WARNING("Could not get meta row");

  if (NS_FAILED(thumbErr))
    err = thumbErr;

  return err ? NS_ERROR_FAILURE : NS_OK;
}

nsresult
nsFormHistory::CreateNewFile(const char *aPath)
{
  nsIMdbHeap* dbHeap = 0;
  nsCOMPtr<nsIMdbFile> newFile;
  mdb_err err = mMdbFactory->CreateNewFile(mEnv, dbHeap, aPath, getter_AddRefs(newFile));
  NS_ENSURE_TRUE(!err && newFile, NS_ERROR_FAILURE);

  nsCOMPtr <nsIMdbTable> oldTable = mTable;;
  nsCOMPtr <nsIMdbStore> oldStore = mStore;
  mdbOpenPolicy policy = {{0, 0}, 0, 0};
  err = mMdbFactory->CreateNewFileStore(mEnv, dbHeap, newFile, &policy, &mStore);
  NS_ENSURE_TRUE(!err, NS_ERROR_FAILURE);
  
  nsresult rv = CreateTokens();
  NS_ENSURE_SUCCESS(rv, rv);

  
  err = mStore->NewTable(mEnv, kToken_RowScope, kToken_Kind, PR_TRUE, nsnull, &mTable);
  NS_ENSURE_TRUE(!err && mTable, NS_ERROR_FAILURE);

  mdbOid oid = {kToken_RowScope, 1};
  err = mTable->GetMetaRow(mEnv, &oid, nsnull, getter_AddRefs(mMetaRow));
  if (err) {
    NS_WARNING("Could not get meta row");
    return NS_ERROR_FAILURE;
  }

   
   
  if (oldTable)
    CopyRowsFromTable(oldTable);

  
  nsCOMPtr<nsIMdbThumb> thumb;
  err = mStore->CompressCommit(mEnv, getter_AddRefs(thumb));
  NS_ENSURE_TRUE(!err, NS_ERROR_FAILURE);

  PRBool done;
  err = UseThumb(thumb, &done);

  return err || !done ? NS_ERROR_FAILURE : NS_OK;
}

nsresult
nsFormHistory::CloseDatabase()
{
  Flush();

  mMetaRow = nsnull;

  if (mTable)
    mTable->Release();

  if (mStore)
    mStore->Release();

  if (mEnv)
    mEnv->Release();

  mTable = nsnull;
  mEnv = nsnull;
  mStore = nsnull;

  return NS_OK;
}

nsresult
nsFormHistory::CreateTokens()
{
  mdb_err err;

  if (!mStore)
    return NS_ERROR_NOT_INITIALIZED;

  err = mStore->StringToToken(mEnv, "ns:formhistory:db:row:scope:formhistory:all", &kToken_RowScope);
  if (err != 0) return NS_ERROR_FAILURE;
  
  err = mStore->StringToToken(mEnv, "ns:formhistory:db:table:kind:formhistory", &kToken_Kind);
  if (err != 0) return NS_ERROR_FAILURE;
  
  err = mStore->StringToToken(mEnv, "Value", &kToken_ValueColumn);
  if (err != 0) return NS_ERROR_FAILURE;

  err = mStore->StringToToken(mEnv, "Name", &kToken_NameColumn);
  if (err != 0) return NS_ERROR_FAILURE;

  err = mStore->StringToToken(mEnv, "ByteOrder", &kToken_ByteOrder);
  if (err != 0) return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult
nsFormHistory::Flush()
{
  if (!mStore || !mTable)
    return NS_OK;

  mdb_err err;

  nsCOMPtr<nsIMdbThumb> thumb;
  err = mStore->CompressCommit(mEnv, getter_AddRefs(thumb));

  if (err == 0)
    err = UseThumb(thumb, nsnull);
  
  return err ? NS_ERROR_FAILURE : NS_OK;
}

mdb_err
nsFormHistory::UseThumb(nsIMdbThumb *aThumb, PRBool *aDone)
{
  mdb_count total;
  mdb_count current;
  mdb_bool done;
  mdb_bool broken;
  mdb_err err;
  
  do {
    err = aThumb->DoMore(mEnv, &total, &current, &done, &broken);
  } while ((err == 0) && !broken && !done);
  
  if (aDone)
    *aDone = done;
  
  return err ? NS_ERROR_FAILURE : NS_OK;
}

nsresult
nsFormHistory::CopyRowsFromTable(nsIMdbTable *sourceTable)
{
  nsCOMPtr<nsIMdbTableRowCursor> rowCursor;
  mdb_err err = sourceTable->GetTableRowCursor(mEnv, -1, getter_AddRefs(rowCursor));
  NS_ENSURE_TRUE(!err, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIMdbRow> row;
  mdb_pos pos;
  do {
    rowCursor->NextRow(mEnv, getter_AddRefs(row), &pos);
    if (!row)
      break;

    mdbOid rowId;
    rowId.mOid_Scope = kToken_RowScope;
    rowId.mOid_Id = mdb_id(-1);

    nsCOMPtr<nsIMdbRow> newRow;
    mTable->NewRow(mEnv, &rowId, getter_AddRefs(newRow));
    newRow->SetRow(mEnv, row);
    mTable->AddRow(mEnv, newRow);
  } while (row);
  return NS_OK;
}

nsresult
nsFormHistory::AppendRow(const nsAString &aName, const nsAString &aValue, nsIMdbRow **aResult)
{  
  if (!mTable)
    return NS_ERROR_NOT_INITIALIZED;

  if (aName.Length() > FORMFILL_NAME_MAX_LEN ||
      aValue.Length() > FORMFILL_VALUE_MAX_LEN)
    return NS_ERROR_INVALID_ARG;

  PRBool exists = PR_TRUE;
  EntryExists(aName, aValue, &exists);
  if (exists)
    return NS_OK;

  mdbOid rowId;
  rowId.mOid_Scope = kToken_RowScope;
  rowId.mOid_Id = mdb_id(-1);

  nsCOMPtr<nsIMdbRow> row;
  mdb_err err = mTable->NewRow(mEnv, &rowId, getter_AddRefs(row));
  if (err != 0)
    return NS_ERROR_FAILURE;

  SetRowValue(row, kToken_NameColumn, aName);
  SetRowValue(row, kToken_ValueColumn, aValue);

  if (aResult) {
    *aResult = row;
    NS_ADDREF(*aResult);
  }
  
  return NS_OK;  
}

nsresult
nsFormHistory::SetRowValue(nsIMdbRow *aRow, mdb_column aCol, const nsAString &aValue)
{
  PRInt32 len = aValue.Length() * sizeof(PRUnichar);
  PRUnichar *swapval = nsnull;
  mdbYarn yarn = {nsnull, len, len, 0, 0, nsnull};
  const nsPromiseFlatString& buffer = PromiseFlatString(aValue);

  if (mReverseByteOrder) {
    swapval = new PRUnichar[aValue.Length()];
    if (!swapval)
      return NS_ERROR_OUT_OF_MEMORY;
    SwapBytes(swapval, buffer.get(), aValue.Length());
    yarn.mYarn_Buf = swapval;
  }
  else
    yarn.mYarn_Buf = (void*)buffer.get();

  mdb_err err = aRow->AddColumn(mEnv, aCol, &yarn);

  delete swapval;
  
  return err ? NS_ERROR_FAILURE : NS_OK;
}

nsresult
nsFormHistory::GetRowValue(nsIMdbRow *aRow, mdb_column aCol, nsAString &aValue)
{
  mdbYarn yarn;
  mdb_err err = aRow->AliasCellYarn(mEnv, aCol, &yarn);
  if (err != 0)
    return NS_ERROR_FAILURE;

  aValue.Truncate(0);
  if (!yarn.mYarn_Fill)
    return NS_OK;
  
  switch (yarn.mYarn_Form) {
    case 0: { 
      PRUint32 len = yarn.mYarn_Fill / sizeof(PRUnichar);
      if (mReverseByteOrder) {
        PRUnichar *swapval = new PRUnichar[len];
        if (!swapval)
          return NS_ERROR_OUT_OF_MEMORY;
        SwapBytes(swapval, (const PRUnichar*)yarn.mYarn_Buf, len);
        aValue.Assign(swapval, len);
        delete swapval;
      }
      else
        aValue.Assign((const PRUnichar *)yarn.mYarn_Buf, len);
      break;
    }
    default:
      return NS_ERROR_UNEXPECTED;
  }
  
  return NS_OK;
}

nsresult
nsFormHistory::AutoCompleteSearch(const nsAString &aInputName,
                                  const nsAString &aInputValue,
                                  nsIAutoCompleteMdbResult2 *aPrevResult,
                                  nsIAutoCompleteResult **aResult)
{
  if (!FormHistoryEnabled())
    return NS_OK;

  nsresult rv = OpenDatabase(); 
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAutoCompleteMdbResult2> result;
  
  if (aPrevResult) {
    result = aPrevResult;
    
    PRUint32 rowCount;
    result->GetMatchCount(&rowCount);
    
    for (PRInt32 i = rowCount-1; i >= 0; --i) {
      nsIMdbRow *row;
      result->GetRowAt(i, &row);
      if (!RowMatch(row, aInputName, aInputValue, nsnull))
        result->RemoveValueAt(i, PR_FALSE);
    }
  } else {
    result = do_CreateInstance("@mozilla.org/autocomplete/mdb-result;1");

    result->SetSearchString(aInputValue);
    result->Init(mEnv, mTable);
    result->SetTokens(kToken_ValueColumn, nsIAutoCompleteMdbResult2::kUnicharType, nsnull, nsIAutoCompleteMdbResult2::kUnicharType);
    result->SetReverseByteOrder(mReverseByteOrder);

    
    nsCOMPtr<nsIMdbTableRowCursor> rowCursor;
    mdb_err err = mTable->GetTableRowCursor(mEnv, -1, getter_AddRefs(rowCursor));
    NS_ENSURE_TRUE(!err, NS_ERROR_FAILURE);
    
    
    nsAutoVoidArray matchingValues;
    nsCOMArray<nsIMdbRow> matchingRows;

    nsCOMPtr<nsIMdbRow> row;
    mdb_pos pos;
    do {
      rowCursor->NextRow(mEnv, getter_AddRefs(row), &pos);
      if (!row)
        break;

      PRUnichar *value = 0; 
      if (RowMatch(row, aInputName, aInputValue, &value)) {
        matchingRows.AppendObject(row);
        matchingValues.AppendElement(value);
      }
    } while (row);

    
    
    PRUint32 count = matchingRows.Count();

    if (count > 0) {
      PRUint32* items = new PRUint32[count];
      PRUint32 i;
      for (i = 0; i < count; ++i)
        items[i] = i;

      NS_QuickSort(items, count, sizeof(PRUint32),
                   SortComparison, &matchingValues);

      for (i = 0; i < count; ++i) {
        
        result->AddRow(matchingRows[items[i]]);

        
        NS_Free(matchingValues[i]);
      }

      delete[] items;
    }

    PRUint32 matchCount;
    result->GetMatchCount(&matchCount);
    if (matchCount > 0) {
      result->SetSearchResult(nsIAutoCompleteResult::RESULT_SUCCESS);
      result->SetDefaultIndex(0);
    } else {
      result->SetSearchResult(nsIAutoCompleteResult::RESULT_NOMATCH);
      result->SetDefaultIndex(-1);
    }
  }
  
  *aResult = result;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

int PR_CALLBACK 
nsFormHistory::SortComparison(const void *v1, const void *v2, void *closureVoid) 
{
  PRUint32 *index1 = (PRUint32 *)v1;
  PRUint32 *index2 = (PRUint32 *)v2;
  nsAutoVoidArray *array = (nsAutoVoidArray *)closureVoid;
  
  PRUnichar *s1 = (PRUnichar *)array->ElementAt(*index1);
  PRUnichar *s2 = (PRUnichar *)array->ElementAt(*index2);
  
  return nsCRT::strcmp(s1, s2);
}

PRBool
nsFormHistory::RowMatch(nsIMdbRow *aRow, const nsAString &aInputName, const nsAString &aInputValue, PRUnichar **aValue)
{
  nsAutoString name;
  GetRowValue(aRow, kToken_NameColumn, name);

  if (name.Equals(aInputName)) {
    nsAutoString value;
    GetRowValue(aRow, kToken_ValueColumn, value);
    if (Compare(Substring(value, 0, aInputValue.Length()), aInputValue, nsCaseInsensitiveStringComparator()) == 0) {
      if (aValue)
        *aValue = ToNewUnicode(value);
      return PR_TRUE;
    }
  }
  
  return PR_FALSE;
}

nsresult
nsFormHistory::EntriesExistInternal(const nsAString *aName, const nsAString *aValue, PRBool *_retval)
{
  
  
  
  *_retval = PR_FALSE;
  
  nsresult rv = OpenDatabase(); 
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIMdbTableRowCursor> rowCursor;
  mdb_err err = mTable->GetTableRowCursor(mEnv, -1, getter_AddRefs(rowCursor));
  NS_ENSURE_TRUE(!err, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIMdbRow> row;
  mdb_pos pos;
  do {
    rowCursor->NextRow(mEnv, getter_AddRefs(row), &pos);
    if (!row)
      break;

    
    nsAutoString name;
    GetRowValue(row, kToken_NameColumn, name);

    if (Compare(name, *aName, nsCaseInsensitiveStringComparator()) == 0) {
      nsAutoString value;
      GetRowValue(row, kToken_ValueColumn, value);
      if (!aValue || Compare(value, *aValue, nsCaseInsensitiveStringComparator()) == 0) {
        *_retval = PR_TRUE;
        break;
      }
    }
  } while (1);
  
  return NS_OK;
}

nsresult
nsFormHistory::RemoveEntriesInternal(const nsAString *aName)
{
  nsresult rv = OpenDatabase(); 
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mTable) return NS_OK;

  mdb_err err;
  mdb_count count;
  nsAutoString name;
  err = mTable->GetCount(mEnv, &count);
  if (err != 0) return NS_ERROR_FAILURE;

  
  int marker;
  err = mTable->StartBatchChangeHint(mEnv, &marker);
  NS_ASSERTION(err == 0, "unable to start batch");
  if (err != 0) return NS_ERROR_FAILURE;

  for (mdb_pos pos = count - 1; pos >= 0; --pos) {
    nsCOMPtr<nsIMdbRow> row;
    err = mTable->PosToRow(mEnv, pos, getter_AddRefs(row));
    NS_ASSERTION(err == 0, "unable to get row");
    if (err != 0)
      break;

    NS_ASSERTION(row != nsnull, "no row");
    if (! row)
      continue;

    
    GetRowValue(row, kToken_NameColumn, name);
    
    if (!aName || Compare(name, *aName, nsCaseInsensitiveStringComparator()) == 0) {

      
      
      err = mTable->CutRow(mEnv, row);
      NS_ASSERTION(err == 0, "couldn't cut row");
      if (err != 0)
        continue;
  
      
      err = row->CutAllColumns(mEnv);
      NS_ASSERTION(err == 0, "couldn't cut all columns");
      
      
    }

  }
  
  
  err = mTable->EndBatchChangeHint(mEnv, &marker);
  NS_ASSERTION(err == 0, "error ending batch");

  return (err == 0) ? NS_OK : NS_ERROR_FAILURE;

}

nsresult
nsFormHistory::InitByteOrder(PRBool aForce)
{
  
  
  
  nsAutoString bigEndianByteOrder((PRUnichar*)"BBBB", 2);
  nsAutoString littleEndianByteOrder((PRUnichar*)"llll", 2);
#ifdef IS_BIG_ENDIAN
  nsAutoString nativeByteOrder(bigEndianByteOrder);
#else
  nsAutoString nativeByteOrder(littleEndianByteOrder);
#endif

  nsAutoString fileByteOrder;
  nsresult rv = NS_OK;

  if (!aForce)
    rv = GetByteOrder(fileByteOrder);

  if (aForce || NS_FAILED(rv) ||
      !(fileByteOrder.Equals(bigEndianByteOrder) ||
        fileByteOrder.Equals(littleEndianByteOrder))) {
#if defined(XP_MACOSX) && defined(IS_LITTLE_ENDIAN)
    
    
    
    
    
    
    if (aForce) {
      mReverseByteOrder = PR_FALSE;
      rv = SaveByteOrder(nativeByteOrder);
    }
    else {
      mReverseByteOrder = PR_TRUE;
      rv = SaveByteOrder(bigEndianByteOrder);
    }
#else
    mReverseByteOrder = PR_FALSE;
    rv = SaveByteOrder(nativeByteOrder);
#endif
  }
  else
    mReverseByteOrder = !fileByteOrder.Equals(nativeByteOrder);

  return rv;
}

nsresult
nsFormHistory::GetByteOrder(nsAString& aByteOrder)
{
  NS_ENSURE_SUCCESS(OpenDatabase(), NS_ERROR_FAILURE);

  mdb_err err = GetRowValue(mMetaRow, kToken_ByteOrder, aByteOrder);
  NS_ENSURE_FALSE(err, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
nsFormHistory::SaveByteOrder(const nsAString& aByteOrder)
{
  NS_ENSURE_SUCCESS(OpenDatabase(), NS_ERROR_FAILURE);

  mdb_err err = SetRowValue(mMetaRow, kToken_ByteOrder, aByteOrder);
  NS_ENSURE_FALSE(err, NS_ERROR_FAILURE);

  return NS_OK;
}
