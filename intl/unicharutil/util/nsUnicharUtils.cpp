







































#include "nsUnicharUtils.h"
#include "nsUnicharUtilCIID.h"

#include "nsCRT.h"
#include "nsICaseConversion.h"
#include "nsServiceManagerUtils.h"
#include "nsXPCOMStrings.h"

#include <ctype.h>

static nsICaseConversion* gCaseConv = nsnull;

nsICaseConversion*
NS_GetCaseConversion()
{
  if (!gCaseConv) {
    nsresult rv = CallGetService(NS_UNICHARUTIL_CONTRACTID, &gCaseConv);
    if (NS_FAILED(rv)) {
      NS_ERROR("Failed to get the case conversion service!");
      gCaseConv = nsnull;
    }
  }
  return gCaseConv;
}

void
ToLowerCase(nsAString& aString)
{
  nsICaseConversion* caseConv = NS_GetCaseConversion();
  if (caseConv) {
    PRUnichar *buf = aString.BeginWriting();
    caseConv->ToLower(buf, buf, aString.Length());
  }
  else
    NS_WARNING("No case converter: no conversion done");
}

void
ToLowerCase(const nsAString& aSource,
            nsAString& aDest)
{
  const PRUnichar *in;
  PRUint32 len = NS_StringGetData(aSource, &in);

  PRUnichar *out;
  NS_StringGetMutableData(aDest, len, &out);

  nsICaseConversion* caseConv = NS_GetCaseConversion();
  if (out && caseConv)
    caseConv->ToLower(in, out, len);
  else {
    NS_WARNING("No case converter: only copying");
    aDest.Assign(aSource);
  }
}

void
ToUpperCase(nsAString& aString)
{
  nsICaseConversion* caseConv = NS_GetCaseConversion();
  if (caseConv) {
    PRUnichar *buf = aString.BeginWriting();
    caseConv->ToUpper(buf, buf, aString.Length());
  }
  else
    NS_WARNING("No case converter: no conversion done");
}

void
ToUpperCase(const nsAString& aSource,
            nsAString& aDest)
{
  const PRUnichar *in;
  PRUint32 len = NS_StringGetData(aSource, &in);

  PRUnichar *out;
  NS_StringGetMutableData(aDest, len, &out);

  nsICaseConversion* caseConv = NS_GetCaseConversion();
  if (out && caseConv)
    caseConv->ToUpper(in, out, len);
  else {
    NS_WARNING("No case converter: only copying");
    aDest.Assign(aSource);
  }
}

#ifdef MOZILLA_INTERNAL_API

PRInt32
nsCaseInsensitiveStringComparator::operator()(const PRUnichar* lhs,
                                              const PRUnichar* rhs,
                                              PRUint32 aLength) const
{
  PRInt32 result;
  nsICaseConversion* caseConv = NS_GetCaseConversion();
  if (caseConv)
    caseConv->CaseInsensitiveCompare(lhs, rhs, aLength, &result);
  else {
    NS_WARNING("No case converter: using default");
    nsDefaultStringComparator comparator;
    result = comparator(lhs, rhs, aLength);
  }
  return result;
}

PRInt32
nsCaseInsensitiveStringComparator::operator()(PRUnichar lhs,
                                              PRUnichar rhs) const
{
  
  if (lhs == rhs)
    return 0;
  
  nsICaseConversion* caseConv = NS_GetCaseConversion();
  if (caseConv) {
    caseConv->ToLower(lhs, &lhs);
    caseConv->ToLower(rhs, &rhs);
  }
  else {
    if (lhs < 256)
      lhs = tolower(char(lhs));
    if (rhs < 256)
      rhs = tolower(char(rhs));
    NS_WARNING("No case converter: no conversion done");
  }
  
  if (lhs == rhs)
    return 0;
  else if (lhs < rhs)
    return -1;
  else
    return 1;
}

#else 

PRInt32
CaseInsensitiveCompare(const PRUnichar *a,
                       const PRUnichar *b,
                       PRUint32 len)
{
  nsICaseConversion* caseConv = NS_GetCaseConversion();
  if (!caseConv)
    return NS_strcmp(a, b);

  PRInt32 result;
  caseConv->CaseInsensitiveCompare(a, b, len, &result);
  return result;
}

#endif 

PRUnichar
ToLowerCase(PRUnichar aChar)
{
  PRUnichar result;
  nsICaseConversion* caseConv = NS_GetCaseConversion();
  if (caseConv)
    caseConv->ToLower(aChar, &result);
  else {
    NS_WARNING("No case converter: no conversion done");
    if (aChar < 256)
      result = tolower(char(aChar));
    else
      result = aChar;
  }
  return result;
}

PRUnichar
ToUpperCase(PRUnichar aChar)
{
  PRUnichar result;
  nsICaseConversion* caseConv = NS_GetCaseConversion();
  if (caseConv)
    caseConv->ToUpper(aChar, &result);
  else {
    NS_WARNING("No case converter: no conversion done");
    if (aChar < 256)
      result = toupper(char(aChar));
    else
      result = aChar;
  }
  return result;
}
