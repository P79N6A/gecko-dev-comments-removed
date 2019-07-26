





















#ifndef NS_IPARSERNODE__
#define NS_IPARSERNODE__

#include "nsStringGlue.h"

class nsIAtom;









class nsIParserNode {
  public:
    




    virtual int32_t GetNodeType()  const =0;

    




    virtual int32_t GetTokenType()  const =0;

    




    virtual int32_t GetAttributeCount() const =0;

    





    virtual const nsAString& GetKeyAt(uint32_t anIndex) const = 0;

    





    virtual const nsAString& GetValueAt(uint32_t anIndex) const = 0;
};

#endif
