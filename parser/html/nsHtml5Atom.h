



#ifndef nsHtml5Atom_h
#define nsHtml5Atom_h

#include "nsIAtom.h"
#include "mozilla/Attributes.h"








class nsHtml5Atom MOZ_FINAL : public nsIAtom
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIATOM

    nsHtml5Atom(const nsAString& aString);
    ~nsHtml5Atom();
};

#endif 
