





#ifndef NSDEFAULTURIFIXUP_H
#define NSDEFAULTURIFIXUP_H

#include "nsIURIFixup.h"

class nsDefaultURIFixupInfo;


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
    nsresult FixupURIProtocol(const nsACString& aIn,
                              nsDefaultURIFixupInfo* aFixupInfo,
                              nsIURI** aURI);
    void KeywordURIFixup(const nsACString &aStringURI,
                         nsDefaultURIFixupInfo* aFixupInfo,
                         nsIInputStream** aPostData);
    bool PossiblyByteExpandedFileName(const nsAString& aIn);
    bool PossiblyHostPortUrl(const nsACString& aUrl);
    bool MakeAlternateURI(nsIURI *aURI);
    bool IsLikelyFTP(const nsCString& aHostSpec);
};

class nsDefaultURIFixupInfo : public nsIURIFixupInfo
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIURIFIXUPINFO

    explicit nsDefaultURIFixupInfo(const nsACString& aOriginalInput);

    friend class nsDefaultURIFixup;

protected:
    virtual ~nsDefaultURIFixupInfo();

private:
    nsCOMPtr<nsISupports> mConsumer;
    nsCOMPtr<nsIURI> mPreferredURI;
    nsCOMPtr<nsIURI> mFixedURI;
    bool mFixupUsedKeyword;
    bool mFixupChangedProtocol;
    bool mFixupCreatedAlternateURI;
    nsAutoCString mOriginalInput;
};
#endif
