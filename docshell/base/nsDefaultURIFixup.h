






































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
    bool PossiblyByteExpandedFileName(const nsAString& aIn);
    bool PossiblyHostPortUrl(const nsACString& aUrl);
    bool MakeAlternateURI(nsIURI *aURI);
    bool IsLikelyFTP(const nsCString& aHostSpec);
    const char * GetFileSystemCharset();
    const char * GetCharsetForUrlBar();

    nsCString mFsCharset;
};

#endif
