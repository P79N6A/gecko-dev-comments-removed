




































#ifndef ipcList_h__
#define ipcList_h__

#include "prtypes.h"
















template<class T>
class ipcList
{
public:
    ipcList()
        : mHead(NULL)
        , mTail(NULL)
        { }
   ~ipcList() { DeleteAll(); }

    
    
    
    void Prepend(T *obj)
    {
        obj->mNext = mHead;
        mHead = obj;
        if (!mTail)
            mTail = mHead;
    }

    
    
    
    void Append(T *obj)
    {
        obj->mNext = NULL;
        if (mTail) {
            mTail->mNext = obj;
            mTail = obj;
        }
        else
            mTail = mHead = obj;
    }

    
    
    
    void InsertAfter(T *a, T *b)
    {
        b->mNext = a->mNext;
        a->mNext = b;
        if (mTail == a)
            mTail = b;
    }

    
    
    
    void RemoveFirst()
    {
        if (mHead)
            AdvanceHead();
    }

    
    
    
    void RemoveAfter(T *obj)
    {
        T *rej = obj->mNext;
        if (rej) {
            obj->mNext = rej->mNext;
            if (rej == mTail)
                mTail = obj;
        }
    }

    
    
    
    void DeleteFirst()
    {
        T *first = mHead;
        if (first) {
            AdvanceHead();
            delete first;
        }
    }

    
    
    
    void DeleteAfter(T *obj)
    {
        T *rej = obj->mNext;
        if (rej) {
            RemoveAfter(obj);
            delete rej;
        }
    }

    
    
    
    void DeleteAll()
    {
        while (mHead)
            DeleteFirst();
    }

    const T *First() const { return mHead; }
    T       *First()       { return mHead; }
    const T *Last() const  { return mTail; }
    T       *Last()        { return mTail; }

    PRBool  IsEmpty() const { return mHead == NULL; }

    
    
    
    void MoveTo(ipcList<T> &other)
    {
        other.mHead = mHead;
        other.mTail = mTail;
        mHead = NULL;
        mTail = NULL;
    }

protected:
    void AdvanceHead()
    {
        mHead = mHead->mNext;
        if (!mHead)
            mTail = NULL;
    }

    T *mHead;
    T *mTail;
};

template<class T>
class ipcListNode
{
public:
    ipcListNode() : mNext(nsnull) {}

    T *mNext;
};

#endif 
