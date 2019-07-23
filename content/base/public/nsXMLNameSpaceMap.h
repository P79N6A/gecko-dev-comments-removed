





































#ifndef nsXMLNameSpaceMap_h_
#define nsXMLNameSpaceMap_h_

#include "nsString.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"

struct nsNameSpaceEntry
{
  nsNameSpaceEntry(nsIAtom *aPrefix)
    : prefix(aPrefix) {}

  nsCOMPtr<nsIAtom> prefix;
  PRInt32 nameSpaceID;
};





class nsXMLNameSpaceMap
{
public:
  



  static NS_HIDDEN_(nsXMLNameSpaceMap*) Create(PRBool aForXML);

  




  NS_HIDDEN_(nsresult) AddPrefix(nsIAtom *aPrefix, PRInt32 aNameSpaceID);

  



  NS_HIDDEN_(nsresult) AddPrefix(nsIAtom *aPrefix, nsString &aURI);

  
  NS_HIDDEN_(void) RemovePrefix(nsIAtom *aPrefix);

  





  NS_HIDDEN_(PRInt32) FindNameSpaceID(nsIAtom *aPrefix) const;

  



  NS_HIDDEN_(nsIAtom*) FindPrefix(PRInt32 aNameSpaceID) const;

  
  NS_HIDDEN_(void) Clear();

  ~nsXMLNameSpaceMap() { Clear(); }

private:
  nsXMLNameSpaceMap() NS_HIDDEN;  

  nsTArray<nsNameSpaceEntry> mNameSpaces;
};

#endif
