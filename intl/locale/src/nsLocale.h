


















































#ifndef nsLocale_h__
#define nsLocale_h__

#include "nsString.h"
#include "nsILocale.h"
#include "plhash.h"

class nsStringArray;

class nsLocale : public nsILocale {
	friend class nsLocaleService;
	NS_DECL_ISUPPORTS

public:
	nsLocale(void);
	nsLocale(const nsStringArray& categoryList, const nsStringArray& valueList);
	nsLocale(nsLocale* other);
	virtual ~nsLocale(void);
	
	
	NS_DECL_NSILOCALE

protected:
	
	NS_IMETHOD AddCategory(const nsAString& category, const nsAString& value);

	static PLHashNumber PR_CALLBACK Hash_HashFunction(const void* key);
	static PRIntn PR_CALLBACK Hash_CompareNSString(const void* s1, const void* s2);
	static PRIntn PR_CALLBACK Hash_EnumerateDelete(PLHashEntry *he, PRIntn hashIndex, void *arg);
	static PRIntn PR_CALLBACK Hash_EnumerateCopy(PLHashEntry *he, PRIntn hashIndex, void *arg);

	PLHashTable*	fHashtable;
	PRUint32		fCategoryCount;

};


#endif
