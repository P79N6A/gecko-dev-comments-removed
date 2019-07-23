












































# ifdef IPLUGINW_OUTOFTREE
#  ifdef __IBMCPP__
#   include "moz_VACDefines.h"
#  else
#   include "moz_GCCDefines.h"
#  endif
# endif


#define INCL_DOSSEMAPHORES
#define INCL_DOSMODULEMGR
#define INCL_ERRORS
#define INCL_PM
#include <os2.h>
#include <float.h>



#include "npapi.h"

#include "nscore.h"
#include "nsError.h"
#include "nsDebug.h"
#include "nsID.h"
#include "nsrootidl.h"
#include "nsISupports.h"
#include "nsISupportsBase.h"
#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsIClassInfo.h"
#include "nsIFile.h"
#include "nsIEnumerator.h"
#include "nsCOMPtr.h"

#include "nsIServiceManager.h"
#include "nsIServiceManagerObsolete.h"
#include "nsIComponentManagerObsolete.h"
#include "nsIComponentManager.h"
#include "nsIFactory.h"
#include "nsIMemory.h"
#include "nsIOutputStream.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nspluginroot.h"
#include "nsplugindefs.h"
#include "nsIPlugin.h"
#include "nsIPluginManager.h"
#include "nsIEventHandler.h"
#include "nsIPluginManager2.h"
#include "nsIPluginStreamInfo.h"
#include "nsIPluginStreamListener.h"
#include "nsIPluginInstance.h"
#include "nsIPluginInstancePeer.h"
#include "nsIPluginInstancePeer2.h"
#include "nsIPluginTagInfo.h"
#include "nsIPluginTagInfo2.h"
#include "nsIHTTPHeaderListener.h"

#include "nsIJRIPlugin.h"
#include "nsIJVMPluginInstance.h"
#include "nsIJVMManager.h"
#include "nsIJVMWindow.h"
#include "nsIJVMConsole.h"
#include "nsISerializable.h"
#include "nsIPrincipal.h"
#include "nsIJVMPlugin.h"
#include "nsIJVMPluginTagInfo.h"
#include "nsIJVMPrefsWindow.h"
#include "nsILiveConnectManager.h"
#include "nsISecurityContext.h"
#include "nsILiveConnect.h"
#include "nsISecureLiveConnect.h"
#include "nsISecureEnv.h"
#include "nsISymantecDebugger.h"
#include "nsISymantecDebugManager.h"
#include "nsIJVMThreadManager.h"
#include "nsIRunnable.h"
#include "nsIJVMConsole.h"

#include "nsILoadGroup.h"
#include "nsIInterfaceRequestor.h"

#ifdef IPLUGINW_OUTOFTREE
#include "nsStringDefines.h"
#include "nsStringFwd.h"
#include "nsBufferHandle.h"
#include "nsStringIteratorUtils.h"
#include "nsCharTraits.h"
#include "nsStringFragment.h"
#include "nsCharTraits.h"
#include "nsStringTraits.h"
#include "nsAlgorithm.h"
#include "nsStringIterator.h"
#include "nsAString.h"
#endif
#include "domstubs.h"
#include "nsIDOM3Node.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"

#include "prlink.h"

#include "nsInnoTekPluginWrapper.h"

#include "wrap_XPCOM_3rdparty.h"
#include "wrap_VFTs.h"






#define DO_EXTRA_FAILURE_HANDLING


#ifdef __IBMCPP__
# define BREAK_POINT()   __interrupt(3)
#else
# define BREAK_POINT()   asm("int $3")
#endif


#ifdef DEBUG
# define DEBUG_GLOBAL_BREAKPOINT(cond) do { if (gfGlobalBreakPoint && (cond)) BREAK_POINT(); } while (0)
#else
# define DEBUG_GLOBAL_BREAKPOINT(cond) do {} while (0)
#endif


#ifdef __IBMCPP__
#define XPFUNCTION  __FUNCTION__
#else
#define XPFUNCTION  __PRETTY_FUNCTION__
#endif


#ifdef DEBUG
# define DEBUG_FUNCTIONNAME() static const char *pszFunction = XPFUNCTION
#else
# define DEBUG_FUNCTIONNAME() do {} while (0)
#endif





#define MAKE_SAFE_VFT(VFTType, name)    \
    const struct VFTType##WithPaddings                                  \
    {                                                                   \
        const VFTType   vft;                                            \
        unsigned        aulNulls[10];                                   \
    }   name = {



#define SAFE_VFT_ZEROS()  ,{ 0x81000001, 0x81000002, 0x81000003, 0x81000004, \
    0x81000005, 0x81000006, 0x81000007, 0x81000008, 0x81000009, 0x8100000a} }



#define DPRINTF_NSID(refID)  \
    if (VALID_REF(refID))                                                                     \
        dprintf(("%s: %s={%08x,%04x,%04x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x} %s (%p)",   \
                pszFunction, #refID, (refID).m0, (refID).m1, (refID).m2, (refID).m3[0], (refID).m3[1],   \
                (refID).m3[2], (refID).m3[3], (refID).m3[4], (refID).m3[5], (refID).m3[6], (refID).m3[7], \
                getIIDCIDName(refID), &(refID)));                                             \
    else                                                                                      \
        dprintf(("%s: %s=%p (illegal pointer!!!)", pszFunction, #refID, &(refID)))


#define DPRINTF_CONTRACTID(pszContractID)  \
    DPRINTF_STR(pszContractID)


#define DPRINTF_STR(pszStr)  \
    do                                                                             \
    {                                                                              \
        if (VALID_PTR(pszStr))                                                     \
            dprintf(("%s: %s='%s' (%p)", pszFunction, #pszStr, pszStr, pszStr));   \
        else if (pszStr)                                                           \
            dprintf(("%s: %s=%p (illegal pointer!!!)", pszFunction, #pszStr, pszStr));\
        else                                                                       \
            dprintf(("%s: %s=<NULL>", pszFunction, #pszStr));                       \
    } while (0)


#define DPRINTF_STRNULL(pszStr)  \
    do                                                                             \
    {                                                                              \
        if (VALID_PTR(pszStr))                                                     \
            dprintf(("%s: %s='%s' (%p)", pszFunction, #pszStr, pszStr, pszStr));   \
        else if (pszStr)                                                           \
            dprintf(("%s: %s=%p (illegal pointer!!!)", pszFunction, #pszStr, pszStr)); \
    } while (0)



#if defined(DEBUG) && 0
#define DEBUG_STACK_ENTER()                                                        \
    void *  pvStackGuard = memset(alloca(sizeof(achDebugStack)), 'k', sizeof(achDebugStack))
#else
#define DEBUG_STACK_ENTER()     do {} while (0)
#endif


#if defined(DEBUG) && 0
static char achDebugStack[0x1000];
#define DEBUG_STACK_LEAVE()                                                                 \
    do {                                                                                    \
        if (achDebugStack[0] != 'k')                                                        \
            memset(achDebugStack, 'k', sizeof(achDebugStack));                              \
        if (memcmp(achDebugStack, pvStackGuard, sizeof(achDebugStack)))                     \
        {                                                                                   \
            dprintf(("%s: ARRRRRRRRRGGGGGGGGGGG!!!!!!!! Stack is f**ked !!!", pszFunction)); \
            DebugInt3();                                                                    \
        }                                                                                   \
    } while (0)
#else
#define DEBUG_STACK_LEAVE()     do {} while (0)
#endif



#define SAVELOAD_FPU_CW(usNewCw) \
    unsigned short usSavedCw = _control87(usNewCw, 0xffff)


#define RESTORE_FPU_CW() \
    _control87(usSavedCw, 0xffff)



#define DOWN_MAKE_pMozI(interface, pvDownThis) \
    interface *pMozI = (interface *)((PDOWNTHIS)pvDownThis)->pvThis


#define DOWN_VALID(pvDownThis)  \
    (  VALID_PTR(pvDownThis)    \
     && !memcmp(&((PDOWNTHIS)(pvDownThis))->achMagic[0], gszDownMagicString, sizeof(gszDownMagicString)) )


#define DOWN_VALIDATE_RET(pvDownThis)  \
    if (!DOWN_VALID(pvDownThis))                                             \
    {                                                                        \
        dprintf(("%s: invalid this pointer %p!", pszFunction, pvDownThis));   \
        DebugInt3();                                                         \
        DOWN_TRACE_LEAVE_INT(pvDownThis, NS_ERROR_INVALID_POINTER);          \
        return NS_ERROR_INVALID_POINTER;                                     \
    }


#define DOWN_VALIDATE_NORET(pvDownThis)  \
    if (!DOWN_VALID(pvDownThis))                                             \
    {                                                                        \
        dprintf(("%s: invalid this pointer %p!", pszFunction, pvDownThis));   \
        DebugInt3();                                                         \
    }



#define DOWN_TRACE_ENTER(pvDownThis) \
    dprintf(("%s: enter this=%p (down)", pszFunction, pvDownThis));


#define DOWN_ENTER(pvDownThis, interface) \
    DEBUG_STACK_ENTER();                                                    \
    DEBUG_FUNCTIONNAME();                                                   \
    DOWN_TRACE_ENTER(pvDownThis);                                           \
    DOWN_VALIDATE_RET(pvDownThis);                                          \
    DOWN_MAKE_pMozI(interface, pvDownThis);                                 \
    DEBUG_GLOBAL_BREAKPOINT(1);


#define DOWN_ENTER_NORET(pvDownThis, interface) \
    DEBUG_STACK_ENTER();                                                    \
    DEBUG_FUNCTIONNAME();                                                   \
    DOWN_TRACE_ENTER(pvDownThis);                                           \
    DOWN_VALIDATE_NORET(pvDownThis);                                        \
    DOWN_MAKE_pMozI(interface, pvDownThis);                                 \
    DEBUG_GLOBAL_BREAKPOINT(1);


#define DOWN_ENTER_RC(pvDownThis, interface) \
    DOWN_ENTER(pvDownThis, interface);       \
    nsresult    rc = NS_ERROR_UNEXPECTED



#define DOWN_TRACE_LEAVE(pvDownThis) \
    dprintf(("%s: leave this=%p (down)", pszFunction, pvDownThis))


#define DOWN_TRACE_LEAVE_INT(pvDownThis, rc) \
    dprintf(("%s: leave this=%p rc=%x (down)", pszFunction, pvDownThis, rc))


#define DOWN_LEAVE(pvDownThis) \
    DOWN_TRACE_LEAVE(pvDownThis);                                           \
    DEBUG_GLOBAL_BREAKPOINT(1);                                             \
    DEBUG_STACK_LEAVE();


#define DOWN_LEAVE_INT(pvDownThis, rc) \
    DOWN_TRACE_LEAVE_INT(pvDownThis, rc);                                   \
    DEBUG_GLOBAL_BREAKPOINT(1);                                             \
    DEBUG_STACK_LEAVE();


#define DOWN_MAGIC_STRING       "DownMagicString"


#define DOWN_LOCK()     downLock()


#define DOWN_UNLOCK()   downUnLock()



#define UP_VALID() \
    (*(void**)mpvThis == mpvVFTable)


#define UP_VALIDATE_RET() \
    if (*(void**)mpvThis != mpvVFTable)                                         \
    {                                                                           \
        dprintf(("%s: invalid object!!! VFTable entery have changed! %x != %x", \
                 pszFunction, *(void**)mpvThis, mpvVFTable));                   \
        DebugInt3();                                                            \
        return NS_ERROR_UNEXPECTED;                                             \
    }


#define UP_TRACE_ENTER() \
    dprintf(("%s: enter this=%p (up)", pszFunction, mpvThis));



#define UP_ENTER__(fGDB) \
    DEBUG_FUNCTIONNAME();                          \
    UP_TRACE_ENTER();                              \
    DEBUG_GLOBAL_BREAKPOINT(fGDB)


#define UP_ENTER_RC__(fGDB) \
    DEBUG_FUNCTIONNAME();                          \
    UP_TRACE_ENTER();                              \
    nsresult    rc = NS_ERROR_UNEXPECTED;          \
    UP_VALIDATE_RET();                             \
    DEBUG_GLOBAL_BREAKPOINT(fGDB)



#define UP_ENTER() \
    UP_ENTER__(1)


#define UP_ENTER_NODBGBRK() \
    UP_ENTER__(0)


#define UP_ENTER_RC() \
    UP_ENTER_RC__(1)


#define UP_ENTER_RC_NODBGBRK() \
    UP_ENTER_RC__(0)




#define UP_TRACE_LEAVE() \
    dprintf(("%s: leave this=%p (up)", pszFunction, mpvThis));


#define UP_TRACE_LEAVE_INT(rc) \
    dprintf(("%s: leave this=%p rc=%x (up)", pszFunction, mpvThis, rc));



#define UP_LEAVE() \
    UP_TRACE_LEAVE();                                              \
    DEBUG_GLOBAL_BREAKPOINT(1);                                    \
    DEBUG_STACK_LEAVE()


#define UP_LEAVE_INT(rc) \
    UP_TRACE_LEAVE_INT(rc);                                        \
    DEBUG_GLOBAL_BREAKPOINT(1);                                    \
    DEBUG_STACK_LEAVE()


#define UP_LEAVE_INT_NODBGBRK(rc) \
    UP_TRACE_LEAVE_INT(rc);                                        \
    DEBUG_STACK_LEAVE()


#define UP_IMPL_NSISUPPORTS() \
    NS_IMETHOD QueryInterface(REFNSIID aIID, void **aInstancePtr) { return hlpQueryInterface(aIID, aInstancePtr); } \
    NS_IMETHOD_(nsrefcnt) AddRef(void)  { return hlpAddRef(); } \
    NS_IMETHOD_(nsrefcnt) Release(void) { return hlpRelease(); } \
    //


#define UP_IMPL_NSISUPPORTS_NOT_RELEASE() \
    NS_IMETHOD QueryInterface(REFNSIID aIID, void **aInstancePtr) { return hlpQueryInterface(aIID, aInstancePtr); } \
    NS_IMETHOD_(nsrefcnt) AddRef(void)  { return hlpAddRef(); } \
    //


#define UP_IMPL_NSIFACTORY() \
    NS_IMETHOD CreateInstance(nsISupports *aOuter, const nsIID & aIID, void * *result)  \
    { return hlpCreateInstance(aOuter, aIID, result); }                                 \
    NS_IMETHOD LockFactory(PRBool lock)                                                 \
    { return hlpLockFactory(lock); }                                                    \
    //


#define UP_IMPL_NSIJVMWINDOW() \
    NS_IMETHOD Show(void)                       \
    { return hlpShow(); }                       \
    NS_IMETHOD Hide(void)                       \
    { return hlpHide(); }                       \
    NS_IMETHOD IsVisible(PRBool *result)        \
    { return hlpIsVisible(result); }            \
    //


#define UP_IMPL_NSIPLUGININSTANCEPEER() \
    NS_IMETHOD GetValue(nsPluginInstancePeerVariable aVariable, void * aValue)              \
    { return hlpGetValue(aVariable, aValue); }                                              \
    NS_IMETHOD GetMIMEType(nsMIMEType *aMIMEType)                                           \
    { return hlpGetMIMEType(aMIMEType); }                                                   \
    NS_IMETHOD GetMode(nsPluginMode *aMode)                                                 \
    { return hlpGetMode(aMode); }                                                           \
    NS_IMETHOD NewStream(nsMIMEType aType, const char *aTarget, nsIOutputStream **aResult)  \
    { return hlpNewStream(aType, aTarget, aResult); }                                       \
    NS_IMETHOD ShowStatus(const char *aMessage)                                             \
    { return hlpShowStatus(aMessage); }                                                     \
    NS_IMETHOD SetWindowSize(PRUint32 aWidth, PRUint32 aHeight)                             \
    { return hlpSetWindowSize(aWidth, aHeight); }                                           \
    //


#define UP_IMPL_NSIREQUESTOBSERVER() \
    NS_IMETHOD OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)                      \
    { return hlpOnStartRequest(aRequest, aContext); }                                           \
    NS_IMETHOD OnStopRequest(nsIRequest *aRequest, nsISupports *aContext, nsresult aStatusCode) \
    { return hlpOnStopRequest(aRequest, aContext, aStatusCode); }                               \
    //


#define UP_IMPL_NSIREQUEST() \
    NS_IMETHOD GetName(nsACString & aName)                      \
    { return hlpGetName(aName); }                               \
    NS_IMETHOD IsPending(PRBool *_retval)                       \
    { return hlpIsPending(_retval); }                           \
    NS_IMETHOD GetStatus(nsresult *aStatus)                     \
    { return hlpGetStatus(aStatus); }                           \
    NS_IMETHOD Cancel(nsresult aStatus)                         \
    { return hlpCancel(aStatus); }                              \
    NS_IMETHOD Suspend(void)                                    \
    { return hlpSuspend(); }                                    \
    NS_IMETHOD Resume(void)                                     \
    { return hlpResume(); }                                     \
    NS_IMETHOD GetLoadGroup(nsILoadGroup * *aLoadGroup)         \
    { return hlpGetLoadGroup(aLoadGroup); }                     \
    NS_IMETHOD SetLoadGroup(nsILoadGroup * aLoadGroup)          \
    { return hlpSetLoadGroup(aLoadGroup); }                     \
    NS_IMETHOD GetLoadFlags(nsLoadFlags *aLoadFlags)            \
    { return hlpGetLoadFlags(aLoadFlags); }                     \
    NS_IMETHOD SetLoadFlags(nsLoadFlags aLoadFlags)             \
    { return hlpSetLoadFlags(aLoadFlags); }                     \
    //


#if 1
#define ZERO_JAVAVALUE(value, type) memset(&value, 0, sizeof(jvalue))
#else
#define ZERO_JAVAVALUE(value, type) \
    do                                                                  \
    {                                                                   \
        switch (type)                                                   \
        {                                                               \
            case jobject_type:  (value).l = 0; break;                   \
            case jboolean_type: (value).z = 0; break;                   \
            case jbyte_type:    (value).b = 0; break;                   \
            case jchar_type:    (value).c = 0; break;                   \
            case jshort_type:   (value).s = 0; break;                   \
            case jint_type:     (value).i = 0; break;                   \
            case jlong_type:    (value).j = 0; break;                   \
            case jfloat_type:   (value).f = 0; break;                   \
            case jdouble_type:  (value).d = 0; break;                   \
        }                                                               \
    } while (0)
#endif







struct DownThis;
class UpWrapperBase;




typedef struct WrapperNode
{
    
    enum enmWrapperType
    { enmDown, enmUp }
        enmType;

    
    volatile struct WrapperNode  *pNext;

    
    void *pvThis;

    
    union WrapperPointer
    {
        struct DownThis *   pDown;
        UpWrapperBase *     pUp;
    } u;
} WNODE, *PWNODE;






typedef struct DownThis
{
    


    
    const void *    pvVFT;

    
#ifdef DEBUG
    char            achZeros[12 + 256];
#else
    char            achZeros[12 + 16];
#endif

    
    void *          pvThis;

    
    char            achMagic[16];

    
    volatile struct DownThis * pNext;
    

    


    inline void initialize(void *zpvThis, const void *zpvVFT)
    {
        this->pvVFT = zpvVFT;
        memset(&achZeros[0], 0, sizeof(achZeros));
        for (unsigned i = 0; i < sizeof(achZeros) / sizeof(unsigned); i += sizeof(unsigned))
            achZeros[i] = 0xC0000000 | i;
        this->pvThis = zpvThis;
        memcpy(achMagic, DOWN_MAGIC_STRING, sizeof(achMagic));
    }
} DOWNTHIS, *PDOWNTHIS;






int             giXPCOM;




#ifdef DEBUG
int             gfGlobalBreakPoint = FALSE;
#endif





const char gszDownMagicString[16] = DOWN_MAGIC_STRING;






volatile struct DownThis * gpDownHead = NULL;


HMTX ghmtxDown = NULLHANDLE;






#include "moz_IDs_Generated.h"
#undef NP_DEF_ID
#define NP_DEF_ID NS_DEFINE_IID
#define NP_INCL_LOOKUP
#include "moz_IDs_Generated.h"





const void *    downIsSupportedInterface(REFNSIID aIID);
nsresult        downCreateWrapper(void **ppvResult, const void *pvVFT, nsresult rc);
nsresult        downCreateWrapper(void **ppvResult, REFNSIID aIID, nsresult rc);
void            downLock(void);
void            downUnLock(void);
BOOL            upIsSupportedInterface(REFNSIID aIID);
nsresult        upCreateWrapper(void **ppvResult, REFNSIID aIID, nsresult rc);
int             downCreateJNIEnvWrapper(JNIEnv **ppJNIEnv, int rc);
int             upCreateJNIEnvWrapper(JNIEnv **ppJNIEnv, int rc);
void            verifyAndFixUTF8String(const char *pszString, const char *pszFunction);










class UpWrapperBase
{
protected:
    
    void *      mpvThis;

    

    void *      mpvVFTable;

    
    REFNSIID    miid;

    




    void *      mpvInterface;

    
    volatile UpWrapperBase * mpNext;

    


    static volatile UpWrapperBase * mpHead;

    
    static HMTX                     mhmtx;


protected:
    







    UpWrapperBase(void *pvThis, void *pvInterface, REFNSIID iid) :
        mpvThis(pvThis), mpvVFTable(*(void**)pvThis), miid(iid), mpvInterface(pvInterface), mpNext(NULL)
    {
    }
public:
    


    virtual ~UpWrapperBase()
    {
    }


    



    void    upInsertWrapper(void)
    {
        mpNext = mpHead;
        mpHead = this;
    }


    





    int     upRemoveWrapper(void)
    {
        int fUnchained;
        UpWrapperBase * pUp, *pUpPrev;
        for (pUp = (UpWrapperBase*)mpHead, pUpPrev = NULL, fUnchained = 0;
             pUp;
             pUpPrev = pUp, pUp = (UpWrapperBase*)pUp->mpNext)
        {
            if (pUp->mpvThis == mpvThis)
            {
                if (pUpPrev)
                    pUpPrev->mpNext = pUp->mpNext;
                else
                    mpHead = pUp->mpNext;

                #ifdef DEBUG
                
                if (fUnchained)
                {
                    dprintf(("upRemoveWrapper: this=%x is linked twice !!!", this));
                    DebugInt3();
                }
                fUnchained = 1;
                #else
                fUnchained = 1;
                break;
                #endif
            }
        }

        if (fUnchained)
            this->mpNext = NULL;
        return fUnchained;
    }


    


    void *  getInterfacePointer()
    {
        return mpvInterface;
    }

    


    void *  getThis()
    {
        return mpvThis;
    }

    


    void *  getVFT()
    {
        return mpvVFTable;
    }

    


    static void upLock(void)
    {
        DEBUG_FUNCTIONNAME();

        if (!mhmtx)
        {
            int rc = DosCreateMutexSem(NULL, &mhmtx, 0, TRUE);
            if (rc)
            {
                dprintf(("%s: DosCreateMutexSem failed with rc=%d.", pszFunction, rc));
                ReleaseInt3(0xdeadbee1, 0xe000, rc);
            }
        }
        else
        {
            int rc = DosRequestMutexSem(mhmtx, SEM_INDEFINITE_WAIT);
            if (rc)
            {
                dprintf(("%s: DosRequestMutexSem failed with rc=%d.", pszFunction, rc));
                ReleaseInt3(0xdeadbee1, 0xe001, rc);
            }
            
        }
    }

    


    static void upUnLock()
    {
        DEBUG_FUNCTIONNAME();

        int rc = DosReleaseMutexSem(mhmtx);
        if (rc)
        {
            dprintf(("%s: DosRequestMutexSem failed with rc=%d.", pszFunction, rc));
            ReleaseInt3(0xdeadbee1, 0xe002, rc);
        }
        
    }

    









    static UpWrapperBase * findUpWrapper(void *pvThis, REFNSIID iid)
    {
        DEBUG_FUNCTIONNAME();

        


        for (UpWrapperBase * pUp = (UpWrapperBase*)mpHead; pUp; pUp = (UpWrapperBase*)pUp->mpNext)
            if (pUp->mpvThis == pvThis)
            {
                if (pUp->mpvVFTable == *((void**)pvThis))
                {
                    if (iid.Equals(pUp->miid) || iid.Equals(kSupportsIID))
                    {
                        dprintf(("%s: Found existing UP wrapper %p/%p for %p.",
                                 pszFunction, pUp, pUp->mpvInterface, pvThis));
                        return pUp;
                    }
                    else
                    {
                        dprintf(("%s: Found wrapper %p/%p for %p, but iid's wasn't matching.",
                                 pszFunction, pUp, pUp->getInterfacePointer(), pvThis));
                        DPRINTF_NSID(iid);
                        DPRINTF_NSID(pUp->miid);
                    }
                }
                else
                {
                    dprintf(("%s: Seems like an object have been release and reused again... (pvThis=%x)",
                             pszFunction, pvThis));
                    



                    if (pUp->upRemoveWrapper())
                        pUp = (UpWrapperBase*)mpHead;
                }
            }


        
        return NULL;
    }

    









    static void * findUpWrapper(void *pvThis)
    {
        DEBUG_FUNCTIONNAME();

        


        upLock();
        for (UpWrapperBase * pUp = (UpWrapperBase *)mpHead; pUp; pUp = (UpWrapperBase *)pUp->mpNext)
            if ((void*)pUp == pvThis)
            {
                void *pvRet = pUp->mpvThis;
                upUnLock();
                dprintf(("%s: pvThis(=%x) was pointing to an up wrapper. Returning pointer to real object(=%x)",
                         pszFunction, pvThis, pvRet));
                return pvRet;
            }
        upUnLock();

        
        return NULL;
    }



    












    static void * findDownWrapper(void *pvThis)
    {
        DEBUG_FUNCTIONNAME();

        


        DOWN_LOCK();
        for (PDOWNTHIS pDown = (PDOWNTHIS)gpDownHead; pDown; pDown = (PDOWNTHIS)pDown->pNext)
            if (pDown == pvThis)
            {
                void *pvRet = pDown->pvThis;
                DOWN_UNLOCK();
                dprintf(("%s: pvThis(=%x) was pointing to a down wrapper. Returning pointer to real object(=%x)",
                         pszFunction, pvThis, pvRet));
                return pvRet;
            }
        DOWN_UNLOCK();

        
        return NULL;
    }

};

volatile UpWrapperBase * UpWrapperBase::mpHead = NULL;
HMTX                     UpWrapperBase::mhmtx = NULLHANDLE;











class UpSupportsBase : public UpWrapperBase
{
protected:
    







    UpSupportsBase(void *pvThis, void *pvInterface, REFNSIID iid) :
        UpWrapperBase(pvThis, pvInterface, iid)
    {
    }

    








    nsresult hlpQueryInterface(REFNSIID aIID, void **aInstancePtr)
    {
        UP_ENTER_RC_NODBGBRK();
        DPRINTF_NSID(aIID);

        rc = VFTCALL2((VFTnsISupports*)mpvVFTable, QueryInterface, mpvThis, aIID, aInstancePtr);
        rc = upCreateWrapper(aInstancePtr, aIID, rc);

        UP_LEAVE_INT_NODBGBRK(rc);
        return rc;
    }

    






    nsrefcnt hlpAddRef()
    {
        UP_ENTER_NODBGBRK();
        nsrefcnt c = 0;
        if (UP_VALID())
            c = VFTCALL0((VFTnsISupports*)mpvVFTable, AddRef, mpvThis);
        else
        {
            dprintf(("%s: Invalid object!", pszFunction));
            DebugInt3();
        }
        UP_LEAVE_INT_NODBGBRK(c);
        return c;
    }

    






    nsrefcnt hlpRelease()
    {
        UP_ENTER_NODBGBRK();
        nsrefcnt c = 0;
        if (UP_VALID())
        {
            c = VFTCALL0((VFTnsISupports*)mpvVFTable, Release, mpvThis);
            if (!c)
            {
                dprintf(("%s: the reference count is zero! Delete this wrapper", pszFunction));
                
                UP_LEAVE_INT_NODBGBRK(0); 

                #ifdef DO_DELETE
                


                upLock();
                if (upRemoveWrapper())
                {
                    upUnLock();
                    


                    delete this;
                }
                else
                {
                    upUnLock();
                    




                    dprintf(("%s: pvThis=%p not found in the list !!!!", pszFunction, mpvThis));
                    DebugInt3();
                }
                #endif
                return 0;
            }
        }
        else
        {
            dprintf(("%s: Invalid object!", pszFunction));
            DebugInt3();
        }
        UP_LEAVE_INT_NODBGBRK(c);
        return c;
    }
};










class UpSupports : public nsISupports, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    


    UpSupports(void *pvThis) :
        UpSupportsBase(pvThis, (nsISupports*)this, kSupportsIID)
    {
    }
};






class UpFactoryBase : public UpSupportsBase
{
protected:
    















    
    NS_IMETHOD hlpCreateInstance(nsISupports *aOuter, const nsIID & aIID, void * *result)
    {
        UP_ENTER_RC();
        DPRINTF_NSID(aIID);

        


        if (upIsSupportedInterface(aIID))
        {
            


            nsISupports * pDownOuter = aOuter;
            rc = downCreateWrapper((void**)&pDownOuter, downIsSupportedInterface(kSupportsIID), NS_OK);
            if (rc == NS_OK)
            {
                rc = VFTCALL3((VFTnsIFactory*)mpvVFTable, CreateInstance, mpvThis, pDownOuter, aIID, result);
                rc = upCreateWrapper(result, aIID, rc);
            }
            else
            {
                dprintf(("%s: downCreateWrapper failed for nsISupports/aOuter. rc=%x", pszFunction, rc));
                DebugInt3();
            }
        }
        else
        {
            dprintf(("%s: Unsupported interface!!!", pszFunction));
            rc = NS_NOINTERFACE;
            ReleaseInt3(0xbaddbeef, 7, aIID.m0);
        }

        UP_LEAVE_INT(rc);
        return rc;
    }

    









    
    NS_IMETHOD hlpLockFactory(PRBool lock)
    {
        UP_ENTER_RC();
        dprintf(("%s: lock=%d", pszFunction, lock));
        rc = VFTCALL1((VFTnsIFactory*)mpvVFTable, LockFactory, mpvThis, lock);
        UP_LEAVE_INT(rc);
        return rc;
    }


    
    UpFactoryBase(void *pvThis, void *pvInterface, REFNSIID iid = kFactoryIID) :
        UpSupportsBase(pvThis, pvInterface, iid)
    {
    }

};




class UpFactory : public nsIFactory, public UpFactoryBase
{
public:
    UP_IMPL_NSISUPPORTS();
    UP_IMPL_NSIFACTORY();


    
    UpFactory(void *pvThis) :
        UpFactoryBase(pvThis, (nsIFactory*)this, kFactoryIID)
    {
    }
};










class UpPlugin : public nsIPlugin, public UpFactoryBase
{
public:
    UP_IMPL_NSISUPPORTS();
    UP_IMPL_NSIFACTORY();

    




    
    NS_IMETHOD CreatePluginInstance(nsISupports *aOuter, const nsIID & aIID, const char *aPluginMIMEType, void * *aResult)
    {
        UP_ENTER_RC();
        DPRINTF_NSID(aIID);
        DPRINTF_STR(aPluginMIMEType);

        


        if (upIsSupportedInterface(aIID))
        {
            


            nsISupports * pDownOuter = aOuter;
            rc = downCreateWrapper((void**)&pDownOuter, downIsSupportedInterface(kSupportsIID), NS_OK);
            if (rc == NS_OK)
            {
                rc = VFTCALL4((VFTnsIPlugin*)mpvVFTable, CreatePluginInstance, mpvThis, aOuter, aIID, aPluginMIMEType, aResult);
                rc = upCreateWrapper(aResult, aIID, rc);
            }
            else
            {
                dprintf(("%s: downCreateWrapper failed for nsISupports/aOuter. rc=%x", pszFunction, rc));
                DebugInt3();
            }
        }
        else
        {
            dprintf(("%s: Unsupported interface!!!", pszFunction));
            rc = NS_NOINTERFACE;
            ReleaseInt3(0xbaddbeef, 8, aIID.m0);
        }

        UP_LEAVE_INT(rc);
        return rc;
    }


    








    
    NS_IMETHOD Initialize(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIPlugin*)mpvVFTable, Initialize, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    







    
    NS_IMETHOD Shutdown(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIPlugin*)mpvVFTable, Shutdown, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    











    
    NS_IMETHOD GetMIMEDescription(const char * *aMIMEDescription)
    {
        
        UP_ENTER_RC();
        dprintf(("%s: aMIMEDescription=%x", pszFunction, aMIMEDescription));
        rc = VFTCALL1((VFTnsIPlugin*)mpvVFTable, GetMIMEDescription, mpvThis, aMIMEDescription);
        if (NS_SUCCEEDED(rc) && VALID_PTR(aMIMEDescription))
            DPRINTF_STR(*aMIMEDescription);
        UP_LEAVE_INT(rc);
        return rc;
    }

    








    
    NS_IMETHOD GetValue(nsPluginVariable aVariable, void * aValue)
    {
        UP_ENTER_RC();
        dprintf(("%s: aVariable=%d aValue=%x", pszFunction, aVariable, aValue));
        rc = VFTCALL2((VFTnsIPlugin*)mpvVFTable, GetValue, mpvThis, aVariable, aValue);
        UP_LEAVE_INT(rc);
        return rc;
    }


    


    UpPlugin(void *pvThis) :
        UpFactoryBase(pvThis, (nsIPlugin*)this, kPluginIID)
    {
    }
};





class UpJVMPlugin : public nsIJVMPlugin, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    
    
    NS_IMETHOD AddToClassPath(const char* dirPath)
    {
        UP_ENTER_RC();
        DPRINTF_STR(dirPath);
        rc = VFTCALL1((VFTnsIJVMPlugin*)mpvVFTable, AddToClassPath, mpvThis, dirPath);
        UP_LEAVE_INT(rc);
        return rc;
    }

    
    
    NS_IMETHOD RemoveFromClassPath(const char* dirPath)
    {
        UP_ENTER_RC();
        DPRINTF_STR(dirPath);
        rc = VFTCALL1((VFTnsIJVMPlugin*)mpvVFTable, RemoveFromClassPath, mpvThis, dirPath);
        UP_LEAVE_INT(rc);
        return rc;
    }

    
    NS_IMETHOD GetClassPath(const char* *result)
    {
        UP_ENTER_RC();
        dprintf(("%s: result=%p", pszFunction, result));
        rc = VFTCALL1((VFTnsIJVMPlugin*)mpvVFTable, GetClassPath, mpvThis, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            DPRINTF_STR(*result);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetJavaWrapper(JNIEnv* jenv, jint obj, jobject *jobj)
    {
        UP_ENTER_RC();
        dprintf(("%s: jenv=%p obj=%p jobj=%p", pszFunction, jenv, obj, jobj));
        rc = VFTCALL3((VFTnsIJVMPlugin*)mpvVFTable, GetJavaWrapper, mpvThis, jenv, obj, jobj);
        if (NS_SUCCEEDED(rc) && VALID_PTR(jobj))
            dprintf(("%s: *jobj=%x", pszFunction, jobj));
        UP_LEAVE_INT(rc);
        return rc;
    }

    






    NS_IMETHOD CreateSecureEnv(JNIEnv* proxyEnv, nsISecureEnv* *outSecureEnv)
    {
        UP_ENTER_RC();
        dprintf(("%s: proxyEnv=%x outSecureEnv=%x", pszFunction, proxyEnv, outSecureEnv));
        JNIEnv *pdownProxyEnv = proxyEnv;
        rc = downCreateJNIEnvWrapper(&pdownProxyEnv, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            rc = VFTCALL2((VFTnsIJVMPlugin*)mpvVFTable, CreateSecureEnv, mpvThis, pdownProxyEnv, outSecureEnv);
            rc = upCreateWrapper((void**)outSecureEnv, kSecureEnvIID, rc);
        }
        else
            dprintf(("%s: Failed to make JNIEnv down wrapper!!!", pszFunction));
        UP_LEAVE_INT(rc);
        return rc;
    }

    



    NS_IMETHOD SpendTime(PRUint32 timeMillis)
    {
        UP_ENTER_RC();
        dprintf(("%s: timeMillis=%d", pszFunction, timeMillis));
        rc = VFTCALL1((VFTnsIJVMPlugin*)mpvVFTable, SpendTime, mpvThis, timeMillis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD UnwrapJavaWrapper(JNIEnv* jenv, jobject jobj, jint* obj)
    {
        UP_ENTER_RC();
        dprintf(("%s: jenv=%p jobj=%p obj=%p", pszFunction, jenv, jobj, obj));
        rc = VFTCALL3((VFTnsIJVMPlugin*)mpvVFTable, UnwrapJavaWrapper, mpvThis, jenv, jobj, obj);
        if (NS_SUCCEEDED(rc) && VALID_PTR(jobj))
            dprintf(("%s: *jobj=%p", pszFunction, jobj));
        UP_LEAVE_INT(rc);
        return rc;
    }


    


    UpJVMPlugin(void *pvThis) :
        UpSupportsBase(pvThis, (nsIJVMPlugin*)this, kJVMPluginIID)
    {
    }

};








class UpJVMPluginInstance : public nsIJVMPluginInstance, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();


    
    NS_IMETHOD GetJavaObject(jobject *result)
    {
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsIJVMPluginInstance*)mpvVFTable, GetJavaObject, mpvThis, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    
    NS_IMETHOD GetText(const char ** result)
    {
        
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsIJVMPluginInstance*)mpvVFTable, GetText, mpvThis, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result) && VALID_PTR(*result))
            DPRINTF_STR(*result);
        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpJVMPluginInstance(void *pvThis) :
        UpSupportsBase(pvThis, (nsIJVMWindow*)this, kJVMPluginInstanceIID)
    {
    }

};










class UpSecureEnv : public nsISecureEnv, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    








    NS_IMETHOD NewObject(  jclass clazz,
                           jmethodID methodID,
                           jvalue *args,
                          jobject* result,
                           nsISecurityContext* ctx = NULL)
    {
        UP_ENTER_RC();
        dprintf(("%s: clazz=%p methodID=%p args=%p result=%p ctx=%p", pszFunction, clazz, methodID, args, result, ctx));

        nsISecurityContext *pDownCtx = ctx;
        rc = downCreateWrapper((void**)&pDownCtx, downIsSupportedInterface(kSecurityContextIID), NS_OK);
        if (rc == NS_OK)
        {
            rc = VFTCALL5((VFTnsISecureEnv*)mpvVFTable, NewObject, mpvThis, clazz, methodID, args, result, pDownCtx);
            if (NS_SUCCEEDED(rc) && VALID_PTR(result))
                dprintf(("%s: *result=%p", pszFunction, result));
        }
        UP_LEAVE_INT(rc);
        return rc;
    }

    









    NS_IMETHOD CallMethod(  jni_type type,
                            jobject obj,
                            jmethodID methodID,
                            jvalue *args,
                           jvalue* result,
                            nsISecurityContext* ctx = NULL)
    {
        UP_ENTER_RC();
        dprintf(("%s: type=%x obj=%p methodID=%p args=%p result=%p ctx=%p", pszFunction, type, obj, methodID, args, result, ctx));

        nsISecurityContext *pDownCtx = ctx;
        rc = downCreateWrapper((void**)&pDownCtx, downIsSupportedInterface(kSecurityContextIID), NS_OK);
        if (rc == NS_OK)
        {
            rc = VFTCALL6((VFTnsISecureEnv*)mpvVFTable, CallMethod, mpvThis, type, obj, methodID, args, result, pDownCtx);
            if (NS_SUCCEEDED(rc) && VALID_PTR(result))
                dprintf(("%s: *result=0x%08x", pszFunction, *(int*)result));
            #ifdef DO_EXTRA_FAILURE_HANDLING
            else if (VALID_PTR(result))
                ZERO_JAVAVALUE(*result, type);
            #endif
        }
        UP_LEAVE_INT(rc);
        return rc;
    }


    










    NS_IMETHOD CallNonvirtualMethod(  jni_type type,
                                      jobject obj,
                                      jclass clazz,
                                      jmethodID methodID,
                                      jvalue *args,
                                     jvalue* result,
                                      nsISecurityContext* ctx = NULL)
    {
        UP_ENTER_RC();
        dprintf(("%s: type=%x obj=%p clazz=%p methodID=%p args=%p result=%p ctx=%p", pszFunction, type, obj, clazz, methodID, args, result, ctx));

        nsISecurityContext *pDownCtx = ctx;
        rc = downCreateWrapper((void**)&pDownCtx, downIsSupportedInterface(kSecurityContextIID), NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            rc = VFTCALL7((VFTnsISecureEnv*)mpvVFTable, CallNonvirtualMethod, mpvThis, type, obj, clazz, methodID, args, result, pDownCtx);
            if (NS_SUCCEEDED(rc) && VALID_PTR(result))
                dprintf(("%s: *result=0x%08x (int)", pszFunction, *(int*)result));
            #ifdef DO_EXTRA_FAILURE_HANDLING
            else if (VALID_PTR(result))
                ZERO_JAVAVALUE(*result, type);
            #endif
        }
        UP_LEAVE_INT(rc);
        return rc;
    }

    








    NS_IMETHOD GetField(  jni_type type,
                          jobject obj,
                          jfieldID fieldID,
                         jvalue* result,
                          nsISecurityContext* ctx = NULL)
    {
        UP_ENTER_RC();
        dprintf(("%s: type=%x obj=%p fieldID= result=%p ctx=%p", pszFunction, type, fieldID, result, ctx));

        nsISecurityContext *pDownCtx = ctx;
        rc = downCreateWrapper((void**)&pDownCtx, downIsSupportedInterface(kSecurityContextIID), NS_OK);
        if (rc == NS_OK)
        {
            rc = VFTCALL5((VFTnsISecureEnv*)mpvVFTable, GetField, mpvThis, type, obj, fieldID, result, pDownCtx);
            if (NS_SUCCEEDED(rc) && VALID_PTR(result))
                dprintf(("%s: *result=0x%08x (int)", pszFunction, *(int*)result));
        }
        UP_LEAVE_INT(rc);
        return rc;
    }

    








    NS_IMETHOD SetField( jni_type type,
                         jobject obj,
                         jfieldID fieldID,
                         jvalue val,
                         nsISecurityContext* ctx = NULL)
    {
        UP_ENTER_RC();
        dprintf(("%s: type=%x, obj=%p fieldID=%p val=0x%08x (int) ctx=%p", pszFunction, type, obj, fieldID, val.i, ctx));

        nsISecurityContext *pDownCtx = ctx;
        rc = downCreateWrapper((void**)&pDownCtx, downIsSupportedInterface(kSecurityContextIID), NS_OK);
        if (rc == NS_OK)
            rc = VFTCALL5((VFTnsISecureEnv*)mpvVFTable, SetField, mpvThis, type, obj, fieldID, val, pDownCtx);
        UP_LEAVE_INT(rc);
        return rc;
    }


    









    NS_IMETHOD CallStaticMethod(  jni_type type,
                                  jclass clazz,
                                  jmethodID methodID,
                                  jvalue *args,
                                 jvalue* result,
                                  nsISecurityContext* ctx = NULL)
    {
        UP_ENTER_RC();
        dprintf(("%s: type=%d clazz=%#x methodID=%#x args=%p result=%p ctx=%p", pszFunction, type, clazz, methodID, args, result,  ctx));

        nsISecurityContext *pDownCtx = ctx;
        rc = downCreateWrapper((void**)&pDownCtx, downIsSupportedInterface(kSecurityContextIID), NS_OK);
        if (rc == NS_OK)
        {
            rc = VFTCALL6((VFTnsISecureEnv*)mpvVFTable, CallStaticMethod, mpvThis, type, clazz, methodID, args, result, pDownCtx);
            if (NS_SUCCEEDED(rc) && VALID_PTR(result))
                dprintf(("%s: *result=0x%08x (int)", pszFunction, *(int*)result));
            #ifdef DO_EXTRA_FAILURE_HANDLING
            else if (VALID_PTR(result))
                ZERO_JAVAVALUE(*result, type);
            #endif
        }
        UP_LEAVE_INT(rc);
        return rc;
    }

    








    NS_IMETHOD GetStaticField(  jni_type type,
                                jclass clazz,
                                jfieldID fieldID,
                               jvalue* result,
                                nsISecurityContext* ctx = NULL)
    {
        UP_ENTER_RC();
        dprintf(("%s: type=%d clazz=%#x fieldID=%#x result=%p ctx=%p", pszFunction, type, clazz, fieldID, result,  ctx));

        nsISecurityContext *pDownCtx = ctx;
        rc = downCreateWrapper((void**)&pDownCtx, kSecurityContextIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            rc = VFTCALL5((VFTnsISecureEnv*)mpvVFTable, GetStaticField, mpvThis, type, clazz, fieldID, result, pDownCtx);
            if (NS_SUCCEEDED(rc) && VALID_PTR(result))
                dprintf(("%s: *result=0x%08x (int)", pszFunction, *(int*)result));
        }

        UP_LEAVE_INT(rc);
        return rc;
    }


    








    NS_IMETHOD SetStaticField( jni_type type,
                               jclass clazz,
                               jfieldID fieldID,
                               jvalue val,
                               nsISecurityContext* ctx = NULL)
    {
        UP_ENTER_RC();
        dprintf(("%s: type=%x, clazz=%p fieldID=%p val=0x%08x (int) ctx=%p", pszFunction, type, clazz, fieldID, val.i, ctx));

        nsISecurityContext *pDownCtx = ctx;
        rc = downCreateWrapper((void**)&pDownCtx, downIsSupportedInterface(kSecurityContextIID), NS_OK);
        if (rc == NS_OK)
            rc = VFTCALL5((VFTnsISecureEnv*)mpvVFTable, SetStaticField, mpvThis, type, clazz, fieldID, val, pDownCtx);
        UP_LEAVE_INT(rc);
        return rc;
    }


    NS_IMETHOD GetVersion( jint* version)
    {
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsISecureEnv*)mpvVFTable, GetVersion, mpvThis, version);
        if (NS_SUCCEEDED(rc) && VALID_PTR(version))
            dprintf(("%s: *version=%d", pszFunction, *version));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD DefineClass(  const char* name,
                             jobject loader,
                             const jbyte *buf,
                             jsize len,
                            jclass* clazz)
    {
        UP_ENTER_RC();
        DPRINTF_STR(name);
        rc = VFTCALL5((VFTnsISecureEnv*)mpvVFTable, DefineClass, mpvThis, name, loader, buf, len, clazz);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD FindClass(  const char* name,
                          jclass* clazz)
    {
        UP_ENTER_RC();
        DPRINTF_STR(name);
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, FindClass, mpvThis, name, clazz);
        if (NS_SUCCEEDED(rc) && VALID_PTR(clazz))
            dprintf(("%s: *clazz=%p", pszFunction, *clazz));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetSuperclass(  jclass sub,
                              jclass* super)
    {
        UP_ENTER_RC();
        dprintf(("%s: sub=%p super=%p", pszFunction, sub, super));
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, GetSuperclass, mpvThis, sub, super);
        if (NS_SUCCEEDED(rc) && VALID_PTR(super))
            dprintf(("%s: *super=%p", pszFunction, *super));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD IsAssignableFrom(  jclass sub,
                                  jclass super,
                                 jboolean* result)
    {
        UP_ENTER_RC();
        dprintf(("%s: sub=%p super=%p result=%p", pszFunction, sub, super, result));
        rc = VFTCALL3((VFTnsISecureEnv*)mpvVFTable, IsAssignableFrom, mpvThis, sub, super, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%d", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }


    NS_IMETHOD Throw(  jthrowable obj,
                      jint* result)
    {
        UP_ENTER_RC();
        dprintf(("%s: obj=%p result=%p", pszFunction, obj, result));
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, Throw, mpvThis, obj, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD ThrowNew(  jclass clazz,
                          const char *msg,
                         jint* result)
    {
        
        UP_ENTER_RC();
        dprintf(("%s: clazz=%p msg=%p result=%p", pszFunction, clazz, msg, result));
        DPRINTF_STR(msg);
        rc = VFTCALL3((VFTnsISecureEnv*)mpvVFTable, ThrowNew, mpvThis, clazz, msg, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD ExceptionOccurred( jthrowable* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsISecureEnv*)mpvVFTable, ExceptionOccurred, mpvThis, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD ExceptionDescribe(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsISecureEnv*)mpvVFTable, ExceptionDescribe, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD ExceptionClear(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsISecureEnv*)mpvVFTable, ExceptionClear, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD FatalError( const char* msg)
    {
        UP_ENTER_RC();
        DPRINTF_STR(msg);
        rc = VFTCALL1((VFTnsISecureEnv*)mpvVFTable, FatalError, mpvThis, msg);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD NewGlobalRef(  jobject lobj,
                             jobject* result)
    {
        UP_ENTER_RC();
        dprintf(("%s: lobj=%p result=%p", pszFunction, lobj, result));
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, NewGlobalRef, mpvThis, lobj, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD DeleteGlobalRef( jobject gref)
    {
        UP_ENTER_RC();
        dprintf(("%s: gref=%p", pszFunction, gref));
        rc = VFTCALL1((VFTnsISecureEnv*)mpvVFTable, DeleteGlobalRef, mpvThis, gref);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD DeleteLocalRef( jobject obj)
    {
        UP_ENTER_RC();
        dprintf(("%s: obj=%p", pszFunction, obj));
        rc = VFTCALL1((VFTnsISecureEnv*)mpvVFTable, DeleteLocalRef, mpvThis, obj);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD IsSameObject(  jobject obj1,
                              jobject obj2,
                             jboolean* result)
    {
        UP_ENTER_RC();
        dprintf(("%s: obj1=%p obj2=%p result=%p", pszFunction, obj1, obj2, result));
        rc = VFTCALL3((VFTnsISecureEnv*)mpvVFTable, IsSameObject, mpvThis, obj1, obj2, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%d", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD AllocObject(  jclass clazz,
                            jobject* result)
    {
        UP_ENTER_RC();
        dprintf(("%s: clazz=%p result=%p", pszFunction, clazz, result));
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, AllocObject, mpvThis, clazz, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetObjectClass(  jobject obj,
                               jclass* result)
    {
        UP_ENTER_RC();
        dprintf(("%s: obj=%p result=%p", pszFunction, obj, result));
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, GetObjectClass, mpvThis, obj, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=0x%08x (int)", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD IsInstanceOf(  jobject obj,
                              jclass clazz,
                             jboolean* result)
    {
        UP_ENTER_RC();
        dprintf(("%s: obj=%p clazz=%p result=%p", pszFunction, obj, clazz, result));
        rc = VFTCALL3((VFTnsISecureEnv*)mpvVFTable, IsInstanceOf, mpvThis, obj, clazz, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%d", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetMethodID(  jclass clazz,
                             const char* name,
                             const char* sig,
                            jmethodID* id)
    {
        UP_ENTER_RC();
        dprintf(("%s: clazz=%#x name=%p sig=%p id=%p", pszFunction, clazz, name, sig, id));
        DPRINTF_STR(name);
        DPRINTF_STR(sig);
        rc = VFTCALL4((VFTnsISecureEnv*)mpvVFTable, GetMethodID, mpvThis, clazz, name, sig, id);
        if (NS_SUCCEEDED(rc) && VALID_PTR(id))
            dprintf(("%s: *id=%#x", pszFunction, *id));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetFieldID(  jclass clazz,
                            const char* name,
                            const char* sig,
                           jfieldID* id)
    {
        UP_ENTER_RC();
        dprintf(("%s: clazz=%#x id=%p", pszFunction, clazz, id));
        DPRINTF_STR(name);
        DPRINTF_STR(sig);
        rc = VFTCALL4((VFTnsISecureEnv*)mpvVFTable, GetFieldID, mpvThis, clazz, name, sig, id);
        if (NS_SUCCEEDED(rc) && VALID_PTR(id))
            dprintf(("%s: *id=%#x", pszFunction, *id));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetStaticMethodID(  jclass clazz,
                                   const char* name,
                                   const char* sig,
                                  jmethodID* id)
    {
        UP_ENTER_RC();
        dprintf(("%s: clazz=%#x id=%p", pszFunction, clazz, id));
        DPRINTF_STR(name);
        DPRINTF_STR(sig);
        rc = VFTCALL4((VFTnsISecureEnv*)mpvVFTable, GetStaticMethodID, mpvThis, clazz, name, sig, id);
        if (NS_SUCCEEDED(rc) && VALID_PTR(id))
            dprintf(("%s: *id=%#x", pszFunction, *id));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetStaticFieldID(  jclass clazz,
                                  const char* name,
                                  const char* sig,
                                 jfieldID* id)
    {
        UP_ENTER_RC();
        dprintf(("%s: clazz=%#x id=%p", pszFunction, clazz, id));
        DPRINTF_STR(name);
        DPRINTF_STR(sig);
        rc = VFTCALL4((VFTnsISecureEnv*)mpvVFTable, GetStaticFieldID, mpvThis, clazz, name, sig, id);
        if (NS_SUCCEEDED(rc) && VALID_PTR(id))
            dprintf(("%s: *id=%#x", pszFunction, *id));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD NewString(  const jchar* unicode,
                           jsize len,
                          jstring* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL3((VFTnsISecureEnv*)mpvVFTable, NewString, mpvThis, unicode, len, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetStringLength(  jstring str,
                                jsize* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, GetStringLength, mpvThis, str, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%d", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetStringChars(  jstring str,
                                jboolean *isCopy,
                               const jchar** result)
    {
        UP_ENTER_RC();
        rc = VFTCALL3((VFTnsISecureEnv*)mpvVFTable, GetStringChars, mpvThis, str, isCopy, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD ReleaseStringChars(  jstring str,
                                    const jchar *chars)
    {
        UP_ENTER_RC();
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, ReleaseStringChars, mpvThis, str, chars);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD NewStringUTF(  const char *utf,
                             jstring* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, NewStringUTF, mpvThis, utf, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetStringUTFLength(  jstring str,
                                   jsize* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, GetStringUTFLength, mpvThis, str, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%d", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetStringUTFChars(  jstring str,
                                   jboolean *isCopy,
                                  const char** result)
    {
        UP_ENTER_RC();
        dprintf(("%s: str=%p isCopy=%p result=%p", pszFunction, str, isCopy, result));
        rc = VFTCALL3((VFTnsISecureEnv*)mpvVFTable, GetStringUTFChars, mpvThis, str, isCopy, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD ReleaseStringUTFChars(  jstring str,
                                       const char *chars)
    {
        UP_ENTER_RC();
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, ReleaseStringUTFChars, mpvThis, str, chars);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetArrayLength(  jarray array,
                               jsize* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, GetArrayLength, mpvThis, array, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%d", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD NewObjectArray(  jsize len,
                                          jclass clazz,
                          jobject init,
                         jobjectArray* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL4((VFTnsISecureEnv*)mpvVFTable, NewObjectArray, mpvThis, len, clazz, init, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetObjectArrayElement(  jobjectArray array,
                                       jsize index,
                                      jobject* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL3((VFTnsISecureEnv*)mpvVFTable, GetObjectArrayElement, mpvThis, array, index, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD SetObjectArrayElement(  jobjectArray array,
                                       jsize index,
                                       jobject val)
    {
        UP_ENTER_RC();
        rc = VFTCALL3((VFTnsISecureEnv*)mpvVFTable, SetObjectArrayElement, mpvThis, array, index, val);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD NewArray( jni_type element_type,
                          jsize len,
                         jarray* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL3((VFTnsISecureEnv*)mpvVFTable, NewArray, mpvThis, element_type, len, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetArrayElements(  jni_type type,
                                  jarray array,
                                  jboolean *isCopy,
                                 void* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL4((VFTnsISecureEnv*)mpvVFTable, GetArrayElements, mpvThis, type, array, isCopy, result);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD ReleaseArrayElements( jni_type type,
                                     jarray array,
                                     void *elems,
                                     jint mode)
    {
        UP_ENTER_RC();
        rc = VFTCALL4((VFTnsISecureEnv*)mpvVFTable, ReleaseArrayElements, mpvThis, type, array, elems, mode);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetArrayRegion(  jni_type type,
                                jarray array,
                                jsize start,
                                jsize len,
                               void* buf)
    {
        UP_ENTER_RC();
        rc = VFTCALL5((VFTnsISecureEnv*)mpvVFTable, GetArrayRegion, mpvThis, type, array, start, len, buf);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD SetArrayRegion(  jni_type type,
                                jarray array,
                                jsize start,
                                jsize len,
                                void* buf)
    {
        UP_ENTER_RC();
        rc = VFTCALL5((VFTnsISecureEnv*)mpvVFTable, SetArrayRegion, mpvThis, type, array, start, len, buf);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD RegisterNatives(  jclass clazz,
                                 const JNINativeMethod *methods,
                                 jint nMethods,
                                jint* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL4((VFTnsISecureEnv*)mpvVFTable, RegisterNatives, mpvThis, clazz, methods, nMethods, result);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD UnregisterNatives(  jclass clazz,
                                  jint* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, UnregisterNatives, mpvThis, clazz, result);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD MonitorEnter(  jobject obj,
                             jint* result)
    {
        UP_ENTER_RC();
        dprintf(("%s: obj=%p result=%p", pszFunction, obj, result));
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, MonitorEnter, mpvThis, obj, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD MonitorExit(  jobject obj,
                            jint* result)
    {
        UP_ENTER_RC();
        dprintf(("%s: obj=%p result=%p", pszFunction, obj, result));
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, MonitorExit, mpvThis, obj, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%p", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetJavaVM(  JavaVM **vm,
                          jint* result)
    {
        UP_ENTER_RC();
        rc = VFTCALL2((VFTnsISecureEnv*)mpvVFTable, GetJavaVM, mpvThis, vm, result);
        UP_LEAVE_INT(rc);
        return rc;
    }

    


    UpSecureEnv(void *pvThis) :
        UpSupportsBase(pvThis, (nsISecureEnv*)this, kSecureEnvIID)
    {
    }

};






class UpPluginInstance : public nsIPluginInstance, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    






    
    NS_IMETHOD Initialize(nsIPluginInstancePeer *aPeer)
    {
        UP_ENTER_RC();
        nsIPluginInstancePeer *pDownPeer = aPeer;
        rc = downCreateWrapper((void**)&pDownPeer, kPluginInstancePeerIID, NS_OK);
        if (rc == NS_OK)
            rc = VFTCALL1((VFTnsIPluginInstance*)mpvVFTable, Initialize, mpvThis, pDownPeer);
        UP_LEAVE_INT(rc);
        return rc;
    }

    








    
    NS_IMETHOD GetPeer(nsIPluginInstancePeer * *aPeer)
    {
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsIPluginInstance*)mpvVFTable, GetPeer, mpvThis, aPeer);
        rc = upCreateWrapper((void**)aPeer, kPluginInstancePeerIID, rc);
        UP_LEAVE_INT(rc);
        return rc;
    }


    







    
    NS_IMETHOD Start(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIPluginInstance*)mpvVFTable, Start, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    







    
    NS_IMETHOD Stop(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIPluginInstance*)mpvVFTable, Stop, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    







    
    NS_IMETHOD Destroy(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIPluginInstance*)mpvVFTable, Destroy, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    







    
    NS_IMETHOD SetWindow(nsPluginWindow * aWindow)
    {
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsIPluginInstance*)mpvVFTable, SetWindow, mpvThis, aWindow);
        UP_LEAVE_INT(rc);
        return rc;
    }

    








    
    NS_IMETHOD NewStream(nsIPluginStreamListener **aListener)
    {
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsIPluginInstance*)mpvVFTable, NewStream, mpvThis, aListener);
        rc = upCreateWrapper((void**)aListener, kPluginStreamListenerIID, rc);
        UP_LEAVE_INT(rc);
        return rc;
    }

    







    
    NS_IMETHOD Print(nsPluginPrint * aPlatformPrint)
    {
        UP_ENTER_RC();
        dprintf(("%s: aPlatformPrint=%p", pszFunction, aPlatformPrint));
        if (aPlatformPrint)
        {
            if (aPlatformPrint->mode == nsPluginMode_Embedded)
                dprintf(("%s: Embed: platformPrint=%08x windows: windows=%08x, (x,y,width,height)=(%d,%d,%d,%d) type=%d",
                         pszFunction,
                         aPlatformPrint->print.embedPrint.platformPrint,
                         aPlatformPrint->print.embedPrint.window.window,
                         aPlatformPrint->print.embedPrint.window.x,
                         aPlatformPrint->print.embedPrint.window.y,
                         aPlatformPrint->print.embedPrint.window.width,
                         aPlatformPrint->print.embedPrint.window.height,
                         aPlatformPrint->print.embedPrint.window.type));
            else if (aPlatformPrint->mode == nsPluginMode_Full)
                dprintf(("%s: Full: platformPrint=%08x pluginPrinted=%d printOne=%d",
                         pszFunction,
                         aPlatformPrint->print.fullPrint.platformPrint,
                         aPlatformPrint->print.fullPrint.pluginPrinted,
                         aPlatformPrint->print.fullPrint.printOne));
            else
                dprintf(("%s: Unknown mode!", pszFunction));
        }

        rc = VFTCALL1((VFTnsIPluginInstance*)mpvVFTable, Print, mpvThis, aPlatformPrint);
        UP_LEAVE_INT(rc);
        return rc;
    }

    






    
    NS_IMETHOD GetValue(nsPluginInstanceVariable aVariable, void * aValue)
    {
        UP_ENTER_RC();
        dprintf(("%s: aVariable=%d, aValue=%p", pszFunction, aVariable, aValue));
        rc = VFTCALL2((VFTnsIPluginInstance*)mpvVFTable, GetValue, mpvThis, aVariable, aValue);
        if (VALID_PTR(aValue))
            dprintf(("%s: *aValue=%p", pszFunction, *(void**)aValue));
        UP_LEAVE_INT(rc);
        return rc;
    }

    
















    
    NS_IMETHOD HandleEvent(nsPluginEvent * aEvent, PRBool *aHandled)
    {
        UP_ENTER_RC();
        dprintf(("%s: aEvent=%p, aHandled=%p", pszFunction, aEvent, aHandled));
        rc = VFTCALL2((VFTnsIPluginInstance*)mpvVFTable, HandleEvent, mpvThis, aEvent, aHandled);
        if (VALID_PTR(aHandled))
            dprintf(("%s: *aHandled=%d", pszFunction, *aHandled));
        UP_LEAVE_INT(rc);
        return rc;
    }


    


    UpPluginInstance(void *pvThis) :
        UpSupportsBase(pvThis, (nsIPluginInstance*)this, kPluginInstanceIID)
    {
    }

    


    ~UpPluginInstance()
    {
    }
};










class UpPluginInstancePeerBase : public UpSupportsBase
{
protected:
    








    
    NS_IMETHOD hlpGetValue(nsPluginInstancePeerVariable aVariable, void * aValue)
    {
        UP_ENTER_RC();
        dprintf(("%s: aVariable=%d, aValue=%p", pszFunction, aVariable, aValue));
        rc = VFTCALL2((VFTnsIPluginInstancePeer*)mpvVFTable, GetValue, mpvThis, aVariable, aValue);
        if (VALID_PTR(aValue))
            dprintf(("%s: *aValue=%p", pszFunction, aValue));
        UP_LEAVE_INT(rc);
        return rc;
    }

    







    
    NS_IMETHOD hlpGetMIMEType(nsMIMEType *aMIMEType)
    {
        UP_ENTER_RC();
        dprintf(("%s: aMIMEType=%p", pszFunction, aMIMEType));
        rc = VFTCALL1((VFTnsIPluginInstancePeer*)mpvVFTable, GetMIMEType, mpvThis, aMIMEType);
        if (NS_SUCCEEDED(rc) && VALID_PTR(aMIMEType) && VALID_PTR(*aMIMEType))
            DPRINTF_STR(*aMIMEType);
        UP_LEAVE_INT(rc);
        return rc;
    }

    








    
    NS_IMETHOD hlpGetMode(nsPluginMode *aMode)
    {
        UP_ENTER_RC();
        dprintf(("%s: aMode=%p", pszFunction, aMode));
        rc = VFTCALL1((VFTnsIPluginInstancePeer*)mpvVFTable, GetMode, mpvThis, aMode);
        if (NS_SUCCEEDED(rc) && VALID_PTR(aMode))
            dprintf(("%s: *aMode=%d", pszFunction, aMode));
        UP_LEAVE_INT(rc);
        return rc;
    }

    












    
    NS_IMETHOD hlpNewStream(nsMIMEType aType, const char *aTarget, nsIOutputStream **aResult)
    {
        
        UP_ENTER_RC();
        dprintf(("%s: aResult=%p", pszFunction, aResult));
        DPRINTF_STR(aTarget);
        DPRINTF_STR(aType);

        rc = VFTCALL3((VFTnsIPluginInstancePeer*)mpvVFTable, NewStream, mpvThis, aType, aTarget, aResult);
        rc = upCreateWrapper((void**)aResult, kOutputStreamIID, rc);

        UP_LEAVE_INT(rc);
        return rc;
    }

    








    
    NS_IMETHOD hlpShowStatus(const char *aMessage)
    {
        
        UP_ENTER_RC();
        DPRINTF_STR(aMessage);
        rc = VFTCALL1((VFTnsIPluginInstancePeer*)mpvVFTable, ShowStatus, mpvThis, aMessage);
        UP_LEAVE_INT(rc);
        return rc;
    }

    






    
    NS_IMETHOD hlpSetWindowSize(PRUint32 aWidth, PRUint32 aHeight)
    {
        UP_ENTER_RC();
        dprintf(("%s: aWidth=%d, aHeight=%d", pszFunction, aWidth, aHeight));
        rc = VFTCALL2((VFTnsIPluginInstancePeer*)mpvVFTable, SetWindowSize, mpvThis, aWidth, aHeight);
        UP_LEAVE_INT(rc);
        return rc;
    }

    







    UpPluginInstancePeerBase(void *pvThis, void *pvInterface, REFNSIID iid) :
        UpSupportsBase(pvThis, pvInterface, iid)
    {
    }
};





class UpPluginInstancePeer : public nsIPluginInstancePeer, public UpPluginInstancePeerBase
{
public:
    UP_IMPL_NSISUPPORTS();
    UP_IMPL_NSIPLUGININSTANCEPEER();

    


    UpPluginInstancePeer(void *pvThis) :
        UpPluginInstancePeerBase(pvThis, (nsIPluginInstancePeer*)this, kPluginInstancePeerIID)
    {
    }


};










class UpPluginInstancePeer2 : public nsIPluginInstancePeer2, public UpPluginInstancePeerBase
{
public:
    UP_IMPL_NSISUPPORTS();
    UP_IMPL_NSIPLUGININSTANCEPEER();


    





    
    NS_IMETHOD GetJSWindow(JSObject * *aJSWindow)
    {
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsIPluginInstancePeer2*)mpvVFTable, GetJSWindow, mpvThis, aJSWindow);
        if (NS_SUCCEEDED(rc) && VALID_PTR(aJSWindow))
        {
            
            
            dprintf(("%s: aJSWindow=%p - JS Wrapping !!!", pszFunction, aJSWindow));
            DebugInt3();
        }
        UP_LEAVE_INT(rc);
        return rc;
    }

    





    
    NS_IMETHOD GetJSThread(PRUint32 *aJSThread)
    {
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsIPluginInstancePeer2*)mpvVFTable, GetJSThread, mpvThis, aJSThread);
        if (NS_SUCCEEDED(rc) && VALID_PTR(aJSThread))
            dprintf(("%s: aJSThread=%u", pszFunction, aJSThread));
        UP_LEAVE_INT(rc);
        return rc;
    }

    





    
    NS_IMETHOD GetJSContext(JSContext * *aJSContext)
    {
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsIPluginInstancePeer2*)mpvVFTable, GetJSContext, mpvThis, aJSContext);
        if (NS_SUCCEEDED(rc) && VALID_PTR(aJSContext))
        {
            
            
            dprintf(("%s: aJSContext=%p - JS Wrapping !!!", pszFunction, aJSContext));
            DebugInt3();
        }
        UP_LEAVE_INT(rc);
        return rc;
    }

    


    NS_IMETHOD InvalidateOwner() { return NS_ERROR_NOT_IMPLEMENTED; }

    


    UpPluginInstancePeer2(void *pvThis) :
        UpPluginInstancePeerBase(pvThis, (nsIPluginInstancePeer2*)this, kPluginInstancePeer2IID)
    {
    }


};





class UpPluginTagInfo : public nsIPluginTagInfo, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    








    
    NS_IMETHOD GetAttributes(PRUint16 & aCount, const char* const* & aNames, const char* const* & aValues)
    {
        UP_ENTER_RC();
        rc = VFTCALL3((VFTnsIPluginTagInfo*)mpvVFTable, GetAttributes, mpvThis, aCount, aNames, aValues);
        if (NS_SUCCEEDED(rc))
        {
            dprintf(("%s: aCount=%d", pszFunction, aCount));
            for (int i = 0; i < aCount; i++)
                dprintf(("%s: aNames[%d]='%s' (%p) aValues[%d]='%s' (%p)", pszFunction,
                         i, aNames[i], aNames[i], i, aValues[i], aValues[i]));
        }
        UP_LEAVE_INT(rc);
        return rc;
    }

    








    
    NS_IMETHOD GetAttribute(const char *aName, const char * *aResult)
    {
        
        UP_ENTER_RC();
        rc = VFTCALL2((VFTnsIPluginTagInfo*)mpvVFTable, GetAttribute, mpvThis, aName, aResult);
        if (NS_SUCCEEDED(rc) && VALID_PTR(aResult) && VALID_PTR(*aResult))
            DPRINTF_STR(*aResult);
        UP_LEAVE_INT(rc);
        return rc;
    }

    


    UpPluginTagInfo(void *pvThis) :
        UpSupportsBase(pvThis, (nsIPluginTagInfo*)pvThis, kPluginTagInfoIID)
    {
    }
};










class UpJVMWindowBase : public UpSupportsBase
{
protected:
    NS_IMETHOD hlpShow(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIJVMWindow*)mpvVFTable, Show, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD hlpHide(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIJVMWindow*)mpvVFTable, Hide, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD hlpIsVisible(PRBool *result)
    {
        UP_ENTER_RC();
        rc = VFTCALL1((VFTnsIJVMWindow*)mpvVFTable, IsVisible, mpvThis, result);
        if (NS_SUCCEEDED(rc) && VALID_PTR(result))
            dprintf(("%s: *result=%d", pszFunction, *result));
        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpJVMWindowBase(void *pvThis, void *pvInterface, REFNSIID aIID) :
        UpSupportsBase(pvThis, pvInterface, aIID)
    {
    }
};




class UpJVMWindow : public nsIJVMWindow, public UpJVMWindowBase
{
public:
    UP_IMPL_NSISUPPORTS();
    UP_IMPL_NSIJVMWINDOW();

    
    UpJVMWindow(void *pvThis) :
        UpJVMWindowBase(pvThis, (nsIJVMWindow*)this, kJVMWindowIID)
    {
    }
};








class UpJVMConsole : public nsIJVMConsole, public UpJVMWindowBase
{
public:
    UP_IMPL_NSISUPPORTS();
    UP_IMPL_NSIJVMWINDOW();


    
    
    
    NS_IMETHOD Print(const char* msg, const char* encodingName = NULL)
    {
        UP_ENTER_RC();
        DPRINTF_STR(msg);
        DPRINTF_STRNULL(encodingName);
        rc = VFTCALL2((VFTnsIJVMConsole*)mpvVFTable, Print, mpvThis, msg, encodingName);
        UP_LEAVE_INT(rc);
        return rc;
    }


    
    UpJVMConsole(void *pvThis) :
        UpJVMWindowBase(pvThis, (nsIJVMWindow*)this, kJVMConsoleIID)
    {
    }
};









class UpEventHandler : public nsIEventHandler, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    
















    
    NS_IMETHOD HandleEvent(nsPluginEvent *aEvent, PRBool *aHandled)
    {
        UP_ENTER_RC();
        rc = VFTCALL2((VFTnsIEventHandler*)mpvVFTable, HandleEvent, mpvThis, aEvent, aHandled);
        if (NS_SUCCEEDED(rc) && VALID_PTR(aHandled))
            dprintf(("%s: *aHandled=%d", pszFunction, *aHandled));
        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpEventHandler(void *pvThis) :
        UpSupportsBase(pvThis, (nsIEventHandler*)this, kEventHandlerIID)
    {
    }
};










class UpRunnable : public nsIRunnable, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    


    NS_IMETHOD Run()
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIRunnable*)mpvVFTable, Run, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpRunnable(void *pvThis) :
        UpSupportsBase(pvThis, (nsIRunnable*)this, kRunnableIID)
    {
    }
};









class UpSecurityContext : public nsISecurityContext, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();


    








    NS_IMETHOD Implies(const char* target, const char* action, PRBool *bAllowedAccess)
    {
        UP_ENTER_RC();
        dprintf(("%s: target=%p action=%p bAllowedAccess=%p", pszFunction, target, action, bAllowedAccess));
        DPRINTF_STR(target);
        DPRINTF_STR(action);
        rc = VFTCALL3((VFTnsISecurityContext*)mpvVFTable, Implies, mpvThis, target, action, bAllowedAccess);
        if (NS_SUCCEEDED(rc) && VALID_PTR(bAllowedAccess))
            dprintf(("%s: *bAllowedAccess=%d", pszFunction, *bAllowedAccess));
        UP_LEAVE_INT(rc);
        return rc;
    }

    







    NS_IMETHOD GetOrigin(char* buf, int len)
    {
        UP_ENTER_RC();
        dprintf(("%s: buf=%p len=%d ", pszFunction, buf, len));
        rc = VFTCALL2((VFTnsISecurityContext*)mpvVFTable, GetOrigin, mpvThis, buf, len);
        if (NS_SUCCEEDED(rc) && VALID_PTR(buf))
            DPRINTF_STR(buf);
        UP_LEAVE_INT(rc);
        return rc;
    }

    







    NS_IMETHOD GetCertificateID(char* buf, int len)
    {
        UP_ENTER_RC();
        dprintf(("%s: buf=%p len=%d ", pszFunction, buf, len));
        rc = VFTCALL2((VFTnsISecurityContext*)mpvVFTable, GetCertificateID, mpvThis, buf, len);
        if (NS_SUCCEEDED(rc) && VALID_PTR(buf))
            DPRINTF_STR(*buf);
        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpSecurityContext(void *pvThis) :
        UpSupportsBase(pvThis, (nsISecurityContext*)this, kSecurityContextIID)
    {
    }
};









class UpRequestObserverBase : public UpSupportsBase
{
protected:
   








    
    NS_IMETHOD hlpOnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
    {
        UP_ENTER_RC();
        dprintf(("%s: aRequest=%p aContext=%p", pszFunction, aRequest, aContext));

        nsIRequest *pDownRequest = aRequest;
        rc = downCreateWrapper((void**)&pDownRequest, kRequestIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            nsISupports *pDownSupports = aContext;
            rc = downCreateWrapper((void**)&pDownSupports, kSupportsIID, NS_OK);
            if (NS_SUCCEEDED(rc))
                rc = VFTCALL2((VFTnsIRequestObserver*)mpvVFTable, OnStartRequest, mpvThis, pDownRequest, pDownSupports);
        }

        UP_LEAVE_INT(rc);
        return rc;
    }

    









    
    NS_IMETHOD hlpOnStopRequest(nsIRequest *aRequest, nsISupports *aContext, nsresult aStatusCode)
    {
        UP_ENTER_RC();
        dprintf(("%s: aRequest=%p aContext=%p aStatusCode", pszFunction, aRequest, aContext, aStatusCode));

        nsIRequest *pDownRequest = aRequest;
        rc = downCreateWrapper((void**)&pDownRequest, kRequestIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            nsISupports *pDownSupports = aContext;
            rc = downCreateWrapper((void**)&pDownSupports, kSupportsIID, NS_OK);
            if (NS_SUCCEEDED(rc))
                rc = VFTCALL3((VFTnsIRequestObserver*)mpvVFTable, OnStopRequest, mpvThis, pDownRequest, pDownSupports, aStatusCode);
        }

        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpRequestObserverBase(void *pvThis, void *pvInterface, REFNSIID aIID) :
        UpSupportsBase(pvThis, pvInterface, aIID)
    {
    }
};





class UpRequestObserver : public nsIRequestObserver, public UpRequestObserverBase
{
 public:
     UP_IMPL_NSISUPPORTS();
     UP_IMPL_NSIREQUESTOBSERVER();

     
     UpRequestObserver(void *pvThis) :
         UpRequestObserverBase(pvThis, (nsIRequestObserver*)this, kRequestObserverIID)
     {
     }
};









class UpStreamListener : public nsIStreamListener, public UpRequestObserverBase
{
public:
    UP_IMPL_NSISUPPORTS();
    UP_IMPL_NSIREQUESTOBSERVER();

    













    
    NS_IMETHOD OnDataAvailable(nsIRequest *aRequest, nsISupports *aContext, nsIInputStream *aInputStream, PRUint32 aOffset, PRUint32 aCount)
    {
        UP_ENTER_RC();
        dprintf(("%s: aRequest=%p aContext=%p aInputStream=%p aOffset=%d aCount=%d", pszFunction, aRequest, aContext, aInputStream, aOffset, aCount));

        nsIRequest *pDownRequest = aRequest;
        rc = downCreateWrapper((void**)&pDownRequest, kRequestIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            nsISupports *pDownSupports = aContext;
            rc = downCreateWrapper((void**)&pDownSupports, kSupportsIID, NS_OK);
            if (NS_SUCCEEDED(rc))
            {
                nsIInputStream *pDownInputStream = aInputStream;
                rc = downCreateWrapper((void**)&pDownInputStream, kInputStreamIID, NS_OK);
                if (NS_SUCCEEDED(rc))
                    rc = VFTCALL5((VFTnsIStreamListener*)mpvVFTable, OnDataAvailable, mpvThis,
                                  pDownRequest, pDownSupports, pDownInputStream, aOffset, aCount);
            }
        }

        UP_LEAVE_INT(rc);
        return rc;
    }


    
    UpStreamListener(void *pvThis) :
        UpRequestObserverBase(pvThis, (nsIStreamListener*)this, kStreamListenerIID)
    {
    }
};









class UpRequestBase : public UpSupportsBase
{
protected:

    


    
    NS_IMETHOD hlpGetName(nsACString & aName)
    {
        UP_ENTER_RC();
        dprintf(("%s: &aName=%p", pszFunction, &aName));
        
        dprintf(("%s: nsACString wrapping isn't supported.", pszFunction));
        ReleaseInt3(0xbaddbeef, 32, 0);

        rc = VFTCALL1((VFTnsIRequest*)mpvVFTable, GetName, mpvThis, aName);

        UP_LEAVE_INT(rc);
        return rc;
    }

    





    
    NS_IMETHOD hlpIsPending(PRBool *_retval)
    {
        UP_ENTER_RC();
        dprintf(("%s: _retval=%p", pszFunction, _retval));

        rc = VFTCALL1((VFTnsIRequest*)mpvVFTable, IsPending, mpvThis, _retval);
        if (VALID_PTR(_retval))
            dprintf(("%s: *_retval=%d", pszFunction, *_retval));

        UP_LEAVE_INT(rc);
        return rc;
    }

    


    
    NS_IMETHOD hlpGetStatus(nsresult *aStatus)
    {
        UP_ENTER_RC();
        dprintf(("%s: aStatus=%p", pszFunction, aStatus));

        rc = VFTCALL1((VFTnsIRequest*)mpvVFTable, GetStatus, mpvThis, aStatus);
        if (VALID_PTR(aStatus))
            dprintf(("%s: *aStatus=%d", pszFunction, *aStatus));

        UP_LEAVE_INT(rc);
        return rc;
    }

    













    
    NS_IMETHOD hlpCancel(nsresult aStatus)
    {
        UP_ENTER_RC();
        dprintf(("%s: aStatus=%p", pszFunction, aStatus));

        rc = VFTCALL1((VFTnsIRequest*)mpvVFTable, Cancel, mpvThis, aStatus);

        UP_LEAVE_INT(rc);
        return rc;
    }

    










    
    NS_IMETHOD hlpSuspend(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIRequest*)mpvVFTable, Suspend, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    




    
    NS_IMETHOD hlpResume(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIRequest*)mpvVFTable, Resume, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    




    
    NS_IMETHOD hlpGetLoadGroup(nsILoadGroup * *aLoadGroup)
    {
        UP_ENTER_RC();
        dprintf(("%s: aLoadGroup=%p\n", pszFunction, aLoadGroup));

        rc = VFTCALL1((VFTnsIRequest*)mpvVFTable, GetLoadGroup, mpvThis, aLoadGroup);
        rc = upCreateWrapper((void**)aLoadGroup, kLoadGroupIID, rc);

        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD hlpSetLoadGroup(nsILoadGroup * aLoadGroup)
    {
        UP_ENTER_RC();
        dprintf(("%s: aLoadGroup=%p\n", pszFunction, aLoadGroup));

        nsILoadGroup *pDownLoadGroup = aLoadGroup;
        rc = downCreateWrapper((void**)&pDownLoadGroup, kLoadGroupIID, NS_OK);
        if (NS_SUCCEEDED(rc))
          rc = VFTCALL1((VFTnsIRequest*)mpvVFTable, SetLoadGroup, mpvThis, pDownLoadGroup);

        UP_LEAVE_INT(rc);
        return rc;
    }

    





    
    NS_IMETHOD hlpGetLoadFlags(nsLoadFlags *aLoadFlags)
    {
        UP_ENTER_RC();
        dprintf(("%s: aLoadFlags=%p\n", pszFunction, aLoadFlags));

        rc = VFTCALL1((VFTnsIRequest*)mpvVFTable, GetLoadFlags, mpvThis, aLoadFlags);
        if (VALID_PTR(aLoadFlags))
            dprintf(("%s: *aLoadFlags=%d", pszFunction, *aLoadFlags));

        UP_LEAVE_INT(rc);
        return rc;
    }


    NS_IMETHOD hlpSetLoadFlags(nsLoadFlags aLoadFlags)
    {
        UP_ENTER_RC();
        dprintf(("%s: aLoadFlags=%#x\n", pszFunction, aLoadFlags));

        rc = VFTCALL1((VFTnsIRequest*)mpvVFTable, SetLoadFlags, mpvThis, aLoadFlags);

        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpRequestBase(void *pvThis, void *pvInterface, REFNSIID aIID) :
        UpSupportsBase(pvThis, pvInterface, aIID)
    {
    }

};





class UpRequest : public nsIRequest, public UpRequestBase
{
public:
     UP_IMPL_NSISUPPORTS();
     UP_IMPL_NSIREQUEST();

     
     UpRequest(void *pvThis) :
         UpRequestBase(pvThis, (nsIRequest*)this, kRequestIID)
     {
     }
};









class UpLoadGroup : public nsILoadGroup, public UpRequestBase
{
public:
    UP_IMPL_NSISUPPORTS();
    UP_IMPL_NSIREQUEST();

    



    
    NS_IMETHOD GetGroupObserver(nsIRequestObserver * *aGroupObserver)
    {
        UP_ENTER_RC();
        dprintf(("%s: aGroupObserver=%p\n", pszFunction, aGroupObserver));

        rc = VFTCALL1((VFTnsILoadGroup*)mpvVFTable, GetGroupObserver, mpvThis, aGroupObserver);
        rc = upCreateWrapper((void**)aGroupObserver, kRequestObserverIID, rc);

        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD SetGroupObserver(nsIRequestObserver * aGroupObserver)
    {
        UP_ENTER_RC();
        dprintf(("%s: aGroupObserver=%p\n", pszFunction, aGroupObserver));

        nsIRequestObserver *pDownGroupObserver = aGroupObserver;
        rc = downCreateWrapper((void**)&pDownGroupObserver, kRequestObserverIID, NS_OK);
        if (NS_SUCCEEDED(rc))
          rc = VFTCALL1((VFTnsILoadGroup*)mpvVFTable, SetGroupObserver, mpvThis, pDownGroupObserver);

        UP_LEAVE_INT(rc);
        return rc;
    }

    








    
    NS_IMETHOD GetDefaultLoadRequest(nsIRequest * *aDefaultLoadRequest)
    {
        UP_ENTER_RC();
        dprintf(("%s: aDefaultLoadRequest=%p\n", pszFunction, aDefaultLoadRequest));

        rc = VFTCALL1((VFTnsILoadGroup*)mpvVFTable, GetDefaultLoadRequest, mpvThis, aDefaultLoadRequest);
        rc = upCreateWrapper((void**)aDefaultLoadRequest, kRequestIID, rc);

        UP_LEAVE_INT(rc);
        return rc;
    }
    NS_IMETHOD SetDefaultLoadRequest(nsIRequest * aDefaultLoadRequest)
    {
        UP_ENTER_RC();
        dprintf(("%s: aDefaultLoadRequest=%p\n", pszFunction, aDefaultLoadRequest));

        nsIRequest *pDownDefaultLoadRequest = aDefaultLoadRequest;
        rc = downCreateWrapper((void**)&pDownDefaultLoadRequest, kRequestIID, NS_OK);
        if (NS_SUCCEEDED(rc))
          rc = VFTCALL1((VFTnsILoadGroup*)mpvVFTable, SetDefaultLoadRequest, mpvThis, pDownDefaultLoadRequest);

        UP_LEAVE_INT(rc);
        return rc;
    }

    








    
    NS_IMETHOD AddRequest(nsIRequest *aRequest, nsISupports *aContext)
    {
        UP_ENTER_RC();
        dprintf(("%s: aRequest=%p aContext=%p", pszFunction, aRequest, aContext));

        nsIRequest *pDownRequest = aRequest;
        rc = downCreateWrapper((void**)&pDownRequest, kRequestIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            nsISupports *pDownSupports = aContext;
            rc = downCreateWrapper((void**)&pDownSupports, kSupportsIID, NS_OK);
            if (NS_SUCCEEDED(rc))
                rc = VFTCALL2((VFTnsILoadGroup*)mpvVFTable, AddRequest, mpvThis, pDownRequest, pDownSupports);
        }

        UP_LEAVE_INT(rc);
        return rc;
    }


    



    
    NS_IMETHOD RemoveRequest(nsIRequest *aRequest, nsISupports *aContext, nsresult aStatus)
    {
        UP_ENTER_RC();
        dprintf(("%s: aRequest=%p aContext=%p aStatus=%d", pszFunction, aRequest, aContext, aStatus));

        nsIRequest *pDownRequest = aRequest;
        rc = downCreateWrapper((void**)&pDownRequest, kRequestIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            nsISupports *pDownSupports = aContext;
            rc = downCreateWrapper((void**)&pDownSupports, kSupportsIID, NS_OK);
            if (NS_SUCCEEDED(rc))
                rc = VFTCALL3((VFTnsILoadGroup*)mpvVFTable, RemoveRequest, mpvThis, pDownRequest, pDownSupports, aStatus);
        }

        UP_LEAVE_INT(rc);
        return rc;
    }

    



    
    NS_IMETHOD GetRequests(nsISimpleEnumerator * *aRequests)
    {
        UP_ENTER_RC();
        dprintf(("%s: aRequests=%p", pszFunction, aRequests));

        rc = VFTCALL1((VFTnsILoadGroup*)mpvVFTable, GetRequests, mpvThis, aRequests);
        rc = upCreateWrapper((void**)aRequests, kSimpleEnumeratorIID, rc);

        UP_LEAVE_INT(rc);
        return rc;
    }

    



    
    NS_IMETHOD GetActiveCount(PRUint32 *aActiveCount)
    {
        UP_ENTER_RC();
        dprintf(("%s: aActiveCount=%p", pszFunction, aActiveCount));

        rc = VFTCALL1((VFTnsILoadGroup*)mpvVFTable, GetActiveCount, mpvThis, aActiveCount);
        if (VALID_PTR(aActiveCount))
            dprintf(("%s: *aActiveCount=%d", pszFunction, *aActiveCount));

        UP_LEAVE_INT(rc);
        return rc;
    }

    


    
    NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor * *aNotificationCallbacks)
    {
        UP_ENTER_RC();
        dprintf(("%s: aNotificationCallbacks=%p", pszFunction, aNotificationCallbacks));

        rc = VFTCALL1((VFTnsILoadGroup*)mpvVFTable, GetNotificationCallbacks, mpvThis, aNotificationCallbacks);
        rc = upCreateWrapper((void**)aNotificationCallbacks, kInterfaceRequestorIID, rc);

        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor * aNotificationCallbacks)
    {
        UP_ENTER_RC();
        dprintf(("%s: aNotificationCallbacks=%p", pszFunction, aNotificationCallbacks));

        nsIInterfaceRequestor *pDownNotificationCallbacks = aNotificationCallbacks;
        rc = downCreateWrapper((void**)&pDownNotificationCallbacks, kInterfaceRequestorIID, rc);
        if (NS_SUCCEEDED(rc))
            rc = VFTCALL1((VFTnsILoadGroup*)mpvVFTable, SetNotificationCallbacks, mpvThis, pDownNotificationCallbacks);

        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpLoadGroup(void *pvThis) :
        UpRequestBase(pvThis, (nsILoadGroup*)this, kLoadGroupIID)
    {
    }

};









class UpSimpleEnumerator : public nsISimpleEnumerator, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    











    
    NS_IMETHOD HasMoreElements(PRBool *_retval)
    {
        UP_ENTER_RC();
        dprintf(("%s: _retval=%p\n", pszFunction, _retval));

        rc = VFTCALL1((VFTnsISimpleEnumerator*)mpvVFTable, HasMoreElements, mpvThis, _retval);
        if (VALID_PTR(_retval))
            dprintf(("%s: *_retval=%d", pszFunction, *_retval));

        UP_LEAVE_INT(rc);
        return rc;
    }

    













    
    NS_IMETHOD GetNext(nsISupports **_retval)
    {
        UP_ENTER_RC();
        dprintf(("%s: _retval=%p", pszFunction, _retval));

        rc = VFTCALL1((VFTnsISimpleEnumerator*)mpvVFTable, GetNext, mpvThis, _retval);
        rc = upCreateWrapper((void**)_retval, kSupportsIID, rc);

        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpSimpleEnumerator(void *pvThis) :
        UpSupportsBase(pvThis, (nsISimpleEnumerator*)this, kSimpleEnumeratorIID)
    {
    }
};









class UpInterfaceRequestor : public nsIInterfaceRequestor, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    









    
    NS_IMETHOD GetInterface(const nsIID & uuid, void * *result)
    {
        UP_ENTER_RC();
        DPRINTF_NSID(uuid);

        rc = VFTCALL2((VFTnsIInterfaceRequestor*)mpvVFTable, GetInterface, mpvThis, uuid, result);
        rc = upCreateWrapper(result, uuid, rc);

        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpInterfaceRequestor(void *pvThis) :
        UpSupportsBase(pvThis, (nsIInterfaceRequestor*)this, kInterfaceRequestorIID)
    {
    }
};











nsresult __cdecl UpOutputStream_nsReadSegmentWrapper(nsReadSegmentFun pfnOrg, void *pvRet,
     nsIOutputStream *aOutStream, void *aClosure, char *aToSegment,
     PRUint32 aFromOffset, PRUint32 aCount, PRUint32 *aReadCount)
{
    DEBUG_FUNCTIONNAME();
    nsresult rc;
    dprintf(("%s: pfnOrg=%p pvRet=%p aOutStream=%p aClosure=%p aToSegment=%p aFromOffset=%d aCount=%d aReadCount=%p\n",
             pszFunction, pfnOrg, pvRet, aOutStream, aClosure, aToSegment, aFromOffset, aCount, aReadCount));

    nsIOutputStream * pupOutStream = aOutStream;
    rc = upCreateWrapper((void**)&pupOutStream, kOutputStreamIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        rc = pfnOrg(pupOutStream, aClosure, aToSegment, aFromOffset, aCount, aReadCount);
        if (VALID_PTR(aReadCount))
            dprintf(("%s: *aReadCount=%d\n", pszFunction, *aReadCount));
    }

    dprintf(("%s: rc=%d\n", pszFunction, rc));
    return rc;
}





class UpOutputStream : public nsIOutputStream, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    





    
    NS_IMETHOD Close(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIOutputStream*)mpvVFTable, Close, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    





    
    NS_IMETHOD Flush(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIOutputStream*)mpvVFTable, Flush, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    











    
    NS_IMETHOD Write(const char *aBuf, PRUint32 aCount, PRUint32 *_retval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aBuf=%p aCount=%d _retval=%p", pszFunction, aBuf, aCount, _retval));
        rc = VFTCALL3((VFTnsIOutputStream*)mpvVFTable, Write, mpvThis, aBuf, aCount, _retval);
        if (VALID_PTR(_retval))
            dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    

















    
    NS_IMETHOD WriteFrom(nsIInputStream *aFromStream, PRUint32 aCount, PRUint32 *_retval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aFromStream=%p aCount=%d _retval=%p", pszFunction, aFromStream, aCount, _retval));

        nsIInputStream *pDownFromStream = aFromStream;
        rc = downCreateWrapper((void**)&pDownFromStream, kInputStreamIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            rc = VFTCALL3((VFTnsIOutputStream*)mpvVFTable, WriteFrom, mpvThis, pDownFromStream, aCount, _retval);
            if (VALID_PTR(_retval))
                dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));
        }
        UP_LEAVE_INT(rc);
        return rc;
    }

    



















    
    NS_IMETHOD WriteSegments(nsReadSegmentFun aReader, void * aClosure, PRUint32 aCount, PRUint32 *_retval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aReader=%p aClosure=%p aCount=%d _retval=%p", pszFunction, aReader, aClosure, aCount, _retval));

        int i;
        char achWrapper[32];

        
        achWrapper[0] = 0x68;
        *((unsigned*)&achWrapper[1]) = (unsigned)aReader;
        i = 5;

#if VFT_VAC365
        
        achWrapper[i++] = 0x89;
        achWrapper[i++] = 0x45;
        achWrapper[i++] = 0x08;
        
        achWrapper[i++] = 0x89;
        achWrapper[i++] = 0x55;
        achWrapper[i++] = 0x0c;
        
        achWrapper[i++] = 0x89;
        achWrapper[i++] = 0x4d;
        achWrapper[i++] = 0x10;
#endif
        
        achWrapper[i] = 0xe8;
        *((unsigned*)&achWrapper[i+1]) = (unsigned)UpOutputStream_nsReadSegmentWrapper - (unsigned)&achWrapper[i+5];
        i += 5;

        
        achWrapper[i++] = 0x83;
        achWrapper[i++] = 0xc4;
        achWrapper[i++] = 0x04;

#if VFT_VAC365
        
        achWrapper[i++] = 0xc3;
#elif VFT_VC60
        
        achWrapper[i++] = 0xc2;
        achWrapper[i++] = 6*4;
        achWrapper[i++] = 0;
#else
#error fixme! neither VFT_VC60 nor VFT_VAC365 was set.
#endif
        achWrapper[i++] = 0xcc;
        achWrapper[i] = 0xcc;

        
        rc = VFTCALL4((VFTnsIOutputStream*)mpvVFTable, WriteSegments, mpvThis,
                      (nsReadSegmentFun)((void*)&achWrapper[0]), aClosure, aCount, _retval);
        if (VALID_PTR(_retval))
            dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));

        UP_LEAVE_INT(rc);
        return rc;
    }

    





    
    NS_IMETHOD IsNonBlocking(PRBool *_retval)
    {
        UP_ENTER_RC();
        dprintf(("%s: _retval=%p", pszFunction, _retval));

        rc = VFTCALL1((VFTnsIOutputStream*)mpvVFTable, IsNonBlocking, mpvThis, _retval);
        if (VALID_PTR(_retval))
            dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));

        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpOutputStream(void *pvThis) :
        UpSupportsBase(pvThis, (nsIOutputStream*)this, kOutputStreamIID)
    {
    }
};










class UpPluginStreamListener : public nsIPluginStreamListener, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    







    
    NS_IMETHOD OnStartBinding(nsIPluginStreamInfo *aPluginInfo)
    {
        UP_ENTER_RC();
        dprintf(("%s: sPluginInfo=%p", pszFunction, aPluginInfo));

        nsIPluginStreamInfo *pDownPluginInfo = aPluginInfo;
        rc = downCreateWrapper((void**)&pDownPluginInfo, kPluginStreamInfoIID, NS_OK);
        if (NS_SUCCEEDED(rc))
            rc = VFTCALL1((VFTnsIPluginStreamListener*)mpvVFTable, OnStartBinding, mpvThis, pDownPluginInfo);

        UP_LEAVE_INT(rc);
        return rc;
    }

    










    
    NS_IMETHOD OnDataAvailable(nsIPluginStreamInfo *aPluginInfo, nsIInputStream *aInputStream, PRUint32 aLength)
    {
        UP_ENTER_RC();
        dprintf(("%s: sPluginInfo=%p aInputStream=%p aLength=%d", pszFunction, aPluginInfo, aInputStream, aLength));

        nsIPluginStreamInfo *pDownPluginInfo = aPluginInfo;
        rc = downCreateWrapper((void**)&pDownPluginInfo, kPluginStreamInfoIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            nsIInputStream *pDownInputStream = aInputStream;
            rc = downCreateWrapper((void**)&pDownInputStream, kInputStreamIID, NS_OK);
            if (NS_SUCCEEDED(rc))
                rc = VFTCALL3((VFTnsIPluginStreamListener*)mpvVFTable, OnDataAvailable, mpvThis, pDownPluginInfo, pDownInputStream, aLength);
        }

        UP_LEAVE_INT(rc);
        return rc;
    }


    






    
    NS_IMETHOD OnFileAvailable(nsIPluginStreamInfo *aPluginInfo, const char *aFileName)
    {
        UP_ENTER_RC();
        
        dprintf(("%s: sPluginInfo=%p aFileName=%p", pszFunction, aPluginInfo, aFileName));
        DPRINTF_STR(aFileName);

        nsIPluginStreamInfo *pDownPluginInfo = aPluginInfo;
        rc = downCreateWrapper((void**)&pDownPluginInfo, kPluginStreamInfoIID, NS_OK);
        if (NS_SUCCEEDED(rc))
            rc = VFTCALL2((VFTnsIPluginStreamListener*)mpvVFTable, OnFileAvailable, mpvThis, pDownPluginInfo, aFileName);

        UP_LEAVE_INT(rc);
        return rc;
    }


    










    
    NS_IMETHOD OnStopBinding(nsIPluginStreamInfo *aPluginInfo, nsresult aStatus)
    {
        UP_ENTER_RC();
        dprintf(("%s: sPluginInfo=%p aStatus=%d", pszFunction, aPluginInfo, aStatus));

        nsIPluginStreamInfo *pDownPluginInfo = aPluginInfo;
        rc = downCreateWrapper((void**)&pDownPluginInfo, kPluginStreamInfoIID, NS_OK);
        if (NS_SUCCEEDED(rc))
            rc = VFTCALL2((VFTnsIPluginStreamListener*)mpvVFTable, OnStopBinding, mpvThis, pDownPluginInfo, aStatus);

        UP_LEAVE_INT(rc);
        return rc;
    }

    




    
    NS_IMETHOD GetStreamType(nsPluginStreamType *aStreamType)
    {
        UP_ENTER_RC();
        dprintf(("%s: aStreamType=%p", pszFunction, aStreamType));

        rc = VFTCALL1((VFTnsIPluginStreamListener*)mpvVFTable, GetStreamType, mpvThis, aStreamType);
        if (VALID_PTR(aStreamType))
            dprintf(("%s: *aStreamType=%d\n", pszFunction, aStreamType));

        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpPluginStreamListener(void *pvThis) :
        UpSupportsBase(pvThis, (nsIPluginStreamListener*)this, kPluginStreamListenerIID)
    {
    }
};











class UpFlashIObject7 : public FlashIObject7, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    







    
    NS_IMETHOD Evaluate(const char *aString, FlashIObject7 **aFlashObject)
    {
        UP_ENTER_RC();
        dprintf(("%s: aString=%p aFlashObject=%p", pszFunction, aString, aFlashObject));
        DPRINTF_STR(aString);

        rc = VFTCALL2((VFTFlashIObject7*)mpvVFTable, Evaluate, mpvThis, aString, aFlashObject);
        rc = upCreateWrapper((void**)aFlashObject, kFlashIObject7IID, NS_OK);

        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpFlashIObject7(void *pvThis) :
        UpSupportsBase(pvThis, (FlashIObject7*)this, kFlashIObject7IID)
    {
    }
};











class UpFlashIScriptablePlugin7 : public FlashIScriptablePlugin7, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    NS_IMETHOD IsPlaying(PRBool *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aretval=%p", pszFunction, aretval));
        rc = VFTCALL1((VFTFlashIScriptablePlugin7*)mpvVFTable, IsPlaying, mpvThis, aretval);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Play(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTFlashIScriptablePlugin7*)mpvVFTable, Play, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD StopPlay(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTFlashIScriptablePlugin7*)mpvVFTable, StopPlay, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TotalFrames(PRInt32 *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aretval=%p", pszFunction, aretval));
        rc = VFTCALL1((VFTFlashIScriptablePlugin7*)mpvVFTable, TotalFrames, mpvThis, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD CurrentFrame(PRInt32 *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aretval=%p", pszFunction, aretval));
        rc = VFTCALL1((VFTFlashIScriptablePlugin7*)mpvVFTable, CurrentFrame, mpvThis, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GotoFrame(PRInt32 aFrame)
    {
        UP_ENTER_RC();
        dprintf(("%s: aFrame=%d", pszFunction, aFrame));
        rc = VFTCALL1((VFTFlashIScriptablePlugin7*)mpvVFTable, GotoFrame, mpvThis, aFrame);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Rewind(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTFlashIScriptablePlugin7*)mpvVFTable, Rewind, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Back(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTFlashIScriptablePlugin7*)mpvVFTable, Back, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Forward(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTFlashIScriptablePlugin7*)mpvVFTable, Forward, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Pan(PRInt32 aX, PRInt32 aY, PRInt32 aMode)
    {
        UP_ENTER_RC();
        dprintf(("%s: aX=%d aY=%d aMode=%d", pszFunction, aX, aY, aMode));
        rc = VFTCALL3((VFTFlashIScriptablePlugin7*)mpvVFTable, Pan, mpvThis, aX, aY, aMode);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD PercentLoaded(PRInt32 *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aretval=%p", pszFunction, aretval));
        rc = VFTCALL1((VFTFlashIScriptablePlugin7*)mpvVFTable, PercentLoaded, mpvThis, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD FrameLoaded(PRInt32 aFrame, PRBool *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aFrame=%d aretval=%p", pszFunction, aFrame, aretval));
        rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, FrameLoaded, mpvThis, aFrame, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD FlashVersion(PRInt32 *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aretval=%p", pszFunction, aretval));
        rc = VFTCALL1((VFTFlashIScriptablePlugin7*)mpvVFTable, FlashVersion, mpvThis, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Zoom(PRInt32 aZoom)
    {
        UP_ENTER_RC();
        dprintf(("%s: aZoom=%p", pszFunction, aZoom));
        rc = VFTCALL1((VFTFlashIScriptablePlugin7*)mpvVFTable, Zoom, mpvThis, aZoom);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD SetZoomRect(PRInt32 aLeft, PRInt32 aTop, PRInt32 aRight, PRInt32 aBottom)
    {
        UP_ENTER_RC();
        dprintf(("%s: aLeft=%d aTop=%d aRight=%d aBottom=%d", pszFunction, aLeft, aTop, aRight, aBottom));
        rc = VFTCALL4((VFTFlashIScriptablePlugin7*)mpvVFTable, SetZoomRect, mpvThis, aLeft, aTop, aRight, aBottom);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD LoadMovie(PRInt32 aLayer, PRUnichar *aURL)
    {
        UP_ENTER_RC();
        dprintf(("%s: aLayer=%d aURL=%ls", pszFunction, aLayer, aURL));
        rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, LoadMovie, mpvThis, aLayer, aURL);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TGotoFrame(PRUnichar *aTarget, PRInt32 aFrameNumber)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls aFrameNumber=%d", pszFunction, aTarget, aFrameNumber));
        rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, TGotoFrame, mpvThis, aTarget, aFrameNumber);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TGotoLabel(PRUnichar *aTarget, PRUnichar *aLabel)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls aLabel=%ls", pszFunction, aTarget, aLabel));
        rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, TGotoLabel, mpvThis, aTarget, aLabel);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TCurrentFrame(PRUnichar *aTarget, PRInt32 *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls aretval=%p", pszFunction, aTarget, aretval));
        rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, TCurrentFrame, mpvThis, aTarget, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TCurrentLabel(PRUnichar *aTarget, PRUnichar **aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls aretval=%p", pszFunction, aTarget, aretval));
        rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, TCurrentLabel, mpvThis, aTarget, aretval);
        if (VALID_PTR(aretval) && VALID_PTR(*aretval))
            dprintf(("%s: *aretval=%ls", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TPlay(PRUnichar *aTarget)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls", pszFunction, aTarget));
        rc = VFTCALL1((VFTFlashIScriptablePlugin7*)mpvVFTable, TPlay, mpvThis, aTarget);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TStopPlay(PRUnichar *aTarget)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls", pszFunction, aTarget));
        rc = VFTCALL1((VFTFlashIScriptablePlugin7*)mpvVFTable, TStopPlay, mpvThis, aTarget);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD SetVariable(PRUnichar *aVariable, PRUnichar *aValue)
    {
        UP_ENTER_RC();
        dprintf(("%s: aVariable=%ls aValue=%ls", pszFunction, aVariable, aValue));
        rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, SetVariable, mpvThis, aVariable, aValue);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetVariable(PRUnichar *aVariable, PRUnichar **aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aVariable=%ls aretval=%p", pszFunction, aVariable, aretval));
        rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, GetVariable, mpvThis, aVariable, aretval);
        if (VALID_PTR(aretval) && VALID_PTR(*aretval))
            dprintf(("%s: *aretval=%ls", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TSetProperty(PRUnichar *aTarget, PRInt32 aProperty, PRUnichar *aValue)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls aProperty=%d aValue=%ls", pszFunction, aTarget, aProperty, aValue));
        rc = VFTCALL3((VFTFlashIScriptablePlugin7*)mpvVFTable, TSetProperty, mpvThis, aTarget, aProperty, aValue);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TGetProperty(PRUnichar *aTarget, PRInt32 aProperty, PRUnichar **aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls aProperty=%d aretval=%p", pszFunction, aTarget, aProperty, aretval));
        rc = VFTCALL3((VFTFlashIScriptablePlugin7*)mpvVFTable, TGetProperty, mpvThis, aTarget, aProperty, aretval);
        if (VALID_PTR(aretval) && VALID_PTR(*aretval))
            dprintf(("%s: *aretval=%ls", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TGetPropertyAsNumber(PRUnichar *aTarget, PRInt32 aProperty, double **aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls aProperty=%d aretval=%p", pszFunction, aTarget, aProperty, aretval));
        rc = VFTCALL3((VFTFlashIScriptablePlugin7*)mpvVFTable, TGetPropertyAsNumber, mpvThis, aTarget, aProperty, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%f", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TCallLabel(PRUnichar *aTarget, PRUnichar *aLabel)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls aLabel=%ls", pszFunction, aTarget, aLabel));
        rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, TCallLabel, mpvThis, aTarget, aLabel);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TCallFrame(PRUnichar *aTarget, PRInt32 aFrameNumber)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%ls aFrameNumber=%d", pszFunction, aTarget, aFrameNumber));
        rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, TCallFrame, mpvThis, aTarget, aFrameNumber);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD SetWindow(FlashIObject7 *aFlashObject, PRInt32 a1)
    {
        UP_ENTER_RC();
        dprintf(("%s: aFlashObject=%p a1=%d", pszFunction, aFlashObject, a1));

        FlashIObject7 *pDownFlashObject = aFlashObject;
        rc = downCreateWrapper((void**)&pDownFlashObject, kFlashIObject7IID, NS_OK);
        if (NS_SUCCEEDED(rc))
            rc = VFTCALL2((VFTFlashIScriptablePlugin7*)mpvVFTable, SetWindow, mpvThis, pDownFlashObject, a1);

        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpFlashIScriptablePlugin7(void *pvThis) :
        UpSupportsBase(pvThis, (FlashIScriptablePlugin7*)this, kFlashIScriptablePlugin7IID)
    {
    }
};












class UpFlash5 : public nsIFlash5, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    NS_IMETHOD IsPlaying(PRBool *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aretval=%p", pszFunction, aretval));
        rc = VFTCALL1((VFTnsIFlash5*)mpvVFTable, IsPlaying, mpvThis, aretval);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Play(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIFlash5*)mpvVFTable, Play, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD StopPlay(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIFlash5*)mpvVFTable, StopPlay, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TotalFrames(PRInt32 *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aretval=%p", pszFunction, aretval));
        rc = VFTCALL1((VFTnsIFlash5*)mpvVFTable, TotalFrames, mpvThis, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD CurrentFrame(PRInt32 *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aretval=%p", pszFunction, aretval));
        rc = VFTCALL1((VFTnsIFlash5*)mpvVFTable, CurrentFrame, mpvThis, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GotoFrame(PRInt32 aFrame)
    {
        UP_ENTER_RC();
        dprintf(("%s: aFrame=%d", pszFunction, aFrame));
        rc = VFTCALL1((VFTnsIFlash5*)mpvVFTable, GotoFrame, mpvThis, aFrame);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Rewind(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIFlash5*)mpvVFTable, Rewind, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Back(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIFlash5*)mpvVFTable, Back, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Forward(void)
    {
        UP_ENTER_RC();
        rc = VFTCALL0((VFTnsIFlash5*)mpvVFTable, Forward, mpvThis);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD PercentLoaded(PRInt32 *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aretval=%p", pszFunction, aretval));
        rc = VFTCALL1((VFTnsIFlash5*)mpvVFTable, PercentLoaded, mpvThis, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD FrameLoaded(PRInt32 aFrame, PRBool *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aFrame=%d aretval=%p", pszFunction, aFrame, aretval));
        rc = VFTCALL2((VFTnsIFlash5*)mpvVFTable, FrameLoaded, mpvThis, aFrame, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD FlashVersion(PRInt32 *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aretval=%p", pszFunction, aretval));
        rc = VFTCALL1((VFTnsIFlash5*)mpvVFTable, FlashVersion, mpvThis, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Pan(PRInt32 aX, PRInt32 aY, PRInt32 aMode)
    {
        UP_ENTER_RC();
        dprintf(("%s: aX=%d aY=%d aMode=%d", pszFunction, aX, aY, aMode));
        rc = VFTCALL3((VFTnsIFlash5*)mpvVFTable, Pan, mpvThis, aX, aY, aMode);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD Zoom(PRInt32 aZoom)
    {
        UP_ENTER_RC();
        dprintf(("%s: aZoom=%p", pszFunction, aZoom));
        rc = VFTCALL1((VFTnsIFlash5*)mpvVFTable, Zoom, mpvThis, aZoom);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD SetZoomRect(PRInt32 aLeft, PRInt32 aTop, PRInt32 aRight, PRInt32 aBottom)
    {
        UP_ENTER_RC();
        dprintf(("%s: aLeft=%d aTop=%d aRight=%d aBottom=%d", pszFunction, aLeft, aTop, aRight, aBottom));
        rc = VFTCALL4((VFTnsIFlash5*)mpvVFTable, SetZoomRect, mpvThis, aLeft, aTop, aRight, aBottom);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD LoadMovie(PRInt32 aLayer, const char *aURL)
    {
        UP_ENTER_RC();
        dprintf(("%s: aLayer=%d aURL=%p", pszFunction, aLayer, aURL));
        DPRINTF_STR(aURL);
        rc = VFTCALL2((VFTnsIFlash5*)mpvVFTable, LoadMovie, mpvThis, aLayer, aURL);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TGotoFrame(const char *aTarget, PRInt32 aFrameNumber)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p aFrameNumber=%d", pszFunction, aTarget, aFrameNumber));
        DPRINTF_STR(aTarget);
        rc = VFTCALL2((VFTnsIFlash5*)mpvVFTable, TGotoFrame, mpvThis, aTarget, aFrameNumber);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TGotoLabel(const char *aTarget, const char *aLabel)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p aLabel=%p", pszFunction, aTarget, aLabel));
        DPRINTF_STR(aTarget);
        DPRINTF_STR(aLabel);
        rc = VFTCALL2((VFTnsIFlash5*)mpvVFTable, TGotoLabel, mpvThis, aTarget, aLabel);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TCurrentFrame(const char *aTarget, PRInt32 *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p aretval=%p", pszFunction, aTarget, aretval));
        DPRINTF_STR(aTarget);
        rc = VFTCALL2((VFTnsIFlash5*)mpvVFTable, TCurrentFrame, mpvThis, aTarget, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%d", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TCurrentLabel(const char *aTarget, char **aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p aretval=%p", pszFunction, aTarget, aretval));
        DPRINTF_STR(aTarget);
        rc = VFTCALL2((VFTnsIFlash5*)mpvVFTable, TCurrentLabel, mpvThis, aTarget, aretval);
        if (VALID_PTR(aretval))
            DPRINTF_STR(*aretval);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TPlay(const char *aTarget)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p", pszFunction, aTarget));
        DPRINTF_STR(aTarget);
        rc = VFTCALL1((VFTnsIFlash5*)mpvVFTable, TPlay, mpvThis, aTarget);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TStopPlay(const char *aTarget)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p", pszFunction, aTarget));
        DPRINTF_STR(aTarget);
        rc = VFTCALL1((VFTnsIFlash5*)mpvVFTable, TStopPlay, mpvThis, aTarget);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD SetVariable(const char *aVariable, const char *aValue)
    {
        UP_ENTER_RC();
        dprintf(("%s: aVariable=%p aValue=%p", pszFunction, aVariable, aValue));
        DPRINTF_STR(aVariable);
        DPRINTF_STR(aValue);
        rc = VFTCALL2((VFTnsIFlash5*)mpvVFTable, SetVariable, mpvThis, aVariable, aValue);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD GetVariable(const char *aVariable, char **aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aVariable=%p aretval=%p", pszFunction, aVariable, aretval));
        DPRINTF_STR(aVariable);
        rc = VFTCALL2((VFTnsIFlash5*)mpvVFTable, GetVariable, mpvThis, aVariable, aretval);
        if (VALID_PTR(aretval) && VALID_PTR(*aretval))
            DPRINTF_STR(aretval);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TSetProperty(const char *aTarget, PRInt32 aProperty, const char *aValue)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p aProperty=%d aValue=%p", pszFunction, aTarget, aProperty, aValue));
        DPRINTF_STR(aTarget);
        DPRINTF_STR(aValue);
        rc = VFTCALL3((VFTnsIFlash5*)mpvVFTable, TSetProperty, mpvThis, aTarget, aProperty, aValue);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TGetProperty(const char *aTarget, PRInt32 aProperty, char **aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p aProperty=%d aretval=%p", pszFunction, aTarget, aProperty, aretval));
        DPRINTF_STR(aTarget);
        rc = VFTCALL3((VFTnsIFlash5*)mpvVFTable, TGetProperty, mpvThis, aTarget, aProperty, aretval);
        if (VALID_PTR(aretval))
            DPRINTF_STR(*aretval);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TCallFrame(const char *aTarget, PRInt32 aFrame)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p aFrameNumber=%d", pszFunction, aTarget, aFrame));
        DPRINTF_STR(aTarget);
        rc = VFTCALL2((VFTnsIFlash5*)mpvVFTable, TCallFrame, mpvThis, aTarget, aFrame);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TCallLabel(const char *aTarget, const char *aLabel)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p aLabel=%p", pszFunction, aTarget, aLabel));
        DPRINTF_STR(aTarget);
        DPRINTF_STR(aLabel);
        rc = VFTCALL2((VFTnsIFlash5*)mpvVFTable, TCallLabel, mpvThis, aTarget, aLabel);
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TGetPropertyAsNumber(const char *aTarget, PRInt32 aProperty, double *aretval)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p aProperty=%d aretval=%p", pszFunction, aTarget, aProperty, aretval));
        DPRINTF_STR(aTarget);
        rc = VFTCALL3((VFTnsIFlash5*)mpvVFTable, TGetPropertyAsNumber, mpvThis, aTarget, aProperty, aretval);
        if (VALID_PTR(aretval))
            dprintf(("%s: *aretval=%f", pszFunction, *aretval));
        UP_LEAVE_INT(rc);
        return rc;
    }

    NS_IMETHOD TSetPropertyAsNumber(const char *aTarget, PRInt32 aProperty, double aValue)
    {
        UP_ENTER_RC();
        dprintf(("%s: aTarget=%p aProperty=%d aValue=%f", pszFunction, aTarget, aProperty, aValue));
        DPRINTF_STR(aTarget);
        rc = VFTCALL3((VFTnsIFlash5*)mpvVFTable, TSetPropertyAsNumber, mpvThis, aTarget, aProperty, aValue);
        UP_LEAVE_INT(rc);
        return rc;
    }

    
    UpFlash5(void *pvThis) :
        UpSupportsBase(pvThis, (nsIFlash5*)this, kFlash5IID)
    {
    }

};









class UpClassInfo : public nsIClassInfo, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    







    
    NS_IMETHOD GetInterfaces(PRUint32 *count, nsIID * **array)
    {
        UP_ENTER_RC();
        dprintf(("%s: count=%p array=%p", pszFunction, count, array));
        rc = VFTCALL2((VFTnsIClassInfo*)mpvVFTable, GetInterfaces, mpvThis, count, array);
        if (NS_SUCCEEDED(rc) && VALID_PTR(count) && VALID_PTR(array) && VALID_PTR(*array))
        {
            dprintf(("%s: *count=%d\n", *count));
            for (unsigned i = 0; i < *count; i++)
                DPRINTF_NSID(*((*array)[i]));
        }
        UP_LEAVE_INT(rc);
        return rc;
    }

    











    
    NS_IMETHOD GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
    {
        UP_ENTER_RC();
        dprintf(("%s: language=%d _retval=%p", pszFunction, language, _retval));
        rc = VFTCALL2((VFTnsIClassInfo*)mpvVFTable, GetHelperForLanguage, mpvThis, language, _retval);
        rc = upCreateWrapper((void**)_retval, kSupportsIID, rc);
        UP_LEAVE_INT(rc);
        return rc;
    }

    



    
    NS_IMETHOD GetContractID(char * *aContractID)
    {
        UP_ENTER_RC();
        dprintf(("%s: aContractID=%p", pszFunction, aContractID));
        rc = VFTCALL1((VFTnsIClassInfo*)mpvVFTable, GetContractID, mpvThis, aContractID);
        DPRINTF_STRNULL(aContractID);
        UP_LEAVE_INT(rc);
        return rc;
    }

    


    
    NS_IMETHOD GetClassDescription(char * *aClassDescription)
    {
        UP_ENTER_RC();
        dprintf(("%s: aClassDescription=%p", pszFunction, aClassDescription));
        rc = VFTCALL1((VFTnsIClassInfo*)mpvVFTable, GetClassDescription, mpvThis, aClassDescription);
        DPRINTF_STRNULL(aClassDescription);
        UP_LEAVE_INT(rc);
        return rc;
    }

    



    
    NS_IMETHOD GetClassID(nsCID * *aClassID)
    {
        UP_ENTER_RC();
        dprintf(("%s: aClassID=%p", pszFunction, aClassID));
        rc = VFTCALL1((VFTnsIClassInfo*)mpvVFTable, GetClassID, mpvThis, aClassID);
        DPRINTF_NSID(**aClassID);
        UP_LEAVE_INT(rc);
        return rc;
    }

    


    
    NS_IMETHOD GetImplementationLanguage(PRUint32 *aImplementationLanguage)
    {
        UP_ENTER_RC();
        dprintf(("%s: aImplementationLanguage=%p", pszFunction, aImplementationLanguage));
        rc = VFTCALL1((VFTnsIClassInfo*)mpvVFTable, GetImplementationLanguage, mpvThis, aImplementationLanguage);
        if (VALID_PTR(aImplementationLanguage))
            dprintf(("%s: *aImplementationLanguage=%d", pszFunction, *aImplementationLanguage));
        UP_LEAVE_INT(rc);
        return rc;
    }

    
    NS_IMETHOD GetFlags(PRUint32 *aFlags)
    {
        UP_ENTER_RC();
        dprintf(("%s: aFlags=%p", pszFunction, aFlags));
        rc = VFTCALL1((VFTnsIClassInfo*)mpvVFTable, GetFlags, mpvThis, aFlags);
        if (VALID_PTR(aFlags))
            dprintf(("%s: *aFlags=%d", pszFunction, *aFlags));
        UP_LEAVE_INT(rc);
        return rc;
    }

    






    
    NS_IMETHOD GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
    {
        UP_ENTER_RC();
        dprintf(("%s: aClassIDNoAlloc=%p", pszFunction, aClassIDNoAlloc));
        rc = VFTCALL1((VFTnsIClassInfo*)mpvVFTable, GetClassIDNoAlloc, mpvThis, aClassIDNoAlloc);
        DPRINTF_NSID(*aClassIDNoAlloc);
        UP_LEAVE_INT(rc);
        return rc;
    }


    
    UpClassInfo(void *pvThis) :
        UpSupportsBase(pvThis, (nsIClassInfo*)this, kClassInfoIID)
    {
    }
};







class UpHTTPHeaderListener : public nsIHTTPHeaderListener, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    



    
    NS_IMETHOD NewResponseHeader(const char *headerName, const char *headerValue)
    {
        UP_ENTER_RC();
        DPRINTF_STR(headerName);
        DPRINTF_STR(headerValue);
        rc = VFTCALL2((VFTnsIHTTPHeaderListener *)mpvVFTable, NewResponseHeader, mpvThis, headerName, headerValue);
        UP_LEAVE_INT(rc);
        return rc;
    }


    
    UpHTTPHeaderListener(void *pvThis) :
        UpSupportsBase(pvThis, (nsIHTTPHeaderListener *)this, kHTTPHeaderListenerIID)
    {
    }

    nsresult StatusLine(const char *line) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }
};


class UpMemory : public nsIMemory, public UpSupportsBase
{
public:
    UP_IMPL_NSISUPPORTS();

    







    
    NS_IMETHOD_(void *) Alloc(size_t size)
    {
        UP_ENTER();
        dprintf(("%s: size=%d", pszFunction, size));
        void * pv = VFTCALL1((VFTnsIMemory*)mpvVFTable, Alloc, mpvThis, size);
        UP_LEAVE_INT((unsigned)pv);
        return pv;
    }

    













    
    NS_IMETHOD_(void *) Realloc(void * ptr, size_t newSize)
    {
        UP_ENTER();
        dprintf(("%s: ptr=%p newSize=%d", pszFunction, ptr, newSize));
        void * pv = VFTCALL2((VFTnsIMemory*)mpvVFTable, Realloc, mpvThis, ptr, newSize);
        UP_LEAVE_INT((unsigned)pv);
        return pv;
    }

    





    
    NS_IMETHOD_(void) Free(void * ptr)
    {
        UP_ENTER();
        dprintf(("%s: ptr=%d", pszFunction, ptr));
        VFTCALL1((VFTnsIMemory*)mpvVFTable, Free, mpvThis, ptr);
        UP_LEAVE();
        return;
    }

    








    
    NS_IMETHOD HeapMinimize(PRBool immediate)
    {
        UP_ENTER_RC();
        dprintf(("%s: immediate=%d", pszFunction, immediate));
        rc = VFTCALL1((VFTnsIMemory*)mpvVFTable, HeapMinimize, mpvThis, immediate);
        UP_LEAVE_INT(rc);
        return rc;
    }

    




    
    NS_IMETHOD IsLowMemory(PRBool *_retval)
    {
        UP_ENTER_RC();
        dprintf(("%s: immediate=%p", pszFunction, _retval));
        rc = VFTCALL1((VFTnsIMemory*)mpvVFTable, IsLowMemory, mpvThis, _retval);
        if (VALID_PTR(_retval))
            dprintf(("%s: *_retval=%d\n", *_retval));
        UP_LEAVE_INT(rc);
        return rc;
    }


    
    UpMemory(void *pvThis) :
        UpSupportsBase(pvThis, (nsIMemory *)this, kMemoryIID)
    {
    }
};












BOOL            upIsSupportedInterface(REFNSIID aIID)
{
    static const nsID * aIIDs[] =
    {
        &kSupportsIID,
        &kFactoryIID,
        &kPluginIID,
        &kJVMPluginIID,
        &kSecureEnvIID,
        &kPluginInstanceIID,
        &kPluginInstancePeerIID,
        &kPluginInstancePeer2IID,
        &kPluginTagInfoIID,
        &kJVMWindowIID,
        &kJVMConsoleIID,
        &kEventHandlerIID,
        &kJVMPluginInstanceIID,
        &kRunnableIID,
        &kSecurityContextIID,
        &kRequestObserverIID,
        &kStreamListenerIID,
        &kRequestIID,
        &kLoadGroupIID,
        &kSimpleEnumeratorIID,
        &kInterfaceRequestorIID,
        &kOutputStreamIID,
        &kPluginStreamListenerIID,
        &kFlashIObject7IID,
        &kFlashIScriptablePlugin7IID,
        &kFlash5IID,
        &kClassInfoIID,
        &kHTTPHeaderListenerIID,
        &kMemoryIID,
    };

    for (unsigned iInterface = 0; iInterface < sizeof(aIIDs) / sizeof(aIIDs[0]); iInterface++)
        if (aIIDs[iInterface]->Equals(aIID))
            return TRUE;
    return FALSE;
}











nsresult    upCreateWrapper(void **ppvResult, REFNSIID aIID, nsresult rc)
{
    DEBUG_FUNCTIONNAME();
    dprintf(("%s: pvvResult=%x,,rc=%x", pszFunction, ppvResult, rc));
    DPRINTF_NSID(aIID);

    if (VALID_PTR(ppvResult))
    {
        if (VALID_PTR(*ppvResult))
        {
            dprintf(("%s: *pvvResult=%x", pszFunction, *ppvResult));
            if (NS_SUCCEEDED(rc))
            {
                void *pvThis = *ppvResult;
                



                void *pvNoWrapper = UpWrapperBase::findDownWrapper(pvThis);
                if (pvNoWrapper)
                {
                    dprintf(("%s: COOL! pvThis(%x) is an down wrapper, no wrapping needed. returns real obj=%x",
                             pszFunction,  pvThis, pvNoWrapper));
                    *ppvResult = pvNoWrapper;
                    return rc;
                }

                #if 1 
                


                UpWrapperBase::upLock();
                UpWrapperBase *pExisting = UpWrapperBase::findUpWrapper(pvThis, aIID);
                UpWrapperBase::upUnLock();
                if (pExisting)
                {
                    dprintf(("%s: Reusing existing wrapper %p/%p for %p!", pszFunction, pExisting, pExisting->getInterfacePointer(), pvThis));
                    *ppvResult = pExisting->getInterfacePointer();
                    return rc;
                }
                #endif

                


















                




                BOOL fFound = TRUE;
                UpWrapperBase * pWrapper = NULL;
                if (aIID.Equals(kSupportsIID))
                    pWrapper =  new UpSupports(pvThis);
                else if (aIID.Equals(kFactoryIID))
                    pWrapper =  new UpFactory(pvThis);
                else if (aIID.Equals(kPluginIID))
                    pWrapper =  new UpPlugin(pvThis);
                else if (aIID.Equals(kJVMPluginIID))
                    pWrapper =  new UpJVMPlugin(pvThis);
                else if (aIID.Equals(kSecureEnvIID))
                    pWrapper =  new UpSecureEnv(pvThis);
                else if (aIID.Equals(kPluginInstanceIID))
                    pWrapper =  new UpPluginInstance(pvThis);
                else if (aIID.Equals(kPluginInstancePeerIID))
                    pWrapper =  new UpPluginInstancePeer(pvThis);
                else if (aIID.Equals(kPluginInstancePeer2IID))
                    pWrapper =  new UpPluginInstancePeer2(pvThis);
                else if (aIID.Equals(kPluginTagInfoIID))
                    pWrapper =  new UpPluginTagInfo(pvThis);
                else if (aIID.Equals(kJVMWindowIID))
                    pWrapper =  new UpJVMWindow(pvThis);
                else if (aIID.Equals(kJVMConsoleIID))
                    pWrapper =  new UpJVMConsole(pvThis);
                else if (aIID.Equals(kEventHandlerIID))
                    pWrapper =  new UpEventHandler(pvThis);
                else if (aIID.Equals(kJVMPluginInstanceIID))
                    pWrapper =  new UpJVMPluginInstance(pvThis);
                else if (aIID.Equals(kRunnableIID))
                    pWrapper =  new UpRunnable(pvThis);
                else if (aIID.Equals(kSecurityContextIID))
                    pWrapper = new UpSecurityContext(pvThis);
                else if (aIID.Equals(kRequestObserverIID))
                    pWrapper = new UpRequestObserver(pvThis);
                else if (aIID.Equals(kStreamListenerIID))
                    pWrapper = new UpStreamListener(pvThis);
                else if (aIID.Equals(kRequestIID))
                    pWrapper = new UpRequest(pvThis);
                else if (aIID.Equals(kLoadGroupIID))
                    pWrapper = new UpLoadGroup(pvThis);
                else if (aIID.Equals(kSimpleEnumeratorIID))
                    pWrapper = new UpSimpleEnumerator(pvThis);
                else if (aIID.Equals(kInterfaceRequestorIID))
                    pWrapper = new UpInterfaceRequestor(pvThis);
                else if (aIID.Equals(kOutputStreamIID))
                    pWrapper = new UpOutputStream(pvThis);
                else if (aIID.Equals(kPluginStreamListenerIID))
                    pWrapper = new UpPluginStreamListener(pvThis);
                else if (aIID.Equals(kFlashIScriptablePlugin7IID))
                    pWrapper = new UpFlashIScriptablePlugin7(pvThis);
                else if (aIID.Equals(kFlashIObject7IID))
                    pWrapper = new UpFlashIObject7(pvThis);
                else if (aIID.Equals(kFlash5IID))
                    pWrapper = new UpFlash5(pvThis);
                else if (aIID.Equals(kClassInfoIID))
                    pWrapper = new UpClassInfo(pvThis);
                else if (aIID.Equals(kHTTPHeaderListenerIID))
                    pWrapper = new UpHTTPHeaderListener(pvThis);
                else if (aIID.Equals(kMemoryIID))
                    pWrapper = new UpMemory(pvThis);
                else
                    fFound = FALSE;

                


                if (fFound && pWrapper)
                {
                    


                    UpWrapperBase::upLock();
                    UpWrapperBase *pExisting2 = UpWrapperBase::findUpWrapper(pvThis, aIID);
                    if (pExisting2)
                    {
                        UpWrapperBase::upUnLock();
                        delete pWrapper;
                        pWrapper = pExisting2;
                        dprintf(("%s: Reusing existing wrapper %p/%p for %p!", pszFunction, pWrapper, pWrapper->getInterfacePointer(), pvThis));
                    }
                    else
                    {
                        pWrapper->upInsertWrapper();
                        UpWrapperBase::upUnLock();
                        dprintf(("%s: Successfully create wrapper %p/%p for %p!", pszFunction, pWrapper, pWrapper->getInterfacePointer(), pvThis));
                    }

                    *ppvResult = pWrapper->getInterfacePointer();
                    return rc;
                }


                


                if (!fFound)
                {
                    ReleaseInt3(0xbaddbeef, 11, aIID.m0);
                    dprintf(("%s: Unsupported interface!!!", pszFunction));
                    rc = NS_ERROR_NOT_IMPLEMENTED;
                }
                else
                {
                    dprintf(("%s: new failed! (how is that possible?)", pszFunction));
                    rc = NS_ERROR_OUT_OF_MEMORY;
                }
            }
            else
                dprintf(("%s: The passed in rc means failure (rc=%x)", pszFunction, rc));
            *ppvResult = nsnull;
        }
        else
            dprintf(("%s: *ppvResult (=%p) is invalid (rc=%x)", pszFunction, *ppvResult, rc));
    }
    else
        dprintf(("%s: ppvResult (=%p) is invalid (rc=%x)", pszFunction, ppvResult, rc));

    return rc;
}












void *  upCreateWrapper2(void *pvThis, REFNSIID aIID)
{

    void *      pvResult = pvThis;
    nsresult    rc = upCreateWrapper(&pvResult, aIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        return pvResult;
    return NULL;
}









int upCreateJNIEnvWrapper(JNIEnv **ppJNIEnv, int rc)
{
    DEBUG_FUNCTIONNAME();
    dprintf(("%s: ppJNIEnv=%x, rc=%x", pszFunction, ppJNIEnv, rc));

    if (VALID_PTR(ppJNIEnv))
    {
        if (VALID_PTR(*ppJNIEnv))
        {
            if (NS_SUCCEEDED(rc))
            {
                    


                    return rc;
            }
            else
                dprintf(("%s: The query method failed with rc=%x", pszFunction, rc));
            *ppJNIEnv = nsnull;
        }
        else if (*ppJNIEnv || rc != NS_OK) 
            dprintf(("%s: *ppJNIEnv (=%p) is invalid (rc=%x)", pszFunction, *ppJNIEnv, rc));
    }
    else
        dprintf(("%s: ppJNIEnv (=%p) is invalid (rc=%x)", pszFunction, ppJNIEnv, rc));

    return rc;
}





nsresult VFTCALL downQueryInterface(void *pvThis, REFNSIID aIID, void** aInstancePtr)
{
    DOWN_ENTER_RC(pvThis, nsISupports);
    DPRINTF_NSID(aIID);

    


    const void *pvVFT = downIsSupportedInterface(aIID);
    if (pvVFT)
    {
        dprintf(("%s: Supported interface. Calling the real QueryInterface...", pszFunction));
        rc = pMozI->QueryInterface(aIID, aInstancePtr);
        rc = downCreateWrapper(aInstancePtr, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        if (aInstancePtr)
            *aInstancePtr = nsnull;
        ReleaseInt3(0xbaddbeef, 1, aIID.m0);
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




nsrefcnt VFTCALL downAddRef(void *pvThis)
{
    DOWN_ENTER(pvThis, nsISupports);
    nsrefcnt c = pMozI->AddRef();
    DOWN_LEAVE_INT(pvThis, c);
    return c;
}




nsrefcnt VFTCALL downRelease(void *pvThis)
{
    DOWN_ENTER(pvThis, nsISupports);
    nsrefcnt c = pMozI->Release();
    if (!c)
    {
        dprintf(("%s: c=0! deleting wrapper structure!", pszFunction));

        #ifdef DO_DELETE
        


        DOWN_LOCK();
        int fUnchained;
        PDOWNTHIS pDown, pPrev;
        for (pDown = (PDOWNTHIS)gpDownHead, pPrev = NULL, fUnchained = 0;
             pDown;
             pPrev = pDown, pDown = (PDOWNTHIS)pDown->pNext)
        {
            if (pDown == pvThis)
            {
                if (pPrev)
                    pPrev->pNext = pDown->pNext;
                else
                    gpDownHead = pDown->pNext;

                #ifdef DEBUG
                
                if (fUnchained)
                {
                    dprintf(("%s: pvThis=%x was linked twice!", pszFunction, pvThis));
                    DebugInt3();
                }
                fUnchained = 1;
                #else
                fUnchained = 1;
                break;
                #endif
            }
        }
        DOWN_UNLOCK();

        if (fUnchained)
        {   




            pDown = (PDOWNTHIS)pvThis;
            memset(pDown, 0, sizeof(*pDown));
            delete pDown;
        }
        else
        {   




            dprintf(("%s: pvThis=%p not found in the list !!!!", pszFunction, pvThis));
            DebugInt3();
        }
        #endif
        pvThis = NULL;
    }
    DOWN_LEAVE_INT(pvThis, c);
    return c;
}



MAKE_SAFE_VFT(VFTnsISupports, downVFTnsISupports)
{
    VFTFIRST_VAL()
    downQueryInterface,         VFTDELTA_VAL()
    downAddRef,                 VFTDELTA_VAL()
    downRelease,                VFTDELTA_VAL()

}
SAFE_VFT_ZEROS();



















nsresult VFTCALL downGetService(void *pvThis, const nsCID & aClass, const nsIID & aIID, void * *result)
{
    DOWN_ENTER_RC(pvThis, nsIServiceManager);
    DPRINTF_NSID(aClass);
    DPRINTF_NSID(aIID);

    


    const void *pvVFT = downIsSupportedInterface(&aIID ? aIID : kSupportsIID);
    if (pvVFT)
    {
        dprintf(("%s: Supported interface. Calling the real GetService...", pszFunction));
        rc = pMozI->GetService(aClass, aIID, result);
        rc = downCreateWrapper(result, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        if (result)
            *result = nsnull;
        ReleaseInt3(0xbaddbeef, 2, aIID.m0);
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


nsresult VFTCALL downGetServiceByContractID(void *pvThis, const char *aContractID, const nsIID & aIID, void * *result)
{
    
    DOWN_ENTER_RC(pvThis, nsIServiceManager);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_NSID(aIID);

    


    const void *pvVFT = downIsSupportedInterface(&aIID ? aIID : kSupportsIID);
    if (pvVFT)
    {
        dprintf(("%s: Supported interface. Calling the real GetServiceByContractID...", pszFunction));
        rc = pMozI->GetServiceByContractID(aContractID, aIID, result);
        rc = downCreateWrapper(result, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        if (result)
            *result = nsnull;
        ReleaseInt3(0xbaddbeef, 3, aIID.m0);
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}













nsresult VFTCALL downIsServiceInstantiated(void *pvThis, const nsCID & aClass, const nsIID & aIID, PRBool *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIServiceManager);
    DPRINTF_NSID(aClass);
    DPRINTF_NSID(aIID);
    rc = pMozI->IsServiceInstantiated(aClass, aIID, _retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d", pszFunction, *_retval));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


nsresult VFTCALL downIsServiceInstantiatedByContractID(void *pvThis, const char *aContractID, const nsIID & aIID, PRBool *_retval)
{
    
    DOWN_ENTER_RC(pvThis, nsIServiceManager);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_NSID(aIID);
    rc = pMozI->IsServiceInstantiatedByContractID(aContractID, aIID, _retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d", pszFunction, *_retval));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}



MAKE_SAFE_VFT(VFTnsIServiceManager, downVFTnsIServiceManager)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                 VFTDELTA_VAL()
    downAddRef,                                         VFTDELTA_VAL()
    downRelease,                                        VFTDELTA_VAL()
 },
    downGetService,                                     VFTDELTA_VAL()
    downGetServiceByContractID,                         VFTDELTA_VAL()
    downIsServiceInstantiated,                          VFTDELTA_VAL()
    downIsServiceInstantiatedByContractID,              VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();













nsresult VFTCALL downISMORegisterService(void *pvThis, const nsCID& aClass, nsISupports* aService)
{
    DOWN_ENTER_RC(pvThis, nsIServiceManagerObsolete);
    DPRINTF_NSID(aClass);

    nsISupports *pupService = aService;
    rc = upCreateWrapper((void**)&pupService, kSupportsIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->RegisterService(aClass, pupService);
    else
        dprintf(("%s: Unable to create wrapper for nsISupports!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downISMOUnregisterService(void *pvThis, const nsCID& aClass)
{
    DOWN_ENTER_RC(pvThis, nsIServiceManagerObsolete);
    DPRINTF_NSID(aClass);
    rc = pMozI->UnregisterService(aClass);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}

nsresult VFTCALL downISMOGetService(void *pvThis, const nsCID& aClass, const nsIID& aIID,
                                       nsISupports* *result,
                                       nsIShutdownListener* shutdownListener)
{
    DOWN_ENTER_RC(pvThis, nsIServiceManagerObsolete);
    DPRINTF_NSID(aClass);
    DPRINTF_NSID(aIID);

    


    const void *pvVFT = downIsSupportedInterface(&aIID ? aIID : kSupportsIID);
    if (pvVFT)
    {
        dprintf(("%s: Supported interface. Calling the real downISMOGetService...", pszFunction));
        nsIShutdownListener *pupShutdownListener = shutdownListener;
        rc = upCreateWrapper((void**)&pupShutdownListener, kShutdownListenerIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            rc = pMozI->GetService(aClass, aIID, result, pupShutdownListener);
            rc = downCreateWrapper((void**)result, pvVFT, rc);
        }
        else
            dprintf(("%s: Unable to create 'UP' wrapper for nsIShutdownListener!", pszFunction));
    }
    else
    {
        dprintf(("%s: Unsupported interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        if (result)
            *result = nsnull;
        ReleaseInt3(0xbaddbeef, 4, aIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}



nsresult VFTCALL downISMOReleaseService(void *pvThis, const nsCID& aClass, nsISupports* service,
                                           nsIShutdownListener* shutdownListener)
{
    DOWN_ENTER_RC(pvThis, nsIServiceManagerObsolete);
    DPRINTF_NSID(aClass);

    nsISupports *pupService = service;
    rc = upCreateWrapper((void**)&pupService, kSupportsIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsIShutdownListener *pupShutdownListener = shutdownListener;
        rc = upCreateWrapper((void**)&pupShutdownListener, kShutdownListenerIID, rc);
        if (NS_SUCCEEDED(rc))
            rc = pMozI->ReleaseService(aClass, pupService, pupShutdownListener);
        else
            dprintf(("%s: Unable to create 'UP' wrapper for nsIShutdownListener!", pszFunction));
    }
    else
        dprintf(("%s: Unable to create 'UP' wrapper for nsISupports!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;

}





nsresult VFTCALL downISMORegisterServiceByContractID(void *pvThis, const char* aContractID, nsISupports* aService)
{
    DOWN_ENTER_RC(pvThis, nsIServiceManagerObsolete);
    DPRINTF_CONTRACTID(aContractID);

    nsISupports *pupService = aService;
    rc = upCreateWrapper((void**)&pupService, kSupportsIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->RegisterService(aContractID, pupService);
    else
        dprintf(("%s: Unable to create wrapper for nsISupports!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


nsresult VFTCALL downISMOUnregisterServiceByContractID(void *pvThis, const char* aContractID)
{
    DOWN_ENTER_RC(pvThis, nsIServiceManagerObsolete);
    DPRINTF_CONTRACTID(aContractID);
    rc = pMozI->UnregisterService(aContractID);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


nsresult VFTCALL downISMOGetServiceByContractID(void *pvThis, const char* aContractID, const nsIID& aIID,
                                                   nsISupports* *result,
                                                   nsIShutdownListener* shutdownListener)
{
    DOWN_ENTER_RC(pvThis, nsIServiceManagerObsolete);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_NSID(aIID);

    


    const void *pvVFT = downIsSupportedInterface(&aIID ? aIID : kSupportsIID);
    if (pvVFT)
    {
        nsIShutdownListener *pupShutdownListener = shutdownListener;
        rc = upCreateWrapper((void**)&pupShutdownListener, kShutdownListenerIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            rc = pMozI->GetService(aContractID, aIID, result, pupShutdownListener);
            rc = downCreateWrapper((void**)result, pvVFT, rc);
        }
        else
            dprintf(("%s: Unable to create 'UP' wrapper for nsIShutdownListener!", pszFunction));
    }
    else
    {
        dprintf(("%s: Unsupported interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 5, aIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}



nsresult VFTCALL downISMOReleaseServiceByContractID(void *pvThis, const char* aContractID, nsISupports* service,
                                                       nsIShutdownListener* shutdownListener)
{
    DOWN_ENTER_RC(pvThis, nsIServiceManagerObsolete);
    DPRINTF_CONTRACTID(aContractID);

    nsISupports *pupService = service;
    rc = upCreateWrapper((void**)&pupService, kSupportsIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsIShutdownListener *pupShutdownListener = shutdownListener;
        rc = upCreateWrapper((void**)&pupShutdownListener, kShutdownListenerIID, rc);
        if (NS_SUCCEEDED(rc))
            rc = pMozI->ReleaseService(aContractID, pupService, pupShutdownListener);
        else
            dprintf(("%s: Unable to create 'UP' wrapper for nsIShutdownListener!", pszFunction));
    }
    else
        dprintf(("%s: Unable to create 'UP' wrapper for nsISupports!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;

}


MAKE_SAFE_VFT(VFTnsIServiceManagerObsolete, downVFTnsIServiceManagerObsolete)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
#ifdef VFT_VAC365
    downISMORegisterService,                                VFTDELTA_VAL()
    downISMORegisterServiceByContractID,                    VFTDELTA_VAL()
    downISMOUnregisterService,                              VFTDELTA_VAL()
    downISMOUnregisterServiceByContractID,                  VFTDELTA_VAL()
    downISMOGetService,                                     VFTDELTA_VAL()
    downISMOGetServiceByContractID,                         VFTDELTA_VAL()
    downISMOReleaseService,                                 VFTDELTA_VAL()
    downISMOReleaseServiceByContractID,                     VFTDELTA_VAL()
#else
    downISMORegisterServiceByContractID,                    VFTDELTA_VAL()
    downISMORegisterService,                                VFTDELTA_VAL()
    downISMOUnregisterServiceByContractID,                  VFTDELTA_VAL()
    downISMOUnregisterService,                              VFTDELTA_VAL()
    downISMOGetServiceByContractID,                         VFTDELTA_VAL()
    downISMOGetService,                                     VFTDELTA_VAL()
    downISMOReleaseServiceByContractID,                     VFTDELTA_VAL()
    downISMOReleaseService,                                 VFTDELTA_VAL()
#endif
}
SAFE_VFT_ZEROS();

















nsresult VFTCALL downIPMGetValue(void *pvThis, nsPluginManagerVariable variable, void * value)
{   
    DOWN_ENTER_RC(pvThis, nsIPluginManager);
    dprintf(("%s: variable=%d value=%x", pszFunction, variable, value));
    rc = pMozI->GetValue(variable, value);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downIPMReloadPlugins(void *pvThis, PRBool reloadPages)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager);
    dprintf(("%s: reloadPages=%d", pszFunction, reloadPages));
    rc = pMozI->ReloadPlugins(reloadPages);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downIPMUserAgent(void *pvThis, const char * * resultingAgentString)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager);
    dprintf(("%s: resultingAgentString=%p", pszFunction, resultingAgentString));
    rc = pMozI->UserAgent(resultingAgentString);
    if (NS_SUCCEEDED(rc) && VALID_PTR(resultingAgentString))
        DPRINTF_STR(*resultingAgentString);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}






















nsresult VFTCALL downIPMGetURL(void *pvThis, nsISupports* pluginInst, const char* url, const char* target = NULL,
                                  nsIPluginStreamListener* streamListener = NULL, const char* altHost = NULL, const char* referrer = NULL,
                                  PRBool forceJSEnabled = PR_FALSE)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager);
    dprintf(("%s: pluginInst=%x  streamListener=%p  forceJSEnabled=%d",
             pszFunction, pluginInst, streamListener, forceJSEnabled));
    DPRINTF_STR(url);
    DPRINTF_STR(target);
    DPRINTF_STR(altHost);
    DPRINTF_STR(referrer);

    nsIPluginStreamListener *pupStreamListener = streamListener;
    rc = upCreateWrapper((void**)&pupStreamListener, kPluginStreamListenerIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports * pupPluginInst = pluginInst;
        rc = upCreateWrapper((void**)&pupPluginInst, kSupportsIID, rc);
        if (rc == NS_OK)
            rc = pMozI->GetURL(pupPluginInst, url, target, pupStreamListener, altHost, referrer, forceJSEnabled);
        else
            dprintf(("%s: Failed to create wrapper for nsISupports (pluginInst=%p)!!!!", pszFunction, pluginInst));
    }
    else
        dprintf(("%s: Failed to create wrapper for nsIPluginStreamListener!!!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}
































nsresult VFTCALL downIPMPostURL(void *pvThis, nsISupports* pluginInst, const char* url, PRUint32 postDataLen, const char* postData,
                                   PRBool isFile = PR_FALSE, const char* target = NULL, nsIPluginStreamListener* streamListener = NULL,
                                   const char* altHost = NULL, const char* referrer = NULL, PRBool forceJSEnabled = PR_FALSE,
                                   PRUint32 postHeadersLength = 0, const char* postHeaders = NULL)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager);
    dprintf(("%s: pluginInst=%x  postDataLen=%d postData='%.*s' (%p) isFile=%d, streamListener=%p",
             pszFunction, pluginInst, postDataLen, postDataLen, postData, postData, isFile, streamListener));
    dprintf(("%s: forceJSEnabled=%d postHeadersLength=%d postHeaders='%*.s' (%p)",
             pszFunction, forceJSEnabled, postHeadersLength, postHeadersLength, postHeaders, postHeaders));
    DPRINTF_STR(url);
    DPRINTF_STR(target);
    DPRINTF_STR(altHost);
    DPRINTF_STR(referrer);

    nsIPluginStreamListener *pupStreamListener = streamListener;
    rc = upCreateWrapper((void**)&pupStreamListener, kPluginStreamListenerIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports * pupPluginInst = pluginInst;
        rc = upCreateWrapper((void**)&pupPluginInst, kSupportsIID, rc);
        if (rc == NS_OK)
            rc = pMozI->PostURL(pupPluginInst, url, postDataLen, postData, isFile, target, pupStreamListener,
                                altHost, referrer, forceJSEnabled, postHeadersLength, postHeaders);
        else
            dprintf(("%s: Failed to create wrapper for nsISupports (pluginInst=%p)!!!!", pszFunction, pluginInst));
    }
    else
        dprintf(("%s: Failed to create wrapper for nsIPluginStreamListener!!!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





















nsresult VFTCALL downIPMRegisterPlugin(void *pvThis, REFNSIID aCID, const char *aPluginName, const char *aDescription, const char * * aMimeTypes,
                                          const char * * aMimeDescriptions, const char * * aFileExtensions, PRInt32 aCount)
{
    
    DOWN_ENTER_RC(pvThis, nsIPluginManager);
    dprintf(("%s: aCount=%d", pszFunction, aCount));
    DPRINTF_NSID(aCID);
    DPRINTF_STR(aPluginName);
    DPRINTF_STR(aDescription);

    for (int i = 0; i < aCount; i++)
        dprintf(("%s: aMimeTypes[%d]='%s' (%p)  aMimeDescriptions[%d]='%s' (%p)  aFileExtensions[%d]='%s' (%p)",
                 pszFunction, i, aMimeTypes[i], aMimeTypes[i],
                 i, aMimeDescriptions[i], aMimeDescriptions[i],
                 i, aFileExtensions[i], aFileExtensions[i]));
    rc = pMozI->RegisterPlugin(aCID, aPluginName, aDescription, aMimeTypes, aMimeDescriptions, aFileExtensions, aCount);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downIPMUnregisterPlugin(void *pvThis, REFNSIID aCID)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager);
    DPRINTF_NSID(aCID);
    rc = pMozI->UnregisterPlugin(aCID);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}












nsresult VFTCALL downIPMGetURLWithHeaders(void *pvThis, nsISupports* pluginInst, const char* url, const char* target = NULL,
                                             nsIPluginStreamListener* streamListener = NULL, const char* altHost = NULL, const char* referrer = NULL,
                                             PRBool forceJSEnabled = PR_FALSE, PRUint32 getHeadersLength = 0, const char* getHeaders = NULL)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager);
    dprintf(("%s: pluginInst=%x  streamListener=%p  forceJSEnabled=%d getHeadersLength=%d getHeaders='%.*s' (%p)",
             pszFunction, pluginInst, streamListener, forceJSEnabled, getHeadersLength, getHeadersLength, getHeaders, getHeaders));
    DPRINTF_STR(url);
    DPRINTF_STR(target);
    DPRINTF_STR(altHost);
    DPRINTF_STR(referrer);

    nsIPluginStreamListener *pupStreamListener = streamListener;
    rc = upCreateWrapper((void**)&pupStreamListener, kPluginStreamListenerIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports * pupPluginInst = pluginInst;
        rc = upCreateWrapper((void**)&pupPluginInst, kSupportsIID, rc);
        if (rc == NS_OK)
            rc = pMozI->GetURLWithHeaders(pupPluginInst, url, target, pupStreamListener, altHost, referrer, forceJSEnabled, getHeadersLength, getHeaders);
        else
            dprintf(("%s: Failed to create wrapper for nsISupports (pluginInst=%p)!!!!", pszFunction, pluginInst));
    }
    else
        dprintf(("%s: Failed to create wrapper for nsIPluginStreamListener!!!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIPluginManager, downVFTnsIPluginManager)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downIPMGetValue,                                        VFTDELTA_VAL()
    downIPMReloadPlugins,                                   VFTDELTA_VAL()
    downIPMUserAgent,                                       VFTDELTA_VAL()
    downIPMGetURL,                                          VFTDELTA_VAL()
    downIPMPostURL,                                         VFTDELTA_VAL()
    downIPMRegisterPlugin,                                  VFTDELTA_VAL()
    downIPMUnregisterPlugin,                                VFTDELTA_VAL()
    downIPMGetURLWithHeaders,                               VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();













nsresult VFTCALL downIPM2BeginWaitCursor(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager2);
    rc = pMozI->BeginWaitCursor();
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}







nsresult VFTCALL downIPM2EndWaitCursor(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager2);
    rc = pMozI->EndWaitCursor();
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downIPM2SupportsURLProtocol(void *pvThis, const char *aProtocol, PRBool *aResult)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager2);
    DPRINTF_STR(aProtocol);

    rc = pMozI->SupportsURLProtocol(aProtocol, aResult);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aResult))
        dprintf(("%s: *aResult=%d", pszFunction, *aResult));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}












nsresult VFTCALL downIPM2NotifyStatusChange(void *pvThis, nsIPlugin *aPlugin, nsresult aStatus)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager2);
    dprintf(("%s: aPlugin=%x aStatus=%d", pszFunction, aPlugin, aStatus));

    nsIPlugin * pupPlugin = aPlugin;
    rc = upCreateWrapper((void**)&pupPlugin, kPluginIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->NotifyStatusChange(pupPlugin, aStatus);
    else
        dprintf(("%s: failed to create up wrapper for aPlugin=%x!!!", pszFunction, aPlugin));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}















nsresult VFTCALL downIPM2FindProxyForURL(void *pvThis, const char *aURL, char **aResult)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager2);
    DPRINTF_STR(aURL);
    rc = pMozI->FindProxyForURL(aURL, aResult);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aResult) && VALID_PTR(*aResult))
    {
        DPRINTF_STR(*aResult);

        






        char        szModName[9];
        HMODULE     hmod;
        ULONG       iObj;
        ULONG       offObj;
        if (!DosQueryModFromEIP(&hmod, &iObj, sizeof(szModName), &szModName[0], &offObj, ((unsigned *)(void*)&pvThis)[-1]))
        {
            if (!stricmp(szModName, "NPOJI6"))
            {
                if (!DosQueryModuleHandle("JV12MI36", &hmod))
                {
                    char *(*_Optlink pfnstrdup)(const char *);
                    
                    int rc2 = DosQueryProcAddr(hmod, 0, "strdup", (PFN*)&pfnstrdup);
                    if (   rc2 == ERROR_INVALID_HANDLE
                        && !DosLoadModule(NULL, 0, "JV12MI36", &hmod))
                        rc2 = DosQueryProcAddr(hmod, 0, "strdup", (PFN*)&pfnstrdup);
                    if (!rc2)
                    {
                        char *psz = pfnstrdup(*aResult);
                        if (psz)
                        {
                            free(*aResult); 
                            *aResult = psz;
                        }
                    }
                }
            }
        }
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downIPM2RegisterWindow(void *pvThis, nsIEventHandler *aHandler, nsPluginPlatformWindowRef aWindow)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager2);
    dprintf(("%s: Incomplete!!!", pszFunction));
    dprintf(("%s: aHandler=%p aWindow=%x", pszFunction, aHandler, aWindow));

    nsIEventHandler * pupHandler = aHandler;
    rc = upCreateWrapper((void**)&pupHandler, kEventHandlerIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        
        DebugInt3();
        nsPluginPlatformWindowRef   os2Window = aWindow;
        if (NS_SUCCEEDED(rc))
        {
            rc = pMozI->RegisterWindow(pupHandler, os2Window);
        }
        else
            dprintf(("%s: failed to create window handle wrapper for %x!!!", pszFunction, aWindow));
    }
    else
        dprintf(("%s: failed to create up wrapper for nsIEventHandler %x!!!", pszFunction, aHandler));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downIPM2UnregisterWindow(void *pvThis, nsIEventHandler *aHandler, nsPluginPlatformWindowRef aWindow)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager2);
    dprintf(("%s: Incomplete!!!", pszFunction));
    dprintf(("%s: aHandler=%p aWindow=%x", pszFunction, aHandler, aWindow));

    nsIEventHandler * pupHandler = aHandler;
    rc = upCreateWrapper((void**)&pupHandler, kEventHandlerIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        
        DebugInt3();
        nsPluginPlatformWindowRef   os2Window = aWindow;
        if (NS_SUCCEEDED(rc))
        {
            rc = pMozI->UnregisterWindow(pupHandler, os2Window);
        }
        else
            dprintf(("%s: failed to create window handle wrapper for %x!!!", pszFunction, aWindow));
    }
    else
        dprintf(("%s: failed to create up wrapper for nsIEventHandler %x!!!", pszFunction, aHandler));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downIPM2AllocateMenuID(void *pvThis, nsIEventHandler *aHandler, PRBool aIsSubmenu, PRInt16 *aResult)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager2);
    dprintf(("%s: Incomplete!!!", pszFunction));
    dprintf(("%s: aHandler=%p aIsSubmenu=%d aResult=%p", pszFunction, aHandler, aIsSubmenu, aResult));

    nsIEventHandler * pupHandler = aHandler;
    rc = upCreateWrapper((void**)&pupHandler, kEventHandlerIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        rc = pMozI->AllocateMenuID(pupHandler, aIsSubmenu, aResult);
        if (NS_SUCCEEDED(rc) && VALID_PTR(aResult))
            dprintf(("%s: *aResult=%d (0x%x)", pszFunction, *aResult, *aResult));
    }
    else
        dprintf(("%s: failed to create up wrapper for nsIEventHandler %x !!!", pszFunction, aHandler));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downIPM2DeallocateMenuID(void *pvThis, nsIEventHandler *aHandler, PRInt16 aMenuID)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager2);
    dprintf(("%s: aHandler=%p aMenuID=%d", pszFunction, aHandler, aMenuID));

    nsIEventHandler * pupHandler = aHandler;
    rc = upCreateWrapper((void**)&pupHandler, kEventHandlerIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->DeallocateMenuID(pupHandler, aMenuID);
    else
        dprintf(("%s: failed to create up wrapper for nsIEventHandler %x !!!", pszFunction, aHandler));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downIPM2HasAllocatedMenuID(void *pvThis, nsIEventHandler *aHandler, PRInt16 aMenuID, PRBool *aResult)
{
    DOWN_ENTER_RC(pvThis, nsIPluginManager2);
    dprintf(("%s: aHandler=%p aMenuID=%d aResult=%p", pszFunction, aHandler, aMenuID, aResult));

    nsIEventHandler * pupHandler = aHandler;
    rc = upCreateWrapper((void**)&pupHandler, kEventHandlerIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        rc = pMozI->HasAllocatedMenuID(pupHandler, aMenuID, aResult);
        if (NS_SUCCEEDED(rc) && VALID_PTR(aResult))
            dprintf(("%s: *aResult=%d", pszFunction, *aResult));
    }
    else
        dprintf(("%s: failed to create up wrapper for nsIEventHandler %x !!!", pszFunction, aHandler));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIPluginManager2, downVFTnsIPluginManager2)
{
 {
  {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
  },
    downIPMGetValue,                                        VFTDELTA_VAL()
    downIPMReloadPlugins,                                   VFTDELTA_VAL()
    downIPMUserAgent,                                       VFTDELTA_VAL()
    downIPMGetURL,                                          VFTDELTA_VAL()
    downIPMPostURL,                                         VFTDELTA_VAL()
    downIPMRegisterPlugin,                                  VFTDELTA_VAL()
    downIPMUnregisterPlugin,                                VFTDELTA_VAL()
    downIPMGetURLWithHeaders,                               VFTDELTA_VAL()
 },
    downIPM2BeginWaitCursor,                                VFTDELTA_VAL()
    downIPM2EndWaitCursor,                                  VFTDELTA_VAL()
    downIPM2SupportsURLProtocol,                            VFTDELTA_VAL()
    downIPM2NotifyStatusChange,                             VFTDELTA_VAL()
    downIPM2FindProxyForURL,                                VFTDELTA_VAL()
    downIPM2RegisterWindow,                                 VFTDELTA_VAL()
    downIPM2UnregisterWindow,                               VFTDELTA_VAL()
    downIPM2AllocateMenuID,                                 VFTDELTA_VAL()
    downIPM2DeallocateMenuID,                               VFTDELTA_VAL()
    downIPM2HasAllocatedMenuID,                             VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();


















nsresult VFTCALL downIPIPGetValue(void *pvThis, nsPluginInstancePeerVariable aVariable, void * aValue)
{
    DOWN_ENTER_RC(pvThis, nsIPluginInstancePeer);
    dprintf(("%s: aVariable=%d aValue=%p", pszFunction, aVariable, aValue));
    rc = pMozI->GetValue(aVariable, aValue);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downIPIPGetMIMEType(void *pvThis, nsMIMEType *aMIMEType)
{
    DOWN_ENTER_RC(pvThis, nsIPluginInstancePeer);
    dprintf(("%s: aMIMEType=%p", pszFunction, aMIMEType));
    rc = pMozI->GetMIMEType(aMIMEType);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aMIMEType) && VALID_PTR(*aMIMEType))
        DPRINTF_STR(*aMIMEType);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downIPIPGetMode(void *pvThis, nsPluginMode *aMode)
{
    DOWN_ENTER_RC(pvThis, nsIPluginInstancePeer);
    dprintf(("%s: aMode=%p", pszFunction, aMode));
    rc = pMozI->GetMode(aMode);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aMode))
        dprintf(("%s: *aMode=%d", pszFunction, *aMode));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}















nsresult VFTCALL downIPIPNewStream(void *pvThis, nsMIMEType aType, const char *aTarget, nsIOutputStream **aResult)
{
    
    DOWN_ENTER_RC(pvThis, nsIPluginInstancePeer);
    dprintf(("%s: aResult=%p", pszFunction, aResult));
    DPRINTF_STR(aType);
    DPRINTF_STR(aTarget);

    const void *pvVFT = downIsSupportedInterface(kOutputStreamIID);
    if (pvVFT)
    {
        rc = pMozI->NewStream(aType, aTarget, aResult);
        rc = downCreateWrapper((void**)aResult, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported Interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 21, kOutputStreamIID.m0);
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downIPIPShowStatus(void *pvThis, const char *aMessage)
{
    
    DOWN_ENTER_RC(pvThis, nsIPluginInstancePeer);
    DPRINTF_STR(aMessage);
    verifyAndFixUTF8String(aMessage, XPFUNCTION);
    rc = pMozI->ShowStatus(aMessage);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downIPIPSetWindowSize(void *pvThis, PRUint32 aWidth, PRUint32 aHeight)
{
    DOWN_ENTER_RC(pvThis, nsIPluginInstancePeer);
    dprintf(("%s: aWidth=%d aHeight=%d", pszFunction, aWidth, aHeight));
    rc = pMozI->SetWindowSize(aWidth, aHeight);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIPluginInstancePeer, downVFTnsIPluginInstancePeer)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downIPIPGetValue,                                       VFTDELTA_VAL()
    downIPIPGetMIMEType,                                    VFTDELTA_VAL()
    downIPIPGetMode,                                        VFTDELTA_VAL()
    downIPIPNewStream,                                      VFTDELTA_VAL()
    downIPIPShowStatus,                                     VFTDELTA_VAL()
    downIPIPSetWindowSize,                                  VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();















nsresult VFTCALL downIPIP2GetJSWindow(void *pvThis, JSObject * *aJSWindow)
{
    DOWN_ENTER_RC(pvThis, nsIPluginInstancePeer2);
    rc = pMozI->GetJSWindow(aJSWindow);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aJSWindow))
    {
        
        dprintf(("%s: aJSWindow=%p - not really wrapped !!!", pszFunction, aJSWindow));
        DebugInt3();
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downIPIP2GetJSThread(void *pvThis, PRUint32 *aJSThread)
{
    DOWN_ENTER_RC(pvThis, nsIPluginInstancePeer2);
    rc = pMozI->GetJSThread(aJSThread);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aJSThread))
        dprintf(("%s: aJSThread=%u", pszFunction, aJSThread));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downIPIP2GetJSContext(void *pvThis, JSContext * *aJSContext)
{
    DOWN_ENTER_RC(pvThis, nsIPluginInstancePeer2);
    rc = pMozI->GetJSContext(aJSContext);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aJSContext))
    {
        
        dprintf(("%s: aJSContext=%p - not really wrapped !!!", pszFunction, aJSContext));
        DebugInt3();
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}

MAKE_SAFE_VFT(VFTnsIPluginInstancePeer2, downVFTnsIPluginInstancePeer2)
{
 {
  {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
  },
    downIPIPGetValue,                                       VFTDELTA_VAL()
    downIPIPGetMIMEType,                                    VFTDELTA_VAL()
    downIPIPGetMode,                                        VFTDELTA_VAL()
    downIPIPNewStream,                                      VFTDELTA_VAL()
    downIPIPShowStatus,                                     VFTDELTA_VAL()
    downIPIPSetWindowSize,                                  VFTDELTA_VAL()
 },
    downIPIP2GetJSWindow,                                   VFTDELTA_VAL()
    downIPIP2GetJSThread,                                   VFTDELTA_VAL()
    downIPIP2GetJSContext,                                  VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();
















nsresult VFTCALL downIPTIGetAttributes(void *pvThis, PRUint16 & aCount, const char* const* & aNames, const char* const* & aValues)
{
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo);
    rc = pMozI->GetAttributes(aCount, aNames, aValues);
    if (NS_SUCCEEDED(rc))
    {
        dprintf(("%s: aCount=%d", pszFunction, aCount));
        for (int i = 0; i < aCount; i++)
            dprintf(("%s: aNames[%d]='%s' (%p) aValues[%d]='%s' (%p)", pszFunction,
                     i, aNames[i], aNames[i], i, aValues[i], aValues[i]));
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downIPTIGetAttribute(void *pvThis, const char *aName, const char * *aResult)
{
    
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo);
    DPRINTF_STR(aName);
    rc = pMozI->GetAttribute(aName, aResult);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aResult) && VALID_PTR(*aResult))
        DPRINTF_STR(*aResult);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIPluginTagInfo, downVFTnsIPluginTagInfo)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downIPTIGetAttributes,                                  VFTDELTA_VAL()
    downIPTIGetAttribute,                                   VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();











nsresult VFTCALL downIPTI2GetTagType(void *pvThis, nsPluginTagType *aTagType)
{
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetTagType(aTagType);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aTagType))
        dprintf(("%s: *aTagType=%d", pszFunction, *aTagType));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downIPTI2GetTagText(void *pvThis, const char * *aTagText)
{
    
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetTagText(aTagText);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aTagText) && VALID_PTR(*aTagText))
        DPRINTF_STR(*aTagText);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downIPTI2GetParameters(void *pvThis, PRUint16 & aCount, const char* const* & aNames, const char* const* & aValues)
{
    
    
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetParameters(aCount, aNames, aValues);
    if (NS_SUCCEEDED(rc))
    {
        dprintf(("%s: aCount=%d", pszFunction, aCount));
        for (int i = 0; i < aCount; i++)
            dprintf(("%s: aNames[%d]='%s' (%p) aValues[%d]='%s' (%p)", pszFunction,
                     i, aNames[i], aNames[i], i, aValues[i], aValues[i]));
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downIPTI2GetParameter(void *pvThis, const char *aName, const char * *aResult)
{
    
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    DPRINTF_STR(aName);
    rc = pMozI->GetParameter(aName, aResult);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aResult) && VALID_PTR(*aResult))
        DPRINTF_STR(*aResult);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downIPTI2GetDocumentBase(void *pvThis, const char * *aDocumentBase)
{
    
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetDocumentBase(aDocumentBase);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aDocumentBase) && VALID_PTR(*aDocumentBase))
        DPRINTF_STR(*aDocumentBase);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}






nsresult VFTCALL downIPTI2GetDocumentEncoding(void *pvThis, const char * *aDocumentEncoding)
{
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetDocumentEncoding(aDocumentEncoding);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aDocumentEncoding) && VALID_PTR(*aDocumentEncoding))
        DPRINTF_STR(*aDocumentEncoding);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downIPTI2GetAlignment(void *pvThis, const char * *aElignment)
{
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetAlignment(aElignment);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aElignment) && VALID_PTR(*aElignment))
        DPRINTF_STR(*aElignment);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downIPTI2GetWidth(void *pvThis, PRUint32 *aWidth)
{
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetWidth(aWidth);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aWidth))
        dprintf(("%s: *aWidth=%d", pszFunction, *aWidth));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downIPTI2GetHeight(void *pvThis, PRUint32 *aHeight)
{
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetHeight(aHeight);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aHeight))
        dprintf(("%s: *aHeight=%d", pszFunction, *aHeight));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downIPTI2GetBorderVertSpace(void *pvThis, PRUint32 *aBorderVertSpace)
{
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetBorderVertSpace(aBorderVertSpace);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aBorderVertSpace))
        dprintf(("%s: *aBorderVertSpace=%d", pszFunction, *aBorderVertSpace));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downIPTI2GetBorderHorizSpace(void *pvThis, PRUint32 *aBorderHorizSpace)
{
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetBorderHorizSpace(aBorderHorizSpace);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aBorderHorizSpace))
        dprintf(("%s: *aBorderHorizSpace=%d", pszFunction, *aBorderHorizSpace));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downIPTI2GetUniqueID(void *pvThis, PRUint32 *aUniqueID)
{
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);
    rc = pMozI->GetUniqueID(aUniqueID);
    if (NS_SUCCEEDED(rc) && VALID_PTR(aUniqueID))
        dprintf(("%s: *aUniqueID=%d", pszFunction, *aUniqueID));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downIPTI2GetDOMElement(void *pvThis, nsIDOMElement * *aDOMElement)
{
    DOWN_ENTER_RC(pvThis, nsIPluginTagInfo2);

    const void *pvVFT = downIsSupportedInterface(kDOMElementIID);
    if (pvVFT)
    {
        rc = pMozI->GetDOMElement(aDOMElement);
        rc = downCreateWrapper((void**)aDOMElement, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported Interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 14, kDOMElementIID.m0);
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}



MAKE_SAFE_VFT(VFTnsIPluginTagInfo2, downVFTnsIPluginTagInfo2)
{
 {
  {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
  },
    downIPTIGetAttributes,                                  VFTDELTA_VAL()
    downIPTIGetAttribute,                                   VFTDELTA_VAL()
 },
    downIPTI2GetTagType,                                    VFTDELTA_VAL()
    downIPTI2GetTagText,                                    VFTDELTA_VAL()
    downIPTI2GetParameters,                                 VFTDELTA_VAL()
    downIPTI2GetParameter,                                  VFTDELTA_VAL()
    downIPTI2GetDocumentBase,                               VFTDELTA_VAL()
    downIPTI2GetDocumentEncoding,                           VFTDELTA_VAL()
    downIPTI2GetAlignment,                                  VFTDELTA_VAL()
    downIPTI2GetWidth,                                      VFTDELTA_VAL()
    downIPTI2GetHeight,                                     VFTDELTA_VAL()
    downIPTI2GetBorderVertSpace,                            VFTDELTA_VAL()
    downIPTI2GetBorderHorizSpace,                           VFTDELTA_VAL()
    downIPTI2GetUniqueID,                                   VFTDELTA_VAL()
    downIPTI2GetDOMElement,                                 VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();









#ifdef IPLUGINW_OUTOFTREE
nsresult VFTCALL downITMGetCurrentThread(void *pvThis, nsPluginThread* *threadID)
#else
nsresult VFTCALL downITMGetCurrentThread(void *pvThis, PRThread **threadID)
#endif
{
    DOWN_ENTER_RC(pvThis, nsIJVMThreadManager);
    rc = pMozI->GetCurrentThread(threadID);
    if (NS_SUCCEEDED(rc) && VALID_PTR(threadID))
        dprintf(("%s: *threadID=%d (0x%x)", pszFunction, *threadID, *threadID));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}






nsresult VFTCALL downITMSleep(void *pvThis, PRUint32 milli)
{
    DOWN_ENTER_RC(pvThis, nsIJVMThreadManager);
    dprintf(("%s: milli=%d", pszFunction, milli));
    rc = pMozI->Sleep(milli);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downITMEnterMonitor(void *pvThis, void* address)
{
    DOWN_ENTER_RC(pvThis, nsIJVMThreadManager);
    dprintf(("%s: address=%p", pszFunction, address));
    rc = pMozI->EnterMonitor(address);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




nsresult VFTCALL downITMExitMonitor(void *pvThis, void* address)
{
    DOWN_ENTER_RC(pvThis, nsIJVMThreadManager);
    dprintf(("%s: address=%p", pszFunction, address));
    rc = pMozI->ExitMonitor(address);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downITMWait(void *pvThis, void* address, PRUint32 milli)
{
    DOWN_ENTER_RC(pvThis, nsIJVMThreadManager);
    dprintf(("%s: address=%p milli=%u", pszFunction, address, milli));
    rc = pMozI->Wait(address, milli);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




nsresult VFTCALL downITMNotify(void *pvThis, void* address)
{
    DOWN_ENTER_RC(pvThis, nsIJVMThreadManager);
    dprintf(("%s: address=%p", pszFunction, address));
    rc = pMozI->Notify(address);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




nsresult VFTCALL downITMNotifyAll(void *pvThis, void* address)
{
    DOWN_ENTER_RC(pvThis, nsIJVMThreadManager);
    dprintf(("%s: address=%p", pszFunction, address));
    rc = pMozI->NotifyAll(address);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




#ifdef IPLUGINW_OUTOFTREE
nsresult VFTCALL downITMCreateThread(void *pvThis, PRUint32* threadID, nsIRunnable* runnable)
#else
nsresult VFTCALL downITMCreateThread(void *pvThis, PRThread **threadID, nsIRunnable* runnable)
#endif
{
    DOWN_ENTER_RC(pvThis, nsIJVMThreadManager);
    dprintf(("%s: threadID=%p runnable=%p", pszFunction, threadID, runnable));

    nsIRunnable * pupRunnable = runnable;
    rc = upCreateWrapper((void**)&pupRunnable, kRunnableIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        rc = pMozI->CreateThread(threadID, pupRunnable);
        if (NS_SUCCEEDED(rc) && VALID_PTR(threadID))
            dprintf(("%s: *threadID=%d", pszFunction, *threadID));
    }
    else
        dprintf(("%s: failed to create up wrapper for nsIRunnable %x !!!", pszFunction, runnable));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}







#ifdef IPLUGINW_OUTOFTREE
nsresult VFTCALL downITMPostEvent(void *pvThis, PRUint32 threadID, nsIRunnable* runnable, PRBool async)
#else
nsresult VFTCALL downITMPostEvent(void *pvThis, PRThread *threadID, nsIRunnable* runnable, PRBool async)
#endif
{
    DOWN_ENTER_RC(pvThis, nsIJVMThreadManager);
    dprintf(("%s: threadID=%d runnable=%p async=%d", pszFunction, threadID, runnable, async));

    nsIRunnable * pupRunnable = runnable;
    rc = upCreateWrapper((void**)&pupRunnable, kRunnableIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->PostEvent(threadID, pupRunnable, async);
    else
        dprintf(("%s: failed to create up wrapper for nsIRunnable %x !!!", pszFunction, runnable));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}



MAKE_SAFE_VFT(VFTnsIJVMThreadManager, downVFTnsIJVMThreadManager)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downITMGetCurrentThread,                                VFTDELTA_VAL()
    downITMSleep,                                           VFTDELTA_VAL()
    downITMEnterMonitor,                                    VFTDELTA_VAL()
    downITMExitMonitor,                                     VFTDELTA_VAL()
    downITMWait,                                            VFTDELTA_VAL()
    downITMNotify,                                          VFTDELTA_VAL()
    downITMNotifyAll,                                       VFTDELTA_VAL()
    downITMCreateThread,                                    VFTDELTA_VAL()
    downITMPostEvent,                                       VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();












nsresult VFTCALL downCreateProxyJNI(void *pvThis, nsISecureEnv *secureEnv, JNIEnv * *outProxyEnv)
{
    DOWN_ENTER_RC(pvThis, nsIJVMManager);

    nsISecureEnv *pupSecureEnv = secureEnv;
    rc = upCreateWrapper((void**)&pupSecureEnv, kSecureEnvIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        rc = pMozI->CreateProxyJNI(pupSecureEnv, outProxyEnv);
        rc = downCreateJNIEnvWrapper(outProxyEnv, rc);
    }
    else
        dprintf(("%s: failed to create up wrapper for kSecureEnvIID %x !!!", pszFunction, secureEnv));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downGetProxyJNI(void *pvThis, JNIEnv * *outProxyEnv)
{
    DOWN_ENTER_RC(pvThis, nsIJVMManager);

    rc = pMozI->GetProxyJNI(outProxyEnv);
    rc = downCreateJNIEnvWrapper(outProxyEnv, rc);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downShowJavaConsole(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIJVMManager);

    dprintf(("%s", pszFunction));
    rc = pMozI->ShowJavaConsole();

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




nsresult VFTCALL downIsAllPermissionGranted(void *pvThis, const char *lastFingerprint, const char *lastCommonName, const char *rootFingerprint, const char *rootCommonName, PRBool *_retval)
{
    
    DOWN_ENTER_RC(pvThis, nsIJVMManager);
    dprintf(("%s _retval=%x", pszFunction, _retval));
    DPRINTF_STR(lastFingerprint);
    DPRINTF_STR(lastCommonName);
    DPRINTF_STR(rootFingerprint);
    DPRINTF_STR(rootCommonName);

    rc = pMozI->IsAllPermissionGranted(lastFingerprint, lastCommonName, rootFingerprint, rootCommonName, _retval);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




nsresult VFTCALL downIsAppletTrusted(void *pvThis, const char *aRSABuf, PRUint32 aRSABufLen, const char *aPlaintext, PRUint32 aPlaintextLen, PRBool *isTrusted, nsIPrincipal **_retval)
{
    
    DOWN_ENTER_RC(pvThis, nsIJVMManager);

    dprintf(("%s aRSABuf=%x aRSABufLen=%d aPlaintext=%x aPlaintextLen=%d isTrusted=%x _retval=%x", pszFunction, aRSABuf, aRSABufLen, aPlaintext, aPlaintextLen, isTrusted, _retval));
    rc = pMozI->IsAppletTrusted(aRSABuf, aRSABufLen, aPlaintext, aPlaintextLen, isTrusted, _retval);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




nsresult VFTCALL downGetJavaEnabled(void *pvThis, PRBool *aJavaEnabled)
{
    DOWN_ENTER_RC(pvThis, nsIJVMManager);

    dprintf(("%s aJavaEnabled=%x", pszFunction, aJavaEnabled));
    rc = pMozI->GetJavaEnabled(aJavaEnabled);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIJVMManager, downVFTnsIJVMManager)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downCreateProxyJNI,                                     VFTDELTA_VAL()
    downGetProxyJNI,                                        VFTDELTA_VAL()
    downShowJavaConsole,                                    VFTDELTA_VAL()
    downIsAllPermissionGranted,                             VFTDELTA_VAL()
    downIsAppletTrusted,                                    VFTDELTA_VAL()
    downGetJavaEnabled,                                     VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();


















nsresult VFTCALL downGetMember(void *pvThis, JNIEnv *jEnv, jsobject jsobj, const jchar *name, jsize length, void* principalsArray[],
                                  int numPrincipals, nsISupports *securitySupports, jobject *pjobj)
{
    DOWN_ENTER_RC(pvThis, nsILiveconnect);
    dprintf(("%s: jEnv=%p jsobj=%p name=%p length=%d principalsArray=%p numPrincipals=%d securitySupports=%p pjobj=%p",
             pszFunction, jEnv, jsobj, name, length, principalsArray, numPrincipals, securitySupports, pjobj));
    if (VALID_PTR(name))
        dprintf(("%s: name=%ls", pszFunction, name));

    if (numPrincipals)
    {
        


        dprintf(("%s: numPrincipals/princiapalsArray isn't supported !!!", pszFunction));
        ReleaseInt3(0xdead0000, 0, 7);
    }

    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports *pupSecuritySupports = securitySupports;
        rc = upCreateWrapper((void**)&pupSecuritySupports, kSupportsIID, rc);
        if (NS_SUCCEEDED(rc))
        {
            rc = pMozI->GetMember(pupJNIEnv, jsobj, name, length, principalsArray, numPrincipals, pupSecuritySupports, pjobj);
            if (NS_SUCCEEDED(rc) && VALID_PTR(pjobj))
                dprintf(("%s: *pjobj=%p", pszFunction, *pjobj));
        }
        else
            dprintf(("%s: Failed make up wrapper for nsISupports !!!", pszFunction));
    }
    else
        dprintf(("%s: Failed make up wrapper for JNIEnv !!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downGetSlot(void *pvThis, JNIEnv *jEnv, jsobject jsobj, jint slot, void* principalsArray[],
                                int numPrincipals, nsISupports *securitySupports, jobject *pjobj)
{
    DOWN_ENTER_RC(pvThis, nsILiveconnect);
    dprintf(("%s: jEnv=%p jsobj=%p slot=%p principalsArray=%p numPrincipals=%d securitySupports=%p pjobj=%p",
             pszFunction, jEnv, jsobj, slot, principalsArray, numPrincipals, securitySupports, pjobj));

    if (numPrincipals)
    {
        


        dprintf(("%s: numPrincipals/princiapalsArray isn't supported !!!", pszFunction));
        ReleaseInt3(0xdead0000, 0, 8);
    }

    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports *pupSecuritySupports = securitySupports;
        rc = upCreateWrapper((void**)&pupSecuritySupports, kSupportsIID, rc);
        if (NS_SUCCEEDED(rc))
        {
            rc = pMozI->GetSlot(pupJNIEnv, jsobj, slot, principalsArray, numPrincipals, pupSecuritySupports, pjobj);
            if (NS_SUCCEEDED(rc) && VALID_PTR(pjobj))
                dprintf(("%s: *pjobj=%p", pszFunction, *pjobj));
        }
        else
            dprintf(("%s: Failed make up wrapper for nsISupports !!!", pszFunction));
    }
    else
        dprintf(("%s: Failed make up wrapper for JNIEnv !!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downSetMember(void *pvThis, JNIEnv *jEnv, jsobject jsobj, const jchar* name, jsize length, jobject jobj, void* principalsArray[],
                                  int numPrincipals, nsISupports *securitySupports)
{
    DOWN_ENTER_RC(pvThis, nsILiveconnect);
    dprintf(("%s: jEnv=%p jsobj=%p name=%p length=%d jobj=%p principalsArray=%p numPrincipals=%d securitySupports=%p",
             pszFunction, jEnv, jsobj, name, length, jobj, principalsArray, numPrincipals, securitySupports));
    DPRINTF_STR(name);

    if (numPrincipals)
    {
        


        dprintf(("%s: numPrincipals/princiapalsArray isn't supported !!!", pszFunction));
        ReleaseInt3(0xdead0000, 0, 9);
    }

    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports *pupSecuritySupports = securitySupports;
        rc = upCreateWrapper((void**)&pupSecuritySupports, kSupportsIID, rc);
        if (NS_SUCCEEDED(rc))
            rc = pMozI->SetMember(pupJNIEnv, jsobj, name, length, jobj, principalsArray, numPrincipals, pupSecuritySupports);
        else
            dprintf(("%s: Failed make up wrapper for nsISupports !!!", pszFunction));
    }
    else
        dprintf(("%s: Failed make up wrapper for JNIEnv !!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downSetSlot(void *pvThis, JNIEnv *jEnv, jsobject jsobj, jint slot, jobject jobj, void* principalsArray[],
                                int numPrincipals, nsISupports *securitySupports)
{
    DOWN_ENTER_RC(pvThis, nsILiveconnect);
    dprintf(("%s: jEnv=%p jsobj=%p slot=%p jobj=%p principalsArray=%p numPrincipals=%d securitySupports=%p",
             pszFunction, jEnv, jsobj, slot, jobj, principalsArray, numPrincipals, securitySupports));

    if (numPrincipals)
    {
        


        dprintf(("%s: numPrincipals/princiapalsArray isn't supported !!!", pszFunction));
        ReleaseInt3(0xdead0000, 0, 8);
    }

    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports *pupSecuritySupports = securitySupports;
        rc = upCreateWrapper((void**)&pupSecuritySupports, kSupportsIID, rc);
        if (NS_SUCCEEDED(rc))
            rc = pMozI->SetSlot(pupJNIEnv, jsobj, slot, jobj, principalsArray, numPrincipals, pupSecuritySupports);
        else
            dprintf(("%s: Failed make up wrapper for nsISupports !!!", pszFunction));
    }
    else
        dprintf(("%s: Failed make up wrapper for JNIEnv !!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}







nsresult VFTCALL downRemoveMember(void *pvThis, JNIEnv *jEnv, jsobject jsobj, const jchar* name, jsize length,  void* principalsArray[],
                                     int numPrincipals, nsISupports *securitySupports)
{
    DOWN_ENTER_RC(pvThis, nsILiveconnect);
    dprintf(("%s: jEnv=%p jsobj=%p name=%p length=%d principalsArray=%p numPrincipals=%d securitySupports=%p",
             pszFunction, jEnv, jsobj, name, length, principalsArray, numPrincipals, securitySupports));
    if (VALID_PTR(name))
        dprintf(("%s: name=%ls", pszFunction, name));

    if (numPrincipals)
    {
        


        dprintf(("%s: numPrincipals/princiapalsArray isn't supported !!!", pszFunction));
        ReleaseInt3(0xdead0000, 0, 10);
    }

    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports *pupSecuritySupports = securitySupports;
        rc = upCreateWrapper((void**)&pupSecuritySupports, kSupportsIID, rc);
        if (NS_SUCCEEDED(rc))
            rc = pMozI->RemoveMember(pupJNIEnv, jsobj, name, length, principalsArray, numPrincipals, pupSecuritySupports);
        else
            dprintf(("%s: Failed make up wrapper for nsISupports !!!", pszFunction));
    }
    else
        dprintf(("%s: Failed make up wrapper for JNIEnv !!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downCall(void *pvThis, JNIEnv *jEnv, jsobject jsobj, const jchar* name, jsize length, jobjectArray jobjArr,  void* principalsArray[],
                             int numPrincipals, nsISupports *securitySupports, jobject *pjobj)
{
    DOWN_ENTER_RC(pvThis, nsILiveconnect);
    dprintf(("%s: jEnv=%p jsobj=%p name=%p length=%d jobjArr=%p principalsArray=%p numPrincipals=%d securitySupports=%p pjobj=%p",
             pszFunction, jEnv, jsobj, name, length, jobjArr, principalsArray, numPrincipals, securitySupports, pjobj));
    if (VALID_PTR(name))
        dprintf(("%s: name=%ls", pszFunction, name));

    if (numPrincipals)
    {
        


        dprintf(("%s: numPrincipals/princiapalsArray isn't supported !!!", pszFunction));
        ReleaseInt3(0xdead0000, 0, 11);
    }

    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports *pupSecuritySupports = securitySupports;
        rc = upCreateWrapper((void**)&pupSecuritySupports, kSupportsIID, rc);
        if (NS_SUCCEEDED(rc))
        {
            rc = pMozI->Call(pupJNIEnv, jsobj, name, length, jobjArr, principalsArray, numPrincipals, pupSecuritySupports, pjobj);
            if (NS_SUCCEEDED(rc) && VALID_PTR(pjobj))
                dprintf(("%s: *pjobj=%p", pszFunction, *pjobj));
        }
        else
            dprintf(("%s: Failed make up wrapper for nsISupports !!!", pszFunction));
    }
    else
        dprintf(("%s: Failed make up wrapper for JNIEnv !!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downEval(void *pvThis, JNIEnv *jEnv, jsobject jsobj, const jchar *script, jsize length, void* principalsArray[],
                             int numPrincipals, nsISupports *securitySupports, jobject *pjobj)
{
    DOWN_ENTER_RC(pvThis, nsILiveconnect);
    dprintf(("%s: jEnv=%p jsobj=%p script=%p length=%d principalsArray=%p numPrincipals=%d securitySupports=%p pjobj=%p",
             pszFunction, jEnv, jsobj, script, length, principalsArray, numPrincipals, securitySupports, pjobj));
    if (VALID_PTR(script))
        dprintf(("%s: script=%ls", pszFunction, script));

    if (numPrincipals)
    {
        


        dprintf(("%s: numPrincipals/princiapalsArray isn't supported !!!", pszFunction));
        ReleaseInt3(0xdead0000, 0, 12);
    }

    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports *pupSecuritySupports = securitySupports;
        rc = upCreateWrapper((void**)&pupSecuritySupports, kSupportsIID, rc);
        if (NS_SUCCEEDED(rc))
        {
            rc = pMozI->Eval(pupJNIEnv, jsobj, script, length, principalsArray, numPrincipals, pupSecuritySupports, pjobj);
            if (NS_SUCCEEDED(rc) && VALID_PTR(pjobj))
                dprintf(("%s: *pjobj=%p", pszFunction, *pjobj));
        }
        else
            dprintf(("%s: Failed make up wrapper for nsISupports !!!", pszFunction));
    }
    else
        dprintf(("%s: Failed make up wrapper for JNIEnv !!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downGetWindow(void *pvThis, JNIEnv *jEnv, void *pJavaObject, void* principalsArray[],
                                  int numPrincipals, nsISupports *securitySupports, jsobject *pobj)
{
    DOWN_ENTER_RC(pvThis, nsILiveconnect);
    dprintf(("%s: jEnv=%p pJavaObject=%p principalsArray=%p numPrincipals=%d securitySupports=%p pObj=%p",
             pszFunction, jEnv, pJavaObject, principalsArray, numPrincipals, securitySupports, pobj));
    if (numPrincipals)
    {
        
        dprintf(("%s: numPrincipals/princiapalsArray isn't supported !!!", pszFunction));
        ReleaseInt3(0xdead0000, 0, 6);
    }

    


    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports *pupSecuritySupports = securitySupports;
        rc = upCreateWrapper((void**)&pupSecuritySupports, kSupportsIID, rc);
        if (NS_SUCCEEDED(rc))
        {
            








            nsIPluginInstance *pupIPluginInstance = (nsIPluginInstance *)pJavaObject;
            rc = upCreateWrapper((void**)&pupIPluginInstance, kPluginInstanceIID, rc);
            if (NS_SUCCEEDED(rc))
            {
                rc = pMozI->GetWindow(pupJNIEnv, pupIPluginInstance, principalsArray, numPrincipals, pupSecuritySupports, pobj);
                if (NS_SUCCEEDED(rc) && VALID_PTR(pobj))
                    
                    
                    dprintf(("%s: *pobj=%d (0x%x)", XPFUNCTION, *pobj, *pobj));
            }
            else
                dprintf(("%s: Failed to create up wrapper for nsISupports %p !!!", pszFunction, securitySupports));
        }
        else
            dprintf(("%s: Failed to create up wrapper for nsISupports %p !!!", pszFunction, securitySupports));
    }
    else
    {
        dprintf(("%s: Failed to make up wrapper for JNIEnv !!!", pszFunction));
        rc = NS_ERROR_UNEXPECTED;
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}







nsresult VFTCALL downFinalizeJSObject(void *pvThis, JNIEnv *jEnv, jsobject jsobj)
{
    DOWN_ENTER_RC(pvThis, nsILiveconnect);
    dprintf(("%s: jEnv=%p jsobj=%p", pszFunction, jEnv, jsobj));

    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->FinalizeJSObject(pupJNIEnv, jsobj);
    else
        dprintf(("%s: Failed make up wrapper for JNIEnv !!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downToString(void *pvThis, JNIEnv *jEnv, jsobject obj, jstring *pjstring)
{
    DOWN_ENTER_RC(pvThis, nsILiveconnect);
    dprintf(("%s: jEnv=%p obj=%p pjstring=%p", pszFunction, jEnv, obj, pjstring));

    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        rc = pMozI->ToString(pupJNIEnv, obj, pjstring);
        if (NS_SUCCEEDED(rc) && VALID_PTR(pjstring))
            dprintf(("%s: *pjstring=%p", pszFunction, pjstring));
    }
    else
        dprintf(("%s: Failed make up wrapper for JNIEnv !!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsILiveconnect, downVFTnsILiveconnect)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downGetMember,                                          VFTDELTA_VAL()
    downGetSlot,                                            VFTDELTA_VAL()
    downSetMember,                                          VFTDELTA_VAL()
    downSetSlot,                                            VFTDELTA_VAL()
    downRemoveMember,                                       VFTDELTA_VAL()
    downCall,                                               VFTDELTA_VAL()
    downEval,                                               VFTDELTA_VAL()
    downGetWindow,                                          VFTDELTA_VAL()
    downFinalizeJSObject,                                   VFTDELTA_VAL()
    downToString,                                           VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();

















nsresult VFTCALL downSecureLiveConnectEval(
     void *pvThis, JNIEnv *jEnv, jsobject jsobj, const jchar *script, jsize length, void **pNSIPrincipaArray,
     int numPrincipals, void *pNSISecurityContext, jobject *pjobj)
{
    DOWN_ENTER_RC(pvThis, nsISecureLiveconnect);
    dprintf(("%s: jEnv=%p jsobj=%p script=%p length=%d principalsArray=%p numPrincipals=%d pNSISecurityContext=%p pjobj=%p",
             pszFunction, jEnv, jsobj, script, length, pNSIPrincipaArray, numPrincipals, pNSISecurityContext, pjobj));
    if (VALID_PTR(script))
        dprintf(("%s: script=%ls", pszFunction, script));

    


    DebugInt3();

    if (numPrincipals)
    {
        


        dprintf(("%s: numPrincipals/princiapalsArray isn't supported !!!", pszFunction));
        ReleaseInt3(0xdead0000, 0, 14);
    }

    JNIEnv * pupJNIEnv = jEnv;
    rc = upCreateJNIEnvWrapper(&pupJNIEnv, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        DebugInt3(); 
        nsISecurityContext *pupSecurityContext = (nsISecurityContext*)pNSISecurityContext;
        rc = upCreateWrapper((void**)&pupSecurityContext, kSecurityContextIID, rc);
        if (NS_SUCCEEDED(rc))
        {
            rc = pMozI->Eval(pupJNIEnv, jsobj, script, length, pNSIPrincipaArray, numPrincipals, pupSecurityContext, pjobj);
            if (NS_SUCCEEDED(rc) && VALID_PTR(pjobj))
                dprintf(("%s: *pjobj=%p", pszFunction, *pjobj));
        }
        else
            dprintf(("%s: Failed make up wrapper for nsISupports !!!", pszFunction));
    }
    else
        dprintf(("%s: Failed make up wrapper for JNIEnv !!!", pszFunction));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}

MAKE_SAFE_VFT(VFTnsISecureLiveconnect, downVFTnsISecureLiveconnect)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downSecureLiveConnectEval,                              VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();















nsresult VFTCALL downImplies(void *pvThis, const char* target, const char* action, PRBool *bAllowedAccess)
{
    DOWN_ENTER_RC(pvThis, nsISecurityContext);
    dprintf(("%s: target=%x action=%x bAllowedAccess=%x", pszFunction, target, action, bAllowedAccess));
    DPRINTF_STR(target);
    DPRINTF_STR(action);

    rc = pMozI->Implies(target, action, bAllowedAccess);
    if (VALID_PTR(bAllowedAccess))
        dprintf(("%s: *bAllowedAccess=%d", pszFunction, *bAllowedAccess));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downGetOrigin(void *pvThis, char* buf, int len)
{
    DOWN_ENTER_RC(pvThis, nsISecurityContext);
    dprintf(("%s: buf=%p len=%d", pszFunction, buf, len));

    rc = pMozI->GetOrigin(buf, len);
    if (NS_SUCCEEDED(rc) && VALID_PTR(buf))
        DPRINTF_STR(buf);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downGetCertificateID(void *pvThis, char* buf, int len)
{
    DOWN_ENTER_RC(pvThis, nsISecurityContext);
    dprintf(("%s: buf=%p len=%d", pszFunction, buf, len));

    rc = pMozI->GetCertificateID(buf, len);
    if (NS_SUCCEEDED(rc) && VALID_PTR(buf))
        DPRINTF_STR(buf);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsISecurityContext, downVFTnsISecurityContext)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downImplies,                                            VFTDELTA_VAL()
    downGetOrigin,                                          VFTDELTA_VAL()
    downGetCertificateID,                                   VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();















nsresult VFTCALL downFindFactory(void *pvThis, const nsCID & aClass, nsIFactory **_retval)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);

    const void * pvVFT = downIsSupportedInterface(kFactoryIID);
    if (pvVFT)
    {
        rc = pMozI->FindFactory(aClass, _retval);
        rc = downCreateWrapper((void**)_retval, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported down interface kFactoryIID!!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 29, kFactoryIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}










nsresult VFTCALL downGetClassObject(void *pvThis, const nsCID & aClass, const nsIID & aIID, void * *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);
    DPRINTF_NSID(aIID);

    


    const void * pvVFT = downIsSupportedInterface(aIID);
    if (pvVFT)
    {
        rc = pMozI->GetClassObject(aClass, aIID, _retval);
        rc = downCreateWrapper(_retval, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported down interface !!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 27, aIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}












nsresult VFTCALL downContractIDToClassID(void *pvThis, const char *aContractID, nsCID *aClass)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_CONTRACTID(aContractID);

    rc = pMozI->ContractIDToClassID(aContractID, aClass);
    if (NS_SUCCEEDED(rc))
        DPRINTF_NSID(*aClass);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}












nsresult VFTCALL downCLSIDToContractID(void *pvThis, const nsCID & aClass, char **aClassName, char **_retval)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);

    rc = pMozI->CLSIDToContractID(aClass, aClassName, _retval);
    if (NS_SUCCEEDED(rc))
    {
        if (VALID_PTR(aClassName))
            DPRINTF_STR(aClassName);
        if (VALID_PTR(_retval))
            DPRINTF_STR(_retval);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downCreateInstance(void *pvThis, const nsCID & aClass, nsISupports *aDelegate, const nsIID & aIID, void * *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);
    DPRINTF_NSID(aIID);
    dprintf(("%s: aDelegate=%p _retval=%p", pszFunction, aDelegate, _retval));


    


    const void * pvVFT = downIsSupportedInterface(aIID);
    if (pvVFT)
    {
        




        nsISupports * pupOuter = aDelegate;
        rc = upCreateWrapper((void**)&pupOuter, kSupportsIID, NS_OK);
        if (rc == NS_OK)
        {
            rc = pMozI->CreateInstance(aClass, pupOuter, aIID, _retval);
            rc = downCreateWrapper(_retval, pvVFT, rc);
        }
        else
            dprintf(("%s: failed to create up wrapper for kSupportsIID %x !!!", pszFunction, aDelegate));
    }
    else
    {
        dprintf(("%s: Unsupported down interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 9, aIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}













nsresult VFTCALL downCreateInstanceByContractID(void *pvThis, const char *aContractID, nsISupports *aDelegate, const nsIID & aIID, void * *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_NSID(aIID);
    dprintf(("%s: aDelegate=%p _retval=%p", pszFunction, aDelegate, _retval));


    


    const void * pvVFT = downIsSupportedInterface(aIID);
    if (pvVFT)
    {
        




        nsISupports * pupOuter = aDelegate;
        rc = upCreateWrapper((void**)&pupOuter, kSupportsIID, NS_OK);
        if (rc == NS_OK)
        {
            rc = pMozI->CreateInstanceByContractID(aContractID, pupOuter, aIID, _retval);
            rc = downCreateWrapper(_retval, pvVFT, rc);
        }
        else
            dprintf(("%s: failed to create up wrapper for kSupportsIID %x !!!", pszFunction, aDelegate));
    }
    else
    {
        dprintf(("%s: Unsupported down interface!!!", pszFunction));
        ReleaseInt3(0xbaddbeef, 30, aIID.m0);
        rc = NS_NOINTERFACE;
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downRegistryLocationForSpec(void *pvThis, nsIFile *aSpec, char **_retval)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    dprintf(("%s: aSpec=%p _retval=%p", pszFunction, aSpec, _retval));

    nsIFile *   pupFile = aSpec;
    rc = upCreateWrapper((void**)&pupFile, kFileIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        rc = pMozI->RegistryLocationForSpec(pupFile, _retval);
        if (NS_SUCCEEDED(rc) && VALID_PTR(_retval) && VALID_PTR(*_retval))
            DPRINTF_STR(*_retval);
    }
    else
    {
        dprintf(("%s: Unsupported up interface nsIFile !!!", pszFunction));
        ReleaseInt3(0xbaddbeef, 20, kFileIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downSpecForRegistryLocation(void *pvThis, const char *aLocation, nsIFile **_retval)
{
    
    
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_STR(aLocation);
    dprintf(("%s: _retval=%p", pszFunction, _retval));

    const void *pvVFT = downIsSupportedInterface(kFileIID);
    if (pvVFT)
    {
        rc = pMozI->SpecForRegistryLocation(aLocation, _retval);
        rc = downCreateWrapper((void**)_retval, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported down interface nsIFile !!!", pszFunction));
        ReleaseInt3(0xbaddbeef, 22, kFileIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}














nsresult VFTCALL downRegisterFactory(void *pvThis, const nsCID & aClass, const char *aClassName, const char *aContractID, nsIFactory *aFactory, PRBool aReplace)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_STR(aClassName);
    dprintf(("%s: aFactory=%p aReplace=%d", pszFunction, aFactory, aReplace));

    nsIFactory  *pupFactory = aFactory;
    rc = upCreateWrapper((void**)&pupFactory, kFactoryIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->RegisterFactory(aClass, aClassName, aContractID, pupFactory, aReplace);
    else
    {
        dprintf(("%s: Failed to make up wrapper for nsIFactoryIID %p !!!", pszFunction, aFactory));
        ReleaseInt3(0xbaddbeef, 13, kFactoryIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




















nsresult VFTCALL downRegisterComponent(void *pvThis, const nsCID & aClass, const char *aClassName, const char *aContractID, const char *aLocation, PRBool aReplace, PRBool aPersist)
{
    
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_STR(aClassName);
    DPRINTF_STR(aLocation);
    dprintf(("%s: aReplace=%d aPersist=%d", pszFunction, aReplace, aPersist));

    rc = pMozI->RegisterComponent(aClass, aClassName, aContractID, aLocation, aReplace, aPersist);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





















nsresult VFTCALL downRegisterComponentWithType(void *pvThis, const nsCID & aClass, const char *aClassName, const char *aContractID, nsIFile *aSpec, const char *aLocation, PRBool aReplace, PRBool aPersist, const char *aType)
{
    
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_STR(aClassName);
    DPRINTF_STR(aLocation);
    DPRINTF_STR(aType);
    dprintf(("%s: aReplace=%d aPersist=%d", pszFunction, aReplace, aPersist));

    rc = pMozI->RegisterComponentWithType(aClass, aClassName, aContractID, aSpec, aLocation, aReplace, aPersist, aType);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}

















nsresult VFTCALL downRegisterComponentSpec(void *pvThis, const nsCID & aClass, const char *aClassName, const char *aContractID, nsIFile *aLibrary, PRBool aReplace, PRBool aPersist)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_STR(aClassName);
    DPRINTF_STR(aLibrary);
    dprintf(("%s: aReplace=%d aPersist=%d", pszFunction, aReplace, aPersist));

    nsIFile *pupLibrary = aLibrary;
    rc = upCreateWrapper((void**)&pupLibrary, kFileIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->RegisterComponentSpec(aClass, aClassName, aContractID, pupLibrary, aReplace, aPersist);
    else
    {
        dprintf(("%s: Failed to make up wrapper for nsIFile %p !!!", pszFunction, aLibrary));
        ReleaseInt3(0xbaddbeef, 23, kFileIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


















nsresult VFTCALL downRegisterComponentLib(void *pvThis, const nsCID & aClass, const char *aClassName, const char *aContractID, const char *aDllName, PRBool aReplace, PRBool aPersist)
{
    
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_STR(aClassName);
    DPRINTF_STR(aDllName);
    dprintf(("%s: aReplace=%d aPersist=%d", pszFunction, aReplace, aPersist));

    rc = pMozI->RegisterComponentLib(aClass, aClassName, aContractID, aDllName, aReplace, aPersist);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downUnregisterFactory(void *pvThis, const nsCID & aClass, nsIFactory *aFactory)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);
    dprintf(("%s: aFactory=%p", pszFunction, aFactory));

    nsIFactory *pupFactory = aFactory;
    rc = upCreateWrapper((void**)&pupFactory, kFactoryIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->UnregisterFactory(aClass, pupFactory);
    else
    {
        dprintf(("%s: Failed to make up wrapper for nsIFactory %p !!!", pszFunction, aFactory));
        ReleaseInt3(0xbaddbeef, 15, kFactoryIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}













nsresult VFTCALL downUnregisterComponent(void *pvThis, const nsCID & aClass, const char *aLocation)
{
    
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);
    DPRINTF_STR(aLocation);

    rc = pMozI->UnregisterComponent(aClass, aLocation);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downUnregisterComponentSpec(void *pvThis, const nsCID & aClass, nsIFile *aLibrarySpec)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);
    dprintf(("%s: aLibrarySpec=%p", pszFunction, aLibrarySpec));

    nsIFile * pupLibrarySpec = aLibrarySpec;
    rc = upCreateWrapper((void**)&pupLibrarySpec, kFileIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->UnregisterComponentSpec(aClass, pupLibrarySpec);
    else
    {
        dprintf(("%s: Failed to make up wrapper for nsIFile %p !!!", pszFunction, aLibrarySpec));
        ReleaseInt3(0xbaddbeef, 16, kFileIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}







nsresult VFTCALL downFreeLibraries(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);

    rc = pMozI->FreeLibraries();

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}












nsresult VFTCALL downAutoRegister(void *pvThis, PRInt32 when, nsIFile *directory)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    dprintf(("%s: when=%d directory=%p", pszFunction, when, directory));

    nsIFile * pupDirectory = directory;
    rc = upCreateWrapper((void**)&pupDirectory, kFileIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->AutoRegister(when, pupDirectory);
    else
    {
        dprintf(("%s: Failed to make up wrapper for nsIFile %p !!!", pszFunction, directory));
        ReleaseInt3(0xbaddbeef, 24, kFileIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}












nsresult VFTCALL downAutoRegisterComponent(void *pvThis, PRInt32 when, nsIFile *aFileLocation)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    dprintf(("%s: when=%d aFileLocation=%p", pszFunction, when, aFileLocation));

    nsIFile * pupFileLocation = aFileLocation;
    rc = upCreateWrapper((void**)&pupFileLocation, kFileIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->AutoRegisterComponent(when, pupFileLocation);
    else
    {
        dprintf(("%s: Failed to make up wrapper for nsIFile %p !!!", pszFunction, aFileLocation));
        ReleaseInt3(0xbaddbeef, 25, kFileIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downAutoUnregisterComponent(void *pvThis, PRInt32 when, nsIFile *aFileLocation)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    dprintf(("%s: when=%d aFileLocation=%p", pszFunction, when, aFileLocation));

    nsIFile * pupFileLocation = aFileLocation;
    rc = upCreateWrapper((void**)&pupFileLocation, kFileIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->AutoUnregisterComponent(when, aFileLocation);
    else
    {
        dprintf(("%s: Failed to make up wrapper for nsIFile %p !!!", pszFunction, aFileLocation));
        ReleaseInt3(0xbaddbeef, 26, kFileIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downIsRegistered(void *pvThis, const nsCID & aClass, PRBool *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);
    DPRINTF_NSID(aClass);

    rc = pMozI->IsRegistered(aClass, _retval);
    if (NS_SUCCEEDED(rc) && VALID_PTR(_retval))
        dprintf(("%s: _retval=%d", pszFunction, *_retval));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downEnumerateCLSIDs(void *pvThis, nsIEnumerator **_retval)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);

    const void *pvVFT = downIsSupportedInterface(kEnumeratorIID);
    if (pvVFT)
    {
        rc = pMozI->EnumerateCLSIDs(_retval);
        rc = downCreateWrapper((void**)_retval, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported down interface nsIEnumerator !!!", pszFunction));
        ReleaseInt3(0xbaddbeef, 17, kEnumeratorIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}









nsresult VFTCALL downEnumerateContractIDs(void *pvThis, nsIEnumerator **_retval)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManagerObsolete);

    const void *pvVFT = downIsSupportedInterface(kEnumeratorIID);
    if (pvVFT)
    {
        rc = pMozI->EnumerateContractIDs(_retval);
        rc = downCreateWrapper((void**)_retval, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported down interface nsIEnumerator !!!", pszFunction));
        ReleaseInt3(0xbaddbeef, 18, kEnumeratorIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIComponentManagerObsolete, downVFTnsIComponentManagerObsolete)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downFindFactory,                                        VFTDELTA_VAL()
    downGetClassObject,                                     VFTDELTA_VAL()
    downContractIDToClassID,                                VFTDELTA_VAL()
    downCLSIDToContractID,                                  VFTDELTA_VAL()
    downCreateInstance,                                     VFTDELTA_VAL()
    downCreateInstanceByContractID,                         VFTDELTA_VAL()
    downRegistryLocationForSpec,                            VFTDELTA_VAL()
    downSpecForRegistryLocation,                            VFTDELTA_VAL()
    downRegisterFactory,                                    VFTDELTA_VAL()
    downRegisterComponent,                                  VFTDELTA_VAL()
    downRegisterComponentWithType,                          VFTDELTA_VAL()
    downRegisterComponentSpec,                              VFTDELTA_VAL()
    downRegisterComponentLib,                               VFTDELTA_VAL()
    downUnregisterFactory,                                  VFTDELTA_VAL()
    downUnregisterComponent,                                VFTDELTA_VAL()
    downUnregisterComponentSpec,                            VFTDELTA_VAL()
    downFreeLibraries,                                      VFTDELTA_VAL()
    downAutoRegister,                                       VFTDELTA_VAL()
    downAutoRegisterComponent,                              VFTDELTA_VAL()
    downAutoUnregisterComponent,                            VFTDELTA_VAL()
    downIsRegistered,                                       VFTDELTA_VAL()
    downEnumerateCLSIDs,                                    VFTDELTA_VAL()
    downEnumerateContractIDs,                               VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();














nsresult VFTCALL downCMGetClassObject(void *pvThis, const nsCID & aClass, const nsIID & aIID, void * *result)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManager);
    DPRINTF_NSID(aClass);
    DPRINTF_NSID(aIID);

    


    const void * pvVFT = downIsSupportedInterface(aIID);
    if (pvVFT)
    {
        rc = pMozI->GetClassObject(aClass, aIID, result);
        rc = downCreateWrapper(result, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported down interface !!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 28, aIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downCMGetClassObjectByContractID(void *pvThis, const char *aContractID, const nsIID & aIID, void * *result)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManager);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_NSID(aIID);

    


    const void * pvVFT = downIsSupportedInterface(aIID);
    if (pvVFT)
    {
        rc = pMozI->GetClassObjectByContractID(aContractID, aIID, result);
        rc = downCreateWrapper(result, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported down interface !!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 19, aIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downCMCreateInstance(void *pvThis, const nsCID & aClass, nsISupports *aDelegate, const nsIID & aIID, void * *result)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManager);
    DPRINTF_NSID(aClass);
    DPRINTF_NSID(aIID);
    dprintf(("%s: aDelegate=%p result=%p", pszFunction, aDelegate, result));


    


    const void * pvVFT = downIsSupportedInterface(aIID);
    if (pvVFT)
    {
        




        nsISupports * pupOuter = aDelegate;
        rc = upCreateWrapper((void**)&pupOuter, kSupportsIID, NS_OK);
        if (rc == NS_OK)
        {
            rc = pMozI->CreateInstance(aClass, pupOuter, aIID, result);
            rc = downCreateWrapper(result, pvVFT, rc);
        }
        else
            dprintf(("%s: failed to create up wrapper for kSupportsIID %x !!!", pszFunction, aDelegate));
    }
    else
    {
        dprintf(("%s: Unsupported down interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 31, aIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}













nsresult VFTCALL downCMCreateInstanceByContractID(void *pvThis, const char *aContractID, nsISupports *aDelegate, const nsIID & aIID, void * *result)
{
    DOWN_ENTER_RC(pvThis, nsIComponentManager);
    DPRINTF_CONTRACTID(aContractID);
    DPRINTF_NSID(aIID);
    dprintf(("%s: aDelegate=%p result=%p", pszFunction, aDelegate, result));


    


    const void * pvVFT = downIsSupportedInterface(aIID);
    if (pvVFT)
    {
        




        nsISupports * pupOuter = aDelegate;
        rc = upCreateWrapper((void**)&pupOuter, kSupportsIID, NS_OK);
        if (NS_SUCCEEDED(rc))
        {
            rc = pMozI->CreateInstanceByContractID(aContractID, pupOuter, aIID, result);
            rc = downCreateWrapper(result, pvVFT, rc);
        }
        else
            dprintf(("%s: failed to create up wrapper for kSupportsIID %x !!!", pszFunction, aDelegate));
    }
    else
    {
        dprintf(("%s: Unsupported down interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 10, aIID.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




MAKE_SAFE_VFT(VFTnsIComponentManager, downVFTnsIComponentManager)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downCMGetClassObject,                                   VFTDELTA_VAL()
    downCMGetClassObjectByContractID,                       VFTDELTA_VAL()
    downCMCreateInstance,                                   VFTDELTA_VAL()
    downCMCreateInstanceByContractID,                       VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();













nsresult VFTCALL downEFirst(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIEnumerator);
    rc = pMozI->First();
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




nsresult VFTCALL  downENext(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIEnumerator);
    rc = pMozI->Next();
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL  downECurrentItem(void *pvThis, nsISupports **_retval)
{
    DOWN_ENTER_RC(pvThis, nsIEnumerator);
    rc = pMozI->First();
    rc = downCreateWrapper((void**)_retval, kSupportsIID, rc);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL  downEIsDone(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIEnumerator);
    rc = pMozI->IsDone();
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIEnumerator, downVFTnsIEnumerator)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downEFirst,                                             VFTDELTA_VAL()
    downENext,                                              VFTDELTA_VAL()
    downECurrentItem,                                       VFTDELTA_VAL()
    downEIsDone,                                            VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();



























nsresult VFTCALL downFactory_CreateInstance(void *pvThis, nsISupports *aOuter, const nsIID & iid, void * *result)
{
    DOWN_ENTER_RC(pvThis, nsIFactory);
    dprintf(("%s: aOuter result=%p", pszFunction, aOuter, result));
    DPRINTF_NSID(iid);

    


    const void * pvVFT = downIsSupportedInterface(iid);
    if (pvVFT)
    {
        




        nsISupports * pupOuter = aOuter;
        rc = upCreateWrapper((void**)&pupOuter, kSupportsIID, NS_OK);
        if (rc == NS_OK)
        {
            rc = pMozI->CreateInstance(pupOuter, iid, result);
            rc = downCreateWrapper(result, pvVFT, rc);
        }
        else
            dprintf(("%s: failed to create up wrapper for kSupportsIID %x !!!", pszFunction, aOuter));
    }
    else
    {
        dprintf(("%s: Unsupported down interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        ReleaseInt3(0xbaddbeef, 33, iid.m0);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}













nsresult VFTCALL downFactory_LockFactory(void *pvThis, PRBool lock)
{
    DOWN_ENTER_RC(pvThis, nsIFactory);
    dprintf(("%s: lock=%d\n", pszFunction, lock));
    rc = pMozI->LockFactory(lock);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIFactory, downVFTnsIFactory)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downFactory_CreateInstance,                             VFTDELTA_VAL()
    downFactory_LockFactory,                                VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();









nsresult VFTCALL downInputStream_Close(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIInputStream);
    rc = pMozI->Close();
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}






nsresult VFTCALL downInputStream_Available(void *pvThis, PRUint32 *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIInputStream);
    dprintf(("%s: _retval=%p\n", pszFunction, _retval));
    rc = pMozI->Available(_retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}















nsresult VFTCALL downInputStream_Read(void *pvThis, char * aBuf, PRUint32 aCount, PRUint32 *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIInputStream);
    dprintf(("%s: aBuf=%p aCount=%d _retval=%p\n", pszFunction, aBuf, aCount, _retval));
    rc = pMozI->Read(aBuf, aCount, _retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult __cdecl downOutputStream_nsWriteSegmentWrapper(
    nsresult (VFTCALL pfnOrg)(nsIInputStream *aInStream, void *aClosure, const char *aFromSegment, PRUint32 aToOffset, PRUint32 aCount, PRUint32 *aWriteCount),
    void *pvRet,
    nsIInputStream *aInStream, void *aClosure, const char *aFromSegment,
    PRUint32 aToOffset, PRUint32 aCount, PRUint32 *aWriteCount)
{
    DEBUG_FUNCTIONNAME();
    nsresult rc;
    dprintf(("%s: pfnOrg=%p pvRet=%p aInStream=%p aClosure=%p aFromSegment=%p aToOffset=%d aCount=%d aWriteCount=%p\n",
             pszFunction, pfnOrg, pvRet, aInStream, aClosure, aFromSegment, aToOffset, aCount, aWriteCount));

    nsIInputStream * pDownInStream = aInStream;
    rc = downCreateWrapper((void**)&pDownInStream, kInputStreamIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        rc = pfnOrg(pDownInStream, aClosure, aFromSegment, aToOffset, aCount, aWriteCount);
        if (VALID_PTR(aWriteCount))
            dprintf(("%s: *aWriteCount=%d\n", pszFunction, *aWriteCount));
    }

    dprintf(("%s: rc=%d\n", pszFunction, rc));
    return rc;
}























nsresult VFTCALL downInputStream_ReadSegments(void *pvThis, nsWriteSegmentFun aWriter, void * aClosure, PRUint32 aCount, PRUint32 *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIInputStream);
    dprintf(("%s: aWriter=%p aClosure=%p aCount=%d _retval=%p", pszFunction, aWriter, aClosure, aCount, _retval));


    int i;
    char achWrapper[32];

    
    achWrapper[0] = 0x68;
    *((unsigned*)&achWrapper[1]) = (unsigned)aWriter;
    i = 5;

#ifdef __IBMCPP__
    
    achWrapper[i++] = 0x89;
    achWrapper[i++] = 0x45;
    achWrapper[i++] = 0x08;
    
    achWrapper[i++] = 0x89;
    achWrapper[i++] = 0x55;
    achWrapper[i++] = 0x0c;
    
    achWrapper[i++] = 0x89;
    achWrapper[i++] = 0x4d;
    achWrapper[i++] = 0x10;
#endif
    
    achWrapper[i] = 0xe8;
    *((unsigned*)&achWrapper[i+1]) = (unsigned)downOutputStream_nsWriteSegmentWrapper - (unsigned)&achWrapper[i+5];
    i += 5;

    
    achWrapper[i++] = 0x83;
    achWrapper[i++] = 0xc4;
    achWrapper[i++] = 0x04;

    
    achWrapper[i++] = 0xc3;

    achWrapper[i] = 0xcc;
    achWrapper[i] = 0xcc;

    
    rc = pMozI->ReadSegments((nsWriteSegmentFun)((void*)&achWrapper[0]), aClosure, aCount, _retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}






nsresult VFTCALL downInputStream_IsNonBlocking(void *pvThis, PRBool *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIInputStream);
    dprintf(("%s: _retval=%p\n", pszFunction, _retval));
    rc = pMozI->IsNonBlocking(_retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




MAKE_SAFE_VFT(VFTnsIInputStream, downVFTnsIInputStream)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downInputStream_Close,                                  VFTDELTA_VAL()
    downInputStream_Available,                              VFTDELTA_VAL()
    downInputStream_Read,                                   VFTDELTA_VAL()
    downInputStream_ReadSegments,                           VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();














nsresult VFTCALL downOutputStream_Close(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIOutputStream);
    rc = pMozI->Close();
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downOutputStream_Flush(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIOutputStream);
    rc = pMozI->Flush();
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}














nsresult VFTCALL downOutputStream_Write(void *pvThis, const char *aBuf, PRUint32 aCount, PRUint32 *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIOutputStream);
    dprintf(("%s: aBuf=%p aCount=%d _retval=%p\n", pszFunction, aBuf, aCount, _retval));
    rc = pMozI->Write(aBuf, aCount, _retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




















nsresult VFTCALL downOutputStream_WriteFrom(void *pvThis, nsIInputStream *aFromStream, PRUint32 aCount, PRUint32 *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIOutputStream);
    dprintf(("%s: aFromStream=%p aCount=%d _retval=%p\n", pszFunction, aFromStream, aCount, _retval));

    nsIInputStream *pUpFromStream = aFromStream;
    rc = upCreateWrapper((void**)&pUpFromStream, kInputStreamIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        rc = pMozI->WriteFrom(pUpFromStream, aCount, _retval);
        if (VALID_PTR(_retval))
            dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}







nsresult __cdecl downOutputStream_nsReadSegmentWrapper(nsReadSegmentFun pfnOrg, void *pvRet,
     nsIOutputStream *aOutStream, void *aClosure, char *aToSegment,
     PRUint32 aFromOffset, PRUint32 aCount, PRUint32 *aReadCount)
{
    DEBUG_FUNCTIONNAME();
    nsresult rc;
    dprintf(("%s: pfnOrg=%p pvRet=%p aOutStream=%p aClosure=%p aToSegment=%p aFromOffset=%d aCount=%d aReadCount=%p\n",
             pszFunction, pfnOrg, pvRet, aOutStream, aClosure, aToSegment, aFromOffset, aCount, aReadCount));

    nsIOutputStream * pupOutStream = aOutStream;
    rc = upCreateWrapper((void**)&pupOutStream, kOutputStreamIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        rc = pfnOrg(pupOutStream, aClosure, aToSegment, aFromOffset, aCount, aReadCount);
        if (VALID_PTR(aReadCount))
            dprintf(("%s: *aReadCount=%d\n", pszFunction, *aReadCount));
    }

    dprintf(("%s: rc=%d\n", pszFunction, rc));
    return rc;
}























nsresult VFTCALL downOutputStream_WriteSegments(void *pvThis, nsReadSegmentFun aReader, void * aClosure, PRUint32 aCount, PRUint32 *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIOutputStream);
    dprintf(("%s: aReader=%p aClosure=%p aCount=%d _retval=%p", pszFunction, aReader, aClosure, aCount, _retval));

    int i;
    char achWrapper[32];

    
    achWrapper[0] = 0x68;
    *((unsigned*)&achWrapper[1]) = (unsigned)aReader;
    i = 5;

#ifdef __IBMCPP__

    
    achWrapper[i++] = 0x89;
    achWrapper[i++] = 0x45;
    achWrapper[i++] = 0x08;
    
    achWrapper[i++] = 0x89;
    achWrapper[i++] = 0x55;
    achWrapper[i++] = 0x0c;
    
    achWrapper[i++] = 0x89;
    achWrapper[i++] = 0x4d;
    achWrapper[i++] = 0x10;
#endif
    
    achWrapper[i] = 0xe8;
    *((unsigned*)&achWrapper[i+1]) = (unsigned)downOutputStream_nsReadSegmentWrapper - (unsigned)&achWrapper[i+5];
    i += 5;

    
    achWrapper[i++] = 0x83;
    achWrapper[i++] = 0xc4;
    achWrapper[i++] = 0x04;

    
    achWrapper[i++] = 0xc3;
    achWrapper[i++] = 0xcc;
    achWrapper[i] = 0xcc;

    
    rc = pMozI->WriteSegments((nsReadSegmentFun)((void*)&achWrapper[0]), aClosure, aCount, _retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downOutputStream_IsNonBlocking(void *pvThis, PRBool *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIOutputStream);
    dprintf(("%s: _retval=%p\n", pszFunction, _retval));
    rc = pMozI->IsNonBlocking(_retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIOutputStream, downVFTnsIOutputStream)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downOutputStream_Close,                                 VFTDELTA_VAL()
    downOutputStream_Flush,                                 VFTDELTA_VAL()
    downOutputStream_Write,                                 VFTDELTA_VAL()
    downOutputStream_WriteFrom,                             VFTDELTA_VAL()
    downOutputStream_WriteSegments,                         VFTDELTA_VAL()
    downOutputStream_IsNonBlocking,                         VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();




















nsresult VFTCALL downInterfaceRequestor_GetInterface(void *pvThis, const nsIID & uuid, void * *result)
{
    DOWN_ENTER_RC(pvThis, nsIInterfaceRequestor);
    DPRINTF_NSID(uuid);

    


    const void *pvVFT = downIsSupportedInterface(uuid);
    if (pvVFT)
    {
        rc = pMozI->GetInterface(uuid, result);
        rc = downCreateWrapper(result, pvVFT, rc);
    }
    else
    {
        dprintf(("%s: Unsupported interface!!!", pszFunction));
        rc = NS_NOINTERFACE;
        if (result)
            *result = nsnull;
        ReleaseInt3(0xbaddbeef, 34, uuid.m0);
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}



MAKE_SAFE_VFT(VFTnsIInterfaceRequestor, downVFTnsIInterfaceRequestor)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downInterfaceRequestor_GetInterface,                    VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();












nsresult VFTCALL downRequest_GetName(void *pvThis, nsACString & aName)
{
    DOWN_ENTER_RC(pvThis, nsIRequest);
    dprintf(("%s: &aName=%p", pszFunction, &aName));
    
    dprintf(("%s: nsACString wrapping isn't supported.", pszFunction));
    ReleaseInt3(0xbaddbeef, 35, 0);

    rc = pMozI->GetName(aName);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downRequest_IsPending(void *pvThis, PRBool *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIRequest);
    dprintf(("%s: _retval=%p\n", pszFunction, _retval));
    rc = pMozI->IsPending(_retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downRequest_GetStatus(void *pvThis, nsresult *aStatus)
{
    DOWN_ENTER_RC(pvThis, nsIRequest);
    dprintf(("%s: aStatus=%p\n", pszFunction, aStatus));
    rc = pMozI->GetStatus(aStatus);
    if (VALID_PTR(aStatus))
        dprintf(("%s: *aStatus=%#x\n", pszFunction, *aStatus));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}
















nsresult VFTCALL downRequest_Cancel(void *pvThis, nsresult aStatus)
{
    DOWN_ENTER_RC(pvThis, nsIRequest);
    dprintf(("%s: aStatus=%#x\n", pszFunction, aStatus));
    rc = pMozI->Cancel(aStatus);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}













nsresult VFTCALL downRequest_Suspend(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIRequest);
    rc = pMozI->Suspend();
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}







nsresult VFTCALL downRequest_Resume(void *pvThis)
{
    DOWN_ENTER_RC(pvThis, nsIRequest);
    rc = pMozI->Resume();
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}







nsresult VFTCALL downRequest_GetLoadGroup(void *pvThis, nsILoadGroup * *aLoadGroup)
{
    DOWN_ENTER_RC(pvThis, nsIRequest);
    dprintf(("%s: aLoadGroup=%p\n", pszFunction, aLoadGroup));

    rc = pMozI->GetLoadGroup(aLoadGroup);
    rc = downCreateWrapper((void**)aLoadGroup, kLoadGroupIID, rc);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}

nsresult VFTCALL downRequest_SetLoadGroup(void *pvThis, nsILoadGroup * aLoadGroup)
{
    DOWN_ENTER_RC(pvThis, nsIRequest);
    dprintf(("%s: aLoadGroup=%p\n", pszFunction, aLoadGroup));

    nsILoadGroup * pUpLoadGroup = aLoadGroup;
    rc = upCreateWrapper((void**)&pUpLoadGroup, kLoadGroupIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->SetLoadGroup(pUpLoadGroup);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}








nsresult VFTCALL downRequest_GetLoadFlags(void *pvThis, nsLoadFlags *aLoadFlags)
{
    DOWN_ENTER_RC(pvThis, nsIRequest);
    dprintf(("%s: aLoadFlags=%p\n", pszFunction, aLoadFlags));
    rc = pMozI->GetLoadFlags(aLoadFlags);
    if (VALID_PTR(aLoadFlags))
        dprintf(("%s: *aLoadFlags=%#x\n", pszFunction, *aLoadFlags));
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}

nsresult VFTCALL downRequest_SetLoadFlags(void *pvThis, nsLoadFlags aLoadFlags)
{
    DOWN_ENTER_RC(pvThis, nsIRequest);
    dprintf(("%s: aLoadFlags=%p\n", pszFunction, aLoadFlags));
    rc = pMozI->SetLoadFlags(aLoadFlags);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIRequest, downVFTnsIRequest)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downRequest_GetName,                                    VFTDELTA_VAL()
    downRequest_IsPending,                                  VFTDELTA_VAL()
    downRequest_GetStatus,                                  VFTDELTA_VAL()
    downRequest_Cancel,                                     VFTDELTA_VAL()
    downRequest_Suspend,                                    VFTDELTA_VAL()
    downRequest_Resume,                                     VFTDELTA_VAL()
    downRequest_GetLoadGroup,                               VFTDELTA_VAL()
    downRequest_SetLoadGroup,                               VFTDELTA_VAL()
    downRequest_GetLoadFlags,                               VFTDELTA_VAL()
    downRequest_SetLoadFlags,                               VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();











nsresult VFTCALL downLoadGroup_GetGroupObserver(void *pvThis, nsIRequestObserver * *aGroupObserver)
{
    DOWN_ENTER_RC(pvThis, nsILoadGroup);
    dprintf(("%s: aGroupObserver=%p\n", pszFunction, aGroupObserver));

    rc = pMozI->GetGroupObserver(aGroupObserver);
    rc = downCreateWrapper((void**)aGroupObserver, kRequestObserverIID, rc);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}

nsresult VFTCALL downLoadGroup_SetGroupObserver(void *pvThis, nsIRequestObserver * aGroupObserver)
{
    DOWN_ENTER_RC(pvThis, nsILoadGroup);
    dprintf(("%s: aGroupObserver=%p\n", pszFunction, aGroupObserver));

    nsIRequestObserver * pUpGroupObserver = aGroupObserver;
    rc = upCreateWrapper((void**)&pUpGroupObserver, kRequestObserverIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->SetGroupObserver(pUpGroupObserver);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downLoadGroup_GetDefaultLoadRequest(void *pvThis, nsIRequest * *aDefaultLoadRequest)
{
    DOWN_ENTER_RC(pvThis, nsILoadGroup);
    dprintf(("%s: aDefaultLoadRequest=%p\n", pszFunction, aDefaultLoadRequest));

    rc = pMozI->GetDefaultLoadRequest(aDefaultLoadRequest);
    rc = downCreateWrapper((void**)aDefaultLoadRequest, kRequestIID, rc);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;

}

nsresult VFTCALL downLoadGroup_SetDefaultLoadRequest(void *pvThis, nsIRequest * aDefaultLoadRequest)
{
    DOWN_ENTER_RC(pvThis, nsILoadGroup);
    dprintf(("%s: aDefaultLoadRequest=%p\n", pszFunction, aDefaultLoadRequest));

    nsIRequest * pUpDefaultLoadRequest = aDefaultLoadRequest;
    rc = upCreateWrapper((void**)&pUpDefaultLoadRequest, kRequestIID, NS_OK);
    if (NS_SUCCEEDED(rc))
        rc = pMozI->SetDefaultLoadRequest(pUpDefaultLoadRequest);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}











nsresult VFTCALL downLoadGroup_AddRequest(void *pvThis, nsIRequest *aRequest, nsISupports *aContext)
{
    DOWN_ENTER_RC(pvThis, nsILoadGroup);
    dprintf(("%s: aRequest=%p aContext=%p\n", pszFunction, aRequest, aContext));

    nsIRequest * pUpRequest = aRequest;
    rc = upCreateWrapper((void**)&pUpRequest, kRequestIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports * pUpContext = aContext;
        rc = upCreateWrapper((void**)&pUpContext, kSupportsIID, rc);
        if (NS_SUCCEEDED(rc))
            rc = pMozI->AddRequest(pUpRequest, pUpContext);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}






nsresult VFTCALL downLoadGroup_RemoveRequest(void *pvThis, nsIRequest *aRequest, nsISupports *aContext, nsresult aStatus)
{
    DOWN_ENTER_RC(pvThis, nsILoadGroup);
    dprintf(("%s: aRequest=%p aContext=%p aStatus=%#x\n", pszFunction, aRequest, aContext, aStatus));

    nsIRequest * pUpRequest = aRequest;
    rc = upCreateWrapper((void**)&pUpRequest, kRequestIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports * pUpContext = aContext;
        rc = upCreateWrapper((void**)&pUpContext, kSupportsIID, rc);
        if (NS_SUCCEEDED(rc))
            rc = pMozI->RemoveRequest(pUpRequest, pUpContext, aStatus);
    }

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}






nsresult VFTCALL downLoadGroup_GetRequests(void *pvThis, nsISimpleEnumerator * *aRequests)
{
    DOWN_ENTER_RC(pvThis, nsILoadGroup);
    dprintf(("%s: aRequests=%p\n", pszFunction, aRequests));

    rc = pMozI->GetRequests(aRequests);
    rc = downCreateWrapper((void**)aRequests, kSimpleEnumeratorIID, rc);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}






nsresult VFTCALL downLoadGroup_GetActiveCount(void *pvThis, PRUint32 *aActiveCount)
{
    DOWN_ENTER_RC(pvThis, nsILoadGroup);
    dprintf(("%s: aActiveCount=%p\n", pszFunction, aActiveCount));

    rc = pMozI->GetActiveCount(aActiveCount);
    if (VALID_PTR(aActiveCount))
        dprintf(("%s: *aActiveCount=%d\n", pszFunction, aActiveCount));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}





nsresult VFTCALL downLoadGroup_GetNotificationCallbacks(void *pvThis, nsIInterfaceRequestor * *aNotificationCallbacks)
{
    DOWN_ENTER_RC(pvThis, nsILoadGroup);
    dprintf(("%s: aNotificationCallbacks=%p\n", pszFunction, aNotificationCallbacks));

    rc = pMozI->GetNotificationCallbacks(aNotificationCallbacks);
    rc = downCreateWrapper((void**)aNotificationCallbacks, kInterfaceRequestorIID, rc);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}

nsresult VFTCALL downLoadGroup_SetNotificationCallbacks(void *pvThis, nsIInterfaceRequestor * aNotificationCallbacks)
{
    DOWN_ENTER_RC(pvThis, nsILoadGroup);
    dprintf(("%s: aNotificationCallbacks=%p\n", pszFunction, aNotificationCallbacks));

    nsIInterfaceRequestor * pUpNotificationCallbacks = aNotificationCallbacks;
    rc = upCreateWrapper((void**)&pUpNotificationCallbacks, kInterfaceRequestorIID, rc);
    rc = pMozI->SetNotificationCallbacks(pUpNotificationCallbacks);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsILoadGroup, downVFTnsILoadGroup)
{
 {
   {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
   },
    
    downRequest_GetName,                                    VFTDELTA_VAL()
    downRequest_IsPending,                                  VFTDELTA_VAL()
    downRequest_GetStatus,                                  VFTDELTA_VAL()
    downRequest_Cancel,                                     VFTDELTA_VAL()
    downRequest_Suspend,                                    VFTDELTA_VAL()
    downRequest_Resume,                                     VFTDELTA_VAL()
    downRequest_GetLoadGroup,                               VFTDELTA_VAL()
    downRequest_SetLoadGroup,                               VFTDELTA_VAL()
    downRequest_GetLoadFlags,                               VFTDELTA_VAL()
    downRequest_SetLoadFlags,                               VFTDELTA_VAL()
 },
    downLoadGroup_GetGroupObserver,                         VFTDELTA_VAL()
    downLoadGroup_SetGroupObserver,                         VFTDELTA_VAL()
    downLoadGroup_GetDefaultLoadRequest,                    VFTDELTA_VAL()
    downLoadGroup_SetDefaultLoadRequest,                    VFTDELTA_VAL()
    downLoadGroup_AddRequest,                               VFTDELTA_VAL()
    downLoadGroup_RemoveRequest,                            VFTDELTA_VAL()
    downLoadGroup_GetRequests,                              VFTDELTA_VAL()
    downLoadGroup_GetActiveCount,                           VFTDELTA_VAL()
    downLoadGroup_GetNotificationCallbacks,                 VFTDELTA_VAL()
    downLoadGroup_SetNotificationCallbacks,                 VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();









nsresult VFTCALL downPluginStreamInfo_GetContentType(void *pvThis, nsMIMEType *aContentType)
{
    DOWN_ENTER_RC(pvThis, nsIPluginStreamInfo);
    dprintf(("%s: aContentType=%p\n", pszFunction, aContentType));

    rc = pMozI->GetContentType(aContentType);
    if (VALID_PTR(aContentType))
        DPRINTF_STR(*aContentType);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


nsresult VFTCALL downPluginStreamInfo_IsSeekable(void *pvThis, PRBool *aSeekable)
{
    DOWN_ENTER_RC(pvThis, nsIPluginStreamInfo);
    dprintf(("%s: aSeekable=%p\n", pszFunction, aSeekable));

    rc = pMozI->IsSeekable(aSeekable);
    if (VALID_PTR(aSeekable))
        dprintf(("%s: *aSeekable=%d\n", pszFunction, *aSeekable));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


nsresult VFTCALL downPluginStreamInfo_GetLength(void *pvThis, PRUint32 *aLength)
{
    DOWN_ENTER_RC(pvThis, nsIPluginStreamInfo);
    dprintf(("%s: aLength=%p\n", pszFunction, aLength));

    rc = pMozI->GetLength(aLength);
    if (VALID_PTR(aLength))
        dprintf(("%s: *aLength=%d\n", pszFunction, *aLength));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


nsresult VFTCALL downPluginStreamInfo_GetLastModified(void *pvThis, PRUint32 *aLastModified)
{
    DOWN_ENTER_RC(pvThis, nsIPluginStreamInfo);
    dprintf(("%s: aLastModified=%p\n", pszFunction, aLastModified));

    rc = pMozI->GetLastModified(aLastModified);
    if (VALID_PTR(aLastModified))
        dprintf(("%s: *aLastModified=%d\n", pszFunction, *aLastModified));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


nsresult VFTCALL downPluginStreamInfo_GetURL(void *pvThis, const char * *aURL)
{
    DOWN_ENTER_RC(pvThis, nsIPluginStreamInfo);
    dprintf(("%s: aURL=%p\n", pszFunction, aURL));

    rc = pMozI->GetURL(aURL);
    if (VALID_PTR(aURL))
        DPRINTF_STR(*aURL);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


nsresult VFTCALL downPluginStreamInfo_RequestRead(void *pvThis, nsByteRange * aRangeList)
{
    DOWN_ENTER_RC(pvThis, nsIPluginStreamInfo);
    dprintf(("%s: aRangeList=%p\n", pszFunction, aRangeList));
    rc = pMozI->RequestRead(aRangeList);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


nsresult VFTCALL downPluginStreamInfo_GetStreamOffset(void *pvThis, PRInt32 *aStreamOffset)
{
    DOWN_ENTER_RC(pvThis, nsIPluginStreamInfo);
    dprintf(("%s: aStreamOffset=%p\n", pszFunction, aStreamOffset));

    rc = pMozI->GetStreamOffset(aStreamOffset);
    if (VALID_PTR(aStreamOffset))
        dprintf(("%s: *aStreamOffset=%p\n", pszFunction, *aStreamOffset));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}

nsresult VFTCALL downPluginStreamInfo_SetStreamOffset(void *pvThis, PRInt32 aStreamOffset)
{
    DOWN_ENTER_RC(pvThis, nsIPluginStreamInfo);
    dprintf(("%s: aStreamOffset=%d\n", pszFunction, aStreamOffset));
    rc = pMozI->SetStreamOffset(aStreamOffset);
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}



MAKE_SAFE_VFT(VFTnsIPluginStreamInfo, downVFTnsIPluginStreamInfo)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downPluginStreamInfo_GetContentType,                    VFTDELTA_VAL()
    downPluginStreamInfo_IsSeekable,                        VFTDELTA_VAL()
    downPluginStreamInfo_GetLength,                         VFTDELTA_VAL()
    downPluginStreamInfo_GetLastModified,                   VFTDELTA_VAL()
    downPluginStreamInfo_GetURL,                            VFTDELTA_VAL()
    downPluginStreamInfo_RequestRead,                       VFTDELTA_VAL()
    downPluginStreamInfo_GetStreamOffset,                   VFTDELTA_VAL()
    downPluginStreamInfo_SetStreamOffset,                   VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();



















nsresult VFTCALL downRequestObserver_OnStartRequest(void *pvThis, nsIRequest *aRequest, nsISupports *aContext)
{
    DOWN_ENTER_RC(pvThis, nsIRequestObserver);
    dprintf(("%s: aRequest=%p aContext=%p\n", pszFunction, aRequest, aContext));

    nsIRequest *pUpRequest = aRequest;
    rc = upCreateWrapper((void**)&pUpRequest, kRequestIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports *pUpContext = aContext;
        rc = upCreateWrapper((void**)&pUpContext, kSupportsIID, NS_OK);
        if (NS_SUCCEEDED(rc))
            rc = pMozI->OnStartRequest(pUpRequest, pUpContext);
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}












nsresult VFTCALL downRequestObserver_OnStopRequest(void *pvThis, nsIRequest *aRequest, nsISupports *aContext, nsresult aStatusCode)
{
    DOWN_ENTER_RC(pvThis, nsIRequestObserver);
    dprintf(("%s: aRequest=%p aContext=%p aStatusCode=%d\n", pszFunction, aRequest, aContext, aStatusCode));

    nsIRequest *pUpRequest = aRequest;
    rc = upCreateWrapper((void**)&pUpRequest, kRequestIID, NS_OK);
    if (NS_SUCCEEDED(rc))
    {
        nsISupports *pUpContext = aContext;
        rc = upCreateWrapper((void**)&pUpContext, kSupportsIID, NS_OK);
        if (NS_SUCCEEDED(rc))
            rc = pMozI->OnStopRequest(pUpRequest, pUpContext, aStatusCode);
    }
    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIRequestObserver, downVFTnsIRequestObserver)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downRequestObserver_OnStartRequest,                     VFTDELTA_VAL()
    downRequestObserver_OnStopRequest,                      VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();




















nsresult VFTCALL downSimpleEnumerator_HasMoreElements(void *pvThis, PRBool *_retval)
{
    DOWN_ENTER_RC(pvThis, nsISimpleEnumerator);
    dprintf(("%s: _retval=%p\n", pszFunction, _retval));

    rc = pMozI->HasMoreElements(_retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d\n", pszFunction, *_retval));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}
















nsresult VFTCALL downSimpleEnumerator_GetNext(void *pvThis, nsISupports **_retval)
{
    DOWN_ENTER_RC(pvThis, nsISimpleEnumerator);
    dprintf(("%s: _retval=%p\n", pszFunction, _retval));

    rc = pMozI->GetNext(_retval);
    rc = downCreateWrapper((void**)_retval, kSupportsIID, rc);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}



MAKE_SAFE_VFT(VFTnsISimpleEnumerator, downVFTnsISimpleEnumerator)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downSimpleEnumerator_HasMoreElements,                   VFTDELTA_VAL()
    downSimpleEnumerator_GetNext,                           VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();



















nsresult VFTCALL downFlashIObject7_Evaluate(void *pvThis, const char *aString, FlashIObject7 **aFlashObject)
{
    DOWN_ENTER_RC(pvThis, FlashIObject7);
    dprintf(("%s: aString=%p aFlashObject=%p", pszFunction, aString, aFlashObject));
    DPRINTF_STR(aString);

    rc = pMozI->Evaluate(aString, aFlashObject);
    rc = downCreateWrapper((void**)aFlashObject, kFlashIObject7IID, NS_OK);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}




MAKE_SAFE_VFT(VFTFlashIObject7, downVFTFlashIObject7)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downFlashIObject7_Evaluate,                             VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();














nsresult VFTCALL downHTTPHeaderListener_NewResponseHeader(void *pvThis, const char *headerName, const char *headerValue)
{
    DOWN_ENTER_RC(pvThis, nsIHTTPHeaderListener);
    DPRINTF_STR(headerName);
    DPRINTF_STR(headerValue);

    rc = pMozI->NewResponseHeader(headerName, headerValue);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}


MAKE_SAFE_VFT(VFTnsIHTTPHeaderListener, downVFTnsIHTTPHeaderListener)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downHTTPHeaderListener_NewResponseHeader,               VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();

















void * VFTCALL downMemory_Alloc(void *pvThis, size_t size)
{
    DOWN_ENTER_NORET(pvThis, nsIMemory);
    dprintf(("%s: size=%d\n", pszFunction, size));

    void *pv = pMozI->Alloc(size);

    DOWN_LEAVE_INT(pvThis, (unsigned)pv);
    return pv;
}
















void * VFTCALL downMemory_Realloc(void *pvThis, void * ptr, size_t newSize)
{
    DOWN_ENTER_NORET(pvThis, nsIMemory);
    dprintf(("%s: ptr=%p newSize=%d\n", pszFunction, ptr, newSize));

    void *pv = pMozI->Realloc(ptr, newSize);

    DOWN_LEAVE_INT(pvThis, (unsigned)pv);
    return pv;
}








void VFTCALL downMemory_Free(void *pvThis, void * ptr)
{
    DOWN_ENTER_NORET(pvThis, nsIMemory);
    dprintf(("%s: ptr=%p\n", pszFunction, ptr));

    pMozI->Free(ptr);

    DOWN_LEAVE(pvThis);
    return;
}











nsresult VFTCALL  downMemory_HeapMinimize(void *pvThis, PRBool immediate)
{
    DOWN_ENTER_RC(pvThis, nsIMemory);
    dprintf(("%s: immediate=%d\n", pszFunction, immediate));

    rc = pMozI->HeapMinimize(immediate);

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}







nsresult VFTCALL  downMemory_IsLowMemory(void *pvThis, PRBool *_retval)
{
    DOWN_ENTER_RC(pvThis, nsIMemory);
    dprintf(("%s: _retval=%p\n", pszFunction, _retval));

    rc = pMozI->IsLowMemory(_retval);
    if (VALID_PTR(_retval))
        dprintf(("%s: *_retval=%d", pszFunction, *_retval));

    DOWN_LEAVE_INT(pvThis, rc);
    return rc;
}

MAKE_SAFE_VFT(VFTnsIMemory, downVFTnsIMemory)
{
 {
    VFTFIRST_VAL()
    downQueryInterface,                                     VFTDELTA_VAL()
    downAddRef,                                             VFTDELTA_VAL()
    downRelease,                                            VFTDELTA_VAL()
 },
    downMemory_Alloc,                                       VFTDELTA_VAL()
    downMemory_Realloc,                                     VFTDELTA_VAL()
    downMemory_Free,                                        VFTDELTA_VAL()
    downMemory_HeapMinimize,                                VFTDELTA_VAL()
    downMemory_IsLowMemory,                                 VFTDELTA_VAL()
}
SAFE_VFT_ZEROS();








static struct SupportedInterface_Down
{
    
    const nsIID *   pIID;
    
    const void *    pvVFT;
}   aDownInterfaces[] =
{
    { &kServiceManagerIID,          &downVFTnsIServiceManager },
    { &kServiceManagerObsoleteIID,  &downVFTnsIServiceManagerObsolete },
    { &kSupportsIID,                &downVFTnsISupports },
    { &kPluginManagerIID,           &downVFTnsIPluginManager },
    { &kPluginManager2IID,          &downVFTnsIPluginManager2 },
    { &kPluginInstancePeerIID,      &downVFTnsIPluginInstancePeer },
    { &kPluginInstancePeer2IID,     &downVFTnsIPluginInstancePeer2 },
    { &kPluginTagInfoIID,           &downVFTnsIPluginTagInfo },
    { &kPluginTagInfo2IID,          &downVFTnsIPluginTagInfo2 },
    { &kJVMThreadManagerIID,        &downVFTnsIJVMThreadManager },
    { &kJVMManagerIID,              &downVFTnsIJVMManager },
    { &kLiveconnectIID,             &downVFTnsILiveconnect },
    { &kSecureLiveconnectIID,       &downVFTnsISecureLiveconnect },
    { &kComponentManagerIID,        &downVFTnsIComponentManager },
    { &kComponentManagerObsoleteIID,&downVFTnsIComponentManagerObsolete },
    { &kSecurityContextIID,         &downVFTnsISecurityContext },
    { &kEnumeratorIID,              &downVFTnsIEnumerator },
    { &kFactoryIID,                 &downVFTnsIFactory },
    { &kInputStreamIID,             &downVFTnsIInputStream },
    { &kOutputStreamIID,            &downVFTnsIOutputStream },
    { &kInterfaceRequestorIID,      &downVFTnsIInterfaceRequestor },
    { &kRequestIID,                 &downVFTnsIRequest },
    { &kLoadGroupIID,               &downVFTnsILoadGroup },
    { &kPluginStreamInfoIID,        &downVFTnsIPluginStreamInfo },
    { &kRequestObserverIID,         &downVFTnsIRequestObserver },
    { &kSimpleEnumeratorIID,        &downVFTnsISimpleEnumerator },
    { &kFlashIObject7IID,           &downVFTFlashIObject7 },
    { &kHTTPHeaderListenerIID,      &downVFTnsIHTTPHeaderListener },
    { &kMemoryIID,                  &downVFTnsIMemory },
};










const void * downIsSupportedInterface(REFNSIID aIID)
{
    for (unsigned iInterface = 0; iInterface < sizeof(aDownInterfaces) / sizeof(aDownInterfaces[0]); iInterface++)
        if (aDownInterfaces[iInterface].pIID->Equals(aIID))
            return aDownInterfaces[iInterface].pvVFT;
    return NULL;
}
















nsresult downCreateWrapper(void **ppvResult, const void *pvVFT, nsresult rc)
{
    DEBUG_FUNCTIONNAME();
    dprintf(("%s: pvvResult=%x, pvVFT=%x, rc=%x", pszFunction, pvVFT, ppvResult, rc));
    #ifdef DEBUG
    for (unsigned iInterface = 0; iInterface < sizeof(aDownInterfaces) / sizeof(aDownInterfaces[0]); iInterface++)
        if (aDownInterfaces[iInterface].pvVFT == pvVFT)
        {
            DPRINTF_NSID(*aDownInterfaces[iInterface].pIID);
            break;
        }
    #endif

    if (VALID_PTR(ppvResult))
    {
        if (VALID_PTR(*ppvResult))
        {
            if (pvVFT)
            {
                if (NS_SUCCEEDED(rc))
                {
                    void *pvThis = *ppvResult;

                    


                    void *pvNoWrapper = UpWrapperBase::findUpWrapper(pvThis);
                    if (pvNoWrapper)
                    {
                        dprintf(("%s: COOL! pvThis(%x) is an up wrapper, no wrapping needed. returns real obj=%x",
                                 pszFunction,  pvThis, pvNoWrapper));
                        *ppvResult = pvNoWrapper;
                        return rc;
                    }

                    #if 1 
                    


                    DOWN_LOCK();
                    for (PDOWNTHIS pDown = (PDOWNTHIS)gpDownHead; pDown; pDown = (PDOWNTHIS)pDown->pNext)
                        if (pDown->pvThis == pvThis)
                        {
                            if (pDown->pvVFT == pvVFT)
                            {
                                DOWN_UNLOCK();
                                dprintf(("%s: Found existing wrapper %x for %x.", pszFunction, pDown, pvThis));
                                *ppvResult = pDown;
                                return rc;
                            }
                        }
                    DOWN_UNLOCK();
                    #endif

                    



                    PDOWNTHIS pWrapper = new DOWNTHIS;
                    if (pWrapper)
                    {
                        pWrapper->initialize(pvThis, pvVFT);

                        


                        DOWN_LOCK();
                        for (PDOWNTHIS pDown = (PDOWNTHIS)gpDownHead; pDown; pDown = (PDOWNTHIS)pDown->pNext)
                            if (pDown->pvThis == pvThis)
                            {
                                if (pDown->pvVFT == pvVFT)
                                {
                                    DOWN_UNLOCK();
                                    delete pWrapper;
                                    dprintf(("%s: Found existing wrapper %x for %x. (2)", pszFunction, pDown, pvThis));
                                    *ppvResult = pDown;
                                    return rc;
                                }
                            }
                        


                        pWrapper->pNext = gpDownHead;
                        gpDownHead = pWrapper;
                        DOWN_UNLOCK();

                        dprintf(("%s: Created wrapper %x for %x.", pszFunction, pWrapper, pvThis));
                        *ppvResult = pWrapper;
                        return rc;
                    }
                    dprintf(("new failed!!"));
                    rc = NS_ERROR_OUT_OF_MEMORY;
                }
                else
                    dprintf(("%s: Passed in rc means failure, (rc=%x)", pszFunction, rc));
            }
            else
            {
                dprintf(("%s: No VFT, that can only mean that it's an Unsupported interface!!!", pszFunction));
                rc = NS_ERROR_NOT_IMPLEMENTED;
                ReleaseInt3(0xbaddbeef, 6, 0);
            }
            *ppvResult = nsnull;
        }
        else if (*ppvResult || rc != NS_OK) 
            dprintf(("%s: *ppvResult (=%p) is invalid (rc=%x)", pszFunction, *ppvResult, rc));
    }
    else
        dprintf(("%s: ppvResult (=%p) is invalid (rc=%x)", pszFunction, ppvResult, rc));

    return rc;
}




nsresult downCreateWrapper(void **ppvResult, REFNSIID aIID, nsresult rc)
{
    return downCreateWrapper(ppvResult, downIsSupportedInterface(aIID), rc);
}









int downCreateJNIEnvWrapper(JNIEnv **ppJNIEnv, int rc)
{
    DEBUG_FUNCTIONNAME();
    dprintf(("%s: ppJNIEnv=%x, rc=%x", pszFunction, ppJNIEnv, rc));

    if (VALID_PTR(ppJNIEnv))
    {
        if (VALID_PTR(*ppJNIEnv))
        {
            if (NS_SUCCEEDED(rc))
            {
                    


                    return rc;
            }
            else
                dprintf(("%s: The query method failed with rc=%x", pszFunction, rc));
            *ppJNIEnv = nsnull;
        }
        else if (*ppJNIEnv || rc != NS_OK) 
            dprintf(("%s: *ppJNIEnv (=%p) is invalid (rc=%x)", pszFunction, *ppJNIEnv, rc));
    }
    else
        dprintf(("%s: ppJNIEnv (=%p) is invalid (rc=%x)", pszFunction, ppJNIEnv, rc));

    return rc;
}






void            downLock(void)
{
    DEBUG_FUNCTIONNAME();

    if (!ghmtxDown)
    {
        int rc = DosCreateMutexSem(NULL, &ghmtxDown, 0, TRUE);
        if (rc)
        {
            dprintf(("%s: DosCreateMutexSem failed with rc=%d.", pszFunction, rc));
            ReleaseInt3(0xdeadbee1, 0xd000, rc);
        }
    }
    else
    {
        int rc = DosRequestMutexSem(ghmtxDown, SEM_INDEFINITE_WAIT);
        if (rc)
        {
            dprintf(("%s: DosRequestMutexSem failed with rc=%d.", pszFunction, rc));
            ReleaseInt3(0xdeadbee1, 0xd001, rc);
        }
        
    }
}




void            downUnLock(void)
{
    DEBUG_FUNCTIONNAME();

    int rc = DosReleaseMutexSem(ghmtxDown);
    if (rc)
    {
        dprintf(("%s: DosRequestMutexSem failed with rc=%d.", pszFunction, rc));
        ReleaseInt3(0xdeadbee1, 0xd002, rc);
    }
    
}














const char *    getIIDCIDName(REFNSIID aIIDorCID)
{
    for (unsigned i = 0; i < sizeof(aIDNameLookup) / sizeof(aIDNameLookup[0]); i++)
        if (aIDNameLookup[i].pID->Equals(aIIDorCID))
            return aIDNameLookup[i].pszName;

    return "<unknown>";
}









const nsID *    getIIDCIDFromName(const char *pszStrID)
{
    for (unsigned i = 0; i < sizeof(aIDStrIDLookup) / sizeof(aIDStrIDLookup[0]); i++)
        if (!stricmp(aIDStrIDLookup[i].pszStrID, pszStrID))
            return aIDStrIDLookup[i].pID;

    return NULL;
}












void    verifyAndFixUTF8String(const char *pszString, const char *pszFunction)
{
    






    unsigned char *pszString2 = *(unsigned char**)((void*)&pszString);
    for (unsigned char *pch = pszString2; *pch; pch++)
    {
        unsigned ch = *pch;
        if (ch > 0x7f)
        {
            







            unsigned val;
            unsigned min;
            unsigned c;
            unsigned i;
            if ((ch & 0xe0) == 0xc0)
            {
                c = 1;
                min = 0x80;
                val = ch & ~0xc0;
            }
            else if ((ch & 0xf0) == 0xe0)
            {
                c = 2;
                min = 0x800;
                val = ch & ~0xe0;
            }
            else if ((ch & 0xf8) == 0xf0)
            {
                c = 3;
                min = 0x1000;
                val = ch & ~0xf0;
            }
            else if ((ch & 0xfc) == 0xf8)
            {
                c = 4;
                min = 0x20000;
                val = ch & ~0xf8;
            }
            else if ((ch & 0xfe) == 0xfc)
            {
                c = 5;
                min = 0x400000;
                val = ch & ~0xfc;
            }
            else
                goto invalid;

            for (i = 1; i <= c; i++)
            {
                unsigned chPart = pch[i];
                if ((chPart & 0xc0) != 0x80)
                    goto invalid;
                val <<= 6;
                val |= chPart & 0x3f;
            }
            if (val < min)
                goto invalid;
            pch += c;
            continue;

        invalid:
            *pch = ' ';
        }
    }
}







nsresult npXPCOMGenericGetFactory(nsIServiceManagerObsolete *aServMgr,
                                  REFNSIID aClass,
                                  const char *aClassName,
                                  const char *aContractID,
                                  PRLibrary * aLibrary,
                                  nsIPlugin **aResult)
{
    DEBUG_FUNCTIONNAME();
    typedef nsresult (* _Optlink nsLegacyFactorProc)(PDOWNTHIS aServMgr, const nsCID &aClass, const char *aClassName, const char *aContractID, nsIPlugin **aResult);
    nsresult            rc;
    nsLegacyFactorProc  pfnGetFactory;

    dprintf(("%s: enter", pszFunction));
    DPRINTF_STR(aClassName);
    DPRINTF_NSID(aClass);
    DPRINTF_CONTRACTID(aContractID);

    


    pfnGetFactory = (nsLegacyFactorProc)PR_FindSymbol(aLibrary, "NSGetFactory");
    if (!pfnGetFactory)
    {
        dprintf(("%s: Could not find NSGetFactory.", pszFunction));
        return NS_ERROR_FAILURE;
    }


    


    PDOWNTHIS   pDownServMgr = (PDOWNTHIS)aServMgr;
    rc = downCreateWrapper((void**)&pDownServMgr, downIsSupportedInterface(kSupportsIID), NS_OK);
    if (NS_FAILED(rc))
    {
        dprintf(("%s: NSGetFactory failed.", pszFunction));
        *aResult = nsnull;
        return rc;
    }


    


    rc = pfnGetFactory(pDownServMgr, aClass, aClassName, aContractID, aResult);
    if (NS_FAILED(rc))
    {
        dprintf(("%s: NSGetFactory failed.", pszFunction));
        *aResult = nsnull;
        return rc;
    }


    


    dprintf(("%s: pfnNSGetFactory succeeded!", pszFunction));
    rc = upCreateWrapper((void**)aResult, kPluginIID, rc);

    return rc;
}

















nsresult npXPCOMGenericMaybeWrap(REFNSIID aIID, nsISupports *aIn, nsISupports **aOut)
{
    DEBUG_FUNCTIONNAME();

    dprintf(("%s: Enter. aIn=%p aOut=%p", pszFunction, aIn, aOut));
    DPRINTF_NSID(aIID);

    












    if (!VALID_PTR(aIn))
    {
        dprintf(("%s: Invalid aIn pointer %p!!!", pszFunction, aIn));
        return NS_ERROR_FAILURE;
    }
    VFTnsISupports * pVFT = (VFTnsISupports *)(*(void**)aIn);
    if (    VALID_PTR(pVFT->QueryInterface)
        &&  !VALID_PTR(pVFT->uDeltaQueryInterface)
        &&  VALID_PTR(pVFT->AddRef)
        &&  !VALID_PTR(pVFT->uDeltaAddRef)
        &&  VALID_PTR(pVFT->Release)
        &&  !VALID_PTR(pVFT->uDeltaRelease)
        )
    {
        dprintf(("%s: Detected VAC VFT.", pszFunction));

        





        if (1) 
        {
            *aOut = aIn;
            nsresult rc = upCreateWrapper((void**)aOut,  kSupportsIID, NS_OK); 
            if (NS_SUCCEEDED(rc))
            {
                dprintf(("%s: Successfully created wrapper.", pszFunction));
            }
            else
            {
                dprintf(("%s: failed to create wrapper for known interface!!!", pszFunction));
                ReleaseInt3(0xdeadbee2, 0xdead0001, rc);
                return rc;
            }
        }
        else
        {
            dprintf(("%s: Unsupported Interface !!!", pszFunction));
            *aOut = nsnull;
            ReleaseInt3(0xdeadbee2, aIID.m0, aIID.m1);
            return NS_ERROR_NO_INTERFACE;
        }
    }
    else
    {
        dprintf(("%s: Not a VAC VFT assuming native VFT which doesn't need wrapping!", pszFunction));
        *aOut = aIn;
    }

    return NS_OK;
}







void    npXPCOMInitSems(void)
{
    DOWN_LOCK();
    DOWN_UNLOCK();
    UpWrapperBase::upLock();
    UpWrapperBase::upUnLock();
}
