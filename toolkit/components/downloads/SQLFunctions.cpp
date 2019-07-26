




#include "mozilla/storage.h"
#include "mozilla/storage/Variant.h"
#include "mozilla/mozalloc.h"
#include "nsString.h"
#include "SQLFunctions.h"
#include "nsUTF8Utils.h"
#include "plbase64.h"
#include "prio.h"
#if defined(XP_OS2)
#include "nsIRandomGenerator.h"
#endif


#define GUID_LENGTH 12

namespace mozilla {
namespace downloads {











nsresult
GenerateGUIDFunction::create(mozIStorageConnection *aDBConn)
{
#if defined(XP_OS2)
  
  
  
  nsCOMPtr<nsIRandomGenerator> rg =
    do_GetService("@mozilla.org/security/random-generator;1");
  NS_ENSURE_STATE(rg);
#endif

  nsRefPtr<GenerateGUIDFunction> function = new GenerateGUIDFunction();
  nsresult rv = aDBConn->CreateFunction(
    NS_LITERAL_CSTRING("generate_guid"), 0, function
  );
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(
    GenerateGUIDFunction,
    mozIStorageFunction
)

static
nsresult
Base64urlEncode(const uint8_t* aBytes,
                uint32_t aNumBytes,
                nsCString& _result)
{
  
  
  
  
  uint32_t length = (aNumBytes + 2) / 3 * 4; 
  NS_ENSURE_TRUE(_result.SetCapacity(length + 1, mozilla::fallible_t()),
                 NS_ERROR_OUT_OF_MEMORY);
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
  NS_ENSURE_ARG_MAX(aSize, PR_INT32_MAX);
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
#elif defined(XP_OS2)
  nsCOMPtr<nsIRandomGenerator> rg =
      do_GetService("@mozilla.org/security/random-generator;1");
  NS_ENSURE_STATE(rg);

  uint8_t* temp;
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




NS_IMETHODIMP
GenerateGUIDFunction::OnFunctionCall(mozIStorageValueArray *aArguments,
                                     nsIVariant **_result)
{
  nsAutoCString guid;
  nsresult rv = GenerateGUID(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_result = new mozilla::storage::UTF8TextVariant(guid));
  return NS_OK;
}

} 
} 
