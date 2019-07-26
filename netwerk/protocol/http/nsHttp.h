





#ifndef nsHttp_h__
#define nsHttp_h__

#include "plstr.h"
#include "prtime.h"
#include "nsISupportsUtils.h"
#include "nsPromiseFlatString.h"
#include "nsURLHelper.h"
#include "netCore.h"
#include "mozilla/Mutex.h"


#define NS_HTTP_VERSION_UNKNOWN  0
#define NS_HTTP_VERSION_0_9      9
#define NS_HTTP_VERSION_1_0     10
#define NS_HTTP_VERSION_1_1     11

namespace mozilla {
namespace net {
    enum {
        SPDY_VERSION_2 = 2,
        SPDY_VERSION_3 = 3
    };
} 
} 

typedef uint8_t nsHttpVersion;





#define NS_HTTP_ALLOW_KEEPALIVE      (1<<0)
#define NS_HTTP_ALLOW_PIPELINING     (1<<1)



#define NS_HTTP_STICKY_CONNECTION    (1<<2)



#define NS_HTTP_REFRESH_DNS          (1<<3)



#define NS_HTTP_LOAD_ANONYMOUS       (1<<4)


#define NS_HTTP_TIMING_ENABLED       (1<<5)



#define NS_HTTP_LOAD_AS_BLOCKING     (1<<6)




#define NS_HTTP_DISALLOW_SPDY        (1<<7)



#define NS_HTTP_LOAD_UNBLOCKED       (1<<8)



#define NS_HTTP_ALLOW_RSA_FALSESTART (1<<9)
#define NS_HTTP_ALLOW_RC4_FALSESTART (1<<10)





#define NS_HTTP_DEFAULT_PORT  80
#define NS_HTTPS_DEFAULT_PORT 443

#define NS_HTTP_HEADER_SEPS ", \t"





struct nsHttpAtom
{
    operator const char *() const { return _val; }
    const char *get() const { return _val; }

    void operator=(const char *v) { _val = v; }
    void operator=(const nsHttpAtom &a) { _val = a._val; }

    
    const char *_val;
};

struct nsHttp
{
    static nsresult CreateAtomTable();
    static void DestroyAtomTable();

    
    
    
    static mozilla::Mutex *GetLock();

    
    static nsHttpAtom ResolveAtom(const char *);
    static nsHttpAtom ResolveAtom(const nsACString &s)
    {
        return ResolveAtom(PromiseFlatCString(s).get());
    }

    
    
    static bool IsValidToken(const char *start, const char *end);

    static inline bool IsValidToken(const nsCString &s) {
        const char *start = s.get();
        return IsValidToken(start, start + s.Length());
    }

    
    
    
    
    
    static const char *FindToken(const char *input, const char *token,
                                 const char *separators);

    
    
    
    
    
    
    
    
    static bool ParseInt64(const char *input, const char **next,
                             int64_t *result);

    
    
    static inline bool ParseInt64(const char *input, int64_t *result) {
        const char *next;
        return ParseInt64(input, &next, result) && *next == '\0';
    }

    
    static bool IsPermanentRedirect(uint32_t httpStatus);

    
    
    static bool ShouldRewriteRedirectToGET(uint32_t httpStatus, nsHttpAtom method);

    
    
    static bool IsSafeMethod(nsHttpAtom method);

    
    
    
    
    
    
#define HTTP_ATOM(_name, _value) static nsHttpAtom _name;
#include "nsHttpAtomList.h"
#undef HTTP_ATOM
};





static inline uint32_t
PRTimeToSeconds(PRTime t_usec)
{
    return uint32_t( t_usec / PR_USEC_PER_SEC );
}

#define NowInSeconds() PRTimeToSeconds(PR_Now())


#define QVAL_TO_UINT(q) ((unsigned int) ((q + 0.005) * 100.0))

#define HTTP_LWS " \t"
#define HTTP_HEADER_VALUE_SEPS HTTP_LWS ","

#endif 
