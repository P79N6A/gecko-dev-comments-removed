





































#ifndef nsURLParsers_h__
#define nsURLParsers_h__

#include "nsIURLParser.h"





class nsBaseURLParser : public nsIURLParser
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIURLPARSER

    nsBaseURLParser() { }

protected:
    
    virtual void ParseAfterScheme(const char *spec, PRInt32 specLen,
                                  PRUint32 *authPos, PRInt32 *authLen,
                                  PRUint32 *pathPos, PRInt32 *pathLen) = 0;
};














class nsNoAuthURLParser : public nsBaseURLParser
{
public: 
#if defined(XP_WIN) || defined(XP_OS2)
    NS_IMETHOD ParseFilePath(const char *, PRInt32,
                             PRUint32 *, PRInt32 *,
                             PRUint32 *, PRInt32 *,
                             PRUint32 *, PRInt32 *);
#endif

    void ParseAfterScheme(const char *spec, PRInt32 specLen,
                          PRUint32 *authPos, PRInt32 *authLen,
                          PRUint32 *pathPos, PRInt32 *pathLen);
};










class nsAuthURLParser : public nsBaseURLParser
{
public: 
    NS_IMETHOD ParseAuthority(const char *auth, PRInt32 authLen,
                              PRUint32 *usernamePos, PRInt32 *usernameLen,
                              PRUint32 *passwordPos, PRInt32 *passwordLen,
                              PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                              PRInt32 *port);

    NS_IMETHOD ParseUserInfo(const char *userinfo, PRInt32 userinfoLen,
                             PRUint32 *usernamePos, PRInt32 *usernameLen,
                             PRUint32 *passwordPos, PRInt32 *passwordLen);

    NS_IMETHOD ParseServerInfo(const char *serverinfo, PRInt32 serverinfoLen,
                               PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                               PRInt32 *port);

    void ParseAfterScheme(const char *spec, PRInt32 specLen,
                          PRUint32 *authPos, PRInt32 *authLen,
                          PRUint32 *pathPos, PRInt32 *pathLen);
};











class nsStdURLParser : public nsAuthURLParser
{
public: 
    void ParseAfterScheme(const char *spec, PRInt32 specLen,
                          PRUint32 *authPos, PRInt32 *authLen,
                          PRUint32 *pathPos, PRInt32 *pathLen);
};

#endif 
