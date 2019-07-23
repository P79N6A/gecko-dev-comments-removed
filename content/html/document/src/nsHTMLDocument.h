




































#ifndef nsHTMLDocument_h___
#define nsHTMLDocument_h___

#include "nsDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIScriptElement.h"
#include "jsapi.h"
#include "nsTArray.h"

#include "pldhash.h"
#include "nsIHttpChannel.h"
#include "nsHTMLStyleSheet.h"


#include "nsIWyciwygChannel.h"
#include "nsILoadGroup.h"
#include "nsNetUtil.h"

#include "nsICommandManager.h"

class nsIEditor;
class nsIEditorDocShell;
class nsIParser;
class nsIURI;
class nsIMarkupDocumentViewer;
class nsIDocumentCharsetInfo;
class nsICacheEntryDescriptor;

class nsHTMLDocument : public nsDocument,
                       public nsIHTMLDocument,
                       public nsIDOMHTMLDocument,
                       public nsIDOMNSHTMLDocument
{
public:
  nsHTMLDocument();
  virtual nsresult Init();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  virtual void Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup);
  virtual void ResetToURI(nsIURI* aURI, nsILoadGroup* aLoadGroup,
                          nsIPrincipal* aPrincipal);
  virtual nsStyleSet::sheetType GetAttrSheetType();

  virtual nsresult CreateShell(nsPresContext* aContext,
                               nsIViewManager* aViewManager,
                               nsStyleSet* aStyleSet,
                               nsIPresShell** aInstancePtrResult);

  virtual nsresult StartDocumentLoad(const char* aCommand,
                                     nsIChannel* aChannel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     PRBool aReset = PR_TRUE,
                                     nsIContentSink* aSink = nsnull);
  virtual void StopDocumentLoad();

  virtual void EndLoad();

  virtual nsresult AddImageMap(nsIDOMHTMLMapElement* aMap);

  virtual void RemoveImageMap(nsIDOMHTMLMapElement* aMap);

  virtual nsIDOMHTMLMapElement *GetImageMap(const nsAString& aMapName);

  virtual void SetCompatibilityMode(nsCompatibility aMode);

  virtual PRBool IsWriting()
  {
    return mWriteLevel != PRUint32(0);
  }

  virtual PRBool GetIsFrameset() { return mIsFrameset; }
  virtual void SetIsFrameset(PRBool aFrameset) { mIsFrameset = aFrameset; }

  virtual NS_HIDDEN_(nsContentList*) GetForms();
 
  virtual NS_HIDDEN_(nsContentList*) GetFormControls();
 
  
  NS_DECL_NSIDOMDOCUMENT

  
  NS_IMETHOD GetXmlEncoding(nsAString& aXmlVersion);
  NS_IMETHOD GetXmlStandalone(PRBool *aXmlStandalone);
  NS_IMETHOD SetXmlStandalone(PRBool aXmlStandalone);
  NS_IMETHOD GetXmlVersion(nsAString& aXmlVersion);
  NS_IMETHOD SetXmlVersion(const nsAString& aXmlVersion);

  
  NS_FORWARD_NSIDOMNODE(nsDocument::)

  
  NS_IMETHOD GetBaseURI(nsAString& aBaseURI);

  
  NS_IMETHOD GetTitle(nsAString & aTitle);
  NS_IMETHOD SetTitle(const nsAString & aTitle);
  NS_IMETHOD GetReferrer(nsAString & aReferrer);
  NS_IMETHOD GetURL(nsAString & aURL);
  NS_IMETHOD GetBody(nsIDOMHTMLElement * *aBody);
  NS_IMETHOD SetBody(nsIDOMHTMLElement * aBody);
  NS_IMETHOD GetImages(nsIDOMHTMLCollection * *aImages);
  NS_IMETHOD GetApplets(nsIDOMHTMLCollection * *aApplets);
  NS_IMETHOD GetLinks(nsIDOMHTMLCollection * *aLinks);
  NS_IMETHOD GetForms(nsIDOMHTMLCollection * *aForms);
  NS_IMETHOD GetAnchors(nsIDOMHTMLCollection * *aAnchors);
  NS_IMETHOD GetCookie(nsAString & aCookie);
  NS_IMETHOD SetCookie(const nsAString & aCookie);
  NS_IMETHOD Open(void);
  NS_IMETHOD Close(void);
  NS_IMETHOD Write(const nsAString & text);
  NS_IMETHOD Writeln(const nsAString & text);
  NS_IMETHOD GetElementsByName(const nsAString & elementName,
                               nsIDOMNodeList **_retval);
  virtual nsresult GetDocumentAllResult(const nsAString& aID,
                                        nsISupports** aResult);

  
  NS_DECL_NSIDOMNSHTMLDOCUMENT

  virtual nsresult ResolveName(const nsAString& aName,
                               nsIDOMHTMLFormElement *aForm,
                               nsISupports **aResult);

  virtual void ScriptLoading(nsIScriptElement *aScript);
  virtual void ScriptExecuted(nsIScriptElement *aScript);

  virtual void AddedForm();
  virtual void RemovedForm();
  virtual PRInt32 GetNumFormsSynchronous();
  virtual void TearingDownEditor(nsIEditor *aEditor);
  virtual void SetIsXHTML(PRBool aXHTML) { mIsRegularHTML = !aXHTML; }

  nsresult ChangeContentEditableCount(nsIContent *aElement, PRInt32 aChange);

  virtual EditingState GetEditingState()
  {
    return mEditingState;
  }

  virtual void DisableCookieAccess()
  {
    mDisableCookieAccess = PR_TRUE;
  }

  virtual nsIContent* GetBodyContentExternal();
  
  class nsAutoEditingState {
  public:
    nsAutoEditingState(nsHTMLDocument* aDoc, EditingState aState)
      : mDoc(aDoc), mSavedState(aDoc->mEditingState)
    {
      aDoc->mEditingState = aState;
    }
    ~nsAutoEditingState() {
      mDoc->mEditingState = mSavedState;
    }
  private:
    nsHTMLDocument* mDoc;
    EditingState    mSavedState;
  };
  friend class nsAutoEditingState;

  void EndUpdate(nsUpdateType aUpdateType);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLDocument, nsDocument)

  virtual already_AddRefed<nsIParser> GetFragmentParser() {
    return mFragmentParser.forget();
  }
  virtual void SetFragmentParser(nsIParser* aParser) {
    mFragmentParser = aParser;
  }

  virtual nsresult SetEditingState(EditingState aState);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual NS_HIDDEN_(void) RemovedFromDocShell();

protected:
  nsresult GetBodySize(PRInt32* aWidth,
                       PRInt32* aHeight);

  nsresult PrePopulateIdentifierMap();

  nsIContent *MatchId(nsIContent *aContent, const nsAString& aId);

  static PRBool MatchLinks(nsIContent *aContent, PRInt32 aNamespaceID,
                           nsIAtom* aAtom, void* aData);
  static PRBool MatchAnchors(nsIContent *aContent, PRInt32 aNamespaceID,
                             nsIAtom* aAtom, void* aData);
  static PRBool MatchNameAttribute(nsIContent* aContent, PRInt32 aNamespaceID,
                                   nsIAtom* aAtom, void* aData);

  static void DocumentWriteTerminationFunc(nsISupports *aRef);

  void GetDomainURI(nsIURI **uri);

  nsresult WriteCommon(const nsAString& aText,
                       PRBool aNewlineTerminate);
  nsresult OpenCommon(const nsACString& aContentType, PRBool aReplace);

  nsresult CreateAndAddWyciwygChannel(void);
  nsresult RemoveWyciwygChannel(void);

  


  PRBool IsEditingOnAfterFlush();

  void *GenerateParserKey(void);

  virtual PRInt32 GetDefaultNamespaceID() const
  {
    return kNameSpaceID_XHTML;
  }

  nsCOMArray<nsIDOMHTMLMapElement> mImageMaps;

  nsCOMPtr<nsIDOMHTMLCollection> mImages;
  nsCOMPtr<nsIDOMHTMLCollection> mApplets;
  nsCOMPtr<nsIDOMHTMLCollection> mEmbeds;
  nsCOMPtr<nsIDOMHTMLCollection> mLinks;
  nsCOMPtr<nsIDOMHTMLCollection> mAnchors;
  nsRefPtr<nsContentList> mForms;
  nsRefPtr<nsContentList> mFormControls;

  
  PRInt32 mNumForms;

  static PRUint32 gWyciwygSessionCnt;

  static PRBool TryHintCharset(nsIMarkupDocumentViewer* aMarkupDV,
                               PRInt32& aCharsetSource,
                               nsACString& aCharset);
  static PRBool TryUserForcedCharset(nsIMarkupDocumentViewer* aMarkupDV,
                                     nsIDocumentCharsetInfo*  aDocInfo,
                                     PRInt32& aCharsetSource,
                                     nsACString& aCharset);
  static PRBool TryCacheCharset(nsICacheEntryDescriptor* aCacheDescriptor,
                                PRInt32& aCharsetSource,
                                nsACString& aCharset);
  static PRBool TryBookmarkCharset(nsIDocShell* aDocShell,
                                   nsIChannel* aChannel,
                                   PRInt32& aCharsetSource,
                                   nsACString& aCharset);
  
  PRBool TryParentCharset(nsIDocumentCharsetInfo*  aDocInfo,
                          nsIDocument* aParentDocument,
                          PRInt32& charsetSource, nsACString& aCharset);
  static PRBool UseWeakDocTypeDefault(PRInt32& aCharsetSource,
                                      nsACString& aCharset);
  static PRBool TryDefaultCharset(nsIMarkupDocumentViewer* aMarkupDV,
                                  PRInt32& aCharsetSource,
                                  nsACString& aCharset);

  void StartAutodetection(nsIDocShell *aDocShell, nsACString& aCharset,
                          const char* aCommand);

  
  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID);

  
  
  
  
  
  
  
  
  enum {
    eNotWriting,
    eDocumentOpened,
    ePendingClose,
    eDocumentClosed
  } mWriteState;

  
  
  
  
  PRUint32 mWriteLevel;

  nsAutoTArray<nsIScriptElement*, 1> mPendingScripts;

  
  PRUint32 mLoadFlags;

  PRPackedBool mIsFrameset;

  PRPackedBool mTooDeepWriteRecursion;

  nsCOMPtr<nsIWyciwygChannel> mWyciwygChannel;

  
  nsresult   GetMidasCommandManager(nsICommandManager** aCommandManager);

  nsCOMPtr<nsICommandManager> mMidasCommandManager;

  nsresult TurnEditingOff();
  nsresult EditingStateChanged();
  void MaybeEditingStateChanged();

  PRUint32 mContentEditableCount;
  EditingState mEditingState;

  nsresult   DoClipboardSecurityCheck(PRBool aPaste);
  static jsval       sCutCopyInternal_id;
  static jsval       sPasteInternal_id;

  
  PRBool mDisableCookieAccess;

  
  nsCOMPtr<nsIParser> mFragmentParser;
};

#define NS_HTML_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                        \
    NS_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                                 \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIHTMLDocument)                         \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIDOMHTMLDocument)                      \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIDOMNSHTMLDocument)

#endif 
