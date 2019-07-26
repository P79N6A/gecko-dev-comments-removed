




#ifndef NS_PARSERNODE__
#define NS_PARSERNODE__

#include "nscore.h"
#include "nsIParserNode.h"
#include "nsHTMLTags.h"

class nsParserNode : public nsIParserNode
{
  public:
    


    nsParserNode(eHTMLTags aTag);

    


    virtual ~nsParserNode();

    



    virtual int32_t GetNodeType()  const;

    



    virtual int32_t GetTokenType()  const;


    
    
    

    



    virtual int32_t GetAttributeCount() const;

    




    virtual const nsAString& GetKeyAt(uint32_t anIndex) const;

    





    virtual const nsAString& GetValueAt(uint32_t anIndex) const;

  private:
    eHTMLTags mTag;
};

#endif
