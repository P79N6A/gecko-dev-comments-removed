


















#ifndef nsLocale_h__
#define nsLocale_h__

#include "nsString.h"
#include "nsTArray.h"
#include "nsILocale.h"
#include "plhash.h"

class nsLocale : public nsILocale {
	friend class nsLocaleService;
	NS_DECL_ISUPPORTS

public:
	nsLocale(void);
	virtual ~nsLocale(void);
	
	
	NS_DECL_NSILOCALE

protected:
	
	NS_IMETHOD AddCategory(const nsAString& category, const nsAString& value);

	static PLHashNumber Hash_HashFunction(const void* key);
	static int Hash_CompareNSString(const void* s1, const void* s2);
	static int Hash_EnumerateDelete(PLHashEntry *he, int hashIndex, void *arg);

	PLHashTable*	fHashtable;
	uint32_t		fCategoryCount;

};


#endif
