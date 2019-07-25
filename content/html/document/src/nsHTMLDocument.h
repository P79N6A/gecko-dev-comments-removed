




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
class nsIDocShell;
class nsICachingChannel;

class nsHTMLDocument : public nsDocument,
                       public nsIHTMLDocument,
                       public nsIDOMHTMLDocument
{
public:
  using nsDocument::SetDocumentURI;
  using nsDocument::GetPlugins;

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
                                     bool aReset = true,
                                     nsIContentSink* aSink = nullptr);
  virtual void StopDocumentLoad();

  virtual void BeginLoad();

  virtual void EndLoad();

  virtual void SetCompatibilityMode(nsCompatibility aMode);

  virtual bool IsWriting()
  {
    return mWriteLevel != PRUint32(0);
  }

  virtual NS_HIDDEN_(nsContentList*) GetForms();
 
  virtual NS_HIDDEN_(nsContentList*) GetFormControls();
 
  
  NS_FORWARD_NSIDOMDOCUMENT(nsDocument::)

  
  NS_FORWARD_NSIDOMNODE(nsDocument::)

  
  NS_DECL_NSIDOMHTMLDOCUMENT

  




  nsISupports *GetDocumentAllResult(const nsAString& aID,
                                    nsWrapperCache **aCache,
                                    nsresult *aResult);

  nsIContent *GetBody();
  Element *GetHead() { return GetHeadElement(); }
  already_AddRefed<nsContentList> GetElementsByName(const nsAString & aName)
  {
    return NS_GetFuncStringContentList(this, MatchNameAttribute, nullptr,
                                       UseExistingNameString, aName);
  }

  virtual nsresult ResolveName(const nsAString& aName,
                               nsIContent *aForm,
                               nsISupports **aResult,
                               nsWrapperCache **aCache);

  virtual void AddedForm();
  virtual void RemovedForm();
  virtual PRInt32 GetNumFormsSynchronous();
  virtual void TearingDownEditor(nsIEditor *aEditor);
  virtual void SetIsXHTML(bool aXHTML) { mIsRegularHTML = !aXHTML; }
  virtual void SetDocWriteDisabled(bool aDisabled)
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
    mDisableCookieAccess = true;
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

  virtual nsresult SetEditingState(EditingState aState);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual NS_HIDDEN_(void) RemovedFromDocShell();

  virtual mozilla::dom::Element *GetElementById(const nsAString& aElementId)
  {
    return nsDocument::GetElementById(aElementId);
  }

  virtual nsXPCClassInfo* GetClassInfo();

  virtual void DocSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const;
  

protected:
  nsresult GetBodySize(PRInt32* aWidth,
                       PRInt32* aHeight);

  nsIContent *MatchId(nsIContent *aContent, const nsAString& aId);

  static bool MatchLinks(nsIContent *aContent, PRInt32 aNamespaceID,
                           nsIAtom* aAtom, void* aData);
  static bool MatchAnchors(nsIContent *aContent, PRInt32 aNamespaceID,
                             nsIAtom* aAtom, void* aData);
  static bool MatchNameAttribute(nsIContent* aContent, PRInt32 aNamespaceID,
                                   nsIAtom* aAtom, void* aData);
  static void* UseExistingNameString(nsINode* aRootNode, const nsString* aName);

  static void DocumentWriteTerminationFunc(nsISupports *aRef);

  void GetDomainURI(nsIURI **uri);

  nsresult WriteCommon(JSContext *cx, const nsAString& aText,
                       bool aNewlineTerminate);

  nsresult CreateAndAddWyciwygChannel(void);
  nsresult RemoveWyciwygChannel(void);

  


  bool IsEditingOnAfterFlush();

  void *GenerateParserKey(void);

  nsCOMPtr<nsIDOMHTMLCollection> mImages;
  nsCOMPtr<nsIDOMHTMLCollection> mApplets;
  nsCOMPtr<nsIDOMHTMLCollection> mEmbeds;
  nsCOMPtr<nsIDOMHTMLCollection> mLinks;
  nsCOMPtr<nsIDOMHTMLCollection> mAnchors;
  nsCOMPtr<nsIDOMHTMLCollection> mScripts;
  nsRefPtr<nsContentList> mForms;
  nsRefPtr<nsContentList> mFormControls;

  
  PRInt32 mNumForms;

  static PRUint32 gWyciwygSessionCnt;

  static bool TryHintCharset(nsIMarkupDocumentViewer* aMarkupDV,
                               PRInt32& aCharsetSource,
                               nsACString& aCharset);
  static bool TryUserForcedCharset(nsIMarkupDocumentViewer* aMarkupDV,
                                     nsIDocShell*  aDocShell,
                                     PRInt32& aCharsetSource,
                                     nsACString& aCharset);
  static bool TryCacheCharset(nsICachingChannel* aCachingChannel,
                                PRInt32& aCharsetSource,
                                nsACString& aCharset);
  
  bool TryParentCharset(nsIDocShell*  aDocShell,
                          nsIDocument* aParentDocument,
                          PRInt32& charsetSource, nsACString& aCharset);
  static bool UseWeakDocTypeDefault(PRInt32& aCharsetSource,
                                      nsACString& aCharset);
  static bool TryDefaultCharset(nsIMarkupDocumentViewer* aMarkupDV,
                                  PRInt32& aCharsetSource,
                                  nsACString& aCharset);

  
  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID);

  
  
  
  
  PRUint32 mWriteLevel;

  
  PRUint32 mLoadFlags;

  bool mTooDeepWriteRecursion;

  bool mDisableDocWrite;

  bool mWarnedWidthHeight;

  nsCOMPtr<nsIWyciwygChannel> mWyciwygChannel;

  
  nsresult   GetMidasCommandManager(nsICommandManager** aCommandManager);

  nsCOMPtr<nsICommandManager> mMidasCommandManager;

  nsresult TurnEditingOff();
  nsresult EditingStateChanged();
  void MaybeEditingStateChanged();

  PRUint32 mContentEditableCount;
  EditingState mEditingState;

  nsresult   DoClipboardSecurityCheck(bool aPaste);
  static jsid        sCutCopyInternal_id;
  static jsid        sPasteInternal_id;

  
  bool mDisableCookieAccess;
};

#define NS_HTML_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                        \
    NS_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                                 \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIHTMLDocument)                         \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIDOMHTMLDocument)

#endif 
