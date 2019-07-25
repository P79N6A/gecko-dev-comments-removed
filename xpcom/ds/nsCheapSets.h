




































#ifndef __nsCheapSets_h__
#define __nsCheapSets_h__

#include "nsHashSets.h"





class nsCheapStringSet {
public:
  nsCheapStringSet() : mValOrHash(nsnull)
  {
  }
  ~nsCheapStringSet();

  



  nsresult Put(const nsAString& aVal);

  



  void Remove(const nsAString& aVal);

  




  bool Contains(const nsAString& aVal)
  {
    nsStringHashSet* set = GetHash();
    
    if (set) {
      return set->Contains(aVal);
    }

    
    nsAString* str = GetStr();
    return str && str->Equals(aVal);
  }

private:
  typedef PRUword PtrBits;

  
  nsStringHashSet* GetHash()
  {
    return (PtrBits(mValOrHash) & 0x1) ? nsnull : (nsStringHashSet*)mValOrHash;
  }
  
  nsAString* GetStr()
  {
    return (PtrBits(mValOrHash) & 0x1)
           ? (nsAString*)(PtrBits(mValOrHash) & ~0x1)
           : nsnull;
  }
  
  nsresult SetStr(const nsAString& aVal)
  {
    nsString* str = new nsString(aVal);
    if (!str) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mValOrHash = (nsAString*)(PtrBits(str) | 0x1);
    return NS_OK;
  }
  
  nsresult InitHash(nsStringHashSet** aSet);

private:
  
  void* mValOrHash;
};






class nsCheapInt32Set {
public:
  nsCheapInt32Set() : mValOrHash(nsnull)
  {
  }
  ~nsCheapInt32Set();

  


  nsresult Put(PRInt32 aVal);
 
  



  void Remove(PRInt32 aVal);

  




  bool Contains(PRInt32 aVal)
  {
    nsInt32HashSet* set = GetHash();
    if (set) {
      return set->Contains(aVal);
    }
    if (IsInt()) {
      return GetInt() == aVal;
    }
    return PR_FALSE;
  }

private:
  typedef PRUword PtrBits;

  
  nsInt32HashSet* GetHash()
  {
    return PtrBits(mValOrHash) & 0x1 ? nsnull : (nsInt32HashSet*)mValOrHash;
  }
  
  bool IsInt()
  {
    return !!(PtrBits(mValOrHash) & 0x1);
  }
  
  PRInt32 GetInt()
  {
    return PtrBits(mValOrHash) >> 1;
  }
  
  void SetInt(PRInt32 aInt)
  {
    mValOrHash = (void*)((aInt << 1) | 0x1);
  }
  
  nsresult InitHash(nsInt32HashSet** aSet);

private:
  
  void* mValOrHash;
};







#endif
