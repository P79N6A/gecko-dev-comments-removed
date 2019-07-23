




































#ifndef nsINameSpaceManager_h___
#define nsINameSpaceManager_h___

#include "nsISupports.h"
#include "nsStringGlue.h"

class nsIAtom;
class nsString;

#define kNameSpaceID_Unknown -1


static const PRInt32 kNameSpaceID_None = 0;
#define kNameSpaceID_XMLNS    1 // not really a namespace, but it needs to play the game
#define kNameSpaceID_XML      2
#define kNameSpaceID_XHTML    3
#define kNameSpaceID_XLink    4
#define kNameSpaceID_XSLT     5
#define kNameSpaceID_XBL      6
#define kNameSpaceID_MathML   7
#define kNameSpaceID_RDF      8
#define kNameSpaceID_XUL      9
#define kNameSpaceID_SVG      10
#define kNameSpaceID_XMLEvents 11
#define kNameSpaceID_XHTML2_Unofficial    12
#define kNameSpaceID_WAIRoles  13
#define kNameSpaceID_WAIProperties 14
#define kNameSpaceID_LastBuiltin          14 // last 'built-in' namespace

#define NS_NAMESPACEMANAGER_CONTRACTID "@mozilla.org/content/namespacemanager;1"

#define NS_INAMESPACEMANAGER_IID \
  { 0xd74e83e6, 0xf932, 0x4289, \
    { 0xac, 0x95, 0x9e, 0x10, 0x24, 0x30, 0x88, 0xd6 } }















class nsINameSpaceManager : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INAMESPACEMANAGER_IID)

  virtual nsresult RegisterNameSpace(const nsAString& aURI,
                                     PRInt32& aNameSpaceID) = 0;

  virtual nsresult GetNameSpaceURI(PRInt32 aNameSpaceID, nsAString& aURI) = 0;
  virtual PRInt32 GetNameSpaceID(const nsAString& aURI) = 0;

  virtual PRBool HasElementCreator(PRInt32 aNameSpaceID) = 0;
};
 
NS_DEFINE_STATIC_IID_ACCESSOR(nsINameSpaceManager, NS_INAMESPACEMANAGER_IID)

nsresult NS_GetNameSpaceManager(nsINameSpaceManager** aInstancePtrResult);

void NS_NameSpaceManagerShutdown();

#endif 
