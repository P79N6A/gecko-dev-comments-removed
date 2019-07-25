





































#include "Helpers.h"
#include "mozIStorageError.h"
#include "plbase64.h"
#include "prio.h"
#include "nsString.h"
#include "nsNavHistory.h"
#include "mozilla/Services.h"
#if defined(XP_OS2)
#include "nsIRandomGenerator.h"
#endif


#define GUID_LENGTH 12

namespace mozilla {
namespace places {




NS_IMPL_ISUPPORTS1(
  AsyncStatementCallback
, mozIStorageStatementCallback
)

NS_IMETHODIMP
AsyncStatementCallback::HandleResult(mozIStorageResultSet *aResultSet)
{
  NS_ABORT_IF_FALSE(false, "Was not expecting a resultset, but got it.");
  return NS_OK;
}

NS_IMETHODIMP
AsyncStatementCallback::HandleCompletion(PRUint16 aReason)
{
  return NS_OK;
}

NS_IMETHODIMP
AsyncStatementCallback::HandleError(mozIStorageError *aError)
{
#ifdef DEBUG
  PRInt32 result;
  nsresult rv = aError->GetResult(&result);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString message;
  rv = aError->GetMessage(message);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString warnMsg;
  warnMsg.Append("An error occurred while executing an async statement: ");
  warnMsg.AppendInt(result);
  warnMsg.Append(" ");
  warnMsg.Append(message);
  NS_WARNING(warnMsg.get());
#endif

  return NS_OK;
}

#define URI_TO_URLCSTRING(uri, spec) \
  nsCAutoString spec; \
  if (NS_FAILED(aURI->GetSpec(spec))) { \
    return NS_ERROR_UNEXPECTED; \
  }


nsresult 
URIBinder::Bind(mozIStorageStatement* aStatement,
                PRInt32 aIndex,
                nsIURI* aURI)
{
  NS_ASSERTION(aStatement, "Must have non-null statement");
  NS_ASSERTION(aURI, "Must have non-null uri");

  URI_TO_URLCSTRING(aURI, spec);
  return URIBinder::Bind(aStatement, aIndex, spec);
}


nsresult 
URIBinder::Bind(mozIStorageStatement* aStatement,
                PRInt32 index,
                const nsACString& aURLString)
{
  NS_ASSERTION(aStatement, "Must have non-null statement");
  return aStatement->BindUTF8StringByIndex(
    index, StringHead(aURLString, URI_LENGTH_MAX)
  );
}


nsresult 
URIBinder::Bind(mozIStorageStatement* aStatement,
                const nsACString& aName,
                nsIURI* aURI)
{
  NS_ASSERTION(aStatement, "Must have non-null statement");
  NS_ASSERTION(aURI, "Must have non-null uri");

  URI_TO_URLCSTRING(aURI, spec);
  return URIBinder::Bind(aStatement, aName, spec);
}


nsresult 
URIBinder::Bind(mozIStorageStatement* aStatement,
                const nsACString& aName,
                const nsACString& aURLString)
{
  NS_ASSERTION(aStatement, "Must have non-null statement");
  return aStatement->BindUTF8StringByName(
    aName, StringHead(aURLString, URI_LENGTH_MAX)
  );
}


nsresult 
URIBinder::Bind(mozIStorageBindingParams* aParams,
                PRInt32 aIndex,
                nsIURI* aURI)
{
  NS_ASSERTION(aParams, "Must have non-null statement");
  NS_ASSERTION(aURI, "Must have non-null uri");

  URI_TO_URLCSTRING(aURI, spec);
  return URIBinder::Bind(aParams, aIndex, spec);
}


nsresult 
URIBinder::Bind(mozIStorageBindingParams* aParams,
                PRInt32 index,
                const nsACString& aURLString)
{
  NS_ASSERTION(aParams, "Must have non-null statement");
  return aParams->BindUTF8StringByIndex(
    index, StringHead(aURLString, URI_LENGTH_MAX)
  );
}


nsresult 
URIBinder::Bind(mozIStorageBindingParams* aParams,
                const nsACString& aName,
                nsIURI* aURI)
{
  NS_ASSERTION(aParams, "Must have non-null params array");
  NS_ASSERTION(aURI, "Must have non-null uri");

  URI_TO_URLCSTRING(aURI, spec);
  return URIBinder::Bind(aParams, aName, spec);
}


nsresult 
URIBinder::Bind(mozIStorageBindingParams* aParams,
                const nsACString& aName,
                const nsACString& aURLString)
{
  NS_ASSERTION(aParams, "Must have non-null params array");

  nsresult rv = aParams->BindUTF8StringByName(
    aName, StringHead(aURLString, URI_LENGTH_MAX)
  );
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

#undef URI_TO_URLCSTRING

nsresult
GetReversedHostname(nsIURI* aURI, nsString& aRevHost)
{
  nsCAutoString forward8;
  nsresult rv = aURI->GetHost(forward8);
  
  if (NS_FAILED(rv))
    return rv;

  
  GetReversedHostname(NS_ConvertUTF8toUTF16(forward8), aRevHost);
  return NS_OK;
}

void
GetReversedHostname(const nsString& aForward, nsString& aRevHost)
{
  ReverseString(aForward, aRevHost);
  aRevHost.Append(PRUnichar('.'));
}

void
ReverseString(const nsString& aInput, nsString& aReversed)
{
  aReversed.Truncate(0);
  for (PRInt32 i = aInput.Length() - 1; i >= 0; i--) {
    aReversed.Append(aInput[i]);
  }
}

static
nsresult
Base64urlEncode(const PRUint8* aBytes,
                PRUint32 aNumBytes,
                nsCString& _result)
{
  
  
  
  
  PRUint32 length = (aNumBytes + 2) / 3 * 4; 
  NS_ENSURE_TRUE(_result.SetCapacity(length + 1), NS_ERROR_OUT_OF_MEMORY);
  _result.SetLength(length);
  (void)PL_Base64Encode(reinterpret_cast<const char*>(aBytes), aNumBytes,
                        _result.BeginWriting());

  
  
  _result.ReplaceChar('+', '-');
  _result.ReplaceChar('/', '_');
  return NS_OK;
}

#ifdef XP_WIN


#include <windows.h>
#include <wincrypt.h>
#endif

static
nsresult
GenerateRandomBytes(PRUint32 aSize,
                    PRUint8* _buffer)
{
  
#if defined(XP_WIN)
  HCRYPTPROV cryptoProvider;
  BOOL rc = CryptAcquireContext(&cryptoProvider, 0, 0, PROV_RSA_FULL,
                                CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
  if (rc) {
    rc = CryptGenRandom(cryptoProvider, aSize, _buffer);
    (void)CryptReleaseContext(cryptoProvider, 0);
  }
  return rc ? NS_OK : NS_ERROR_FAILURE;

  
#elif defined(XP_UNIX)
  NS_ENSURE_ARG_MAX(aSize, PR_INT32_MAX);
  PRFileDesc* urandom = PR_Open("/dev/urandom", PR_RDONLY, 0);
  nsresult rv = NS_ERROR_FAILURE;
  if (urandom) {
    PRInt32 bytesRead = PR_Read(urandom, _buffer, aSize);
    if (bytesRead == static_cast<PRInt32>(aSize)) {
      rv = NS_OK;
    }
    (void)PR_Close(urandom);
  }
  return rv;
#elif defined(XP_OS2)
  nsCOMPtr<nsIRandomGenerator> rg =
    do_GetService("@mozilla.org/security/random-generator;1");
  NS_ENSURE_STATE(rg);

  PRUint8* temp;
  nsresult rv = rg->GenerateRandomBytes(aSize, &temp);
  NS_ENSURE_SUCCESS(rv, rv);
  memcpy(_buffer, temp, aSize);
  NS_Free(temp);
  return NS_OK;
#endif
}

nsresult
GenerateGUID(nsCString& _guid)
{
  _guid.Truncate();

  
  
  const PRUint32 kRequiredBytesLength =
    static_cast<PRUint32>(GUID_LENGTH / 4 * 3);

  PRUint8 buffer[kRequiredBytesLength];
  nsresult rv = GenerateRandomBytes(kRequiredBytesLength, buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Base64urlEncode(buffer, kRequiredBytesLength, _guid);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(_guid.Length() == GUID_LENGTH, "GUID is not the right size!");
  return NS_OK;
}

bool
IsValidGUID(const nsCString& aGUID)
{
  nsCString::size_type len = aGUID.Length();
  if (len != GUID_LENGTH) {
    return false;
  }

  for (nsCString::size_type i = 0; i < len; i++ ) {
    char c = aGUID[i];
    if ((c >= 'a' && c <= 'z') || 
        (c >= 'A' && c <= 'Z') || 
        (c >= '0' && c <= '9') || 
        c == '-' || c == '_') { 
      continue;
    }
    return false;
  }
  return true;
}

void
ForceWALCheckpoint(mozIStorageConnection* aDBConn)
{
  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  (void)aDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "pragma wal_checkpoint "
  ), getter_AddRefs(stmt));
  nsCOMPtr<mozIStoragePendingStatement> handle;
  (void)stmt->ExecuteAsync(nsnull, getter_AddRefs(handle));
}

bool
GetHiddenState(bool aIsRedirect,
               PRUint32 aTransitionType)
{
  return aTransitionType == nsINavHistoryService::TRANSITION_FRAMED_LINK ||
         aTransitionType == nsINavHistoryService::TRANSITION_EMBED ||
         aIsRedirect;
}




PlacesEvent::PlacesEvent(const char* aTopic)
: mTopic(aTopic)
, mDoubleEnqueue(false)
{
}

PlacesEvent::PlacesEvent(const char* aTopic,
                         bool aDoubleEnqueue)
: mTopic(aTopic)
, mDoubleEnqueue(aDoubleEnqueue)
{
}

NS_IMETHODIMP
PlacesEvent::Run()
{
  Notify();
  return NS_OK;
}

NS_IMETHODIMP
PlacesEvent::Complete()
{
  Notify();
  return NS_OK;
}

void
PlacesEvent::Notify()
{
  if (mDoubleEnqueue) {
    mDoubleEnqueue = false;
    (void)NS_DispatchToMainThread(this);
  }
  else {
    NS_ASSERTION(NS_IsMainThread(), "Must only be used on the main thread!");
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
      (void)obs->NotifyObservers(nsnull, mTopic, nsnull);
    }
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS2(
  PlacesEvent
, mozIStorageCompletionCallback
, nsIRunnable
)

} 
} 
