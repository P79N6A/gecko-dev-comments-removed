








#ifndef SkIStream_DEFINED
#define SkIStream_DEFINED

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ole2.h>

class SkStream;
class SkWStream;





class SkBaseIStream : public IStream {
private:
    LONG _refcount;

protected:
    explicit SkBaseIStream();
    virtual ~SkBaseIStream();

public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid
                                                   , void ** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    
public:
    virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);

    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv
                                          , ULONG cb
                                          , ULONG* pcbWritten);

    
public:
    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER);
    
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream*
                                           , ULARGE_INTEGER
                                           , ULARGE_INTEGER*
                                           , ULARGE_INTEGER*);
    
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD);
    
    virtual HRESULT STDMETHODCALLTYPE Revert(void);
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER
                                               , ULARGE_INTEGER
                                               , DWORD);
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER
                                                 , ULARGE_INTEGER
                                                 , DWORD);
    
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream **);

    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove
                                         , DWORD dwOrigin
                                         , ULARGE_INTEGER* lpNewFilePointer);

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg
                                         , DWORD grfStatFlag);
};




class SkIStream : public SkBaseIStream {
private:
    SkStream *fSkStream;
    bool fUnrefOnRelease;
    ULARGE_INTEGER fLocation;

    SkIStream(SkStream* stream, bool unrefOnRelease);
    virtual ~SkIStream();

public:
    HRESULT static CreateFromSkStream(SkStream* stream
                                    , bool unrefOnRelease
                                    , IStream ** ppStream);

    virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);

    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv
                                          , ULONG cb
                                          , ULONG* pcbWritten);

    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove
                                         , DWORD dwOrigin
                                         , ULARGE_INTEGER* lpNewFilePointer);

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg
                                         , DWORD grfStatFlag);
};




class SkWIStream : public SkBaseIStream {
private:
    SkWStream *fSkWStream;

    SkWIStream(SkWStream* stream);
    virtual ~SkWIStream();

public:
    HRESULT static CreateFromSkWStream(SkWStream* stream, IStream ** ppStream);

    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv
                                          , ULONG cb
                                          , ULONG* pcbWritten);

    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD);

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg
                                         , DWORD grfStatFlag);
};

#endif
