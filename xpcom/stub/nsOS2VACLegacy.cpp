













































 






#define VFT_VAC365          1

#define VFTCALL             _Optlink

#define VFTFIRST_DECL       unsigned    uFirst[2]
#define VFTFIRST_VAL()      {0, 0},

#define VFTDELTA_DECL(n)    unsigned    uDelta##n
#define VFTDELTA_VAL()      0,

 




#include "nscore.h"
#include "nsServiceManagerUtils.h"

 



#ifndef __cplusplus
typedef struct nsID 
{
  PRUint32  m0;
  PRUint16  m1;
  PRUint16  m2;
  PRUint8   m3[8];
} nsID, nsCID, nsIID;
#define REFNSIID const nsIID *

typedef PRUint32 nsrefcnt;
#endif 





typedef struct vftable_nsISupports
{
    VFTFIRST_DECL;
    nsresult (*VFTCALL QueryInterface)(void *pvThis, REFNSIID aIID, void** aInstancePtr);
    VFTDELTA_DECL(QueryInterface);
    nsrefcnt (*VFTCALL AddRef)(void *pvThis);
    VFTDELTA_DECL(AddRef);
    nsrefcnt (*VFTCALL Release)(void *pvThis);
    VFTDELTA_DECL(Release);
} VFTnsISupports;




typedef struct vftable_nsGetServiceByCID_nsCOMPtr_helper
{
    VFTFIRST_DECL;
    
    nsresult (*VFTCALL __operator_paratheses)(void *pvThis, REFNSIID aIID, void** aInstancePtr);
    VFTDELTA_DECL(__operator_paratheses);
    void *   (*VFTCALL __destructor)(void *pvThis, unsigned __dtorFlags, unsigned __vtt);
    VFTDELTA_DECL(__destructor);
} VFTnsGetServiceByCID_nsCOMPtr_helper;




typedef struct vftable_nsQueryInterface_nsCOMPtr_helper
{
    VFTFIRST_DECL;
    nsresult (*VFTCALL __operator_paratheses)(void *pvThis, REFNSIID aIID, void** aInstancePtr);
    VFTDELTA_DECL(__operator_paratheses);
} VFTnsQueryInterface_nsCOMPtr_helper;







typedef struct obj_nsISupports
{
    VFTnsISupports     *pVFT;
} obj_nsISupports;




typedef struct obj_nsCOMPtr_base
{
    obj_nsISupports    *mRawPtr;
} obj_nsCOMPtr_base;
 



typedef struct obj_nsGetServiceByCID_nsCOMPtr_helper
{
    VFTnsGetServiceByCID_nsCOMPtr_helper   *pVFT;           
    nsID                                   *mCID;           
    void                                   *mServiceManager;
    nsresult                               *mErrorPtr;
} obj_nsGetServiceByCID_nsCOMPtr_helper;




typedef struct obj_nsQueryInterface_nsCOMPtr_helper
{
    VFTnsQueryInterface_nsCOMPtr_helper    *pVFT;           
    obj_nsISupports                        *mRawPtr;        
    nsresult                               *mErrorPtr;
} obj_nsQueryInterface_nsCOMPtr_helper;






















































extern "C" void * VFTCALL __dt__13nsCOMPtr_baseFv(void *pvThis, unsigned __dtorFlags)
{
    obj_nsCOMPtr_base *pThis = (obj_nsCOMPtr_base*)pvThis;

    if (pThis->mRawPtr)
    {
        
        pThis->mRawPtr->pVFT->Release((char*)pThis->mRawPtr + pThis->mRawPtr->pVFT->uDeltaRelease);
    }

    



    #if 0
    if (!(__dtorFlags & 1))
        __dl__FPv(this)
    #endif

    return pvThis;
}


extern "C" void * VFTCALL _dt__13nsCOMPtr_baseFv(void *pvThis, unsigned __dtorFlags)
{
    return __dt__13nsCOMPtr_baseFv(pvThis, __dtorFlags);
}


































































































































































extern "C" nsresult VFTCALL GSBC_COM__operator_paratheses(void *pvThis, REFNSIID aIID, void** aInstancePtr)
{
    obj_nsGetServiceByCID_nsCOMPtr_helper *pThis = (obj_nsGetServiceByCID_nsCOMPtr_helper *)pvThis;
    nsresult status = NS_ERROR_FAILURE;

    
    


    nsCOMPtr<nsIServiceManager>     mgr;
    NS_GetServiceManager(getter_AddRefs(mgr));
    if (mgr)
        status = mgr->GetService(*pThis->mCID, aIID, (void**)aInstancePtr);

    if (NS_FAILED(status))
        *aInstancePtr = 0;

    if (pThis->mErrorPtr)
        *pThis->mErrorPtr = status;
    return status;
}
































extern "C" void * VFTCALL GSBC_COM__destructor(void *pvThis, unsigned __dtorFlags, unsigned __vtt)
{
    obj_nsGetServiceByCID_nsCOMPtr_helper *pThis = (obj_nsGetServiceByCID_nsCOMPtr_helper *)pvThis;


    





    __dtorFlags = __dtorFlags;
    __vtt = __vtt;
    return pThis;
}
















extern const VFTnsGetServiceByCID_nsCOMPtr_helper _vft17nsGetServiceByCID15nsCOMPtr_helper =
{
    VFTFIRST_VAL()
    GSBC_COM__operator_paratheses,                          VFTDELTA_VAL()
    GSBC_COM__destructor,                                   VFTDELTA_VAL()
};


































































































extern "C" nsresult VFTCALL QI_COM__operator_paratheses(void *pvThis, REFNSIID aIID, void** aInstancePtr)
{
    obj_nsQueryInterface_nsCOMPtr_helper *pThis = (obj_nsQueryInterface_nsCOMPtr_helper *)pvThis;
    nsresult status = NS_ERROR_NULL_POINTER;


    if (pThis->mRawPtr)
    {
        status = pThis->mRawPtr->pVFT->QueryInterface(pThis->mRawPtr, aIID, aInstancePtr);
        
    }
    
    if (pThis->mErrorPtr)
        *pThis->mErrorPtr = status;
    return status;
}















extern const VFTnsQueryInterface_nsCOMPtr_helper _vft16nsQueryInterface15nsCOMPtr_helper = 
{
    VFTFIRST_VAL()
    QI_COM__operator_paratheses,                          VFTDELTA_VAL()
};
























extern "C" void VFTCALL assign_assuming_AddRef__13nsCOMPtr_baseFP11nsISupports(void *pvThis, obj_nsISupports *newPtr)
{
    obj_nsCOMPtr_base  *pThis = (obj_nsCOMPtr_base *)pvThis;
    obj_nsISupports    *oldPtr;
     
    oldPtr = pThis->mRawPtr;
    pThis->mRawPtr = newPtr;
    if (oldPtr)
    {
        
        pThis->mRawPtr->pVFT->Release(oldPtr + oldPtr->pVFT->uDeltaRelease);
    }
}





























































extern "C" void VFTCALL assign_from_helper__13nsCOMPtr_baseFRC15nsCOMPtr_helperRC4nsID(
    void *pvThis, void * helper, REFNSIID iid)
{
    obj_nsCOMPtr_base  *pThis = (obj_nsCOMPtr_base *)pvThis;
    obj_nsISupports*    newRawPtr = NULL;
    nsresult            status = NS_ERROR_FAILURE;


    
    obj_nsQueryInterface_nsCOMPtr_helper  * pHelper = (obj_nsQueryInterface_nsCOMPtr_helper*)helper;

    
    status = pHelper->pVFT->__operator_paratheses((char*)pHelper + pHelper->pVFT->uDelta__operator_paratheses, 
                                                   iid, (void**)&newRawPtr);
    if (NS_FAILED(status))
        newRawPtr = 0;

    
    assign_assuming_AddRef__13nsCOMPtr_baseFP11nsISupports(pThis, newRawPtr);
}



