




































#include "nsCheapSets.h"

nsCheapStringSet::~nsCheapStringSet()
{
  nsStringHashSet* set = GetHash();
  if (set) {
    delete set;
  } else {
    delete GetStr();
  }
}




nsresult
nsCheapStringSet::Put(const nsAString& aVal)
{
  
  nsStringHashSet* set = GetHash();
  if (set) {
    return set->Put(aVal);
  }

  
  if (GetStr()) {
    nsAString* oldStr = GetStr();
    nsresult rv = InitHash(&set);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = set->Put(*oldStr);
    delete oldStr;
    NS_ENSURE_SUCCESS(rv, rv);

    return set->Put(aVal);
  }

  
  return SetStr(aVal);
}

void
nsCheapStringSet::Remove(const nsAString& aVal)
{
  
  nsStringHashSet* set = GetHash();
  if (set) {
    set->Remove(aVal);
    return;
  }

  
  nsAString* str = GetStr();
  if (str && str->Equals(aVal)) {
    delete str;
    mValOrHash = nsnull;
  }
}

nsresult
nsCheapStringSet::InitHash(nsStringHashSet** aSet)
{
  nsStringHashSet* newSet = new nsStringHashSet();
  if (!newSet) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = newSet->Init(10);
  NS_ENSURE_SUCCESS(rv, rv);

  mValOrHash = newSet;
  *aSet = newSet;
  return NS_OK;
}


nsCheapInt32Set::~nsCheapInt32Set()
{
  delete GetHash();
}

nsresult
nsCheapInt32Set::Put(PRInt32 aVal)
{
  
  nsInt32HashSet* set = GetHash();
  if (set) {
    return set->Put(aVal);
  }

  
  if (IsInt()) {
    PRInt32 oldInt = GetInt();

    nsresult rv = InitHash(&set);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = set->Put(oldInt);
    NS_ENSURE_SUCCESS(rv, rv);

    return set->Put(aVal);
  }

  
  
  if (aVal < 0) {
    nsresult rv = InitHash(&set);
    NS_ENSURE_SUCCESS(rv, rv);

    return set->Put(aVal);
  }

  
  SetInt(aVal);
  return NS_OK;
}
 
void
nsCheapInt32Set::Remove(PRInt32 aVal)
{
  nsInt32HashSet* set = GetHash();
  if (set) {
    set->Remove(aVal);
  } else if (IsInt() && GetInt() == aVal) {
    mValOrHash = nsnull;
  }
}

nsresult
nsCheapInt32Set::InitHash(nsInt32HashSet** aSet)
{
  nsInt32HashSet* newSet = new nsInt32HashSet();
  if (!newSet) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = newSet->Init(10);
  NS_ENSURE_SUCCESS(rv, rv);

  mValOrHash = newSet;
  *aSet = newSet;
  return NS_OK;
}
