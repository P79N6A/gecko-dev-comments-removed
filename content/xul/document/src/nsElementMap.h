


























































#ifndef nsElementMap_h__
#define nsElementMap_h__

#include "nscore.h"
#include "nsError.h"
#include "plhash.h"
#include "nsIContent.h"
#include "nsFixedSizeAllocator.h"
#include "nsCOMArray.h"

class nsString;
class nsISupportsArray;

class nsElementMap
{
protected:
    PLHashTable* mMap;
    nsFixedSizeAllocator mPool;

    static PLHashAllocOps gAllocOps;

    class ContentListItem {
    public:
        ContentListItem* mNext;
        nsCOMPtr<nsIContent> mContent;

        static ContentListItem*
        Create(nsFixedSizeAllocator& aPool, nsIContent* aContent) {
            void* bytes = aPool.Alloc(sizeof(ContentListItem));
            return bytes ? new (bytes) ContentListItem(aContent) : nsnull; }

        static void
        Destroy(nsFixedSizeAllocator& aPool, ContentListItem* aItem) {
            delete aItem;
            aPool.Free(aItem, sizeof(*aItem)); }

    protected:
        static void* operator new(size_t aSize, void* aPtr) CPP_THROW_NEW {
            return aPtr; }

        static void operator delete(void* aPtr, size_t aSize) {
             }

        ContentListItem(nsIContent* aContent) : mNext(nsnull), mContent(aContent) {
            MOZ_COUNT_CTOR(nsElementMap::ContentListItem); }

        ~ContentListItem() {
            MOZ_COUNT_DTOR(nsElementMap::ContentListItem); }
    };

    static PLHashNumber PR_CALLBACK
    Hash(const void* akey);

    static PRIntn PR_CALLBACK
    Compare(const void* aLeft, const void* aRight);

    static PRIntn PR_CALLBACK
    ReleaseContentList(PLHashEntry* aHashEntry, PRIntn aIndex, void* aClosure);

public:
    nsElementMap(void);
    virtual ~nsElementMap();

    nsresult
    Add(const nsAString& aID, nsIContent* aContent);

    nsresult
    Remove(const nsAString& aID, nsIContent* aContent);

    nsresult
    Find(const nsAString& aID, nsCOMArray<nsIContent>& aResults);

    nsresult
    FindFirst(const nsAString& aID, nsIContent** aContent);

    typedef PRIntn (*nsElementMapEnumerator)(const PRUnichar* aID,
                                             nsIContent* aElement,
                                             void* aClosure);

    nsresult
    Enumerate(nsElementMapEnumerator aEnumerator, void* aClosure);

private:
    struct EnumerateClosure {
        nsElementMap*          mSelf;
        nsElementMapEnumerator mEnumerator;
        void*                  mClosure;
    };
        
    static PRIntn PR_CALLBACK
    EnumerateImpl(PLHashEntry* aHashEntry, PRIntn aIndex, void* aClosure);
};


#endif 
