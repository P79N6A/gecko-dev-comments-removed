




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
  int32_t nameSpaceID;
};





class nsXMLNameSpaceMap
{
public:
  



  static NS_HIDDEN_(nsXMLNameSpaceMap*) Create(bool aForXML);

  




  NS_HIDDEN_(nsresult) AddPrefix(nsIAtom *aPrefix, int32_t aNameSpaceID);

  



  NS_HIDDEN_(nsresult) AddPrefix(nsIAtom *aPrefix, nsString &aURI);

  





  NS_HIDDEN_(int32_t) FindNameSpaceID(nsIAtom *aPrefix) const;

  



  NS_HIDDEN_(nsIAtom*) FindPrefix(int32_t aNameSpaceID) const;

  
  NS_HIDDEN_(void) Clear();

  ~nsXMLNameSpaceMap() { Clear(); }

private:
  nsXMLNameSpaceMap() NS_HIDDEN;  

  nsTArray<nsNameSpaceEntry> mNameSpaces;
};

#endif
