








































#include <stdlib.h>
#include "nsHttp.h"
#include "nsHttpDigestAuth.h"
#include "nsIHttpChannel.h"
#include "nsIServiceManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsEscape.h"
#include "nsNetCID.h"
#include "plbase64.h"
#include "plstr.h"
#include "prprf.h"
#include "prmem.h"
#include "nsCRT.h"





nsHttpDigestAuth::nsHttpDigestAuth()
{}

nsHttpDigestAuth::~nsHttpDigestAuth()
{}





NS_IMPL_ISUPPORTS1(nsHttpDigestAuth, nsIHttpAuthenticator)





nsresult
nsHttpDigestAuth::MD5Hash(const char *buf, PRUint32 len)
{
  nsresult rv;

  
  
  if (!mVerifier) {
    mVerifier = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
      LOG(("nsHttpDigestAuth: no crypto hash!\n"));
      return rv;
    }
  }

  rv = mVerifier->Init(nsICryptoHash::MD5);
  if (NS_FAILED(rv)) return rv;

  rv = mVerifier->Update((unsigned char*)buf, len);
  if (NS_FAILED(rv)) return rv;

  nsCAutoString hashString;
  rv = mVerifier->Finish(PR_FALSE, hashString);
  if (NS_FAILED(rv)) return rv;

  NS_ENSURE_STATE(hashString.Length() == sizeof(mHashBuf));
  memcpy(mHashBuf, hashString.get(), hashString.Length());

  return rv;
}

nsresult
nsHttpDigestAuth::GetMethodAndPath(nsIHttpChannel *httpChannel,
                                   PRBool          isProxyAuth,
                                   nsCString      &httpMethod,
                                   nsCString      &path)
{
  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  rv = httpChannel->GetURI(getter_AddRefs(uri));
  if (NS_SUCCEEDED(rv)) {
    PRBool isSecure;
    rv = uri->SchemeIs("https", &isSecure);
    if (NS_SUCCEEDED(rv)) {
      
      
      
      
      if (isSecure && isProxyAuth) {
        httpMethod.AssignLiteral("CONNECT");
        
        
        
        
        
        PRInt32 port;
        rv  = uri->GetAsciiHost(path);
        rv |= uri->GetPort(&port);
        if (NS_SUCCEEDED(rv)) {
          path.Append(':');
          path.AppendInt(port < 0 ? NS_HTTPS_DEFAULT_PORT : port);
        }
      }
      else { 
        rv  = httpChannel->GetRequestMethod(httpMethod);
        rv |= uri->GetPath(path);
        if (NS_SUCCEEDED(rv)) {
          
          
          
          PRInt32 ref = path.RFindChar('#');
          if (ref != kNotFound)
            path.Truncate(ref);
          
          
          
          
          
          
          
          nsCAutoString buf;
          path = NS_EscapeURL(path, esc_OnlyNonASCII, buf);
        }
      }
    }
  }
  return rv;
}





NS_IMETHODIMP
nsHttpDigestAuth::ChallengeReceived(nsIHttpChannel *httpChannel,
                                    const char *challenge,
                                    PRBool isProxyAuth,
                                    nsISupports **sessionState,
                                    nsISupports **continuationState,
                                    PRBool *result)
{
  nsCAutoString realm, domain, nonce, opaque;
  PRBool stale;
  PRUint16 algorithm, qop;

  nsresult rv = ParseChallenge(challenge, realm, domain, nonce, opaque,
                               &stale, &algorithm, &qop);
  if (NS_FAILED(rv)) return rv;

  
  
  
  *result = !stale;

  
  NS_IF_RELEASE(*sessionState);
  return NS_OK;
}

NS_IMETHODIMP
nsHttpDigestAuth::GenerateCredentials(nsIHttpChannel *httpChannel,
                                      const char *challenge,
                                      PRBool isProxyAuth,
                                      const PRUnichar *userdomain,
                                      const PRUnichar *username,
                                      const PRUnichar *password,
                                      nsISupports **sessionState,
                                      nsISupports **continuationState,
                                      char **creds)

{
  LOG(("nsHttpDigestAuth::GenerateCredentials [challenge=%s]\n", challenge));

  NS_ENSURE_ARG_POINTER(creds);

  PRBool isDigestAuth = !PL_strncasecmp(challenge, "digest ", 7);
  NS_ENSURE_TRUE(isDigestAuth, NS_ERROR_UNEXPECTED);

  
  PRBool requireExtraQuotes = PR_FALSE;
  {
    nsCAutoString serverVal;
    httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Server"), serverVal);
    if (!serverVal.IsEmpty()) {
      requireExtraQuotes = !PL_strncasecmp(serverVal.get(), "Microsoft-IIS", 13);
    }
  }

  nsresult rv;
  nsCAutoString httpMethod;
  nsCAutoString path;
  rv = GetMethodAndPath(httpChannel, isProxyAuth, httpMethod, path);
  if (NS_FAILED(rv)) return rv;

  nsCAutoString realm, domain, nonce, opaque;
  PRBool stale;
  PRUint16 algorithm, qop;

  rv = ParseChallenge(challenge, realm, domain, nonce, opaque,
                      &stale, &algorithm, &qop);
  if (NS_FAILED(rv)) {
    LOG(("nsHttpDigestAuth::GenerateCredentials [ParseChallenge failed rv=%x]\n", rv));
    return rv;
  }

  char ha1_digest[EXPANDED_DIGEST_LENGTH+1];
  char ha2_digest[EXPANDED_DIGEST_LENGTH+1];
  char response_digest[EXPANDED_DIGEST_LENGTH+1];
  char upload_data_digest[EXPANDED_DIGEST_LENGTH+1];

  if (qop & QOP_AUTH_INT) {
    
    qop &= ~QOP_AUTH_INT;

    NS_WARNING("no support for Digest authentication with data integrity quality of protection");

    





#if 0
    if (http_channel != nsnull)
    {
      nsIInputStream * upload;
      nsCOMPtr<nsIUploadChannel> uc = do_QueryInterface(http_channel);
      NS_ENSURE_TRUE(uc, NS_ERROR_UNEXPECTED);
      uc->GetUploadStream(&upload);
      if (upload) {
        char * upload_buffer;
        int upload_buffer_length = 0;
        
        const char * digest = (const char*)
        nsNetwerkMD5Digest(upload_buffer, upload_buffer_length);
        ExpandToHex(digest, upload_data_digest);
        NS_RELEASE(upload);
      }
    }
#endif
  }

  if (!(algorithm & ALGO_MD5 || algorithm & ALGO_MD5_SESS)) {
    
    NS_WARNING("unsupported algorithm requested by Digest authentication");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  
  
  
  
  
  char nonce_count[NONCE_COUNT_LENGTH+1] = "00000001"; 
  if (*sessionState) {
    nsCOMPtr<nsISupportsPRUint32> v(do_QueryInterface(*sessionState));
    if (v) {
      PRUint32 nc;
      v->GetData(&nc);
      PR_snprintf(nonce_count, sizeof(nonce_count), "%08x", ++nc);
      v->SetData(nc);
    }
  }
  else {
    nsCOMPtr<nsISupportsPRUint32> v(
            do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv));
    if (v) {        
      v->SetData(1);
      NS_ADDREF(*sessionState = v);
    }
  }
  LOG(("   nonce_count=%s\n", nonce_count));

  
  
  
  
  nsCAutoString cnonce;
  static const char hexChar[] = "0123456789abcdef"; 
  for (int i=0; i<16; ++i) {
    cnonce.Append(hexChar[(int)(15.0 * rand()/(RAND_MAX + 1.0))]);
  }
  LOG(("   cnonce=%s\n", cnonce.get()));

  
  
  

  NS_ConvertUTF16toUTF8 cUser(username), cPass(password);
  rv = CalculateHA1(cUser, cPass, realm, algorithm, nonce, cnonce, ha1_digest);
  if (NS_FAILED(rv)) return rv;

  rv = CalculateHA2(httpMethod, path, qop, upload_data_digest, ha2_digest);
  if (NS_FAILED(rv)) return rv;

  rv = CalculateResponse(ha1_digest, ha2_digest, nonce, qop, nonce_count,
                         cnonce, response_digest);
  if (NS_FAILED(rv)) return rv;

  
  
  
  
  
  
  
  
  

  nsCAutoString authString;

  authString.AssignLiteral("Digest username=");
  rv = AppendQuotedString(cUser, authString);
  NS_ENSURE_SUCCESS(rv, rv);

  authString.AppendLiteral(", realm=");
  rv = AppendQuotedString(realm, authString);
  NS_ENSURE_SUCCESS(rv, rv);

  authString.AppendLiteral(", nonce=");
  rv = AppendQuotedString(nonce, authString);
  NS_ENSURE_SUCCESS(rv, rv);

  authString.AppendLiteral(", uri=\"");
  authString += path;
  if (algorithm & ALGO_SPECIFIED) {
    authString.AppendLiteral("\", algorithm=");
    if (algorithm & ALGO_MD5_SESS)
      authString.AppendLiteral("MD5-sess");
    else
      authString.AppendLiteral("MD5");
  } else {
    authString += '\"';
  }
  authString.AppendLiteral(", response=\"");
  authString += response_digest;
  authString += '\"';

  if (!opaque.IsEmpty()) {
    authString.AppendLiteral(", opaque=");
    rv = AppendQuotedString(opaque, authString);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (qop) {
    authString.AppendLiteral(", qop=");
    if (requireExtraQuotes)
      authString += '\"';
    authString.AppendLiteral("auth");
    if (qop & QOP_AUTH_INT)
      authString.AppendLiteral("-int");
    if (requireExtraQuotes)
      authString += '\"';
    authString.AppendLiteral(", nc=");
    authString += nonce_count;

    authString.AppendLiteral(", cnonce=");
    rv = AppendQuotedString(cnonce, authString);
    NS_ENSURE_SUCCESS(rv, rv);
  }


  *creds = ToNewCString(authString);
  return NS_OK;
}

NS_IMETHODIMP
nsHttpDigestAuth::GetAuthFlags(PRUint32 *flags)
{
  *flags = REQUEST_BASED | REUSABLE_CHALLENGE | IDENTITY_ENCRYPTED;
  
  
  
  
  return NS_OK;
}

nsresult
nsHttpDigestAuth::CalculateResponse(const char * ha1_digest,
                                    const char * ha2_digest,
                                    const nsAFlatCString & nonce,
                                    PRUint16 qop,
                                    const char * nonce_count,
                                    const nsAFlatCString & cnonce,
                                    char * result)
{
  PRUint32 len = 2*EXPANDED_DIGEST_LENGTH + nonce.Length() + 2;

  if (qop & QOP_AUTH || qop & QOP_AUTH_INT) {
    len += cnonce.Length() + NONCE_COUNT_LENGTH + 3;
    if (qop & QOP_AUTH_INT)
      len += 8; 
    else
      len += 4; 
  }

  nsCAutoString contents;
  contents.SetCapacity(len);

  contents.Assign(ha1_digest, EXPANDED_DIGEST_LENGTH);
  contents.Append(':');
  contents.Append(nonce);
  contents.Append(':');

  if (qop & QOP_AUTH || qop & QOP_AUTH_INT) {
    contents.Append(nonce_count, NONCE_COUNT_LENGTH);
    contents.Append(':');
    contents.Append(cnonce);
    contents.Append(':');
    if (qop & QOP_AUTH_INT)
      contents.AppendLiteral("auth-int:");
    else
      contents.AppendLiteral("auth:");
  }

  contents.Append(ha2_digest, EXPANDED_DIGEST_LENGTH);

  nsresult rv = MD5Hash(contents.get(), contents.Length());
  if (NS_SUCCEEDED(rv))
    rv = ExpandToHex(mHashBuf, result);
  return rv;
}

nsresult
nsHttpDigestAuth::ExpandToHex(const char * digest, char * result)
{
  PRInt16 index, value;

  for (index = 0; index < DIGEST_LENGTH; index++) {
    value = (digest[index] >> 4) & 0xf;
    if (value < 10)
      result[index*2] = value + '0';
    else
      result[index*2] = value - 10 + 'a';

    value = digest[index] & 0xf;
    if (value < 10)
      result[(index*2)+1] = value + '0';
    else
      result[(index*2)+1] = value - 10 + 'a';
  }

  result[EXPANDED_DIGEST_LENGTH] = 0;
  return NS_OK;
}

nsresult
nsHttpDigestAuth::CalculateHA1(const nsAFlatCString & username,
                               const nsAFlatCString & password,
                               const nsAFlatCString & realm,
                               PRUint16 algorithm,
                               const nsAFlatCString & nonce,
                               const nsAFlatCString & cnonce,
                               char * result)
{
  PRInt16 len = username.Length() + password.Length() + realm.Length() + 2;
  if (algorithm & ALGO_MD5_SESS) {
    PRInt16 exlen = EXPANDED_DIGEST_LENGTH + nonce.Length() + cnonce.Length() + 2;
    if (exlen > len)
        len = exlen;
  }

  nsCAutoString contents;
  contents.SetCapacity(len + 1);

  contents.Assign(username);
  contents.Append(':');
  contents.Append(realm);
  contents.Append(':');
  contents.Append(password);

  nsresult rv;
  rv = MD5Hash(contents.get(), contents.Length());
  if (NS_FAILED(rv))
    return rv;

  if (algorithm & ALGO_MD5_SESS) {
    char part1[EXPANDED_DIGEST_LENGTH+1];
    ExpandToHex(mHashBuf, part1);

    contents.Assign(part1, EXPANDED_DIGEST_LENGTH);
    contents.Append(':');
    contents.Append(nonce);
    contents.Append(':');
    contents.Append(cnonce);

    rv = MD5Hash(contents.get(), contents.Length());
    if (NS_FAILED(rv))
      return rv;
  }

  return ExpandToHex(mHashBuf, result);
}

nsresult
nsHttpDigestAuth::CalculateHA2(const nsAFlatCString & method,
                               const nsAFlatCString & path,
                               PRUint16 qop,
                               const char * bodyDigest,
                               char * result)
{
  PRInt16 methodLen = method.Length();
  PRInt16 pathLen = path.Length();
  PRInt16 len = methodLen + pathLen + 1;

  if (qop & QOP_AUTH_INT) {
    len += EXPANDED_DIGEST_LENGTH + 1;
  }

  nsCAutoString contents;
  contents.SetCapacity(len);

  contents.Assign(method);
  contents.Append(':');
  contents.Append(path);

  if (qop & QOP_AUTH_INT) {
    contents.Append(':');
    contents.Append(bodyDigest, EXPANDED_DIGEST_LENGTH);
  }

  nsresult rv = MD5Hash(contents.get(), contents.Length());
  if (NS_SUCCEEDED(rv))
    rv = ExpandToHex(mHashBuf, result);
  return rv;
}

nsresult
nsHttpDigestAuth::ParseChallenge(const char * challenge,
                                 nsACString & realm,
                                 nsACString & domain,
                                 nsACString & nonce,
                                 nsACString & opaque,
                                 PRBool * stale,
                                 PRUint16 * algorithm,
                                 PRUint16 * qop)
{
  const char *p = challenge + 7; 

  *stale = PR_FALSE;
  *algorithm = ALGO_MD5; 
  *qop = 0;

  for (;;) {
    while (*p && (*p == ',' || nsCRT::IsAsciiSpace(*p)))
      ++p;
    if (!*p)
      break;

    
    PRInt16 nameStart = (p - challenge);
    while (*p && !nsCRT::IsAsciiSpace(*p) && *p != '=') 
      ++p;
    if (!*p)
      return NS_ERROR_INVALID_ARG;
    PRInt16 nameLength = (p - challenge) - nameStart;

    while (*p && (nsCRT::IsAsciiSpace(*p) || *p == '=')) 
      ++p;
    if (!*p) 
      return NS_ERROR_INVALID_ARG;

    PRBool quoted = PR_FALSE;
    if (*p == '"') {
      ++p;
      quoted = PR_TRUE;
    }

    
    PRInt16 valueStart = (p - challenge);
    PRInt16 valueLength = 0;
    if (quoted) {
      while (*p && *p != '"') 
        ++p;
      if (*p != '"') 
        return NS_ERROR_INVALID_ARG;
      valueLength = (p - challenge) - valueStart;
      ++p;
    } else {
      while (*p && !nsCRT::IsAsciiSpace(*p) && *p != ',') 
        ++p; 
      valueLength = (p - challenge) - valueStart;
    }

    
    if (nameLength == 5 &&
        nsCRT::strncasecmp(challenge+nameStart, "realm", 5) == 0)
    {
      realm.Assign(challenge+valueStart, valueLength);
    }
    else if (nameLength == 6 &&
        nsCRT::strncasecmp(challenge+nameStart, "domain", 6) == 0)
    {
      domain.Assign(challenge+valueStart, valueLength);
    }
    else if (nameLength == 5 &&
        nsCRT::strncasecmp(challenge+nameStart, "nonce", 5) == 0)
    {
      nonce.Assign(challenge+valueStart, valueLength);
    }
    else if (nameLength == 6 &&
        nsCRT::strncasecmp(challenge+nameStart, "opaque", 6) == 0)
    {
      opaque.Assign(challenge+valueStart, valueLength);
    }
    else if (nameLength == 5 &&
        nsCRT::strncasecmp(challenge+nameStart, "stale", 5) == 0)
    {
      if (nsCRT::strncasecmp(challenge+valueStart, "true", 4) == 0)
        *stale = PR_TRUE;
      else
        *stale = PR_FALSE;
    }
    else if (nameLength == 9 &&
        nsCRT::strncasecmp(challenge+nameStart, "algorithm", 9) == 0)
    {
      
      *algorithm = ALGO_SPECIFIED;
      if (valueLength == 3 &&
          nsCRT::strncasecmp(challenge+valueStart, "MD5", 3) == 0)
        *algorithm |= ALGO_MD5;
      else if (valueLength == 8 &&
          nsCRT::strncasecmp(challenge+valueStart, "MD5-sess", 8) == 0)
        *algorithm |= ALGO_MD5_SESS;
    }
    else if (nameLength == 3 &&
        nsCRT::strncasecmp(challenge+nameStart, "qop", 3) == 0)
    {
      PRInt16 ipos = valueStart;
      while (ipos < valueStart+valueLength) {
        while (ipos < valueStart+valueLength &&
               (nsCRT::IsAsciiSpace(challenge[ipos]) ||
                challenge[ipos] == ',')) 
          ipos++;
        PRInt16 algostart = ipos;
        while (ipos < valueStart+valueLength &&
               !nsCRT::IsAsciiSpace(challenge[ipos]) &&
               challenge[ipos] != ',') 
          ipos++;
        if ((ipos - algostart) == 4 &&
            nsCRT::strncasecmp(challenge+algostart, "auth", 4) == 0)
          *qop |= QOP_AUTH;
        else if ((ipos - algostart) == 8 &&
            nsCRT::strncasecmp(challenge+algostart, "auth-int", 8) == 0)
          *qop |= QOP_AUTH_INT;
      }
    }
  }
  return NS_OK;
}

nsresult
nsHttpDigestAuth::AppendQuotedString(const nsACString & value,
                                     nsACString & aHeaderLine)
{
  nsCAutoString quoted;
  nsACString::const_iterator s, e;
  value.BeginReading(s);
  value.EndReading(e);

  
  
  
  quoted.Append('"');
  for ( ; s != e; ++s) {
    
    
    
    if (*s <= 31 || *s == 127) {
      return NS_ERROR_FAILURE;
    }

    
    if (*s == '"' || *s == '\\') {
      quoted.Append('\\');
    }

    quoted.Append(*s);
  }
  
  
  quoted.Append('"');
  aHeaderLine.Append(quoted);
  return NS_OK;
}


