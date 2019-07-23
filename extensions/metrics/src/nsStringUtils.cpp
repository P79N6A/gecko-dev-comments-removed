





































#include "nsStringUtils.h"
#include "prprf.h"

void AppendInt(nsAString &str, PRInt64 val)
{
  char buf[32];
  PR_snprintf(buf, sizeof(buf), "%lld", val);
  str.Append(NS_ConvertASCIItoUTF16(buf));
}

void AppendInt(nsAString &str, PRInt32 val)
{
  char buf[32];
  PR_snprintf(buf, sizeof(buf), "%ld", val);
  str.Append(NS_ConvertASCIItoUTF16(buf));
}

PRInt32 FindChar(const nsAString &str, PRUnichar c)
{
  const PRUnichar *start;
  PRUint32 len = NS_StringGetData(str, &start);
  const PRUnichar *iter = start, *end = start + len;
  for (; iter != end; ++iter) {
    if (*iter == c)
      return iter - start;
  }
  return -1;
}

PRInt32 RFindChar(const nsAString &str, PRUnichar c)
{
  const PRUnichar *start;
  PRUint32 len = NS_StringGetData(str, &start);
  const PRUnichar *end = start + len, *iter = end - 1;
  for (; iter >= start; --iter) {
    if (*iter == c)
      return iter - start;
  }
  return -1;
}
