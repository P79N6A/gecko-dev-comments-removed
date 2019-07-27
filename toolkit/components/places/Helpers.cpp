




#include "Helpers.h"
#include "mozIStorageError.h"
#include "plbase64.h"
#include "prio.h"
#include "nsString.h"
#include "nsNavHistory.h"
#include "mozilla/Services.h"


#define GUID_LENGTH 12

namespace mozilla {
namespace places {




NS_IMPL_ISUPPORTS(
  AsyncStatementCallback
, mozIStorageStatementCallback
)

NS_IMETHODIMP
AsyncStatementCallback::HandleResult(mozIStorageResultSet *aResultSet)
{
  MOZ_ASSERT(false, "Was not expecting a resultset, but got it.");
  return NS_OK;
}

NS_IMETHODIMP
AsyncStatementCallback::HandleCompletion(uint16_t aReason)
{
  return NS_OK;
}

NS_IMETHODIMP
AsyncStatementCallback::HandleError(mozIStorageError *aError)
{
#ifdef DEBUG
  int32_t result;
  nsresult rv = aError->GetResult(&result);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoCString message;
  rv = aError->GetMessage(message);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString warnMsg;
  warnMsg.AppendLiteral("An error occurred while executing an async statement: ");
  warnMsg.AppendInt(result);
  warnMsg.Append(' ');
  warnMsg.Append(message);
  NS_WARNING(warnMsg.get());
#endif

  return NS_OK;
}

#define URI_TO_URLCSTRING(uri, spec) \
  nsAutoCString spec; \
  if (NS_FAILED(aURI->GetSpec(spec))) { \
    return NS_ERROR_UNEXPECTED; \
  }


nsresult 
URIBinder::Bind(mozIStorageStatement* aStatement,
                int32_t aIndex,
                nsIURI* aURI)
{
  NS_ASSERTION(aStatement, "Must have non-null statement");
  NS_ASSERTION(aURI, "Must have non-null uri");

  URI_TO_URLCSTRING(aURI, spec);
  return URIBinder::Bind(aStatement, aIndex, spec);
}


nsresult 
URIBinder::Bind(mozIStorageStatement* aStatement,
                int32_t index,
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
                int32_t aIndex,
                nsIURI* aURI)
{
  NS_ASSERTION(aParams, "Must have non-null statement");
  NS_ASSERTION(aURI, "Must have non-null uri");

  URI_TO_URLCSTRING(aURI, spec);
  return URIBinder::Bind(aParams, aIndex, spec);
}


nsresult 
URIBinder::Bind(mozIStorageBindingParams* aParams,
                int32_t index,
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
  nsAutoCString forward8;
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
  aRevHost.Append(char16_t('.'));
}

void
ReverseString(const nsString& aInput, nsString& aReversed)
{
  aReversed.Truncate(0);
  for (int32_t i = aInput.Length() - 1; i >= 0; i--) {
    aReversed.Append(aInput[i]);
  }
}

static
nsresult
Base64urlEncode(const uint8_t* aBytes,
                uint32_t aNumBytes,
                nsCString& _result)
{
  
  
  
  
  uint32_t length = (aNumBytes + 2) / 3 * 4; 
  NS_ENSURE_TRUE(_result.SetCapacity(length + 1, fallible),
                 NS_ERROR_OUT_OF_MEMORY);
  _result.SetLength(length);
  (void)PL_Base64Encode(reinterpret_cast<const char*>(aBytes), aNumBytes,
                        _result.BeginWriting());

  
  
  _result.ReplaceChar('+', '-');
  _result.ReplaceChar('/', '_');
  return NS_OK;
}

#ifdef XP_WIN
} 
} 



#include <windows.h>
#include <wincrypt.h>

namespace mozilla {
namespace places {
#endif

static
nsresult
GenerateRandomBytes(uint32_t aSize,
                    uint8_t* _buffer)
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
  NS_ENSURE_ARG_MAX(aSize, INT32_MAX);
  PRFileDesc* urandom = PR_Open("/dev/urandom", PR_RDONLY, 0);
  nsresult rv = NS_ERROR_FAILURE;
  if (urandom) {
    int32_t bytesRead = PR_Read(urandom, _buffer, aSize);
    if (bytesRead == static_cast<int32_t>(aSize)) {
      rv = NS_OK;
    }
    (void)PR_Close(urandom);
  }
  return rv;
#endif
}

nsresult
GenerateGUID(nsCString& _guid)
{
  _guid.Truncate();

  
  
  const uint32_t kRequiredBytesLength =
    static_cast<uint32_t>(GUID_LENGTH / 4 * 3);

  uint8_t buffer[kRequiredBytesLength];
  nsresult rv = GenerateRandomBytes(kRequiredBytesLength, buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Base64urlEncode(buffer, kRequiredBytesLength, _guid);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(_guid.Length() == GUID_LENGTH, "GUID is not the right size!");
  return NS_OK;
}

bool
IsValidGUID(const nsACString& aGUID)
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
TruncateTitle(const nsACString& aTitle, nsACString& aTrimmed)
{
  aTrimmed = aTitle;
  if (aTitle.Length() > TITLE_LENGTH_MAX) {
    aTrimmed = StringHead(aTitle, TITLE_LENGTH_MAX);
  }
}

PRTime
RoundToMilliseconds(PRTime aTime) {
  return aTime - (aTime % PR_USEC_PER_MSEC);
}

PRTime
RoundedPRNow() {
  return RoundToMilliseconds(PR_Now());
}

void
ForceWALCheckpoint()
{
  nsRefPtr<Database> DB = Database::GetDatabase();
  if (DB) {
    nsCOMPtr<mozIStorageAsyncStatement> stmt = DB->GetAsyncStatement(
      "pragma wal_checkpoint "
    );
    if (stmt) {
      nsCOMPtr<mozIStoragePendingStatement> handle;
      (void)stmt->ExecuteAsync(nullptr, getter_AddRefs(handle));
    }
  }
}

bool
GetHiddenState(bool aIsRedirect,
               uint32_t aTransitionType)
{
  return aTransitionType == nsINavHistoryService::TRANSITION_FRAMED_LINK ||
         aTransitionType == nsINavHistoryService::TRANSITION_EMBED ||
         aIsRedirect;
}




PlacesEvent::PlacesEvent(const char* aTopic)
: mTopic(aTopic)
{
}

NS_IMETHODIMP
PlacesEvent::Run()
{
  Notify();
  return NS_OK;
}

void
PlacesEvent::Notify()
{
  NS_ASSERTION(NS_IsMainThread(), "Must only be used on the main thread!");
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    (void)obs->NotifyObservers(nullptr, mTopic, nullptr);
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(
  PlacesEvent
, nsRunnable
)




NS_IMETHODIMP
AsyncStatementCallbackNotifier::HandleCompletion(uint16_t aReason)
{
  if (aReason != mozIStorageStatementCallback::REASON_FINISHED)
    return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (obs) {
    (void)obs->NotifyObservers(nullptr, mTopic, nullptr);
  }

  return NS_OK;
}




NS_IMETHODIMP
AsyncStatementTelemetryTimer::HandleCompletion(uint16_t aReason)
{
  if (aReason == mozIStorageStatementCallback::REASON_FINISHED) {
    Telemetry::AccumulateTimeDelta(mHistogramId, mStart);
  }
  return NS_OK;
}

} 
} 
