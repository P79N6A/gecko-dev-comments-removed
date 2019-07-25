




































#ifndef GFX_DWRITECOMMON_H
#define GFX_DWRITECOMMON_H


#include "nscore.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "cairo-features.h"
#include "gfxFontConstants.h"
#include "nsTArray.h"
#include "gfxWindowsPlatform.h"
#include "nsIUUIDGenerator.h"

#include <windows.h>
#include <dwrite.h>

static DWRITE_FONT_STRETCH
DWriteFontStretchFromStretch(PRInt16 aStretch) 
{
    switch (aStretch) {
        case NS_FONT_STRETCH_ULTRA_CONDENSED:
            return DWRITE_FONT_STRETCH_ULTRA_CONDENSED;
        case NS_FONT_STRETCH_EXTRA_CONDENSED:
            return DWRITE_FONT_STRETCH_EXTRA_CONDENSED;
        case NS_FONT_STRETCH_CONDENSED:
            return DWRITE_FONT_STRETCH_CONDENSED;
        case NS_FONT_STRETCH_SEMI_CONDENSED:
            return DWRITE_FONT_STRETCH_SEMI_CONDENSED;
        case NS_FONT_STRETCH_NORMAL:
            return DWRITE_FONT_STRETCH_NORMAL;
        case NS_FONT_STRETCH_SEMI_EXPANDED:
            return DWRITE_FONT_STRETCH_SEMI_EXPANDED;
        case NS_FONT_STRETCH_EXPANDED:
            return DWRITE_FONT_STRETCH_EXPANDED;
        case NS_FONT_STRETCH_EXTRA_EXPANDED:
            return DWRITE_FONT_STRETCH_EXTRA_EXPANDED;
        case NS_FONT_STRETCH_ULTRA_EXPANDED:
            return DWRITE_FONT_STRETCH_ULTRA_EXPANDED;
        default:
            return DWRITE_FONT_STRETCH_UNDEFINED;
    }
}

static PRInt16
FontStretchFromDWriteStretch(DWRITE_FONT_STRETCH aStretch) 
{
    switch (aStretch) {
        case DWRITE_FONT_STRETCH_ULTRA_CONDENSED:
            return NS_FONT_STRETCH_ULTRA_CONDENSED;
        case DWRITE_FONT_STRETCH_EXTRA_CONDENSED:
            return NS_FONT_STRETCH_EXTRA_CONDENSED;
        case DWRITE_FONT_STRETCH_CONDENSED:
            return NS_FONT_STRETCH_CONDENSED;
        case DWRITE_FONT_STRETCH_SEMI_CONDENSED:
            return NS_FONT_STRETCH_SEMI_CONDENSED;
        case DWRITE_FONT_STRETCH_NORMAL:
            return NS_FONT_STRETCH_NORMAL;
        case DWRITE_FONT_STRETCH_SEMI_EXPANDED:
            return NS_FONT_STRETCH_SEMI_EXPANDED;
        case DWRITE_FONT_STRETCH_EXPANDED:
            return NS_FONT_STRETCH_EXPANDED;
        case DWRITE_FONT_STRETCH_EXTRA_EXPANDED:
            return NS_FONT_STRETCH_EXTRA_EXPANDED;
        case DWRITE_FONT_STRETCH_ULTRA_EXPANDED:
            return NS_FONT_STRETCH_ULTRA_EXPANDED;
        default:
            return NS_FONT_STRETCH_NORMAL;
    }
}

struct ffReferenceKey
{
    FallibleTArray<PRUint8> *mArray;
    nsID mGUID;
};

class gfxDWriteFontFileLoader : public IDWriteFontFileLoader
{
public:
    gfxDWriteFontFileLoader()
    {
    }

    
    IFACEMETHOD(QueryInterface)(IID const& iid, OUT void** ppObject)
    {
        if (iid == __uuidof(IDWriteFontFileLoader)) {
            *ppObject = static_cast<IDWriteFontFileLoader*>(this);
            return S_OK;
        } else if (iid == __uuidof(IUnknown)) {
            *ppObject = static_cast<IUnknown*>(this);
            return S_OK;
        } else {
            return E_NOINTERFACE;
        }
    }

    IFACEMETHOD_(ULONG, AddRef)()
    {
        return 1;
    }

    IFACEMETHOD_(ULONG, Release)()
    {
        return 1;
    }

    
    



    virtual HRESULT STDMETHODCALLTYPE 
        CreateStreamFromKey(void const* fontFileReferenceKey,
                            UINT32 fontFileReferenceKeySize,
                            OUT IDWriteFontFileStream** fontFileStream);

    




    static IDWriteFontFileLoader* Instance()
    {
        if (!mInstance) {
            mInstance = new gfxDWriteFontFileLoader();
            gfxWindowsPlatform::GetPlatform()->GetDWriteFactory()->
                RegisterFontFileLoader(mInstance);
        }
        return mInstance;
    }

private:
    static IDWriteFontFileLoader* mInstance;
}; 

class gfxDWriteFontFileStream : public IDWriteFontFileStream
{
public:
    







    gfxDWriteFontFileStream(FallibleTArray<PRUint8> *aData);
    ~gfxDWriteFontFileStream();

    
    IFACEMETHOD(QueryInterface)(IID const& iid, OUT void** ppObject)
    {
        if (iid == __uuidof(IDWriteFontFileStream)) {
            *ppObject = static_cast<IDWriteFontFileStream*>(this);
            return S_OK;
        } else if (iid == __uuidof(IUnknown)) {
            *ppObject = static_cast<IUnknown*>(this);
            return S_OK;
        } else {
            return E_NOINTERFACE;
        }
    }

    IFACEMETHOD_(ULONG, AddRef)()
    {
        NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
        ++mRefCnt;
        return mRefCnt;
    }

    IFACEMETHOD_(ULONG, Release)()
    {
        NS_PRECONDITION(0 != mRefCnt, "dup release");
        --mRefCnt;
        if (mRefCnt == 0) {
            delete this;
            return 0;
        }
        return mRefCnt;
    }

    
    virtual HRESULT STDMETHODCALLTYPE ReadFileFragment(void const** fragmentStart,
                                                       UINT64 fileOffset,
                                                       UINT64 fragmentSize,
                                                       OUT void** fragmentContext);

    virtual void STDMETHODCALLTYPE ReleaseFileFragment(void* fragmentContext);

    virtual HRESULT STDMETHODCALLTYPE GetFileSize(OUT UINT64* fileSize);

    virtual HRESULT STDMETHODCALLTYPE GetLastWriteTime(OUT UINT64* lastWriteTime);

private:
    FallibleTArray<PRUint8> mData;
    nsAutoRefCnt mRefCnt;
}; 

#endif 
