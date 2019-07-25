




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
public:
    NS_DECL_ISUPPORTS

#if defined(XP_WIN) || defined(XP_OS2)
    NS_IMETHOD ParseFilePath(const char *, int32_t,
                             uint32_t *, int32_t *,
                             uint32_t *, int32_t *,
                             uint32_t *, int32_t *);
#endif

    NS_IMETHOD ParseAuthority(const char *auth, int32_t authLen,
                              uint32_t *usernamePos, int32_t *usernameLen,
                              uint32_t *passwordPos, int32_t *passwordLen,
                              uint32_t *hostnamePos, int32_t *hostnameLen,
                              int32_t *port);

    void ParseAfterScheme(const char *spec, int32_t specLen,
                          uint32_t *authPos, int32_t *authLen,
                          uint32_t *pathPos, int32_t *pathLen);
};










class nsAuthURLParser : public nsBaseURLParser
{
public:
    NS_DECL_ISUPPORTS

    virtual ~nsAuthURLParser() {}

    NS_IMETHOD ParseAuthority(const char *auth, int32_t authLen,
                              uint32_t *usernamePos, int32_t *usernameLen,
                              uint32_t *passwordPos, int32_t *passwordLen,
                              uint32_t *hostnamePos, int32_t *hostnameLen,
                              int32_t *port);

    NS_IMETHOD ParseUserInfo(const char *userinfo, int32_t userinfoLen,
                             uint32_t *usernamePos, int32_t *usernameLen,
                             uint32_t *passwordPos, int32_t *passwordLen);

    NS_IMETHOD ParseServerInfo(const char *serverinfo, int32_t serverinfoLen,
                               uint32_t *hostnamePos, int32_t *hostnameLen,
                               int32_t *port);

    void ParseAfterScheme(const char *spec, int32_t specLen,
                          uint32_t *authPos, int32_t *authLen,
                          uint32_t *pathPos, int32_t *pathLen);
};











class nsStdURLParser : public nsAuthURLParser
{
public: 
    void ParseAfterScheme(const char *spec, int32_t specLen,
                          uint32_t *authPos, int32_t *authLen,
                          uint32_t *pathPos, int32_t *pathLen);
};

#endif 
