




































#ifndef _nsIContentSerializer_h__
#define _nsIContentSerializer_h__

#include "nsISupports.h"

class nsIDOMText; 
class nsIDOMCDATASection; 
class nsIDOMProcessingInstruction; 
class nsIDOMComment; 
class nsIDOMDocumentType; 
class nsIDOMElement; 
class nsIDOMDocument; 
class nsAString;



#define NS_ICONTENTSERIALIZER_IID \
{ 0x0921afbc, 0x4c6f, 0x4249, \
  { 0xa7, 0xf5, 0x32, 0xe4, 0x91, 0xbf, 0x6e, 0x32 } }

class nsIContentSerializer : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTSERIALIZER_IID)

  NS_IMETHOD Init(PRUint32 flags, PRUint32 aWrapColumn,
                  const char* aCharSet, PRBool aIsCopying) = 0;

  NS_IMETHOD AppendText(nsIDOMText* aText, PRInt32 aStartOffset,
                        PRInt32 aEndOffset, nsAString& aStr) = 0;

  NS_IMETHOD AppendCDATASection(nsIDOMCDATASection* aCDATASection,
                                PRInt32 aStartOffset, PRInt32 aEndOffset,
                                nsAString& aStr) = 0;

  NS_IMETHOD AppendProcessingInstruction(nsIDOMProcessingInstruction* aPI,
                                         PRInt32 aStartOffset,
                                         PRInt32 aEndOffset,
                                         nsAString& aStr) = 0;

  NS_IMETHOD AppendComment(nsIDOMComment* aComment, PRInt32 aStartOffset,
                           PRInt32 aEndOffset, nsAString& aStr) = 0;

  NS_IMETHOD AppendDoctype(nsIDOMDocumentType *aDoctype,
                           nsAString& aStr) = 0;

  NS_IMETHOD AppendElementStart(nsIDOMElement *aElement,
                                nsIDOMElement *aOriginalElement,
                                nsAString& aStr) = 0;

  NS_IMETHOD AppendElementEnd(nsIDOMElement *aElement,
                              nsAString& aStr) = 0;

  NS_IMETHOD Flush(nsAString& aStr) = 0;

  




  NS_IMETHOD AppendDocumentStart(nsIDOMDocument *aDocument,
                                 nsAString& aStr) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentSerializer, NS_ICONTENTSERIALIZER_IID)

#define NS_CONTENTSERIALIZER_CONTRACTID_PREFIX \
"@mozilla.org/layout/contentserializer;1?mimetype="

#endif 
