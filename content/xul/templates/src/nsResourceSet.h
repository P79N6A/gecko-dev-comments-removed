




































#ifndef nsResourceSet_h__
#define nsResourceSet_h__

#include "nsIRDFResource.h"

class nsResourceSet
{
public:
    nsResourceSet()
        : mResources(nsnull),
          mCount(0),
          mCapacity(0) {
        MOZ_COUNT_CTOR(nsResourceSet); }

    nsResourceSet(const nsResourceSet& aResourceSet);

    nsResourceSet& operator=(const nsResourceSet& aResourceSet);
    
    ~nsResourceSet();

    nsresult Clear();
    nsresult Add(nsIRDFResource* aProperty);
    void Remove(nsIRDFResource* aProperty);

    PRBool Contains(nsIRDFResource* aProperty) const;

protected:
    nsIRDFResource** mResources;
    PRInt32 mCount;
    PRInt32 mCapacity;

public:
    class ConstIterator {
    protected:
        nsIRDFResource** mCurrent;

    public:
        ConstIterator() : mCurrent(nsnull) {}

        ConstIterator(const ConstIterator& aConstIterator)
            : mCurrent(aConstIterator.mCurrent) {}

        ConstIterator& operator=(const ConstIterator& aConstIterator) {
            mCurrent = aConstIterator.mCurrent;
            return *this; }

        ConstIterator& operator++() {
            ++mCurrent;
            return *this; }

        ConstIterator operator++(int) {
            ConstIterator result(*this);
            ++mCurrent;
            return result; }

         nsIRDFResource* operator*() const {
            return *mCurrent; }

         nsIRDFResource* operator->() const {
            return *mCurrent; }

        PRBool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        PRBool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }

    protected:
        ConstIterator(nsIRDFResource** aProperty) : mCurrent(aProperty) {}
        friend class nsResourceSet;
    };

    ConstIterator First() const { return ConstIterator(mResources); }
    ConstIterator Last() const { return ConstIterator(mResources + mCount); }
};

#endif 

