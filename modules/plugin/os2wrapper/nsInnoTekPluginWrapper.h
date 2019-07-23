









































#ifndef __nsInnoTekPluginWrapper_h__
#define __nsInnoTekPluginWrapper_h__





#define VALID_PTR(pv)   \
    (   ((unsigned)(pv)) >= (unsigned)0x10000L    /* 64KB */ \
     && ((unsigned)(pv)) <  (unsigned)0xc0000000L /* 3GB */  \
        )


#define VALID_REF(ref)  VALID_PTR(&(ref))


#undef dprintf
#ifdef DEBUG
    #define dprintf(a)      npdprintf a
#else
    #define dprintf(a)      do { } while (0)
#endif


#ifdef DEBUG
    #define DebugInt3()     asm("int $3")
#else
    #define DebugInt3()     do { } while (0)
#endif


#ifdef DEBUG
    #define VERIFY_EXCEPTION_CHAIN()     npVerifyExcptChain()
#else
    #define VERIFY_EXCEPTION_CHAIN()     do { } while (0)
#endif





#ifdef __cplusplus
extern "C" {
#endif
int     npdprintf(const char *pszFormat, ...);
void _Optlink   ReleaseInt3(unsigned uEAX, unsigned uEDX, unsigned uECX);
#ifndef INCL_DEBUGONLY
void    npXPCOMInitSems();
nsresult npXPCOMGenericGetFactory(nsIServiceManagerObsolete *aServMgr, REFNSIID aClass, const char *aClassName,
                                  const char *aContractID, PRLibrary * aLibrary, nsIPlugin **aResult);
nsresult npXPCOMGenericMaybeWrap(REFNSIID aIID, nsISupports *aIn, nsISupports **aOut);
#endif

#ifdef __cplusplus
}
nsresult            downCreateWrapper(void **ppvResult, const void *pvVFT, nsresult rc);
nsresult            downCreateWrapper(void **ppvResult, REFNSIID aIID, nsresult rc);
extern "C" {
nsresult            upCreateWrapper(void **ppvResult, REFNSIID aIID, nsresult rc);
const char *        getIIDCIDName(REFNSIID aIID);
const nsID *        getIIDCIDFromName(const char *pszStrID);
#endif

#ifdef __cplusplus
}
#endif
#endif
