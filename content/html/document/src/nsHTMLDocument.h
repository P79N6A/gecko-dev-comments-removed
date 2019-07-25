




































#ifndef nsHTMLDocument_h___
#define nsHTMLDocument_h___

#include "nsDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLBodyElement.h"
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
class nsICachingChannel;

class nsHTMLDocument : public nsDocument,
                       public nsIHTMLDocument,
                       public nsIDOMHTMLDocument
{
public:
  using nsDocument::SetDocumentURI;

  nsHTMLDocument();
  virtual nsresult Init();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  virtual void Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup);
  virtual void ResetToURI(nsIURI* aURI, nsILoadGroup* aLoadGroup,
                          nsIPrincipal* aPrincipal);

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

  virtual void BeginLoad();

  virtual void EndLoad();

  virtual void SetCompatibilityMode(nsCompatibility aMode);

  virtual PRBool IsWriting()
  {
    return mWriteLevel != PRUint32(0);
  }

  virtual PRBool GetIsFrameset() { return mIsFrameset; }
  virtual void SetIsFrameset(PRBool aFrameset) { mIsFrameset = aFrameset; }

  virtual NS_HIDDEN_(nsContentList*) GetForms();
 
  virtual NS_HIDDEN_(nsContentList*) GetFormControls();
 
  
  NS_FORWARD_NSIDOMDOCUMENT(nsDocument::)

  
  NS_FORWARD_NSIDOMNODE(nsDocument::)

  
  NS_DECL_NSIDOMHTMLDOCUMENT

  




  nsISupports *GetDocumentAllResult(const nsAString& aID,
                                    nsWrapperCache **aCache,
                                    nsresult *aResult);

  nsIContent *GetBody(nsresult *aResult);
  already_AddRefed<nsContentList> GetElementsByName(const nsAString & aName)
  {
    return NS_GetFuncStringContentList(this, MatchNameAttribute, nsnull,
                                       UseExistingNameString, aName);
  }


  virtual nsresult ResolveName(const nsAString& aName,
                               nsIContent *aForm,
                               nsISupports **aResult,
                               nsWrapperCache **aCache);

  virtual void ScriptLoading(nsIScriptElement *aScript);
  virtual void ScriptExecuted(nsIScriptElement *aScript);

  virtual void AddedForm();
  virtual void RemovedForm();
  virtual PRInt32 GetNumFormsSynchronous();
  virtual void TearingDownEditor(nsIEditor *aEditor);
  virtual void SetIsXHTML(PRBool aXHTML) { mIsRegularHTML = !aXHTML; }
  virtual void SetDocWriteDisabled(PRBool aDisabled)
  {
    mDisableDocWrite = aDisabled;
  }

  nsresult ChangeContentEditableCount(nsIContent *aElement, PRInt32 aChange);
  void DeferredContentEditableCountChange(nsIContent *aElement);

  virtual EditingState GetEditingState()
  {
    return mEditingState;
  }

  virtual void DisableCookieAccess()
  {
    mDisableCookieAccess = PR_TRUE;
  }

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

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLDocument, nsDocument)

  virtual already_AddRefed<nsIParser> GetFragmentParser() {
    return mFragmentParser.forget();
  }
  virtual void SetFragmentParser(nsIParser* aParser) {
    mFragmentParser = aParser;
  }

  virtual nsresult SetEditingState(EditingState aState);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual NS_HIDDEN_(void) RemovedFromDocShell();

  virtual mozilla::dom::Element *GetElementById(const nsAString& aElementId)
  {
    return nsDocument::GetElementById(aElementId);
  }

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  nsresult GetBodySize(PRInt32* aWidth,
                       PRInt32* aHeight);

  nsIContent *MatchId(nsIContent *aContent, const nsAString& aId);

  static PRBool MatchLinks(nsIContent *aContent, PRInt32 aNamespaceID,
                           nsIAtom* aAtom, void* aData);
  static PRBool MatchAnchors(nsIContent *aContent, PRInt32 aNamespaceID,
                             nsIAtom* aAtom, void* aData);
  static PRBool MatchNameAttribute(nsIContent* aContent, PRInt32 aNamespaceID,
                                   nsIAtom* aAtom, void* aData);
  static void* UseExistingNameString(nsINode* aRootNode, const nsString* aName);

  static void DocumentWriteTerminationFunc(nsISupports *aRef);

  void GetDomainURI(nsIURI **uri);

  nsresult WriteCommon(JSContext *cx, const nsAString& aText,
                       PRBool aNewlineTerminate);
  nsresult OpenCommon(JSContext *cx, const nsAString& aContentType,
                      PRBool aReplace);

  nsresult CreateAndAddWyciwygChannel(void);
  nsresult RemoveWyciwygChannel(void);

  


  PRBool IsEditingOnAfterFlush();

  void *GenerateParserKey(void);

  virtual PRInt32 GetDefaultNamespaceID() const
  {
    return kNameSpaceID_XHTML;
  }

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
  static PRBool TryCacheCharset(nsICachingChannel* aCachingChannel,
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

  PRPackedBool mDisableDocWrite;

  PRPackedBool mWarnedWidthHeight;

  nsCOMPtr<nsIWyciwygChannel> mWyciwygChannel;

  
  nsresult   GetMidasCommandManager(nsICommandManager** aCommandManager);

  nsCOMPtr<nsICommandManager> mMidasCommandManager;

  nsresult TurnEditingOff();
  nsresult EditingStateChanged();
  void MaybeEditingStateChanged();

  PRUint32 mContentEditableCount;
  EditingState mEditingState;

  nsresult   DoClipboardSecurityCheck(PRBool aPaste);
  static jsid        sCutCopyInternal_id;
  static jsid        sPasteInternal_id;

  
  PRBool mDisableCookieAccess;

  
  nsCOMPtr<nsIParser> mFragmentParser;
};

#define NS_HTML_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                        \
    NS_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                                 \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIHTMLDocument)                         \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIDOMHTMLDocument)

#endif 
