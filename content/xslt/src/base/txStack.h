




#ifndef txStack_h___
#define txStack_h___

#include "nsTArray.h"

class txStack : private nsTArray<void*>
{
public:
    





    inline void* peek()
    {
        NS_ASSERTION(!isEmpty(), "peeking at empty stack");
        return !isEmpty() ? ElementAt(Length() - 1) : nullptr;
    }

    





    inline nsresult push(void* aObject)
    {
        return AppendElement(aObject) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }

    





    inline void* pop()
    {
        void* object = nullptr;
        NS_ASSERTION(!isEmpty(), "popping from empty stack");
        if (!isEmpty())
        {
            const uint32_t count = Length() - 1;
            object = ElementAt(count);
            RemoveElementAt(count);
        }
        return object;
    }

    




    inline bool isEmpty()
    {
        return IsEmpty();
    }

    




    inline int32_t size()
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
            return nullptr;
        }
        return mStack->ElementAt(mPosition++);
    }

private:
    txStack* mStack;
    uint32_t mPosition;
};

#endif 
