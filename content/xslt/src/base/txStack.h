





































#ifndef txStack_h___
#define txStack_h___

#include "nsVoidArray.h"

class txStack : private nsVoidArray
{
public:
    





    inline void* peek()
    {
        NS_ASSERTION(!isEmpty(), "peeking at empty stack");
        return ElementAt(Count() - 1);
    }

    





    inline nsresult push(void* aObject)
    {
        return InsertElementAt(aObject, Count()) ? NS_OK :
                                                   NS_ERROR_OUT_OF_MEMORY;
    }

    





    inline void* pop()
    {
        NS_ASSERTION(!isEmpty(), "popping from empty stack");
        const PRInt32 count = Count() - 1;
        void* object = ElementAt(count);
        RemoveElementsAt(count, 1);
        return object;
    }

    




    inline PRBool isEmpty()
    {
        return (Count() <= 0);
    }

    




    inline PRInt32 size()
    {
        return Count();
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

    




    inline PRBool hasNext()
    {
        return (mPosition < mStack->Count());
    }

    




    inline void* next()
    {
        if (mPosition == mStack->Count()) {
            return nsnull;
        }
        return mStack->ElementAt(mPosition++);
    }

private:
    txStack* mStack;
    PRInt32 mPosition;
};

#endif 
