




#ifndef nsURLParsers_h__
#define nsURLParsers_h__

#include "nsIURLParser.h"
#include "mozilla/Attributes.h"





class nsBaseURLParser : public nsIURLParser
{
public:
    NS_DECL_NSIURLPARSER

    nsBaseURLParser() { }

protected:
    
    virtual void ParseAfterScheme(const char *spec, int32_t specLen,
                                  uint32_t *authPos, int32_t *authLen,
                                  uint32_t *pathPos, int32_t *pathLen) = 0;
};














class nsNoAuthURLParser MOZ_FINAL : public nsBaseURLParser
{
    ~nsNoAuthURLParser() {}

public:
    NS_DECL_THREADSAFE_ISUPPORTS

#if defined(XP_WIN)
    NS_IMETHOD ParseFilePath(const char *, int32_t,
                             uint32_t *, int32_t *,
                             uint32_t *, int32_t *,
                             uint32_t *, int32_t *) MOZ_OVERRIDE;
#endif

    NS_IMETHOD ParseAuthority(const char *auth, int32_t authLen,
                              uint32_t *usernamePos, int32_t *usernameLen,
                              uint32_t *passwordPos, int32_t *passwordLen,
                              uint32_t *hostnamePos, int32_t *hostnameLen,
                              int32_t *port) MOZ_OVERRIDE;

    void ParseAfterScheme(const char *spec, int32_t specLen,
                          uint32_t *authPos, int32_t *authLen,
                          uint32_t *pathPos, int32_t *pathLen) MOZ_OVERRIDE;
};










class nsAuthURLParser : public nsBaseURLParser
{
protected:
    virtual ~nsAuthURLParser() {}

public:
    NS_DECL_THREADSAFE_ISUPPORTS

    NS_IMETHOD ParseAuthority(const char *auth, int32_t authLen,
                              uint32_t *usernamePos, int32_t *usernameLen,
                              uint32_t *passwordPos, int32_t *passwordLen,
                              uint32_t *hostnamePos, int32_t *hostnameLen,
                              int32_t *port) MOZ_OVERRIDE;

    NS_IMETHOD ParseUserInfo(const char *userinfo, int32_t userinfoLen,
                             uint32_t *usernamePos, int32_t *usernameLen,
                             uint32_t *passwordPos, int32_t *passwordLen) MOZ_OVERRIDE;

    NS_IMETHOD ParseServerInfo(const char *serverinfo, int32_t serverinfoLen,
                               uint32_t *hostnamePos, int32_t *hostnameLen,
                               int32_t *port) MOZ_OVERRIDE;

    void ParseAfterScheme(const char *spec, int32_t specLen,
                          uint32_t *authPos, int32_t *authLen,
                          uint32_t *pathPos, int32_t *pathLen) MOZ_OVERRIDE;
};











class nsStdURLParser : public nsAuthURLParser
{
    virtual ~nsStdURLParser() {}

public: 
    void ParseAfterScheme(const char *spec, int32_t specLen,
                          uint32_t *authPos, int32_t *authLen,
                          uint32_t *pathPos, int32_t *pathLen);
};

#endif 
