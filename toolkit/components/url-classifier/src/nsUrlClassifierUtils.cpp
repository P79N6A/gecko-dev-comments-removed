



































#include "nsEscape.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsUrlClassifierUtils.h"
#include "nsVoidArray.h"
#include "prprf.h"

static char int_to_hex_digit(PRInt32 i)
{
  NS_ASSERTION((i >= 0) && (i <= 15), "int too big in int_to_hex_digit");
  return static_cast<char>(((i < 10) ? (i + '0') : ((i - 10) + 'A')));
}

static PRBool
IsDecimal(const nsACString & num)
{
  for (PRUint32 i = 0; i < num.Length(); i++) {
    if (!isdigit(num[i])) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

static PRBool
IsHex(const nsACString & num)
{
  if (num.Length() < 3) {
    return PR_FALSE;
  }

  if (num[0] != '0' || !(num[1] == 'x' || num[1] == 'X')) {
    return PR_FALSE;
  }

  for (PRUint32 i = 2; i < num.Length(); i++) {
    if (!isxdigit(num[i])) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

static PRBool
IsOctal(const nsACString & num)
{
  if (num.Length() < 2) {
    return PR_FALSE;
  }

  if (num[0] != '0') {
    return PR_FALSE;
  }

  for (PRUint32 i = 1; i < num.Length(); i++) {
    if (!isdigit(num[i]) || num[i] == '8' || num[i] == '9') {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

nsUrlClassifierUtils::nsUrlClassifierUtils() : mEscapeCharmap(nsnull)
{
}

nsresult
nsUrlClassifierUtils::Init()
{
  
  mEscapeCharmap = new Charmap(0xffffffff, 0xfc009fff, 0xf8000001, 0xf8000001,
                               0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
  if (!mEscapeCharmap)
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsUrlClassifierUtils, nsIUrlClassifierUtils)




NS_IMETHODIMP
nsUrlClassifierUtils::GetKeyForURI(nsIURI * uri, nsACString & _retval)
{
  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(uri);
  if (!innerURI)
    innerURI = uri;

  nsCAutoString host;
  innerURI->GetAsciiHost(host);

  nsresult rv = CanonicalizeHostname(host, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString path;
  rv = innerURI->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt32 ref = path.FindChar('#');
  if (ref != kNotFound)
    path.SetLength(ref);

  ref = path.FindChar('?');
  if (ref != kNotFound)
    path.SetLength(ref);

  nsCAutoString temp;
  rv = CanonicalizePath(path, temp);
  NS_ENSURE_SUCCESS(rv, rv);

  _retval.Append(temp);

  return NS_OK;
}




nsresult
nsUrlClassifierUtils::CanonicalizeHostname(const nsACString & hostname,
                                           nsACString & _retval)
{
  nsCAutoString unescaped;
  if (!NS_UnescapeURL(PromiseFlatCString(hostname).get(),
                      PromiseFlatCString(hostname).Length(),
                      0, unescaped)) {
    unescaped.Assign(hostname);
  }

  nsCAutoString cleaned;
  CleanupHostname(unescaped, cleaned);

  nsCAutoString temp;
  ParseIPAddress(cleaned, temp);
  if (!temp.IsEmpty()) {
    cleaned.Assign(temp);
  }

  ToLowerCase(cleaned);
  SpecialEncode(cleaned, PR_FALSE, _retval);

  return NS_OK;
}


nsresult
nsUrlClassifierUtils::CanonicalizePath(const nsACString & path,
                                       nsACString & _retval)
{
  _retval.Truncate();

  nsCAutoString decodedPath(path);
  nsCAutoString temp;
  while (NS_UnescapeURL(decodedPath.get(), decodedPath.Length(), 0, temp)) {
    decodedPath.Assign(temp);
    temp.Truncate();
  }

  SpecialEncode(decodedPath, PR_TRUE, _retval);
  

  return NS_OK;
}

void
nsUrlClassifierUtils::CleanupHostname(const nsACString & hostname,
                                      nsACString & _retval)
{
  _retval.Truncate();

  const char* curChar = hostname.BeginReading();
  const char* end = hostname.EndReading();
  char lastChar = '\0';
  while (curChar != end) {
    unsigned char c = static_cast<unsigned char>(*curChar);
    if (c == '.' && (lastChar == '\0' || lastChar == '.')) {
      
    } else {
      _retval.Append(*curChar);
    }
    lastChar = c;
    ++curChar;
  }

  
  while (_retval[_retval.Length() - 1] == '.') {
    _retval.SetLength(_retval.Length() - 1);
  }
}

void
nsUrlClassifierUtils::ParseIPAddress(const nsACString & host,
                                     nsACString & _retval)
{
  _retval.Truncate();
  nsACString::const_iterator iter, end;
  host.BeginReading(iter);
  host.EndReading(end);

  if (host.Length() <= 15) {
    
    
    
    
    
    
    

    if (FindCharInReadable(' ', iter, end)) {
      end = iter;
    }
  }

  for (host.BeginReading(iter); iter != end; iter++) {
    if (!(isxdigit(*iter) || *iter == 'x' || *iter == 'X' || *iter == '.')) {
      
      return;
    }
  }

  host.BeginReading(iter);
  nsCStringArray parts;
  parts.ParseString(PromiseFlatCString(Substring(iter, end)).get(), ".");
  if (parts.Count() > 4) {
    return;
  }

  
  
  
  
  PRBool allowOctal = PR_TRUE;
  for (PRInt32 i = 0; i < parts.Count(); i++) {
    const nsCString& part = *parts[i];
    if (part[0] == '0') {
      for (PRUint32 j = 1; j < part.Length(); j++) {
        if (part[j] == 'x') {
          break;
        }
        if (part[j] == '8' || part[j] == '9') {
          allowOctal = PR_FALSE;
          break;
        }
      }
    }
  }

  for (PRInt32 i = 0; i < parts.Count(); i++) {
    nsCAutoString canonical;

    if (i == parts.Count() - 1) {
      CanonicalNum(*parts[i], 5 - parts.Count(), allowOctal, canonical);
    } else {
      CanonicalNum(*parts[i], 1, allowOctal, canonical);
    }

    if (canonical.IsEmpty()) {
      _retval.Truncate();
      return;
    }

    if (_retval.IsEmpty()) {
      _retval.Assign(canonical);
    } else {
      _retval.Append('.');
      _retval.Append(canonical);
    }
  }
  return;
}

void
nsUrlClassifierUtils::CanonicalNum(const nsACString& num,
                                   PRUint32 bytes,
                                   PRBool allowOctal,
                                   nsACString& _retval)
{
  _retval.Truncate();

  if (num.Length() < 1) {
    return;
  }

  PRUint32 val;
  if (allowOctal && IsOctal(num)) {
    if (PR_sscanf(PromiseFlatCString(num).get(), "%o", &val) != 1) {
      return;
    }
  } else if (IsDecimal(num)) {
    if (PR_sscanf(PromiseFlatCString(num).get(), "%u", &val) != 1) {
      return;
    }
  } else if (IsHex(num)) {
  if (PR_sscanf(PromiseFlatCString(num).get(), num[1] == 'X' ? "0X%x" : "0x%x",
                &val) != 1) {
      return;
    }
  } else {
    return;
  }

  while (bytes--) {
    char buf[20];
    PR_snprintf(buf, sizeof(buf), "%u", val & 0xff);
    if (_retval.IsEmpty()) {
      _retval.Assign(buf);
    } else {
      _retval = nsDependentCString(buf) + NS_LITERAL_CSTRING(".") + _retval;
    }
    val >>= 8;
  }
}




PRBool
nsUrlClassifierUtils::SpecialEncode(const nsACString & url,
                                    PRBool foldSlashes,
                                    nsACString & _retval)
{
  PRBool changed = PR_FALSE;
  const char* curChar = url.BeginReading();
  const char* end = url.EndReading();

  unsigned char lastChar = '\0';
  while (curChar != end) {
    unsigned char c = static_cast<unsigned char>(*curChar);
    if (ShouldURLEscape(c)) {
      
      
      if (c == 0)
        c = 1;

      _retval.Append('%');
      _retval.Append(int_to_hex_digit(c / 16));
      _retval.Append(int_to_hex_digit(c % 16));

      changed = PR_TRUE;
    } else if (foldSlashes && (c == '/' && lastChar == '/')) {
      
    } else {
      _retval.Append(*curChar);
    }
    lastChar = c;
    curChar++;
  }
  return changed;
}

PRBool
nsUrlClassifierUtils::ShouldURLEscape(const unsigned char c) const
{
  return c <= 32 || c == '%' || c >=127;
}
