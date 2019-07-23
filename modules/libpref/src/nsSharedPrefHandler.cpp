





































#include "nsSharedPrefHandler.h"
#include "nsPrefService.h"

#include "nsServiceManagerUtils.h"
#include "nsILocalFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsReadableUtils.h"
#include "ipcMessageReader.h"
#include "ipcMessageWriter.h"


#if defined(PR_LOGGING)

PRLogModuleInfo *gPrefsTransactionObserverLog = nsnull;
#define LOG(args) PR_LOG(PrefsTransactionObserver, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

nsSharedPrefHandler *gSharedPrefHandler = nsnull;


#define kPrefsTSQueueName NS_LITERAL_CSTRING("prefs")

#define kExceptionListFileName NS_LITERAL_CSTRING("nonshared.txt")
const char kExceptionListCommentChar = '#';

const PRUint32 kCurrentPrefsTransactionDataVersion = 1;


static PRBool PR_CALLBACK enumFind(void* aElement, void *aData);
static PRBool PR_CALLBACK enumFree(void* aElement, void *aData);
static PRInt32 ReadLine(FILE* inStm, nsACString& destString);























nsSharedPrefHandler::nsSharedPrefHandler() :
  mPrefService(nsnull), mPrefsTSQueueName("prefs"),
  mSessionActive(PR_FALSE),
  mReadingUserPrefs(PR_FALSE),
  mProcessingTransaction(PR_FALSE)
{
#if defined(PR_LOGGING)
  if (!gPrefsTransactionObserverLog)
    gPrefsTransactionObserverLog = PR_NewLogModule("nsSharedPrefHandler");
#endif
}

nsSharedPrefHandler::~nsSharedPrefHandler()
{
  mExceptionList.EnumerateForwards(enumFree, nsnull);
}
        
nsresult nsSharedPrefHandler::OnSessionBegin()
{
  nsresult rv = EnsureTransactionService();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = mTransService->Attach(kPrefsTSQueueName, this, PR_TRUE);
  NS_ASSERTION(NS_SUCCEEDED(rv), "ipcITransactionService::Attach() failed");
  
  if (NS_SUCCEEDED(rv))
    mSessionActive = PR_TRUE;
    
  return rv;
}

nsresult nsSharedPrefHandler::OnSessionEnd()
{
  nsresult rv = EnsureTransactionService();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mTransService->Detach(kPrefsTSQueueName);
  NS_ASSERTION(NS_SUCCEEDED(rv), "ipcITransactionService::Detach() failed");

  mSessionActive = PR_FALSE;

  return rv;
}

nsresult nsSharedPrefHandler::OnSavePrefs()
{
  nsresult rv = EnsureTransactionService();
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  
  
  rv = mTransService->Flush(kPrefsTSQueueName, PR_TRUE);
  NS_ASSERTION(NS_SUCCEEDED(rv), "ipcITransactionService::Flush() failed");

  return NS_OK;
}
    
nsresult nsSharedPrefHandler::OnPrefChanged(PRBool defaultPref,
                                            PrefHashEntry* pref,
                                            PrefValue newValue)
{
  if (!mSessionActive 
    || defaultPref
    || !IsPrefShared(pref->key)
    || mReadingUserPrefs
    || mProcessingTransaction)
    return NS_OK;
    
  nsresult rv = EnsureTransactionService();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 valueLen, prefNameLen = strlen(pref->key);
  
  ipcMessageWriter outMsg(256);
  outMsg.PutInt32(kCurrentPrefsTransactionDataVersion);
  outMsg.PutInt32(defaultPref); 
  outMsg.PutInt32(prefNameLen + 1);
  outMsg.PutBytes(pref->key, prefNameLen + 1);

  switch (pref->flags & PREF_VALUETYPE_MASK) {
    case PREF_STRING:
      outMsg.PutInt32(PREF_STRING);
      valueLen = strlen(newValue.stringVal) + 1;
      outMsg.PutInt32(valueLen);
      outMsg.PutBytes(newValue.stringVal, valueLen);
      break;
    case PREF_INT:
      outMsg.PutInt32(PREF_INT);
      outMsg.PutInt32(sizeof(PRInt32));
      outMsg.PutInt32(newValue.intVal);
      break;
    case PREF_BOOL:
      outMsg.PutInt32(PREF_BOOL);
      outMsg.PutInt32(sizeof(PRInt32));
      outMsg.PutInt32(newValue.boolVal);
      break;
    default:
      return NS_ERROR_UNEXPECTED;
  }
  
  rv = outMsg.HasError() ? NS_ERROR_FAILURE : NS_OK;
  NS_ASSERTION(NS_SUCCEEDED(rv), "OnPrefChanged: outMsg failed");
  if (NS_SUCCEEDED(rv)) {
    rv = mTransService->PostTransaction(kPrefsTSQueueName, outMsg.GetBuffer(), outMsg.GetSize());
    NS_ASSERTION(NS_SUCCEEDED(rv), "ipcITransactionService::PostTransaction() failed");
  }
  return rv;
}

PRBool nsSharedPrefHandler::IsPrefShared(const char* prefName)
{
  if (!mExceptionList.Count()) 
    return PR_TRUE;
    
  
  return mExceptionList.EnumerateForwards(enumFind, const_cast<char*>(prefName));
}





nsresult nsSharedPrefHandler::Init(nsPrefService* aOwner)
{
  NS_ENSURE_ARG(aOwner);
  mPrefService = aOwner;
  (void)ReadExceptionFile(); 
  
  return NS_OK;
}

nsresult nsSharedPrefHandler::ReadExceptionFile()
{
  nsresult rv;
  
  nsCOMPtr<nsIProperties> directoryService =
      do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsILocalFile> exceptionFile;
    rv = directoryService->Get(NS_APP_PREF_DEFAULTS_50_DIR, NS_GET_IID(nsILocalFile),
                              getter_AddRefs(exceptionFile));
    if (NS_SUCCEEDED(rv)) {
      rv = exceptionFile->AppendNative(kExceptionListFileName);
      if (NS_SUCCEEDED(rv)) {
        FILE *inStm;
        rv = exceptionFile->OpenANSIFileDesc("r", &inStm);
        if (NS_SUCCEEDED(rv)) {
          nsCAutoString lineStr;
          while (ReadLine(inStm, lineStr) != EOF) {
            lineStr.CompressWhitespace();
            if (lineStr.IsEmpty() || lineStr.CharAt(0) == kExceptionListCommentChar)
              continue;
              
            char *rawStr = ToNewCString(lineStr);
            if (!rawStr) {
              rv = NS_ERROR_OUT_OF_MEMORY;
              break;
            }
            mExceptionList.AppendElement(rawStr);
          }
          fclose(inStm);
        }
      }
    }
  }
  return rv;
}

nsresult nsSharedPrefHandler::EnsureTransactionService()
{
  if (mTransService)
    return NS_OK;
  nsresult rv;
  mTransService = do_GetService(IPC_TRANSACTIONSERVICE_CONTRACTID, &rv);
  return rv;
}





NS_IMPL_ISUPPORTS1(nsSharedPrefHandler, ipcITransactionObserver)





NS_IMETHODIMP nsSharedPrefHandler::OnTransactionAvailable(PRUint32 aQueueID, const PRUint8 *aData, PRUint32 aDataLen)
{
    LOG(("nsSharedPrefHandler::OnTransactionAvailable [%s]\n", aData));

    ipcMessageReader inMsg(aData, aDataLen);

    PRUint32 dataVersion, prefAction, dataLen, prefKind, tempInt32;
    const char *stringStart;
    
    dataVersion = inMsg.GetInt32();
    NS_ENSURE_TRUE(dataVersion == kCurrentPrefsTransactionDataVersion, NS_ERROR_INVALID_ARG);
    prefAction = inMsg.GetInt32();  
    dataLen = inMsg.GetInt32(); 
    stringStart = (const char *)inMsg.GetPtr();
    nsDependentCString prefNameStr(stringStart);
    inMsg.AdvancePtr(dataLen);
    prefKind = inMsg.GetInt32();
    dataLen = inMsg.GetInt32();
    
    mProcessingTransaction = PR_TRUE; 
    switch (prefKind) {
      case PREF_STRING:
        {
        stringStart = (const char *)inMsg.GetPtr();
        nsDependentCString prefStrValueStr(stringStart);
        inMsg.AdvancePtr(dataLen);
        NS_ASSERTION(!inMsg.HasError(), "error in reading transaction");
        if (!inMsg.HasError())
          PREF_SetCharPref(prefNameStr.get(), prefStrValueStr.get());
        }
        break;
      case PREF_INT:
        tempInt32 = inMsg.GetInt32();
        NS_ASSERTION(!inMsg.HasError(), "error in reading transaction");
        if (!inMsg.HasError())
          PREF_SetIntPref(prefNameStr.get(), tempInt32);
        break;
      case PREF_BOOL:
        tempInt32 = inMsg.GetInt32();
        NS_ASSERTION(!inMsg.HasError(), "error in reading transaction");
        if (!inMsg.HasError())
          PREF_SetBoolPref(prefNameStr.get(), tempInt32);
        break;
    }
    mProcessingTransaction = PR_FALSE;
    
    return NS_OK;
}

NS_IMETHODIMP nsSharedPrefHandler::OnAttachReply(PRUint32 aQueueID, PRUint32 aStatus)
{
    LOG(("nsSharedPrefHandler::OnAttachReply [%d]\n", aStatus));

    
    mPrefService->ResetUserPrefs();
    mPrefService->ReadUserPrefs(nsnull);
    
    return NS_OK;
}

NS_IMETHODIMP nsSharedPrefHandler::OnDetachReply(PRUint32 aQueueID, PRUint32 aStatus)
{
    LOG(("tmModuleTest: nsSharedPrefHandler::OnDetachReply [%d]\n", aStatus));
    return NS_OK;
}

NS_IMETHODIMP nsSharedPrefHandler::OnFlushReply(PRUint32 aQueueID, PRUint32 aStatus)
{
    LOG(("tmModuleTest: nsSharedPrefHandler::OnFlushReply [%d]\n", aStatus));
    
    
    
    mPrefService->SavePrefFileInternal(nsnull);
    return NS_OK;
}





static PRBool PR_CALLBACK enumFind(void* aElement, void *aData)
{
  char *elemStr = static_cast<char*>(aElement);
  char *searchStr = static_cast<char*>(aData);
  
  return (strncmp(elemStr, searchStr, strlen(elemStr)) != 0);
}

static PRBool PR_CALLBACK enumFree(void* aElement, void *aData)
{
  if (aElement)
    nsMemory::Free(aElement);
  return PR_TRUE;
}

static PRInt32 ReadLine(FILE* inStm, nsACString& destString)
{
  char stackBuf[512];
  PRUint32 charsInBuf = 0;
  destString.Truncate();
  int c;
  
  while (1) {
    c = getc(inStm);
    if (c == EOF)
      break;
    else if (c == '\r') {
      c = getc(inStm);
      if (c != '\n')
        ungetc(c, inStm);
      break;
    }
    else if (c == '\n')
      break;
    else {
      if (charsInBuf >= sizeof(stackBuf)) {
        destString.Append(stackBuf, charsInBuf);
        charsInBuf = 0;
      }
      stackBuf[charsInBuf++] = c;
    }
  }
  if (charsInBuf)
    destString.Append(stackBuf, charsInBuf);
  return (c == EOF && destString.IsEmpty()) ? EOF : 1;
}





nsresult NS_CreateSharedPrefHandler(nsPrefService *aOwner)
{
  nsSharedPrefHandler *local = new nsSharedPrefHandler;
  if (!local)
    return NS_ERROR_OUT_OF_MEMORY;
  nsresult rv = local->Init(aOwner);
  if (NS_FAILED(rv)) {
    delete local;
    return rv;
  }
  NS_ADDREF(gSharedPrefHandler = local);
  return NS_OK;
}

