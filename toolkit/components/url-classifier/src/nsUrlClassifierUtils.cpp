



































#include "nsEscape.h"
#include "nsString.h"
#include "nsUrlClassifierUtils.h"

static char int_to_hex_digit(PRInt32 i)
{
  NS_ASSERTION((i >= 0) && (i <= 15), "int too big in int_to_hex_digit");
  return static_cast<char>(((i < 10) ? (i + '0') : ((i - 10) + 'A')));
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
nsUrlClassifierUtils::CanonicalizeURL(const nsACString & url, nsACString & _retval)
{
  nsCAutoString decodedUrl(url);
  nsCAutoString temp;
  while (NS_UnescapeURL(decodedUrl.get(), decodedUrl.Length(), 0, temp)) {
    decodedUrl.Assign(temp);
    temp.Truncate();
  }
  SpecialEncode(decodedUrl, _retval);
  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierUtils::EscapeHostname(const nsACString & hostname,
                                     nsACString & _retval)
{
  const char* curChar = hostname.BeginReading();
  const char* end = hostname.EndReading();
  while (curChar != end) {
    unsigned char c = static_cast<unsigned char>(*curChar);
    if (mEscapeCharmap->Contains(c)) {
      _retval.Append('%');
      _retval.Append(int_to_hex_digit(c / 16));
      _retval.Append(int_to_hex_digit(c % 16));
    } else {
      _retval.Append(*curChar);
    }
    ++curChar;
  }
  
  return NS_OK;
}







PRBool
nsUrlClassifierUtils::SpecialEncode(const nsACString & url, nsACString & _retval)
{
  PRBool changed = PR_FALSE;
  const char* curChar = url.BeginReading();
  const char* end = url.EndReading();

  while (curChar != end) {
    unsigned char c = static_cast<unsigned char>(*curChar);
    if (ShouldURLEscape(c)) {
      
      
      if (c == 0)
        c = 1;

      _retval.Append('%');
      _retval.Append(int_to_hex_digit(c / 16));
      _retval.Append(int_to_hex_digit(c % 16));

      changed = PR_TRUE;
    } else {
      _retval.Append(*curChar);
    }
    curChar++;
  }
  return changed;
}

PRBool
nsUrlClassifierUtils::ShouldURLEscape(const unsigned char c) const
{
  return c <= 32 || c == '%' || c >=127;
}
