




































#ifndef nsHtml5AtomTable_h_
#define nsHtml5AtomTable_h_

#include "nsHashKeys.h"
#include "nsTHashtable.h"
#include "nsAutoPtr.h"
#include "nsIAtom.h"
#include "nsIThread.h"

class nsHtml5Atom;

class nsHtml5AtomEntry : public nsStringHashKey
{
  public:
    nsHtml5AtomEntry(KeyTypePointer aStr);
    nsHtml5AtomEntry(const nsHtml5AtomEntry& aOther);
    ~nsHtml5AtomEntry();
    inline nsHtml5Atom* GetAtom() {
      return mAtom;
    }
  private:
    nsAutoPtr<nsHtml5Atom> mAtom;
};













































class nsHtml5AtomTable
{
  public:
    nsHtml5AtomTable();
    ~nsHtml5AtomTable();
    
    



    inline PRBool Init() {
      return mTable.Init();
    }
    
    


    nsIAtom* GetAtom(const nsAString& aKey);
    
    


    void Clear() {
      mTable.Clear();
    }
    
#ifdef DEBUG
    void SetPermittedLookupThread(nsIThread* aThread) {
      mPermittedLookupThread = aThread;
    }
#endif  
  
  private:
    nsTHashtable<nsHtml5AtomEntry> mTable;
#ifdef DEBUG
    nsCOMPtr<nsIThread>            mPermittedLookupThread;
#endif
};

#endif 
