




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
  



  static nsXMLNameSpaceMap* Create(bool aForXML);

  




  nsresult AddPrefix(nsIAtom *aPrefix, int32_t aNameSpaceID);

  



  nsresult AddPrefix(nsIAtom *aPrefix, nsString &aURI);

  





  int32_t FindNameSpaceID(nsIAtom *aPrefix) const;

  



  nsIAtom* FindPrefix(int32_t aNameSpaceID) const;

  
  void Clear();

  ~nsXMLNameSpaceMap() { Clear(); }

private:
  nsXMLNameSpaceMap();  

  nsTArray<nsNameSpaceEntry> mNameSpaces;
};

#endif
