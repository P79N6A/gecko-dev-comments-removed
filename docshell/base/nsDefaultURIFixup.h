





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
  
  nsresult FileURIFixup(const nsACString& aStringURI, nsIURI** aURI);
  nsresult ConvertFileToStringURI(const nsACString& aIn, nsCString& aResult);
  nsresult FixupURIProtocol(const nsACString& aIn,
                            nsDefaultURIFixupInfo* aFixupInfo,
                            nsIURI** aURI);
  nsresult KeywordURIFixup(const nsACString& aStringURI,
                           nsDefaultURIFixupInfo* aFixupInfo,
                           nsIInputStream** aPostData);
  nsresult TryKeywordFixupForURIInfo(const nsACString& aStringURI,
                                     nsDefaultURIFixupInfo* aFixupInfo,
                                     nsIInputStream** aPostData);
  bool PossiblyByteExpandedFileName(const nsAString& aIn);
  bool PossiblyHostPortUrl(const nsACString& aUrl);
  bool MakeAlternateURI(nsIURI* aURI);
  bool IsLikelyFTP(const nsCString& aHostSpec);
  bool IsDomainWhitelisted(const nsAutoCString aAsciiHost,
                           const uint32_t aDotLoc);
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
  bool mFixupChangedProtocol;
  bool mFixupCreatedAlternateURI;
  nsString mKeywordProviderName;
  nsString mKeywordAsSent;
  nsAutoCString mOriginalInput;
};
#endif
