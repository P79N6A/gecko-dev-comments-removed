



#include "nsCOMPtr.h"
#include "nsXMLContentSink.h"
#include "nsIFragmentContentSink.h"
#include "nsIXMLContentSink.h"
#include "nsContentSink.h"
#include "nsIExpatSink.h"
#include "nsIDTD.h"
#include "nsIDocument.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIContent.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/NodeInfo.h"
#include "nsContentCreatorFunctions.h"
#include "nsError.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsNetUtil.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDocShell.h"
#include "nsScriptLoader.h"
#include "mozilla/css/Loader.h"
#include "mozilla/dom/DocumentFragment.h"
#include "mozilla/dom/ProcessingInstruction.h"

using namespace mozilla::dom;

class nsXMLFragmentContentSink : public nsXMLContentSink,
                                 public nsIFragmentContentSink
{
public:
  nsXMLFragmentContentSink();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsXMLFragmentContentSink,
                                                     nsXMLContentSink)

  
  NS_IMETHOD HandleDoctypeDecl(const nsAString& aSubset,
                               const nsAString& aName,
                               const nsAString& aSystemId,
                               const nsAString& aPublicId,
                               nsISupports* aCatalogData) MOZ_OVERRIDE;
  NS_IMETHOD HandleProcessingInstruction(const char16_t* aTarget,
                                         const char16_t* aData) MOZ_OVERRIDE;
  NS_IMETHOD HandleXMLDeclaration(const char16_t* aVersion,
                                  const char16_t* aEncoding,
                                  int32_t aStandalone) MOZ_OVERRIDE;
  NS_IMETHOD ReportError(const char16_t* aErrorText,
                         const char16_t* aSourceText,
                         nsIScriptError* aError,
                         bool* aRetval) MOZ_OVERRIDE;

  
  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode) MOZ_OVERRIDE;
  NS_IMETHOD DidBuildModel(bool aTerminated) MOZ_OVERRIDE;
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset) MOZ_OVERRIDE;
  virtual nsISupports* GetTarget() MOZ_OVERRIDE;
  NS_IMETHOD DidProcessATokenImpl();

  

  
  NS_IMETHOD FinishFragmentParsing(nsIDOMDocumentFragment** aFragment) MOZ_OVERRIDE;
  NS_IMETHOD SetTargetDocument(nsIDocument* aDocument) MOZ_OVERRIDE;
  NS_IMETHOD WillBuildContent() MOZ_OVERRIDE;
  NS_IMETHOD DidBuildContent() MOZ_OVERRIDE;
  NS_IMETHOD IgnoreFirstContainer() MOZ_OVERRIDE;
  NS_IMETHOD SetPreventScriptExecution(bool aPreventScriptExecution) MOZ_OVERRIDE;

protected:
  virtual ~nsXMLFragmentContentSink();

  virtual bool SetDocElement(int32_t aNameSpaceID,
                               nsIAtom* aTagName,
                               nsIContent* aContent) MOZ_OVERRIDE;
  virtual nsresult CreateElement(const char16_t** aAtts, uint32_t aAttsCount,
                                 mozilla::dom::NodeInfo* aNodeInfo, uint32_t aLineNumber,
                                 nsIContent** aResult, bool* aAppendContent,
                                 mozilla::dom::FromParser aFromParser) MOZ_OVERRIDE;
  virtual nsresult CloseElement(nsIContent* aContent) MOZ_OVERRIDE;

  virtual void MaybeStartLayout(bool aIgnorePendingSheets) MOZ_OVERRIDE;

  
  virtual nsresult ProcessStyleLink(nsIContent* aElement,
                                    const nsSubstring& aHref,
                                    bool aAlternate,
                                    const nsSubstring& aTitle,
                                    const nsSubstring& aType,
                                    const nsSubstring& aMedia) MOZ_OVERRIDE;
  nsresult LoadXSLStyleSheet(nsIURI* aUrl);
  void StartLayout();

  nsCOMPtr<nsIDocument> mTargetDocument;
  
  nsCOMPtr<nsIContent>  mRoot;
  bool                  mParseError;
};

static nsresult
NewXMLFragmentContentSinkHelper(nsIFragmentContentSink** aResult)
{
  nsXMLFragmentContentSink* it = new nsXMLFragmentContentSink();
  
  NS_ADDREF(*aResult = it);
  
  return NS_OK;
}

nsresult
NS_NewXMLFragmentContentSink(nsIFragmentContentSink** aResult)
{
  return NewXMLFragmentContentSinkHelper(aResult);
}

nsXMLFragmentContentSink::nsXMLFragmentContentSink()
 : mParseError(false)
{
  mRunsToCompletion = true;
}

nsXMLFragmentContentSink::~nsXMLFragmentContentSink()
{
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXMLFragmentContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIFragmentContentSink)
NS_INTERFACE_MAP_END_INHERITING(nsXMLContentSink)

NS_IMPL_ADDREF_INHERITED(nsXMLFragmentContentSink, nsXMLContentSink)
NS_IMPL_RELEASE_INHERITED(nsXMLFragmentContentSink, nsXMLContentSink)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXMLFragmentContentSink)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXMLFragmentContentSink,
                                                  nsXMLContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTargetDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRoot)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMETHODIMP 
nsXMLFragmentContentSink::WillBuildModel(nsDTDMode aDTDMode)
{
  if (mRoot) {
    return NS_OK;
  }

  mState = eXMLContentSinkState_InDocumentElement;

  NS_ASSERTION(mTargetDocument, "Need a document!");

  mRoot = new DocumentFragment(mNodeInfoManager);
  
  return NS_OK;
}

NS_IMETHODIMP 
nsXMLFragmentContentSink::DidBuildModel(bool aTerminated)
{
  nsRefPtr<nsParserBase> kungFuDeathGrip(mParser);

  
  
  mParser = nullptr;

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLFragmentContentSink::SetDocumentCharset(nsACString& aCharset)
{
  NS_NOTREACHED("fragments shouldn't set charset");
  return NS_OK;
}

nsISupports *
nsXMLFragmentContentSink::GetTarget()
{
  return mTargetDocument;
}



bool
nsXMLFragmentContentSink::SetDocElement(int32_t aNameSpaceID,
                                        nsIAtom* aTagName,
                                        nsIContent *aContent)
{
  
  return false;
}

nsresult
nsXMLFragmentContentSink::CreateElement(const char16_t** aAtts, uint32_t aAttsCount,
                                        mozilla::dom::NodeInfo* aNodeInfo, uint32_t aLineNumber,
                                        nsIContent** aResult, bool* aAppendContent,
                                        FromParser )
{
  
  
  nsresult rv = nsXMLContentSink::CreateElement(aAtts, aAttsCount,
                                                aNodeInfo, aLineNumber,
                                                aResult, aAppendContent,
                                                NOT_FROM_PARSER);

  
  
  
  if (mContentStack.Length() == 0) {
    *aAppendContent = false;
  }

  return rv;
}

nsresult
nsXMLFragmentContentSink::CloseElement(nsIContent* aContent)
{
  
  if (mPreventScriptExecution &&
      (aContent->IsHTMLElement(nsGkAtoms::script),
       aContent->IsSVGElement(nsGkAtoms::script))) {
    nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(aContent);
    NS_ASSERTION(sele, "script did QI correctly!");
    sele->PreventExecution();
  }
  return NS_OK;
}

void
nsXMLFragmentContentSink::MaybeStartLayout(bool aIgnorePendingSheets)
{
  return;
}



NS_IMETHODIMP
nsXMLFragmentContentSink::HandleDoctypeDecl(const nsAString & aSubset, 
                                            const nsAString & aName, 
                                            const nsAString & aSystemId, 
                                            const nsAString & aPublicId,
                                            nsISupports* aCatalogData)
{
  NS_NOTREACHED("fragments shouldn't have doctype declarations");

  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::HandleProcessingInstruction(const char16_t *aTarget, 
                                                      const char16_t *aData)
{
  FlushText();

  const nsDependentString target(aTarget);
  const nsDependentString data(aData);

  nsRefPtr<ProcessingInstruction> node =
    NS_NewXMLProcessingInstruction(mNodeInfoManager, target, data);

  
  return AddContentAsLeaf(node);
}

NS_IMETHODIMP
nsXMLFragmentContentSink::HandleXMLDeclaration(const char16_t *aVersion,
                                               const char16_t *aEncoding,
                                               int32_t aStandalone)
{
  NS_NOTREACHED("fragments shouldn't have XML declarations");
  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::ReportError(const char16_t* aErrorText, 
                                      const char16_t* aSourceText,
                                      nsIScriptError *aError,
                                      bool *_retval)
{
  NS_PRECONDITION(aError && aSourceText && aErrorText, "Check arguments!!!");

  
  *_retval = true;

  mParseError = true;

#ifdef DEBUG
  
  fprintf(stderr,
          "\n%s\n%s\n\n",
          NS_LossyConvertUTF16toASCII(aErrorText).get(),
          NS_LossyConvertUTF16toASCII(aSourceText).get());
#endif

  
  mState = eXMLContentSinkState_InProlog;

  
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mRoot));
  if (node) {
    for (;;) {
      nsCOMPtr<nsIDOMNode> child, dummy;
      node->GetLastChild(getter_AddRefs(child));
      if (!child)
        break;
      node->RemoveChild(child, getter_AddRefs(dummy));
    }
  }

  
  
  
  mTextLength = 0;

  return NS_OK; 
}

nsresult
nsXMLFragmentContentSink::ProcessStyleLink(nsIContent* aElement,
                                           const nsSubstring& aHref,
                                           bool aAlternate,
                                           const nsSubstring& aTitle,
                                           const nsSubstring& aType,
                                           const nsSubstring& aMedia)
{
  
  return NS_OK;
}

nsresult
nsXMLFragmentContentSink::LoadXSLStyleSheet(nsIURI* aUrl)
{
  NS_NOTREACHED("fragments shouldn't have XSL style sheets");
  return NS_ERROR_UNEXPECTED;
}

void
nsXMLFragmentContentSink::StartLayout()
{
  NS_NOTREACHED("fragments shouldn't layout");
}



NS_IMETHODIMP 
nsXMLFragmentContentSink::FinishFragmentParsing(nsIDOMDocumentFragment** aFragment)
{
  *aFragment = nullptr;
  mTargetDocument = nullptr;
  mNodeInfoManager = nullptr;
  mScriptLoader = nullptr;
  mCSSLoader = nullptr;
  mContentStack.Clear();
  mDocumentURI = nullptr;
  mDocShell = nullptr;
  mDocElement = nullptr;
  mCurrentHead = nullptr;
  if (mParseError) {
    
    mRoot = nullptr;
    mParseError = false;
    return NS_ERROR_DOM_SYNTAX_ERR;
  } else if (mRoot) {
    nsresult rv = CallQueryInterface(mRoot, aFragment);
    mRoot = nullptr;
    return rv;
  } else {
    return NS_OK;
  }
}

NS_IMETHODIMP
nsXMLFragmentContentSink::SetTargetDocument(nsIDocument* aTargetDocument)
{
  NS_ENSURE_ARG_POINTER(aTargetDocument);

  mTargetDocument = aTargetDocument;
  mNodeInfoManager = aTargetDocument->NodeInfoManager();

  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::WillBuildContent()
{
  PushContent(mRoot);

  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::DidBuildContent()
{
  
  
  if (!mParseError) {
    FlushText();
  }
  PopContent();

  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::DidProcessATokenImpl()
{
  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::IgnoreFirstContainer()
{
  NS_NOTREACHED("XML isn't as broken as HTML");
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::SetPreventScriptExecution(bool aPrevent)
{
  mPreventScriptExecution = aPrevent;
  return NS_OK;
}
