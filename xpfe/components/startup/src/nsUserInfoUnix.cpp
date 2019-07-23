





































#include "nsUserInfo.h"
#include "nsCRT.h"

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"


#if defined(NO_PW_GECOS)
#define PW_GECOS pw_name
#else
#define PW_GECOS pw_gecos
#endif

nsUserInfo::nsUserInfo()
{
}

nsUserInfo::~nsUserInfo()
{
}

NS_IMPL_ISUPPORTS1(nsUserInfo,nsIUserInfo)

NS_IMETHODIMP
nsUserInfo::GetFullname(PRUnichar **aFullname)
{
    struct passwd *pw = nsnull;

    pw = getpwuid (geteuid());

    if (!pw || !pw->PW_GECOS) return NS_ERROR_FAILURE;

#ifdef DEBUG_sspitzer
    printf("fullname = %s\n", pw->PW_GECOS);
#endif

    nsCAutoString fullname(pw->PW_GECOS);

    
    
    
    
    
    
    PRInt32 index;
    if ((index = fullname.Find(",")) != kNotFound)
        fullname.Truncate(index);

    
    if (pw->pw_name) {
        nsCAutoString username(pw->pw_name);
        if (!username.IsEmpty() && nsCRT::IsLower(username.CharAt(0)))
            username.SetCharAt(nsCRT::ToUpper(username.CharAt(0)), 0);
            
        fullname.ReplaceSubstring("&", username.get());
    }

    *aFullname = ToNewUnicode(fullname);

    if (*aFullname)
        return NS_OK;

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsUserInfo::GetUsername(char * *aUsername)
{
    struct passwd *pw = nsnull;

    
    pw = getpwuid(geteuid());

    if (!pw || !pw->pw_name) return NS_ERROR_FAILURE;

#ifdef DEBUG_sspitzer
    printf("username = %s\n", pw->pw_name);
#endif

    *aUsername = nsCRT::strdup(pw->pw_name);

    return NS_OK;
}

NS_IMETHODIMP 
nsUserInfo::GetDomain(char * *aDomain)
{
    nsresult rv = NS_ERROR_FAILURE;

    struct utsname buf;
    char *domainname = nsnull;

    
    if (uname(&buf)) { 
        return rv;
    }

#if defined(HAVE_UNAME_DOMAINNAME_FIELD)
    domainname = buf.domainname;
#elif defined(HAVE_UNAME_US_DOMAINNAME_FIELD)
    domainname = buf.__domainname;
#endif

    if (domainname && domainname[0]) {   
        *aDomain = nsCRT::strdup(domainname);
        rv = NS_OK;
    }
    else {
        
        
        
        if (buf.nodename && buf.nodename[0]) {
            
            char *pos = strchr(buf.nodename,'.');
            if (pos) {
                *aDomain = nsCRT::strdup(pos+1);
                rv = NS_OK;
            }
        }
    }
    
    return rv;
}

NS_IMETHODIMP 
nsUserInfo::GetEmailAddress(char * *aEmailAddress)
{
    

    nsresult rv;

    nsCAutoString emailAddress;
    nsXPIDLCString username;
    nsXPIDLCString domain;

    rv = GetUsername(getter_Copies(username));
    if (NS_FAILED(rv)) return rv;

    rv = GetDomain(getter_Copies(domain));
    if (NS_FAILED(rv)) return rv;

    if (!username.IsEmpty() && !domain.IsEmpty()) {
        emailAddress = (const char *)username;
        emailAddress += "@";
        emailAddress += (const char *)domain;
    }
    else {
        return NS_ERROR_FAILURE;
    }

    *aEmailAddress = ToNewCString(emailAddress);
    
    return NS_OK;
}

