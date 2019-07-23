





































#ifndef _IENUMFE_H_
#define _IENUMFE_H_
#include <ole2.h>










class CEnumFormatEtc;
typedef class CEnumFormatEtc *LPCEnumFormatEtc;

class CEnumFormatEtc : public IEnumFORMATETC
    {
    private:
        ULONG           mRefCnt;      
        ULONG           mCurrentInx;  
        ULONG           mNumFEs;      
        LPFORMATETC     mFEList;      
        ULONG           mMaxNumFEs;   

    public:
        CEnumFormatEtc(ULONG, LPFORMATETC);
        CEnumFormatEtc(ULONG aMaxSize);
        ~CEnumFormatEtc(void);

        
        STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        
        STDMETHODIMP Next(ULONG, LPFORMATETC, ULONG *);
        STDMETHODIMP Skip(ULONG);
        STDMETHODIMP Reset(void);
        STDMETHODIMP Clone(IEnumFORMATETC **);

        
        void AddFE(LPFORMATETC);
        bool InsertFEAt(LPFORMATETC, ULONG);
    };


#endif 
