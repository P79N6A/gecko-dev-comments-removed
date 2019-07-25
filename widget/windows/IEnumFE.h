




#ifndef IEnumeFE_h__
#define IEnumeFE_h__





#include <ole2.h>

#include "nsTArray.h"
#include "mozilla/Attributes.h"


class FormatEtc
{
public:
  FormatEtc() { memset(&mFormat, 0, sizeof(FORMATETC)); }
  FormatEtc(const FormatEtc& copy) { CopyIn(&copy.mFormat); }
  ~FormatEtc() { if (mFormat.ptd) CoTaskMemFree(mFormat.ptd); }

  void CopyIn(const FORMATETC *aSrc) {
    if (!aSrc) {
        memset(&mFormat, 0, sizeof(FORMATETC));
        return;
    }
    mFormat = *aSrc;
    if (aSrc->ptd) {
        mFormat.ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));
        *(mFormat.ptd) = *(aSrc->ptd);
    }
  }

  void CopyOut(LPFORMATETC aDest) {
    if (!aDest)
        return;
    *aDest = mFormat;
    if (mFormat.ptd) {
        aDest->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));
        *(aDest->ptd) = *(mFormat.ptd);
    }
  }

private:
  FORMATETC mFormat;
};









class CEnumFormatEtc MOZ_FINAL : public IEnumFORMATETC
{
public:
    CEnumFormatEtc(nsTArray<FormatEtc>& aArray);
    CEnumFormatEtc();
    ~CEnumFormatEtc();

    
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    
    STDMETHODIMP Next(ULONG aMaxToFetch, FORMATETC *aResult, ULONG *aNumFetched);
    STDMETHODIMP Skip(ULONG aSkipNum);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(LPENUMFORMATETC *aResult); 

    
    void AddFormatEtc(LPFORMATETC aFormat);

private:
    nsTArray<FormatEtc> mFormatList; 
    ULONG mRefCnt; 
    ULONG mCurrentIdx; 

    void SetIndex(uint32_t aIdx);
};


#endif 
