












































#ifndef mozSanitizingSerializer_h__
#define mozSanitizingSerializer_h__

#include "mozISanitizingSerializer.h"
#include "nsIContentSerializer.h"
#include "nsIHTMLContentSink.h"
#include "nsHTMLTags.h"
#include "nsCOMPtr.h"
#include "nsIParserService.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsIParser.h"
#include "nsHashtable.h"

class mozSanitizingHTMLSerializer : public nsIContentSerializer,
                                    public nsIHTMLContentSink,
                                    public mozISanitizingHTMLSerializer
{
public:
  mozSanitizingHTMLSerializer();
  virtual ~mozSanitizingHTMLSerializer();
  static PRBool ReleaseProperties(nsHashKey* key, void* data, void* closure);

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD Init(PRUint32 flags, PRUint32 dummy, const char* aCharSet, 
                  PRBool aIsCopying, PRBool aIsWholeDocument);

  NS_IMETHOD AppendText(nsIContent* aText, PRInt32 aStartOffset,
                        PRInt32 aEndOffset, nsAString& aStr);
  NS_IMETHOD AppendCDATASection(nsIContent* aCDATASection,
                                PRInt32 aStartOffset, PRInt32 aEndOffset,
                                nsAString& aStr)
                      { return NS_OK; }
  NS_IMETHOD AppendProcessingInstruction(nsIContent* aPI,
                                         PRInt32 aStartOffset,
                                         PRInt32 aEndOffset,
                                         nsAString& aStr)
                      { return NS_OK; }
  NS_IMETHOD AppendComment(nsIContent* aComment, PRInt32 aStartOffset,
                           PRInt32 aEndOffset, nsAString& aStr)
                      { return NS_OK; }
  NS_IMETHOD AppendDoctype(nsIContent *aDoctype, nsAString& aStr)
                      { return NS_OK; }
  NS_IMETHOD AppendElementStart(mozilla::dom::Element* aElement,
                                mozilla::dom::Element* aOriginalElement,
                                nsAString& aStr);
  NS_IMETHOD AppendElementEnd(mozilla::dom::Element* aElement,
                              nsAString& aStr);
  NS_IMETHOD Flush(nsAString& aStr);

  NS_IMETHOD AppendDocumentStart(nsIDocument *aDocument,
                                 nsAString& aStr);

  
  NS_IMETHOD WillParse(void) { return NS_OK; }
  NS_IMETHOD WillInterrupt(void) { return NS_OK; }
  NS_IMETHOD WillResume(void) { return NS_OK; }
  NS_IMETHOD SetParser(nsIParser* aParser) { return NS_OK; }
  NS_IMETHOD OpenContainer(const nsIParserNode& aNode);
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag);
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode);
  NS_IMETHOD AddComment(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode)
                                                    { return NS_OK; }
  NS_IMETHOD AddDocTypeDecl(const nsIParserNode& aNode);
  virtual void FlushPendingNotifications(mozFlushType aType) { }
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset);
  virtual nsISupports *GetTarget() { return nsnull; }

  
  NS_IMETHOD OpenHead();
  NS_IMETHOD IsEnabled(PRInt32 aTag, PRBool* aReturn);
  NS_IMETHOD NotifyTagObservers(nsIParserNode* aNode) { return NS_OK; }
  NS_IMETHOD_(PRBool) IsFormOnStack() { return PR_FALSE; }
  NS_IMETHOD BeginContext(PRInt32 aPosition) { return NS_OK; }
  NS_IMETHOD EndContext(PRInt32 aPosition) { return NS_OK; }
  NS_IMETHOD DidProcessTokens(void) { return NS_OK; }
  NS_IMETHOD WillProcessAToken(void) { return NS_OK; }
  NS_IMETHOD DidProcessAToken(void) { return NS_OK; }

  
  NS_IMETHOD Initialize(nsAString* aOutString,
                        PRUint32 aFlags, const nsAString& allowedTags);

protected:
  nsresult ParsePrefs(const nsAString& aPref);
  nsresult ParseTagPref(const nsCAutoString& tagpref);
  PRBool IsAllowedTag(nsHTMLTag aTag);
  PRBool IsAllowedAttribute(nsHTMLTag aTag, const nsAString& anAttributeName);
  nsresult SanitizeAttrValue(nsHTMLTag aTag, const nsAString& attr_name,
                             nsString& value );
  nsresult SanitizeTextNode(nsString& value );
  PRBool IsContainer(PRInt32 aId);
  static PRInt32 GetIdForContent(nsIContent* aContent);
  nsresult GetParserService(nsIParserService** aParserService);
  nsresult DoOpenContainer(PRInt32 aTag);
  nsresult DoCloseContainer(PRInt32 aTag);
  nsresult DoAddLeaf(PRInt32 aTag, const nsAString& aText);
  void Write(const nsAString& aString);

protected:
  PRInt32                      mFlags;
  PRUint32                     mSkipLevel;
  nsHashtable                  mAllowedTags;

  nsRefPtr<mozilla::dom::Element> mContent;
  nsAString*                   mOutputString;
  nsIParserNode*               mParserNode;
  nsCOMPtr<nsIParserService>   mParserService;
};

nsresult
NS_NewSanitizingHTMLSerializer(nsIContentSerializer** aSerializer);

#endif
