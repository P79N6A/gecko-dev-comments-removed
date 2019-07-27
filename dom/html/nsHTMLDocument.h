




#ifndef nsHTMLDocument_h___
#define nsHTMLDocument_h___

#include "mozilla/Attributes.h"
#include "nsDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIScriptElement.h"
#include "nsTArray.h"

#include "pldhash.h"
#include "nsIHttpChannel.h"
#include "nsHTMLStyleSheet.h"

#include "nsICommandManager.h"
#include "mozilla/dom/HTMLSharedElement.h"

class nsIEditor;
class nsIURI;
class nsIDocShell;
class nsICachingChannel;
class nsIWyciwygChannel;
class nsILoadGroup;

namespace mozilla {
namespace dom {
class HTMLAllCollection;
} 
} 

class nsHTMLDocument : public nsDocument,
                       public nsIHTMLDocument,
                       public nsIDOMHTMLDocument
{
public:
  using nsDocument::SetDocumentURI;
  using nsDocument::GetPlugins;

  nsHTMLDocument();
  virtual nsresult Init() override;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLDocument, nsDocument)

  
  virtual void Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup) override;
  virtual void ResetToURI(nsIURI* aURI, nsILoadGroup* aLoadGroup,
                          nsIPrincipal* aPrincipal) override;

  virtual already_AddRefed<nsIPresShell> CreateShell(nsPresContext* aContext,
                                                     nsViewManager* aViewManager,
                                                     nsStyleSet* aStyleSet) override;

  virtual nsresult StartDocumentLoad(const char* aCommand,
                                     nsIChannel* aChannel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     bool aReset = true,
                                     nsIContentSink* aSink = nullptr) override;
  virtual void StopDocumentLoad() override;

  virtual void BeginLoad() override;
  virtual void EndLoad() override;

  
  virtual void SetCompatibilityMode(nsCompatibility aMode) override;

  virtual bool IsWriting() override
  {
    return mWriteLevel != uint32_t(0);
  }

  virtual nsContentList* GetForms() override;

  virtual nsContentList* GetFormControls() override;

  
  using nsDocument::CreateElement;
  using nsDocument::CreateElementNS;
  NS_FORWARD_NSIDOMDOCUMENT(nsDocument::)

  
  using nsDocument::GetImplementation;
  using nsDocument::GetTitle;
  using nsDocument::SetTitle;
  using nsDocument::GetLastStyleSheetSet;
  using nsDocument::MozSetImageElement;
  using nsDocument::GetMozFullScreenElement;

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_DECL_NSIDOMHTMLDOCUMENT

  mozilla::dom::HTMLAllCollection* All();

  nsISupports* ResolveName(const nsAString& aName, nsWrapperCache **aCache);

  virtual void AddedForm() override;
  virtual void RemovedForm() override;
  virtual int32_t GetNumFormsSynchronous() override;
  virtual void TearingDownEditor(nsIEditor *aEditor) override;
  virtual void SetIsXHTML(bool aXHTML) override
  {
    mType = (aXHTML ? eXHTML : eHTML);
  }
  virtual void SetDocWriteDisabled(bool aDisabled) override
  {
    mDisableDocWrite = aDisabled;
  }

  nsresult ChangeContentEditableCount(nsIContent *aElement, int32_t aChange) override;
  void DeferredContentEditableCountChange(nsIContent *aElement);

  virtual EditingState GetEditingState() override
  {
    return mEditingState;
  }

  virtual void DisableCookieAccess() override
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

  void EndUpdate(nsUpdateType aUpdateType) override;

  virtual nsresult SetEditingState(EditingState aState) override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  virtual void RemovedFromDocShell() override;

  virtual mozilla::dom::Element *GetElementById(const nsAString& aElementId) override
  {
    return nsDocument::GetElementById(aElementId);
  }

  virtual void DocAddSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const override;
  

  virtual bool WillIgnoreCharsetOverride() override;

  
  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
    override;
  void SetDomain(const nsAString& aDomain, mozilla::ErrorResult& rv);
  void GetCookie(nsAString& aCookie, mozilla::ErrorResult& rv);
  void SetCookie(const nsAString& aCookie, mozilla::ErrorResult& rv);
  void NamedGetter(JSContext* cx, const nsAString& aName, bool& aFound,
                   JS::MutableHandle<JSObject*> aRetval,
                   mozilla::ErrorResult& rv);
  bool NameIsEnumerable(const nsAString& aName);
  void GetSupportedNames(unsigned, nsTArray<nsString>& aNames);
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
  mozilla::dom::Selection* GetSelection(mozilla::ErrorResult& aRv);
  
  
  
  already_AddRefed<nsLocation> GetLocation() const {
    return nsIDocument::GetLocation();
  }

  virtual nsHTMLDocument* AsHTMLDocument() override { return this; }

protected:
  ~nsHTMLDocument();

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

  
  already_AddRefed<nsIChannel> CreateDummyChannelForCookies(nsIURI* aCodebaseURI);

  


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

  nsRefPtr<mozilla::dom::HTMLAllCollection> mAll;

  
  int32_t mNumForms;

  static uint32_t gWyciwygSessionCnt;

  static void TryHintCharset(nsIContentViewer* aContentViewer,
                             int32_t& aCharsetSource,
                             nsACString& aCharset);
  void TryUserForcedCharset(nsIContentViewer* aCv,
                            nsIDocShell*  aDocShell,
                            int32_t& aCharsetSource,
                            nsACString& aCharset);
  static void TryCacheCharset(nsICachingChannel* aCachingChannel,
                                int32_t& aCharsetSource,
                                nsACString& aCharset);
  void TryParentCharset(nsIDocShell*  aDocShell,
                        int32_t& charsetSource, nsACString& aCharset);
  void TryTLD(int32_t& aCharsetSource, nsACString& aCharset);
  static void TryFallback(int32_t& aCharsetSource, nsACString& aCharset);

  
  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID) override;

  
  
  
  
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

  
  bool mDisableCookieAccess;

  



  bool mPendingMaybeEditingStateChanged;
};

#define NS_HTML_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                        \
    NS_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                                 \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIHTMLDocument)                         \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIDOMHTMLDocument)

#endif 
