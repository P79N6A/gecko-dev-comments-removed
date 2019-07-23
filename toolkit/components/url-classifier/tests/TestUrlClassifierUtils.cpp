



































#include <stdio.h>
#include <ctype.h>
#include "nsEscape.h"
#include "nsString.h"
#include "nsUrlClassifierUtils.h"

static int gTotalTests = 0;
static int gPassedTests = 0;

static char int_to_hex_digit(PRInt32 i) {
  NS_ASSERTION((i >= 0) && (i <= 15), "int too big in int_to_hex_digit");
  return NS_STATIC_CAST(char, ((i < 10) ? (i + '0') : ((i - 10) + 'A')));
}

static void CheckEquals(nsCString & expected, nsCString & actual)
{
  if (!(expected).Equals((actual))) {
    fprintf(stderr, "FAILED: expected |%s| but was |%s|\n", (expected).get(),
            (actual).get());
  } else {
    gPassedTests++;
  }
  gTotalTests++;
}

void TestUnescapeHelper(const char* in, const char* expected)
{
  nsCString out, strIn(in), strExp(expected);
  nsUrlClassifierUtils utils;
  
  NS_UnescapeURL(strIn.get(), strIn.Length(), esc_AlwaysCopy, out);
  CheckEquals(strExp, out);
}


void TestUnescape()
{
  
  TestUnescapeHelper("\0", "\0");

  
  nsCString allCharsEncoded, allCharsEncodedLowercase, allCharsAsString;
  for (PRInt32 i = 1; i < 256; ++i) {
    allCharsEncoded.Append('%');
    allCharsEncoded.Append(int_to_hex_digit(i / 16));
    allCharsEncoded.Append((int_to_hex_digit(i % 16)));
    
    allCharsEncodedLowercase.Append('%');
    allCharsEncodedLowercase.Append(tolower(int_to_hex_digit(i / 16)));
    allCharsEncodedLowercase.Append(tolower(int_to_hex_digit(i % 16)));
    
    allCharsAsString.Append(NS_STATIC_CAST(char, i));
  }
  
  nsUrlClassifierUtils utils;
  nsCString out;
  NS_UnescapeURL(allCharsEncoded.get(), allCharsEncoded.Length(), esc_AlwaysCopy, out);
  CheckEquals(allCharsAsString, out);
  
  out.Truncate();
  NS_UnescapeURL(allCharsEncodedLowercase.get(), allCharsEncodedLowercase.Length(), esc_AlwaysCopy, out);
  CheckEquals(allCharsAsString, out);

  
  TestUnescapeHelper("%", "%");
  TestUnescapeHelper("%xx", "%xx");
  TestUnescapeHelper("%%", "%%");
  TestUnescapeHelper("%%%", "%%%");
  TestUnescapeHelper("%%%%", "%%%%");
  TestUnescapeHelper("%1", "%1");
  TestUnescapeHelper("%1z", "%1z");
  TestUnescapeHelper("a%1z", "a%1z");
  TestUnescapeHelper("abc%d%e%fg%hij%klmno%", "abc%d%e%fg%hij%klmno%");

  
  TestUnescapeHelper("%25", "%");
  TestUnescapeHelper("%25%32%35", "%25");
}

void TestEncodeHelper(const char* in, const char* expected)
{
  nsCString out, strIn(in), strExp(expected);
  nsUrlClassifierUtils utils;
  
  utils.SpecialEncode(strIn, out);
  CheckEquals(strExp, out);
}

void TestEnc()
{
  
  TestEncodeHelper("", "");

  
  nsCString noenc;
  for (PRInt32 i = 33; i < 127; i++) {
    if (i != 37) {                      
      noenc.Append(NS_STATIC_CAST(char, i));
    }
  }
  nsUrlClassifierUtils utils;
  nsCString out;
  utils.SpecialEncode(noenc, out);
  CheckEquals(noenc, out);

  
  nsCString yesAsString, yesExpectedString;
  for (PRInt32 i = 1; i < 256; i++) {
    if (i < 33 || i == 37 || i > 126) {
      yesAsString.Append(NS_STATIC_CAST(char, i));
      yesExpectedString.Append('%');
      yesExpectedString.Append(int_to_hex_digit(i / 16));
      yesExpectedString.Append(int_to_hex_digit(i % 16));
    }
  }
  
  out.Truncate();
  utils.SpecialEncode(yesAsString, out);
  CheckEquals(yesExpectedString, out);
}

void TestCanonicalizeHelper(const char* in, const char* expected)
{
  nsCString out, strIn(in), strExp(expected);
  nsUrlClassifierUtils utils;
  
  utils.CanonicalizeURL(strIn, out);
  CheckEquals(strExp, out);
}

void TestCanonicalize()
{
  
  TestCanonicalizeHelper("%25", "%25");
  TestCanonicalizeHelper("%25%32%35", "%25");
  TestCanonicalizeHelper("asdf%25%32%35asd", "asdf%25asd");
  TestCanonicalizeHelper("%%%25%32%35asd%%", "%25%25%25asd%25%25");
  TestCanonicalizeHelper("%25%32%35%25%32%35%25%32%35", "%25%25%25");
  TestCanonicalizeHelper("%25", "%25");
  TestCanonicalizeHelper("%257Ea%2521b%2540c%2523d%2524e%25f%255E00%252611%252A22%252833%252944_55%252B",
      "~a!b@c#d$e%25f^00&11*22(33)44_55+");

  TestCanonicalizeHelper("", "");
  TestCanonicalizeHelper("http://www.google.com", "http://www.google.com");
  TestCanonicalizeHelper("http://%31%36%38%2e%31%38%38%2e%39%39%2e%32%36/%2E%73%65%63%75%72%65/%77%77%77%2E%65%62%61%79%2E%63%6F%6D/",
                         "http://168.188.99.26/.secure/www.ebay.com/");
  TestCanonicalizeHelper("http://195.127.0.11/uploads/%20%20%20%20/.verify/.eBaysecure=updateuserdataxplimnbqmn-xplmvalidateinfoswqpcmlx=hgplmcx/",
                         "http://195.127.0.11/uploads/%20%20%20%20/.verify/.eBaysecure=updateuserdataxplimnbqmn-xplmvalidateinfoswqpcmlx=hgplmcx/");
}

int main(int argc, char **argv)
{
  TestUnescape();
  TestEnc();
  TestCanonicalize();
  printf("%d of %d tests passed\n", gPassedTests, gTotalTests);
  
  return (gPassedTests != gTotalTests);
}
