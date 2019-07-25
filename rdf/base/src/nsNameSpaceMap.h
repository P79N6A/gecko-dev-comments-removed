






































#ifndef nsNameSpaceMap_h__
#define nsNameSpaceMap_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIAtom.h"

class nsNameSpaceMap
{
public:
    class Entry {
    public:
        Entry(const nsCSubstring& aURI, nsIAtom* aPrefix)
            : mURI(aURI), mPrefix(aPrefix), mNext(nsnull) {
            MOZ_COUNT_CTOR(nsNameSpaceMap::Entry); }

        ~Entry() { MOZ_COUNT_DTOR(nsNameSpaceMap::Entry); }
        
        nsCString mURI;
        nsCOMPtr<nsIAtom> mPrefix; 

        Entry* mNext;
    };

    nsNameSpaceMap();
    ~nsNameSpaceMap();

    nsresult
    Put(const nsAString& aURI, nsIAtom* aPrefix);

    nsresult
    Put(const nsCSubstring& aURI, nsIAtom* aPrefix);

    class const_iterator {
    protected:
        friend class nsNameSpaceMap;

        const_iterator(const Entry* aCurrent)
            : mCurrent(aCurrent) {}

        const Entry* mCurrent;

    public:
        const_iterator()
            : mCurrent(nsnull) {}

        const_iterator(const const_iterator& iter)
            : mCurrent(iter.mCurrent) {}

        const_iterator&
        operator=(const const_iterator& iter) {
            mCurrent = iter.mCurrent;
            return *this; }

        const_iterator&
        operator++() {
            mCurrent = mCurrent->mNext;
            return *this; }

        const_iterator
        operator++(int) {
            const_iterator tmp(*this);
            mCurrent = mCurrent->mNext;
            return tmp; }

        const Entry* operator->() const { return mCurrent; }

        const Entry& operator*() const { return *mCurrent; }

        PRBool
        operator==(const const_iterator& iter) const {
            return mCurrent == iter.mCurrent; }

        PRBool
        operator!=(const const_iterator& iter) const {
            return ! iter.operator==(*this); }
    };

    const_iterator first() const {
        return const_iterator(mEntries); }

    const_iterator last() const {
        return const_iterator(nsnull); }

    const_iterator GetNameSpaceOf(const nsCSubstring& aURI) const;

protected:
    Entry* mEntries;
};


#endif 
