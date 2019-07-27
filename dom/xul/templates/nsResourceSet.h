



#ifndef nsResourceSet_h__
#define nsResourceSet_h__

#include "nsIRDFResource.h"

class nsResourceSet
{
public:
    nsResourceSet()
        : mResources(nullptr),
          mCount(0),
          mCapacity(0) {
        MOZ_COUNT_CTOR(nsResourceSet); }

    nsResourceSet(const nsResourceSet& aResourceSet);

    nsResourceSet& operator=(const nsResourceSet& aResourceSet);
    
    ~nsResourceSet();

    nsresult Clear();
    nsresult Add(nsIRDFResource* aProperty);
    void Remove(nsIRDFResource* aProperty);

    bool Contains(nsIRDFResource* aProperty) const;

protected:
    nsIRDFResource** mResources;
    int32_t mCount;
    int32_t mCapacity;

public:
    class ConstIterator {
    protected:
        nsIRDFResource** mCurrent;

    public:
        ConstIterator() : mCurrent(nullptr) {}

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

        bool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        bool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }

    protected:
        explicit ConstIterator(nsIRDFResource** aProperty) : mCurrent(aProperty) {}
        friend class nsResourceSet;
    };

    ConstIterator First() const { return ConstIterator(mResources); }
    ConstIterator Last() const { return ConstIterator(mResources + mCount); }
};

#endif 

