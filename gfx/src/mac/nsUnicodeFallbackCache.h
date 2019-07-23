




































#ifndef nsUnicodeFallbackCache_h__
#define nsUnicodeFallbackCache_h__
#include "prtypes.h"
#include "plhash.h"
#include <Script.h>
#include "nsDebug.h"
#include "nscore.h"

class nsUnicodeFallbackCache
{
public:
	nsUnicodeFallbackCache() 
	{
		mTable = PL_NewHashTable(8, (PLHashFunction)HashKey, 
								(PLHashComparator)CompareKeys, 
								(PLHashComparator)CompareValues,
								nsnull, nsnull);
		mCount = 0;
	};
	~nsUnicodeFallbackCache() 
	{
		if (mTable)
		{
			PL_HashTableEnumerateEntries(mTable, FreeHashEntries, 0);
			PL_HashTableDestroy(mTable);
			mTable = nsnull;
		}
	};

	inline PRBool	Get(PRUnichar aChar, ScriptCode& oScript) 
	{
		ScriptCode ret = (ScriptCode) 
		    NS_PTR_TO_INT32(PL_HashTableLookup(mTable, (void*)aChar));
		oScript = 0x00FF & ret ;
		return 0x00 != (0xFF00 & ret);
	};
	
	inline void	Set(PRUnichar aChar, ScriptCode aScript) 
	{
		PL_HashTableAdd(mTable,(void*) aChar, (void*)(aScript | 0xFF00));
		mCount ++;
	};

	inline static nsUnicodeFallbackCache* GetSingleton() 
	{
			if(! gSingleton)
				gSingleton = new nsUnicodeFallbackCache();
			return gSingleton;
	}
private:
	inline static PR_CALLBACK PLHashNumber HashKey(const void *aKey) 
	{
		return (PRUnichar) NS_PTR_TO_INT32(aKey);
	};
	
	inline static PR_CALLBACK PRIntn		CompareKeys(const void *v1, const void *v2) 
	{
		return  (((PRUnichar ) NS_PTR_TO_INT32(v1)) == 
		    ((PRUnichar ) NS_PTR_TO_INT32(v2)));
	};
	
	inline static PR_CALLBACK PRIntn		CompareValues(const void *v1, const void *v2) 
	{
		return (((ScriptCode) NS_PTR_TO_INT32(v1)) == 
		    ((ScriptCode) NS_PTR_TO_INT32(v2)));
	};
	inline static PR_CALLBACK PRIntn		FreeHashEntries(PLHashEntry *he, PRIntn italic, void *arg) 
	{
		return HT_ENUMERATE_REMOVE;
	};

	struct PLHashTable*		mTable;
	PRUint32				mCount;

	static nsUnicodeFallbackCache* gSingleton;
};
#endif nsUnicodeFallbackCache_h__
