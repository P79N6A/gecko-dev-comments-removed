




#ifndef WEBGLOBJECTMODEL_H_
#define WEBGLOBJECTMODEL_H_

#include "nsCycleCollectionNoteChild.h"
#include "nsICanvasRenderingContextInternal.h"


typedef uint32_t WebGLenum;
typedef uint32_t WebGLbitfield;
typedef int32_t WebGLint;
typedef int32_t WebGLsizei;
typedef int64_t WebGLsizeiptr;
typedef int64_t WebGLintptr;
typedef uint32_t WebGLuint;
typedef float WebGLfloat;
typedef float WebGLclampf;
typedef bool WebGLboolean;

namespace mozilla {

class WebGLBuffer;
class WebGLContext;













































































template<typename Derived>
class WebGLRefCountedObject
{
public:
    enum DeletionStatus { Default, DeleteRequested, Deleted };

    WebGLRefCountedObject()
      : mDeletionStatus(Default)
    { }

    ~WebGLRefCountedObject() {
        NS_ABORT_IF_FALSE(mWebGLRefCnt == 0, "destroying WebGL object still referenced by other WebGL objects");
        NS_ABORT_IF_FALSE(mDeletionStatus == Deleted, "Derived class destructor must call DeleteOnce()");
    }

    
    void WebGLAddRef() {
        ++mWebGLRefCnt;
    }

    
    void WebGLRelease() {
        NS_ABORT_IF_FALSE(mWebGLRefCnt > 0, "releasing WebGL object with WebGL refcnt already zero");
        --mWebGLRefCnt;
        MaybeDelete();
    }

    
    void RequestDelete() {
        if (mDeletionStatus == Default)
            mDeletionStatus = DeleteRequested;
        MaybeDelete();
    }

    bool IsDeleted() const {
        return mDeletionStatus == Deleted;
    }

    bool IsDeleteRequested() const {
        return mDeletionStatus != Default;
    }

    void DeleteOnce() {
        if (mDeletionStatus != Deleted) {
            static_cast<Derived*>(this)->Delete();
            mDeletionStatus = Deleted;
        }
    }

private:
    void MaybeDelete() {
        if (mWebGLRefCnt == 0 &&
            mDeletionStatus == DeleteRequested)
        {
            DeleteOnce();
        }
    }

protected:
    nsAutoRefCnt mWebGLRefCnt;
    DeletionStatus mDeletionStatus;
};















template<typename T>
class WebGLRefPtr
{
public:
    WebGLRefPtr()
        : mRawPtr(0)
    { }

    WebGLRefPtr(const WebGLRefPtr<T>& aSmartPtr)
        : mRawPtr(aSmartPtr.mRawPtr)
    {
        AddRefOnPtr(mRawPtr);
    }

    WebGLRefPtr(T *aRawPtr)
        : mRawPtr(aRawPtr)
    {
        AddRefOnPtr(mRawPtr);
    }

    ~WebGLRefPtr() {
        ReleasePtr(mRawPtr);
    }

    WebGLRefPtr<T>&
    operator=(const WebGLRefPtr<T>& rhs)
    {
        assign_with_AddRef(rhs.mRawPtr);
        return *this;
    }

    WebGLRefPtr<T>&
    operator=(T* rhs)
    {
        assign_with_AddRef(rhs);
        return *this;
    }

    T* get() const {
        return static_cast<T*>(mRawPtr);
    }

    operator T*() const {
        return get();
    }

    T* operator->() const {
        NS_ABORT_IF_FALSE(mRawPtr != 0, "You can't dereference a NULL WebGLRefPtr with operator->()!");
        return get();
    }

    T& operator*() const {
        NS_ABORT_IF_FALSE(mRawPtr != 0, "You can't dereference a NULL WebGLRefPtr with operator*()!");
        return *get();
    }

private:

    static void AddRefOnPtr(T* rawPtr) {
        if (rawPtr) {
            rawPtr->WebGLAddRef();
            rawPtr->AddRef();
        }
    }

    static void ReleasePtr(T* rawPtr) {
        if (rawPtr) {
            rawPtr->WebGLRelease(); 
            rawPtr->Release();
        }
    }

    void assign_with_AddRef(T* rawPtr) {
        AddRefOnPtr(rawPtr);
        assign_assuming_AddRef(rawPtr);
    }

    void assign_assuming_AddRef(T* newPtr) {
        T* oldPtr = mRawPtr;
        mRawPtr = newPtr;
        ReleasePtr(oldPtr);
    }

protected:
    T *mRawPtr;
};




class WebGLContextBoundObject
{
public:
    WebGLContextBoundObject(WebGLContext *context);

    bool IsCompatibleWithContext(WebGLContext *other);

    WebGLContext *Context() const { return mContext; }

protected:
    WebGLContext *mContext;
    uint32_t mContextGeneration;
};

}

template <typename T>
inline void
ImplCycleCollectionUnlink(mozilla::WebGLRefPtr<T>& aField)
{
  aField = nullptr;
}

template <typename T>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            mozilla::WebGLRefPtr<T>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  CycleCollectionNoteEdgeName(aCallback, aName, aFlags);
  aCallback.NoteXPCOMChild(aField);
}

#endif
