



































#ifndef nsUnicodeFontMappingCache_h__
#define nsUnicodeFontMappingCache_h__
#include "plhash.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsUnicodeFontMappingMac.h"

class nsUnicodeFontMappingCache
{
public:
	nsUnicodeFontMappingCache() 
	{
		mTable = PL_NewHashTable(8, (PLHashFunction)HashKey, 
									(PLHashComparator)CompareKeys, 
									(PLHashComparator)CompareValues,
									nsnull, nsnull);
		mCount = 0;	
	};
	~nsUnicodeFontMappingCache() 
	{
		if (mTable)
		{
			PL_HashTableEnumerateEntries(mTable, FreeHashEntries, 0);
			PL_HashTableDestroy(mTable);
			mTable = nsnull;
		}
	};

	inline PRBool	Get(const nsString& key, nsUnicodeFontMappingMac **item)
	{
		*item = (nsUnicodeFontMappingMac*)PL_HashTableLookup(mTable, &key);
		return nsnull != (*item);
	};
	inline void	Set(const nsString& key, nsUnicodeFontMappingMac *item)
	{
		nsString *newKey = new nsString(key);
		if (newKey)
		{
			PL_HashTableAdd(mTable, newKey, item);
			mCount ++;
		}
	};

private:
	inline static PR_CALLBACK PLHashNumber HashKey(const void *aKey)
	{
    nsString* key = (nsString*)aKey;
		return nsCRT::HashCode(key->get());
	};
	inline static PR_CALLBACK PRIntn		CompareKeys(const void *v1, const void *v2)
	{
		return ((nsString *)v1) -> Equals(* ((nsString *)v2));
	};
	inline static PR_CALLBACK PRIntn		CompareValues(const void *v1, const void *v2)
	{
		return ((nsUnicodeFontMappingMac*)v1) -> Equals(* ((nsUnicodeFontMappingMac*)v2));
	};
	inline static PR_CALLBACK PRIntn		FreeHashEntries(PLHashEntry *he, PRIntn italic, void *arg)
	{
		delete (nsString*)he->key;
		delete (nsUnicodeFontMappingMac*)he->value;
		return HT_ENUMERATE_REMOVE;
	};

	struct PLHashTable*		mTable;
	PRUint32				mCount;
};

#endif 
