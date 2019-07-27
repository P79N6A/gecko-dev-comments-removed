




#ifndef WEBGLOBJECTMODEL_H_
#define WEBGLOBJECTMODEL_H_

#include "nsCycleCollectionNoteChild.h"
#include "nsICanvasRenderingContextInternal.h"
#include "WebGLTypes.h"

namespace mozilla {

class WebGLContext;














































































template<typename Derived>
class WebGLRefCountedObject
{
public:
    enum DeletionStatus { Default, DeleteRequested, Deleted };

    WebGLRefCountedObject()
      : mDeletionStatus(Default)
    {}

    ~WebGLRefCountedObject() {
        MOZ_ASSERT(mWebGLRefCnt == 0,
                   "Destroying WebGL object still referenced by other WebGL"
                   " objects.");
        MOZ_ASSERT(mDeletionStatus == Deleted,
                   "Derived class destructor must call DeleteOnce().");
    }

    
    void WebGLAddRef() {
        ++mWebGLRefCnt;
    }

    
    void WebGLRelease() {
        MOZ_ASSERT(mWebGLRefCnt > 0,
                   "Releasing WebGL object with WebGL refcnt already zero");
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
    {}

    WebGLRefPtr(const WebGLRefPtr<T>& smartPtr)
        : mRawPtr(smartPtr.mRawPtr)
    {
        AddRefOnPtr(mRawPtr);
    }

    explicit WebGLRefPtr(T* rawPtr)
        : mRawPtr(rawPtr)
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

    T* operator->() const MOZ_NO_ADDREF_RELEASE_ON_RETURN {
        MOZ_ASSERT(mRawPtr != 0, "You can't dereference a nullptr WebGLRefPtr with operator->()!");
        return get();
    }

    T& operator*() const {
        MOZ_ASSERT(mRawPtr != 0, "You can't dereference a nullptr WebGLRefPtr with operator*()!");
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
    T* mRawPtr;
};




class WebGLContextBoundObject
{
public:
    explicit WebGLContextBoundObject(WebGLContext* webgl);

    bool IsCompatibleWithContext(WebGLContext* other);

    WebGLContext* Context() const { return mContext; }

protected:
    WebGLContext* const mContext;
    const uint32_t mContextGeneration;
};



class WebGLRectangleObject
{
public:
    WebGLRectangleObject()
        : mWidth(0)
        , mHeight(0)
    {}

    WebGLRectangleObject(GLsizei width, GLsizei height)
        : mWidth(width)
        , mHeight(height)
    {}

    GLsizei Width() const { return mWidth; }
    void width(GLsizei value) { mWidth = value; }

    GLsizei Height() const { return mHeight; }
    void height(GLsizei value) { mHeight = value; }

    void setDimensions(GLsizei width, GLsizei height) {
        mWidth = width;
        mHeight = height;
    }

    void setDimensions(WebGLRectangleObject* rect) {
        if (rect) {
            mWidth = rect->Width();
            mHeight = rect->Height();
        } else {
            mWidth = 0;
            mHeight = 0;
        }
    }

    bool HasSameDimensionsAs(const WebGLRectangleObject& other) const {
        return Width() == other.Width() && Height() == other.Height();
    }

protected:
    GLsizei mWidth;
    GLsizei mHeight;
};

}

template <typename T>
inline void
ImplCycleCollectionUnlink(mozilla::WebGLRefPtr<T>& field)
{
    field = nullptr;
}

template <typename T>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& callback,
                            mozilla::WebGLRefPtr<T>& field,
                            const char* name,
                            uint32_t flags = 0)
{
    CycleCollectionNoteChild(callback, field.get(), name, flags);
}

#endif
