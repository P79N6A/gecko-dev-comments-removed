




































#ifndef nsIHTMLEditRules_h__
#define nsIHTMLEditRules_h__

#define NS_IHTMLEDITRULES_IID \
{ /* a6cf9121-15b3-11d2-932e-00805f8add32 */ \
0xa6cf9121, 0x15b3, 0x11d2, \
{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }

#include "nsIHTMLEditor.h"

class nsIHTMLEditRules : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLEDITRULES_IID)
  
  NS_IMETHOD GetListState(PRBool *aMixed, PRBool *aOL, PRBool *aUL, PRBool *aDL)=0;
  NS_IMETHOD GetListItemState(PRBool *aMixed, PRBool *aLI, PRBool *aDT, PRBool *aDD)=0;
  NS_IMETHOD GetIndentState(PRBool *aCanIndent, PRBool *aCanOutdent)=0;
  NS_IMETHOD GetAlignment(PRBool *aMixed, nsIHTMLEditor::EAlignment *aAlign)=0;
  NS_IMETHOD GetParagraphState(PRBool *aMixed, nsAString &outFormat)=0;
  NS_IMETHOD MakeSureElemStartsOrEndsOnCR(nsIDOMNode *aNode)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLEditRules, NS_IHTMLEDITRULES_IID)

#endif 
