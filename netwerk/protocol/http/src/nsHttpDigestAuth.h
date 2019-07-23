








































#ifndef nsDigestAuth_h__
#define nsDigestAuth_h__

#include "nsIHttpAuthenticator.h"
#include "nsICryptoHash.h"
#include "nsString.h"
#include "nsCOMPtr.h"

#define ALGO_SPECIFIED 0x01
#define ALGO_MD5 0x02
#define ALGO_MD5_SESS 0x04
#define QOP_AUTH 0x01
#define QOP_AUTH_INT 0x02

#define DIGEST_LENGTH 16
#define EXPANDED_DIGEST_LENGTH 32
#define NONCE_COUNT_LENGTH 8





class nsHttpDigestAuth : public nsIHttpAuthenticator
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPAUTHENTICATOR

    nsHttpDigestAuth();
    ~nsHttpDigestAuth();

  protected:
    nsresult ExpandToHex(const char * digest, char * result);

    nsresult CalculateResponse(const char * ha1_digest,
                               const char * ha2_digest,
                               const nsAFlatCString & nonce,
                               PRUint16 qop,
                               const char * nonce_count,
                               const nsAFlatCString & cnonce,
                               char * result);

    nsresult CalculateHA1(const nsAFlatCString & username,
                          const nsAFlatCString & password,
                          const nsAFlatCString & realm,
                          PRUint16 algorithm,
                          const nsAFlatCString & nonce,
                          const nsAFlatCString & cnonce,
                          char * result);

    nsresult CalculateHA2(const nsAFlatCString & http_method,
                          const nsAFlatCString & http_uri_path,
                          PRUint16 qop,
                          const char * body_digest,
                          char * result);

    nsresult ParseChallenge(const char * challenge,
                            nsACString & realm,
                            nsACString & domain,
                            nsACString & nonce,
                            nsACString & opaque,
                            PRBool * stale,
                            PRUint16 * algorithm,
                            PRUint16 * qop);

    
    nsresult MD5Hash(const char *buf, PRUint32 len);

    nsresult GetMethodAndPath(nsIHttpChannel *, PRBool, nsCString &, nsCString &);

  protected:
    nsCOMPtr<nsICryptoHash>        mVerifier;
    char                           mHashBuf[DIGEST_LENGTH];
};

#endif 
