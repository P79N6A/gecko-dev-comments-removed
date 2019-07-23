




































#include "nsString.h"
#include "nsReadableUtils.h"
#include "pratom.h"
#include "prtypes.h"
#include "nsISupports.h"
#include "nsILocale.h"
#include "nsLocale.h"
#include "nsLocaleCID.h"
#include "nsCOMPtr.h"
#include "nsVoidArray.h"
#include "nsMemory.h"
#include "nsCRT.h"

#define LOCALE_HASH_SIZE  0xFF



NS_IMPL_THREADSAFE_ISUPPORTS1(nsLocale, nsILocale)

nsLocale::nsLocale(void)
:  fHashtable(nsnull), fCategoryCount(0)
{
  fHashtable = PL_NewHashTable(LOCALE_HASH_SIZE,&nsLocale::Hash_HashFunction,
                               &nsLocale::Hash_CompareNSString,
                               &nsLocale::Hash_CompareNSString, NULL, NULL);
  NS_ASSERTION(fHashtable, "nsLocale: failed to allocate PR_Hashtable");
}

nsLocale::nsLocale(nsLocale* other) : fHashtable(nsnull), fCategoryCount(0)
{
  fHashtable = PL_NewHashTable(LOCALE_HASH_SIZE,&nsLocale::Hash_HashFunction,
                               &nsLocale::Hash_CompareNSString,
                               &nsLocale::Hash_CompareNSString, NULL, NULL);
  NS_ASSERTION(fHashtable, "nsLocale: failed to allocate PR_Hashtable");

  
  
  
  PL_HashTableEnumerateEntries(other->fHashtable, 
                               &nsLocale::Hash_EnumerateCopy, fHashtable);
}


nsLocale::nsLocale(const nsStringArray& categoryList, 
                   const nsStringArray& valueList) 
                  : fHashtable(NULL), fCategoryCount(0)
{
  PRInt32 i;
  PRUnichar* key, *value;

  fHashtable = PL_NewHashTable(LOCALE_HASH_SIZE,&nsLocale::Hash_HashFunction,
                               &nsLocale::Hash_CompareNSString,
                               &nsLocale::Hash_CompareNSString,
                               NULL, NULL);
  NS_ASSERTION(fHashtable, "nsLocale: failed to allocate PR_Hashtable");

  if (fHashtable)
  {
    for(i=0; i < categoryList.Count(); ++i) 
    {
      key = ToNewUnicode(*categoryList.StringAt(i));
      NS_ASSERTION(key, "nsLocale: failed to allocate internal hash key");
      value = ToNewUnicode(*valueList.StringAt(i));
      NS_ASSERTION(value, "nsLocale: failed to allocate internal hash value");
      if (!PL_HashTableAdd(fHashtable,key,value)) {
          nsMemory::Free(key);
          nsMemory::Free(value);
      }
    }
  }
}

nsLocale::~nsLocale(void)
{
  
  
  PL_HashTableEnumerateEntries(fHashtable, &nsLocale::Hash_EnumerateDelete,
                               NULL);

  PL_HashTableDestroy(fHashtable);
}

NS_IMETHODIMP
nsLocale::GetCategory(const nsAString& category, nsAString& result)
{
  const PRUnichar *value = (const PRUnichar*) 
    PL_HashTableLookup(fHashtable, PromiseFlatString(category).get());

  if (value)
  {
    result.Assign(value);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsLocale::AddCategory(const nsAString &category, const nsAString &value)
{
  PRUnichar* newKey = ToNewUnicode(category);
  if (!newKey)
    return NS_ERROR_OUT_OF_MEMORY;

  PRUnichar* newValue = ToNewUnicode(value);
  if (!newValue) {
    nsMemory::Free(newKey);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!PL_HashTableAdd(fHashtable, newKey, newValue)) {
    nsMemory::Free(newKey);
    nsMemory::Free(newValue);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}


PLHashNumber
nsLocale::Hash_HashFunction(const void* key)
{
  const PRUnichar* ptr = (const PRUnichar *) key;
  PLHashNumber hash;

  hash = (PLHashNumber)0;

  while (*ptr)
    hash += (PLHashNumber) *ptr++;

  return hash;
}


PRIntn
nsLocale::Hash_CompareNSString(const void* s1, const void* s2)
{
  return !nsCRT::strcmp((const PRUnichar *) s1, (const PRUnichar *) s2);
}


PRIntn
nsLocale::Hash_EnumerateDelete(PLHashEntry *he, PRIntn hashIndex, void *arg)
{
  
  nsMemory::Free((PRUnichar *)he->key);
  nsMemory::Free((PRUnichar *)he->value);

  return (HT_ENUMERATE_NEXT | HT_ENUMERATE_REMOVE);
}

PRIntn
nsLocale::Hash_EnumerateCopy(PLHashEntry *he, PRIntn hashIndex, void* arg)
{
  PRUnichar* newKey = ToNewUnicode(nsDependentString((PRUnichar *)he->key));
  if (!newKey) 
    return HT_ENUMERATE_STOP;

  PRUnichar* newValue = ToNewUnicode(nsDependentString((PRUnichar *)he->value));
  if (!newValue) {
    nsMemory::Free(newKey);
    return HT_ENUMERATE_STOP;
  }

  if (!PL_HashTableAdd((PLHashTable*)arg, newKey, newValue)) {
    nsMemory::Free(newKey);
    nsMemory::Free(newValue);
    return HT_ENUMERATE_STOP;
  }

  return (HT_ENUMERATE_NEXT);
}

