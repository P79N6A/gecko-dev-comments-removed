




#ifndef TRANSFRMX_EXPANDEDNAMEMAP_H
#define TRANSFRMX_EXPANDEDNAMEMAP_H

#include "nsError.h"
#include "txXMLUtils.h"
#include "nsTArray.h"

class txExpandedNameMap_base {
protected:
    






    nsresult addItem(const txExpandedName& aKey, void* aValue);

    






    nsresult setItem(const txExpandedName& aKey, void* aValue,
                     void** aOldValue);

    




    void* getItem(const txExpandedName& aKey) const;

    





    void* removeItem(const txExpandedName& aKey);

    


    void clearItems()
    {
        mItems.Clear();
    }

    class iterator_base {
    public:
        iterator_base(txExpandedNameMap_base& aMap)
            : mMap(aMap),
              mCurrentPos(uint32_t(-1))
        {
        }

        bool next()
        {
            return ++mCurrentPos < mMap.mItems.Length();
        }

        const txExpandedName key()
        {
            NS_ASSERTION(mCurrentPos < mMap.mItems.Length(),
                         "invalid position in txExpandedNameMap::iterator");
            return txExpandedName(mMap.mItems[mCurrentPos].mNamespaceID,
                                  mMap.mItems[mCurrentPos].mLocalName);
        }

    protected:
        void* itemValue()
        {
            NS_ASSERTION(mCurrentPos < mMap.mItems.Length(),
                         "invalid position in txExpandedNameMap::iterator");
            return mMap.mItems[mCurrentPos].mValue;
        }

    private:
        txExpandedNameMap_base& mMap;
        uint32_t mCurrentPos;
    };
    
    friend class iterator_base;

    friend class txMapItemComparator;
    struct MapItem {
        int32_t mNamespaceID;
        nsCOMPtr<nsIAtom> mLocalName;
        void* mValue;
    };
    
    nsTArray<MapItem> mItems;
};

template<class E>
class txExpandedNameMap : public txExpandedNameMap_base
{
public:
    nsresult add(const txExpandedName& aKey, E* aValue)
    {
        return addItem(aKey, (void*)aValue);
    }

    nsresult set(const txExpandedName& aKey, E* aValue)
    {
        void* oldValue;
        return setItem(aKey, (void*)aValue, &oldValue);
    }

    E* get(const txExpandedName& aKey) const
    {
        return (E*)getItem(aKey);
    }

    E* remove(const txExpandedName& aKey)
    {
        return (E*)removeItem(aKey);
    }

    void clear()
    {
        clearItems();
    }

    class iterator : public iterator_base
    {
    public:
        iterator(txExpandedNameMap& aMap)
            : iterator_base(aMap)
        {
        }

        E* value()
        {
            return (E*)itemValue();
        }
    };
};

template<class E>
class txOwningExpandedNameMap : public txExpandedNameMap_base
{
public:
    ~txOwningExpandedNameMap()
    {
        clear();
    }

    nsresult add(const txExpandedName& aKey, E* aValue)
    {
        return addItem(aKey, (void*)aValue);
    }

    nsresult set(const txExpandedName& aKey, E* aValue)
    {
        nsAutoPtr<E> oldValue;
        return setItem(aKey, (void*)aValue, getter_Transfers(oldValue));
    }

    E* get(const txExpandedName& aKey) const
    {
        return (E*)getItem(aKey);
    }

    void remove(const txExpandedName& aKey)
    {
        delete (E*)removeItem(aKey);
    }

    void clear()
    {
        uint32_t i, len = mItems.Length();
        for (i = 0; i < len; ++i) {
            delete (E*)mItems[i].mValue;
        }
        clearItems();
    }

    class iterator : public iterator_base
    {
    public:
        iterator(txOwningExpandedNameMap& aMap)
            : iterator_base(aMap)
        {
        }

        E* value()
        {
            return (E*)itemValue();
        }
    };
};

#endif 
