




















































#include "nsCOMPtr.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsRDFCID.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsUnicharUtils.h"
#include "prtime.h"
#include "rdfutil.h"



nsresult
rdf_MakeRelativeRef(const nsCSubstring& aBaseURI, nsCString& aURI)
{
    
    
    
    PRUint32 prefixLen = aBaseURI.Length();
    if (prefixLen != 0 && StringBeginsWith(aURI, aBaseURI)) {
        if (prefixLen < aURI.Length() && aURI.CharAt(prefixLen) == '/')
            ++prefixLen; 

        aURI.Cut(0, prefixLen);
    }

    return NS_OK;
}

void
rdf_FormatDate(PRTime aTime, nsACString &aResult)
{
    
    
    
    PRExplodedTime t;
    PR_ExplodeTime(aTime, PR_LocalTimeParameters, &t);

    char buf[256];
    PR_FormatTimeUSEnglish(buf, sizeof buf, "%a %b %d %H:%M:%S %Z %Y", &t);
    aResult.Append(buf);

    
    aResult.Append(" +");
    PRInt32 usec = t.tm_usec;
    for (PRInt32 digit = 100000; digit > 1; digit /= 10) {
        aResult.Append(char('0' + (usec / digit)));
        usec %= digit;
    }
    aResult.Append(char('0' + usec));
}

PRTime
rdf_ParseDate(const nsACString &aTime)
{
    PRTime t;
    PR_ParseTimeString(PromiseFlatCString(aTime).get(), PR_TRUE, &t);

    PRInt32 usec = 0;

    nsACString::const_iterator begin, digit, end;
    aTime.BeginReading(begin);
    aTime.EndReading(end);

    
    
    digit = end;
    while (--digit != begin && *digit != '+') {
        if (*digit < '0' || *digit > '9')
            break;
    }

    if (digit != begin && *digit == '+') {
        
        
        while (++digit != end) {
            usec *= 10;
            usec += *digit - '0';
        }

        PRTime temp;
        LL_I2L(temp, usec);
        LL_ADD(t, t, temp);
    }

    return t;
}
