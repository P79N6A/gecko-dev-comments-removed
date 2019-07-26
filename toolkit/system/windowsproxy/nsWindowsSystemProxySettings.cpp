




#include <windows.h>
#include <ras.h>
#include <wininet.h>

#include "mozilla/Attributes.h"
#include "mozilla/Util.h"
#include "nsISystemProxySettings.h"
#include "nsIServiceManager.h"
#include "mozilla/ModuleUtils.h"
#include "nsPrintfCString.h"
#include "nsNetUtil.h"
#include "nsISupportsPrimitives.h"
#include "nsIURI.h"

class nsWindowsSystemProxySettings MOZ_FINAL : public nsISystemProxySettings
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISYSTEMPROXYSETTINGS

    nsWindowsSystemProxySettings() {};
    nsresult Init();

private:
    ~nsWindowsSystemProxySettings() {};

    bool MatchOverride(const nsACString& aHost);
    bool PatternMatch(const nsACString& aHost, const nsACString& aOverride);
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsWindowsSystemProxySettings, nsISystemProxySettings)

NS_IMETHODIMP
nsWindowsSystemProxySettings::GetMainThreadOnly(bool *aMainThreadOnly)
{
  *aMainThreadOnly = false;
  return NS_OK;
}


nsresult
nsWindowsSystemProxySettings::Init()
{
    return NS_OK;
}

static void SetProxyResult(const char* aType, const nsACString& aHostPort,
                           nsACString& aResult)
{
    aResult.AssignASCII(aType);
    aResult.Append(' ');
    aResult.Append(aHostPort);
}

static void SetProxyResultDirect(nsACString& aResult)
{
    
    aResult.AssignASCII("DIRECT");
}

static nsresult ReadInternetOption(uint32_t aOption, uint32_t& aFlags,
                                   nsAString& aValue)
{
    DWORD connFlags = 0;
    WCHAR connName[RAS_MaxEntryName + 1];
    MOZ_SEH_TRY {
        InternetGetConnectedStateExW(&connFlags, connName,
                                     mozilla::ArrayLength(connName), 0);
    } MOZ_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        return NS_ERROR_FAILURE;
    }

    INTERNET_PER_CONN_OPTIONW options[2];
    options[0].dwOption = INTERNET_PER_CONN_FLAGS_UI;
    options[1].dwOption = aOption;

    INTERNET_PER_CONN_OPTION_LISTW list;
    list.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LISTW);
    list.pszConnection = connFlags & INTERNET_CONNECTION_MODEM ?
                         connName : NULL;
    list.dwOptionCount = mozilla::ArrayLength(options);
    list.dwOptionError = 0;
    list.pOptions = options;

    unsigned long size = sizeof(INTERNET_PER_CONN_OPTION_LISTW);
    if (!InternetQueryOptionW(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION,
                              &list, &size)) {
        if (GetLastError() != ERROR_INVALID_PARAMETER) {
            return NS_ERROR_FAILURE;
        }
        options[0].dwOption = INTERNET_PER_CONN_FLAGS;
        size = sizeof(INTERNET_PER_CONN_OPTION_LISTW);
        MOZ_SEH_TRY {
            if (!InternetQueryOptionW(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION,
                                      &list, &size)) {
                return NS_ERROR_FAILURE;
            }
        } MOZ_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            return NS_ERROR_FAILURE;
        }
    }

    aFlags = options[0].Value.dwValue;
    aValue.Assign(options[1].Value.pszValue);
    GlobalFree(options[1].Value.pszValue);

    return NS_OK;
}

bool
nsWindowsSystemProxySettings::MatchOverride(const nsACString& aHost)
{
    nsresult rv;
    uint32_t flags = 0;
    nsAutoString buf;

    rv = ReadInternetOption(INTERNET_PER_CONN_PROXY_BYPASS, flags, buf);
    if (NS_FAILED(rv))
        return false;

    NS_ConvertUTF16toUTF8 cbuf(buf);

    nsAutoCString host(aHost);
    int32_t start = 0;
    int32_t end = cbuf.Length();

    
    
    
    
    
    
    
    while (true) {
        int32_t delimiter = cbuf.FindCharInSet(" ;", start);
        if (delimiter == -1)
            delimiter = end;

        if (delimiter != start) {
            const nsAutoCString override(Substring(cbuf, start,
                                                   delimiter - start));
            if (override.EqualsLiteral("<local>")) {
                
                if (host.EqualsLiteral("localhost") ||
                    host.EqualsLiteral("127.0.0.1"))
                    return true;
            } else if (PatternMatch(host, override)) {
                return true;
            }
        }

        if (delimiter == end)
            break;
        start = ++delimiter;
    }

    return false;
}

bool
nsWindowsSystemProxySettings::PatternMatch(const nsACString& aHost,
                                           const nsACString& aOverride)
{
    nsAutoCString host(aHost);
    nsAutoCString override(aOverride);
    int32_t overrideLength = override.Length();
    int32_t tokenStart = 0;
    int32_t offset = 0;
    bool star = false;

    while (tokenStart < overrideLength) {
        int32_t tokenEnd = override.FindChar('*', tokenStart);
        if (tokenEnd == tokenStart) {
            star = true;
            tokenStart++;
            
            
            if (override.FindChar('.', tokenStart) == tokenStart)
                tokenStart++;
        } else {
            if (tokenEnd == -1)
                tokenEnd = overrideLength;
            nsAutoCString token(Substring(override, tokenStart,
                                          tokenEnd - tokenStart));
            offset = host.Find(token, offset);
            if (offset == -1 || (!star && offset))
                return false;
            star = false;
            tokenStart = tokenEnd;
            offset += token.Length();
        }
    }

    return (star || (offset == host.Length()));
}

nsresult
nsWindowsSystemProxySettings::GetPACURI(nsACString& aResult)
{
    nsresult rv;
    uint32_t flags = 0;
    nsAutoString buf;

    rv = ReadInternetOption(INTERNET_PER_CONN_AUTOCONFIG_URL, flags, buf);
    if (!(flags & PROXY_TYPE_AUTO_PROXY_URL)) {
        aResult.Truncate();
        return rv;
    }

    if (NS_SUCCEEDED(rv))
        aResult = NS_ConvertUTF16toUTF8(buf);
    return rv;
}

nsresult
nsWindowsSystemProxySettings::GetProxyForURI(const nsACString & aSpec,
                                             const nsACString & aScheme,
                                             const nsACString & aHost,
                                             const int32_t      aPort,
                                             nsACString & aResult)
{
    nsresult rv;
    uint32_t flags = 0;
    nsAutoString buf;

    rv = ReadInternetOption(INTERNET_PER_CONN_PROXY_SERVER, flags, buf);
    if (NS_FAILED(rv) || !(flags & PROXY_TYPE_PROXY)) {
        SetProxyResultDirect(aResult);
        return NS_OK;
    }

    if (MatchOverride(aHost)) {
        SetProxyResultDirect(aResult);
        return NS_OK;
    }

    NS_ConvertUTF16toUTF8 cbuf(buf);

    nsAutoCString prefix;
    ToLowerCase(aScheme, prefix);

    prefix.Append('=');

    nsAutoCString specificProxy;
    nsAutoCString defaultProxy;
    nsAutoCString socksProxy;
    int32_t start = 0;
    int32_t end = cbuf.Length();

    while (true) {
        int32_t delimiter = cbuf.FindCharInSet(" ;", start);
        if (delimiter == -1)
            delimiter = end;

        if (delimiter != start) {
            const nsAutoCString proxy(Substring(cbuf, start,
                                                delimiter - start));
            if (proxy.FindChar('=') == -1) {
                
                
                
                
                defaultProxy = proxy;
            } else if (proxy.Find(prefix) == 0) {
                
                
                
                specificProxy = Substring(proxy, prefix.Length());
                break;
            } else if (proxy.Find("socks=") == 0) {
                
                socksProxy = Substring(proxy, 5); 
            }
        }

        if (delimiter == end)
            break;
        start = ++delimiter;
    }

    if (!specificProxy.IsEmpty())
        SetProxyResult("PROXY", specificProxy, aResult); 
    else if (!defaultProxy.IsEmpty())
        SetProxyResult("PROXY", defaultProxy, aResult); 
    else if (!socksProxy.IsEmpty())
        SetProxyResult("SOCKS", socksProxy, aResult); 
    else
        SetProxyResultDirect(aResult); 

    return NS_OK;
}

#define NS_WINDOWSSYSTEMPROXYSERVICE_CID\
    { 0x4e22d3ea, 0xaaa2, 0x436e, \
        {0xad, 0xa4, 0x92, 0x47, 0xde, 0x57, 0xd3, 0x67 } }

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsWindowsSystemProxySettings, Init)
NS_DEFINE_NAMED_CID(NS_WINDOWSSYSTEMPROXYSERVICE_CID);

static const mozilla::Module::CIDEntry kSysProxyCIDs[] = {
    { &kNS_WINDOWSSYSTEMPROXYSERVICE_CID, false, NULL, nsWindowsSystemProxySettingsConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kSysProxyContracts[] = {
    { NS_SYSTEMPROXYSETTINGS_CONTRACTID, &kNS_WINDOWSSYSTEMPROXYSERVICE_CID },
    { NULL }
};

static const mozilla::Module kSysProxyModule = {
    mozilla::Module::kVersion,
    kSysProxyCIDs,
    kSysProxyContracts
};

NSMODULE_DEFN(nsWindowsProxyModule) = &kSysProxyModule;
