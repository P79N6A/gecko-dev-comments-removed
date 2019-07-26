






#ifndef SkDWriteFontFileStream_DEFINED
#define SkDWriteFontFileStream_DEFINED

#include "SkTypes.h"

#include "SkStream.h"
#include "SkTScopedComPtr.h"

#include <dwrite.h>





class SkDWriteFontFileStream : public SkStream {
public:
    explicit SkDWriteFontFileStream(IDWriteFontFileStream* fontFileStream);
    virtual ~SkDWriteFontFileStream();

    virtual bool rewind() SK_OVERRIDE;
    virtual size_t read(void* buffer, size_t size) SK_OVERRIDE;
    virtual const void* getMemoryBase() SK_OVERRIDE;

private:
    SkTScopedComPtr<IDWriteFontFileStream> fFontFileStream;
    size_t fPos;
    const void* fLockedMemory;
    void* fFragmentLock;
};





class SkDWriteFontFileStreamWrapper : public IDWriteFontFileStream {
public:
    
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    
    virtual HRESULT STDMETHODCALLTYPE ReadFileFragment(
        void const** fragmentStart,
        UINT64 fileOffset,
        UINT64 fragmentSize,
        void** fragmentContext);

    virtual void STDMETHODCALLTYPE ReleaseFileFragment(void* fragmentContext);
    virtual HRESULT STDMETHODCALLTYPE GetFileSize(UINT64* fileSize);
    virtual HRESULT STDMETHODCALLTYPE GetLastWriteTime(UINT64* lastWriteTime);

    static HRESULT Create(SkStream* stream, SkDWriteFontFileStreamWrapper** streamFontFileStream);

private:
    explicit SkDWriteFontFileStreamWrapper(SkStream* stream);

    ULONG fRefCount;
    SkAutoTUnref<SkStream> fStream;
    SkMutex fStreamMutex;
};
#endif
