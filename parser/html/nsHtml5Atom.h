




































#ifndef nsHtml5Atom_h_
#define nsHtml5Atom_h_

#include "nsIAtom.h"








class nsHtml5Atom : public nsIAtom
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIATOM

    nsHtml5Atom(const nsAString& aString);
    ~nsHtml5Atom();

  private:
    nsString mData;
};

#endif 
