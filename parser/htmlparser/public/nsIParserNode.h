





















































#ifndef NS_IPARSERNODE__
#define NS_IPARSERNODE__

#include "nsISupports.h"
#include "prtypes.h"
#include "nsStringGlue.h"
#include "nsDebug.h"




class nsIAtom;
class CToken;


#define NS_IPARSER_NODE_IID      \
  {0x6e59f160, 0x2717,  0x11d1,  \
  {0x92, 0x46, 0x00,    0x80, 0x5f, 0x8a, 0x7a, 0xb6}}









class nsIParserNode { 
            
  public:


    




    virtual const nsAString& GetTagName() const = 0;  

    




    virtual const nsAString& GetText() const = 0;  

    




    virtual PRInt32 GetNodeType()  const =0;

    




    virtual PRInt32 GetTokenType()  const =0;

    




    virtual PRInt32 GetAttributeCount(PRBool askToken=PR_FALSE) const =0;

    





    virtual const nsAString& GetKeyAt(PRUint32 anIndex) const = 0;

    





    virtual const nsAString& GetValueAt(PRUint32 anIndex) const = 0;

    






    virtual PRInt32 TranslateToUnicodeStr(nsString& aString) const = 0;


    virtual void AddAttribute(CToken* aToken)=0;

    





    virtual PRInt32 GetSourceLineNumber(void) const =0;

    




    virtual PRBool  GetGenericState(void) const =0;
    virtual void    SetGenericState(PRBool aState) =0;

    



    virtual void GetSource(nsString& aString)=0;

    



    virtual nsresult ReleaseAll()=0;
};

#endif
