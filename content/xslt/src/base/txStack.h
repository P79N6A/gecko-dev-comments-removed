





































#ifndef txStack_h___
#define txStack_h___

#include "nsTArray.h"

class txStack : private nsTArray<void*>
{
public:
    





    inline void* peek()
    {
        NS_ASSERTION(!isEmpty(), "peeking at empty stack");
        return !isEmpty() ? ElementAt(Length() - 1) : nsnull;
    }

    





    inline nsresult push(void* aObject)
    {
        return AppendElement(aObject) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }

    





    inline void* pop()
    {
        void* object = nsnull;
        NS_ASSERTION(!isEmpty(), "popping from empty stack");
        if (!isEmpty())
        {
            const PRUint32 count = Length() - 1;
            object = ElementAt(count);
            RemoveElementAt(count);
        }
        return object;
    }

    




    inline bool isEmpty()
    {
        return IsEmpty();
    }

    




    inline PRInt32 size()
    {
        return Length();
    }

private:
    friend class txStackIterator;
};

class txStackIterator
{
public:
    




    inline
    txStackIterator(txStack* aStack) : mStack(aStack),
                                       mPosition(0)
    {
    }

    




    inline bool hasNext()
    {
        return (mPosition < mStack->Length());
    }

    




    inline void* next()
    {
        if (mPosition == mStack->Length()) {
            return nsnull;
        }
        return mStack->ElementAt(mPosition++);
    }

private:
    txStack* mStack;
    PRUint32 mPosition;
};

#endif 
