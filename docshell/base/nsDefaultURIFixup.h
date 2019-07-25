






































#ifndef NSDEFAULTURIFIXUP_H
#define NSDEFAULTURIFIXUP_H

#include "nsIURIFixup.h"

#include "nsCOMPtr.h"

#include "nsCDefaultURIFixup.h"


class nsDefaultURIFixup : public nsIURIFixup
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIURIFIXUP

    nsDefaultURIFixup();

protected:
    virtual ~nsDefaultURIFixup();

private:
    
    nsresult FileURIFixup(const nsACString &aStringURI, nsIURI** aURI);
    nsresult ConvertFileToStringURI(const nsACString& aIn, nsCString& aOut);
    nsresult KeywordURIFixup(const nsACString &aStringURI, nsIURI** aURI);
    PRBool PossiblyByteExpandedFileName(const nsAString& aIn);
    PRBool PossiblyHostPortUrl(const nsACString& aUrl);
    PRBool MakeAlternateURI(nsIURI *aURI);
    PRBool IsLikelyFTP(const nsCString& aHostSpec);
    const char * GetFileSystemCharset();
    const char * GetCharsetForUrlBar();

    nsCString mFsCharset;
};

#endif
