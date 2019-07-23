





































#include "nsAOLCiter.h"

#include "nsWrapUtils.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"




nsAOLCiter::nsAOLCiter()
{
}

nsAOLCiter::~nsAOLCiter()
{
}

NS_IMPL_ISUPPORTS1(nsAOLCiter, nsICiter)

NS_IMETHODIMP
nsAOLCiter::GetCiteString(const nsAString& aInString, nsAString& aOutString)
{
  aOutString.AssignLiteral("\n\n>> ");
  aOutString += aInString;

  
  PRUnichar newline ('\n');
  if (aOutString.Last() == newline)
  {
    aOutString.SetLength(aOutString.Length() - 1);
  }

  aOutString.AppendLiteral(" <<\n");

  return NS_OK;
}

NS_IMETHODIMP
nsAOLCiter::StripCites(const nsAString& aInString, nsAString& aOutString)
{
  
  nsAutoString tOutputString;
  nsReadingIterator <PRUnichar> iter, enditer;
  aInString.BeginReading(iter);
  aInString.EndReading(enditer);
  if (StringBeginsWith(aInString, NS_LITERAL_STRING(">>")))
  {
    iter.advance(2);
    while (nsCRT::IsAsciiSpace(*iter))
      ++iter;
    AppendUnicodeTo(iter, enditer, tOutputString);
  }
  else
    CopyUnicodeTo(iter, enditer, tOutputString);

  
  tOutputString.Trim("<", PR_FALSE, PR_TRUE, PR_FALSE);
  aOutString.Assign(tOutputString);
  return NS_OK;
}

NS_IMETHODIMP
nsAOLCiter::Rewrap(const nsAString& aInString,
                   PRUint32 aWrapCol, PRUint32 aFirstLineOffset,
                   PRBool aRespectNewlines,
                   nsAString& aOutString)
{
  nsString citeString;
  return nsWrapUtils::Rewrap(aInString, aWrapCol, aFirstLineOffset,
                             aRespectNewlines, citeString,
                             aOutString);
}

