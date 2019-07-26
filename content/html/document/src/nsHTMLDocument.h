




#ifndef nsHTMLDocument_h___
#define nsHTMLDocument_h___

#include "nsDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLDocument.h"
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
#include "mozilla/dom/HTMLSharedElement.h"
#include "nsDOMEvent.h"

class nsIEditor;
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
                               nsViewManager* aViewManager,
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
    return mWriteLevel != uint32_t(0);
  }

  virtual NS_HIDDEN_(nsContentList*) GetForms();
 
  virtual NS_HIDDEN_(nsContentList*) GetFormControls();
 
  
  NS_FORWARD_NSIDOMDOCUMENT(nsDocument::)

  
  using nsDocument::GetImplementation;
  using nsDocument::GetTitle;
  using nsDocument::SetTitle;
  using nsDocument::GetLastStyleSheetSet;
  using nsDocument::MozSetImageElement;
  using nsDocument::GetMozFullScreenElement;

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_DECL_NSIDOMHTMLDOCUMENT

  void RouteEvent(nsDOMEvent& aEvent)
  {
    RouteEvent(&aEvent);
  }

  




  nsISupports *GetDocumentAllResult(const nsAString& aID,
                                    nsWrapperCache **aCache,
                                    nsresult *aResult);

  nsISupports* ResolveName(const nsAString& aName, nsWrapperCache **aCache);
  virtual already_AddRefed<nsISupports> ResolveName(const nsAString& aName,
                                                    nsIContent *aForm,
                                                    nsWrapperCache **aCache);

  virtual void AddedForm();
  virtual void RemovedForm();
  virtual int32_t GetNumFormsSynchronous();
  virtual void TearingDownEditor(nsIEditor *aEditor);
  virtual void SetIsXHTML(bool aXHTML) { mIsRegularHTML = !aXHTML; }
  virtual void SetDocWriteDisabled(bool aDisabled)
  {
    mDisableDocWrite = aDisabled;
  }

  nsresult ChangeContentEditableCount(nsIContent *aElement, int32_t aChange);
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
  

  virtual bool WillIgnoreCharsetOverride();

  
  void GetDomain(nsAString& aDomain, mozilla::ErrorResult& rv);
  void SetDomain(const nsAString& aDomain, mozilla::ErrorResult& rv);
  void GetCookie(nsAString& aCookie, mozilla::ErrorResult& rv);
  void SetCookie(const nsAString& aCookie, mozilla::ErrorResult& rv);
  nsGenericHTMLElement *GetBody();
  void SetBody(nsGenericHTMLElement* aBody, mozilla::ErrorResult& rv);
  mozilla::dom::HTMLSharedElement *GetHead() {
    return static_cast<mozilla::dom::HTMLSharedElement*>(GetHeadElement());
  }
  nsIHTMLCollection* Images();
  nsIHTMLCollection* Embeds();
  nsIHTMLCollection* Plugins();
  nsIHTMLCollection* Links();
  nsIHTMLCollection* Forms()
  {
    return nsHTMLDocument::GetForms();
  }
  nsIHTMLCollection* Scripts();
  already_AddRefed<nsContentList> GetElementsByName(const nsAString & aName)
  {
    return NS_GetFuncStringNodeList(this, MatchNameAttribute, nullptr,
                                    UseExistingNameString, aName);
  }
  already_AddRefed<nsINodeList> GetItems(const nsAString& aTypeNames);
  already_AddRefed<nsIDocument> Open(JSContext* cx,
                                     const nsAString& aType,
                                     const nsAString& aReplace,
                                     mozilla::ErrorResult& rv);
  already_AddRefed<nsIDOMWindow> Open(JSContext* cx,
                                      const nsAString& aURL,
                                      const nsAString& aName,
                                      const nsAString& aFeatures,
                                      bool aReplace,
                                      mozilla::ErrorResult& rv);
  void Close(mozilla::ErrorResult& rv);
  void Write(JSContext* cx, const mozilla::dom::Sequence<nsString>& aText,
             mozilla::ErrorResult& rv);
  void Writeln(JSContext* cx, const mozilla::dom::Sequence<nsString>& aText,
               mozilla::ErrorResult& rv);
  
  void SetDesignMode(const nsAString& aDesignMode, mozilla::ErrorResult& rv);
  bool ExecCommand(const nsAString& aCommandID, bool aDoShowUI,
                   const nsAString& aValue, mozilla::ErrorResult& rv);
  bool QueryCommandEnabled(const nsAString& aCommandID,
                           mozilla::ErrorResult& rv);
  bool QueryCommandIndeterm(const nsAString& aCommandID,
                            mozilla::ErrorResult& rv);
  bool QueryCommandState(const nsAString& aCommandID, mozilla::ErrorResult& rv);
  bool QueryCommandSupported(const nsAString& aCommandID);
  void QueryCommandValue(const nsAString& aCommandID, nsAString& aValue,
                         mozilla::ErrorResult& rv);
  
  
  
  
  
  nsIHTMLCollection* Anchors();
  nsIHTMLCollection* Applets();
  void Clear() const
  {
    
  }
  already_AddRefed<nsISelection> GetSelection(mozilla::ErrorResult& rv);
  
  
  
  
  already_AddRefed<nsIDOMLocation> GetLocation() const {
    return nsIDocument::GetLocation();
  }

protected:
  nsresult GetBodySize(int32_t* aWidth,
                       int32_t* aHeight);

  nsIContent *MatchId(nsIContent *aContent, const nsAString& aId);

  static bool MatchLinks(nsIContent *aContent, int32_t aNamespaceID,
                           nsIAtom* aAtom, void* aData);
  static bool MatchAnchors(nsIContent *aContent, int32_t aNamespaceID,
                             nsIAtom* aAtom, void* aData);
  static bool MatchNameAttribute(nsIContent* aContent, int32_t aNamespaceID,
                                   nsIAtom* aAtom, void* aData);
  static void* UseExistingNameString(nsINode* aRootNode, const nsString* aName);

  static void DocumentWriteTerminationFunc(nsISupports *aRef);

  already_AddRefed<nsIURI> GetDomainURI();

  nsresult WriteCommon(JSContext *cx, const nsAString& aText,
                       bool aNewlineTerminate);
  
  void WriteCommon(JSContext *cx,
                   const mozilla::dom::Sequence<nsString>& aText,
                   bool aNewlineTerminate,
                   mozilla::ErrorResult& rv);

  nsresult CreateAndAddWyciwygChannel(void);
  nsresult RemoveWyciwygChannel(void);

  


  bool IsEditingOnAfterFlush();

  void *GenerateParserKey(void);

  nsRefPtr<nsContentList> mImages;
  nsRefPtr<nsContentList> mApplets;
  nsRefPtr<nsContentList> mEmbeds;
  nsRefPtr<nsContentList> mLinks;
  nsRefPtr<nsContentList> mAnchors;
  nsRefPtr<nsContentList> mScripts;
  nsRefPtr<nsContentList> mForms;
  nsRefPtr<nsContentList> mFormControls;

  
  int32_t mNumForms;

  static uint32_t gWyciwygSessionCnt;

  static void TryHintCharset(nsIMarkupDocumentViewer* aMarkupDV,
                             int32_t& aCharsetSource,
                             nsACString& aCharset);
  void TryUserForcedCharset(nsIMarkupDocumentViewer* aMarkupDV,
                            nsIDocShell*  aDocShell,
                            int32_t& aCharsetSource,
                            nsACString& aCharset);
  static void TryCacheCharset(nsICachingChannel* aCachingChannel,
                                int32_t& aCharsetSource,
                                nsACString& aCharset);
  
  void TryParentCharset(nsIDocShell*  aDocShell,
                        nsIDocument* aParentDocument,
                        int32_t& charsetSource, nsACString& aCharset);
  static void TryWeakDocTypeDefault(int32_t& aCharsetSource,
                                    nsACString& aCharset);
  static void TryDefaultCharset(nsIMarkupDocumentViewer* aMarkupDV,
                                int32_t& aCharsetSource,
                                nsACString& aCharset);

  
  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID);

  
  
  
  
  uint32_t mWriteLevel;

  
  uint32_t mLoadFlags;

  bool mTooDeepWriteRecursion;

  bool mDisableDocWrite;

  bool mWarnedWidthHeight;

  nsCOMPtr<nsIWyciwygChannel> mWyciwygChannel;

  
  nsresult   GetMidasCommandManager(nsICommandManager** aCommandManager);

  nsCOMPtr<nsICommandManager> mMidasCommandManager;

  nsresult TurnEditingOff();
  nsresult EditingStateChanged();
  void MaybeEditingStateChanged();

  uint32_t mContentEditableCount;
  EditingState mEditingState;

  nsresult   DoClipboardSecurityCheck(bool aPaste);
  static jsid        sCutCopyInternal_id;
  static jsid        sPasteInternal_id;

  
  bool mDisableCookieAccess;

  



  bool mPendingMaybeEditingStateChanged;
};

#define NS_HTML_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                        \
    NS_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                                 \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIHTMLDocument)                         \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIDOMHTMLDocument)

#endif 
