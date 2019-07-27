




#include "txMozillaXMLOutput.h"

#include "nsIDocument.h"
#include "nsIDocShell.h"
#include "nsScriptLoader.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIScriptElement.h"
#include "nsCharsetSource.h"
#include "nsIRefreshURI.h"
#include "nsPIDOMWindow.h"
#include "nsIContent.h"
#include "nsContentCID.h"
#include "nsNetUtil.h"
#include "nsUnicharUtils.h"
#include "nsGkAtoms.h"
#include "txLog.h"
#include "nsIConsoleService.h"
#include "nsIDOMDocumentFragment.h"
#include "nsNameSpaceManager.h"
#include "txStringUtils.h"
#include "txURIUtils.h"
#include "nsIHTMLDocument.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsIDocumentTransformer.h"
#include "mozilla/CSSStyleSheet.h"
#include "mozilla/css/Loader.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/EncodingUtils.h"
#include "nsContentUtils.h"
#include "txXMLUtils.h"
#include "nsContentSink.h"
#include "nsINode.h"
#include "nsContentCreatorFunctions.h"
#include "nsError.h"
#include "nsIFrame.h"
#include <algorithm>
#include "nsTextNode.h"
#include "mozilla/dom/Comment.h"
#include "mozilla/dom/ProcessingInstruction.h"

using namespace mozilla;
using namespace mozilla::dom;

#define TX_ENSURE_CURRENTNODE                           \
    NS_ASSERTION(mCurrentNode, "mCurrentNode is nullptr"); \
    if (!mCurrentNode)                                  \
        return NS_ERROR_UNEXPECTED

txMozillaXMLOutput::txMozillaXMLOutput(txOutputFormat* aFormat,
                                       nsITransformObserver* aObserver)
    : mTreeDepth(0),
      mBadChildLevel(0),
      mTableState(NORMAL),
      mCreatingNewDocument(true),
      mOpenedElementIsHTML(false),
      mRootContentCreated(false),
      mNoFixup(false)
{
    MOZ_COUNT_CTOR(txMozillaXMLOutput);
    if (aObserver) {
        mNotifier = new txTransformNotifier();
        if (mNotifier) {
            mNotifier->Init(aObserver);
        }
    }

    mOutputFormat.merge(*aFormat);
    mOutputFormat.setFromDefaults();
}

txMozillaXMLOutput::txMozillaXMLOutput(txOutputFormat* aFormat,
                                       nsIDOMDocumentFragment* aFragment,
                                       bool aNoFixup)
    : mTreeDepth(0),
      mBadChildLevel(0),
      mTableState(NORMAL),
      mCreatingNewDocument(false),
      mOpenedElementIsHTML(false),
      mRootContentCreated(false),
      mNoFixup(aNoFixup)
{
    MOZ_COUNT_CTOR(txMozillaXMLOutput);
    mOutputFormat.merge(*aFormat);
    mOutputFormat.setFromDefaults();

    mCurrentNode = do_QueryInterface(aFragment);
    mDocument = mCurrentNode->OwnerDoc();
    mNodeInfoManager = mDocument->NodeInfoManager();
}

txMozillaXMLOutput::~txMozillaXMLOutput()
{
    MOZ_COUNT_DTOR(txMozillaXMLOutput);
}

nsresult
txMozillaXMLOutput::attribute(nsIAtom* aPrefix,
                              nsIAtom* aLocalName,
                              nsIAtom* aLowercaseLocalName,
                              const int32_t aNsID,
                              const nsString& aValue)
{
    nsCOMPtr<nsIAtom> owner;
    if (mOpenedElementIsHTML && aNsID == kNameSpaceID_None) {
        if (aLowercaseLocalName) {
            aLocalName = aLowercaseLocalName;
        }
        else {
            owner = TX_ToLowerCaseAtom(aLocalName);
            NS_ENSURE_TRUE(owner, NS_ERROR_OUT_OF_MEMORY);

            aLocalName = owner;
        }
    }

    return attributeInternal(aPrefix, aLocalName, aNsID, aValue);
}

nsresult
txMozillaXMLOutput::attribute(nsIAtom* aPrefix,
                              const nsSubstring& aLocalName,
                              const int32_t aNsID,
                              const nsString& aValue)
{
    nsCOMPtr<nsIAtom> lname;

    if (mOpenedElementIsHTML && aNsID == kNameSpaceID_None) {
        nsAutoString lnameStr;
        nsContentUtils::ASCIIToLower(aLocalName, lnameStr);
        lname = do_GetAtom(lnameStr);
    }
    else {
        lname = do_GetAtom(aLocalName);
    }

    NS_ENSURE_TRUE(lname, NS_ERROR_OUT_OF_MEMORY);

    
    if (!nsContentUtils::IsValidNodeName(lname, aPrefix, aNsID)) {
        
        aPrefix = nullptr;
        if (!nsContentUtils::IsValidNodeName(lname, aPrefix, aNsID)) {
            
            return NS_OK;
        }
    }

    return attributeInternal(aPrefix, lname, aNsID, aValue);
}

nsresult
txMozillaXMLOutput::attributeInternal(nsIAtom* aPrefix,
                                      nsIAtom* aLocalName,
                                      int32_t aNsID,
                                      const nsString& aValue)
{
    if (!mOpenedElement) {
        
        return NS_OK;
    }

    NS_ASSERTION(!mBadChildLevel, "mBadChildLevel set when element is opened");

    return mOpenedElement->SetAttr(aNsID, aLocalName, aPrefix, aValue,
                                   false);
}

nsresult
txMozillaXMLOutput::characters(const nsSubstring& aData, bool aDOE)
{
    nsresult rv = closePrevious(false);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mBadChildLevel) {
        mText.Append(aData);
    }

    return NS_OK;
}

nsresult
txMozillaXMLOutput::comment(const nsString& aData)
{
    nsresult rv = closePrevious(true);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mBadChildLevel) {
        return NS_OK;
    }

    TX_ENSURE_CURRENTNODE;

    nsRefPtr<Comment> comment = new Comment(mNodeInfoManager);

    rv = comment->SetText(aData, false);
    NS_ENSURE_SUCCESS(rv, rv);

    return mCurrentNode->AppendChildTo(comment, true);
}

nsresult
txMozillaXMLOutput::endDocument(nsresult aResult)
{
    TX_ENSURE_CURRENTNODE;

    if (NS_FAILED(aResult)) {
        if (mNotifier) {
            mNotifier->OnTransformEnd(aResult);
        }
        
        return NS_OK;
    }

    nsresult rv = closePrevious(true);
    if (NS_FAILED(rv)) {
        if (mNotifier) {
            mNotifier->OnTransformEnd(rv);
        }
        
        return rv;
    }

    if (mCreatingNewDocument) {
        
        MOZ_ASSERT(mDocument->GetReadyStateEnum() ==
                   nsIDocument::READYSTATE_LOADING, "Bad readyState");
        mDocument->SetReadyStateInternal(nsIDocument::READYSTATE_INTERACTIVE);
        nsScriptLoader* loader = mDocument->ScriptLoader();
        if (loader) {
            loader->ParsingComplete(false);
        }
    }

    if (!mRefreshString.IsEmpty()) {
        nsPIDOMWindow *win = mDocument->GetWindow();
        if (win) {
            nsCOMPtr<nsIRefreshURI> refURI =
                do_QueryInterface(win->GetDocShell());
            if (refURI) {
                refURI->SetupRefreshURIFromHeader(mDocument->GetDocBaseURI(),
                                                  mDocument->NodePrincipal(),
                                                  mRefreshString);
            }
        }
    }

    if (mNotifier) {
        mNotifier->OnTransformEnd();
    }

    return NS_OK;
}

nsresult
txMozillaXMLOutput::endElement()
{
    TX_ENSURE_CURRENTNODE;

    if (mBadChildLevel) {
        --mBadChildLevel;
        PR_LOG(txLog::xslt, PR_LOG_DEBUG,
               ("endElement, mBadChildLevel = %d\n", mBadChildLevel));
        return NS_OK;
    }
    
    --mTreeDepth;

    nsresult rv = closePrevious(true);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(mCurrentNode->IsElement(), "borked mCurrentNode");
    NS_ENSURE_TRUE(mCurrentNode->IsElement(), NS_ERROR_UNEXPECTED);

    Element* element = mCurrentNode->AsElement();

    
    if (!mNoFixup) {
        if (element->IsHTMLElement()) {
            rv = endHTMLElement(element);
            NS_ENSURE_SUCCESS(rv, rv);
        }

        
        if (element->IsAnyOfHTMLElements(nsGkAtoms::title,
                                         nsGkAtoms::object,
                                         nsGkAtoms::applet,
                                         nsGkAtoms::select,
                                         nsGkAtoms::textarea) ||
            element->IsSVGElement(nsGkAtoms::title)) {
            element->DoneAddingChildren(true);
        } else if (element->IsSVGElement(nsGkAtoms::script) ||
                   element->IsHTMLElement(nsGkAtoms::script)) {
            nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(element);
            MOZ_ASSERT(sele, "script elements need to implement nsIScriptElement");
            bool block = sele->AttemptToExecute();
            
            
            if (block) {
                rv = mNotifier->AddScriptElement(sele);
                NS_ENSURE_SUCCESS(rv, rv);
            }
        } else if (element->IsAnyOfHTMLElements(nsGkAtoms::input,
                                                nsGkAtoms::button,
                                                nsGkAtoms::menuitem,
                                                nsGkAtoms::audio,
                                                nsGkAtoms::video)) {
          element->DoneCreatingElement();
        }   
    }

    if (mCreatingNewDocument) {
        
        nsCOMPtr<nsIStyleSheetLinkingElement> ssle =
            do_QueryInterface(mCurrentNode);
        if (ssle) {
            ssle->SetEnableUpdates(true);
            bool willNotify;
            bool isAlternate;
            nsresult rv = ssle->UpdateStyleSheet(mNotifier, &willNotify,
                                                 &isAlternate);
            if (mNotifier && NS_SUCCEEDED(rv) && willNotify && !isAlternate) {
                mNotifier->AddPendingStylesheet();
            }
        }
    }

    
    
    uint32_t last = mCurrentNodeStack.Count() - 1;
    NS_ASSERTION(last != (uint32_t)-1, "empty stack");

    nsCOMPtr<nsINode> parent = mCurrentNodeStack.SafeObjectAt(last);
    mCurrentNodeStack.RemoveObjectAt(last);

    if (mCurrentNode == mNonAddedNode) {
        if (parent == mDocument) {
            NS_ASSERTION(!mRootContentCreated,
                         "Parent to add to shouldn't be a document if we "
                         "have a root content");
            mRootContentCreated = true;
        }

        
        
        if (!mCurrentNode->GetParentNode()) {
            parent->AppendChildTo(mNonAddedNode, true);
        }
        mNonAddedNode = nullptr;
    }

    mCurrentNode = parent;

    mTableState =
        static_cast<TableState>(NS_PTR_TO_INT32(mTableStateStack.pop()));

    return NS_OK;
}

void txMozillaXMLOutput::getOutputDocument(nsIDOMDocument** aDocument)
{
    CallQueryInterface(mDocument, aDocument);
}

nsresult
txMozillaXMLOutput::processingInstruction(const nsString& aTarget, const nsString& aData)
{
    nsresult rv = closePrevious(true);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mOutputFormat.mMethod == eHTMLOutput)
        return NS_OK;

    TX_ENSURE_CURRENTNODE;

    rv = nsContentUtils::CheckQName(aTarget, false);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIContent> pi =
      NS_NewXMLProcessingInstruction(mNodeInfoManager, aTarget, aData);

    nsCOMPtr<nsIStyleSheetLinkingElement> ssle;
    if (mCreatingNewDocument) {
        ssle = do_QueryInterface(pi);
        if (ssle) {
            ssle->InitStyleLinkElement(false);
            ssle->SetEnableUpdates(false);
        }
    }

    rv = mCurrentNode->AppendChildTo(pi, true);
    NS_ENSURE_SUCCESS(rv, rv);

    if (ssle) {
        ssle->SetEnableUpdates(true);
        bool willNotify;
        bool isAlternate;
        rv = ssle->UpdateStyleSheet(mNotifier, &willNotify, &isAlternate);
        if (mNotifier && NS_SUCCEEDED(rv) && willNotify && !isAlternate) {
            mNotifier->AddPendingStylesheet();
        }
    }

    return NS_OK;
}

nsresult
txMozillaXMLOutput::startDocument()
{
    if (mNotifier) {
        mNotifier->OnTransformStart();
    }

    if (mCreatingNewDocument) {
        nsScriptLoader* loader = mDocument->ScriptLoader();
        if (loader) {
            loader->BeginDeferringScripts();
        }
    }

    return NS_OK;
}

nsresult
txMozillaXMLOutput::startElement(nsIAtom* aPrefix, nsIAtom* aLocalName,
                                 nsIAtom* aLowercaseLocalName,
                                 const int32_t aNsID)
{
    NS_PRECONDITION(aNsID != kNameSpaceID_None || !aPrefix,
                    "Can't have prefix without namespace");

    if (mOutputFormat.mMethod == eHTMLOutput && aNsID == kNameSpaceID_None) {
        nsCOMPtr<nsIAtom> owner;
        if (!aLowercaseLocalName) {
            owner = TX_ToLowerCaseAtom(aLocalName);
            NS_ENSURE_TRUE(owner, NS_ERROR_OUT_OF_MEMORY);

            aLowercaseLocalName = owner;
        }
        return startElementInternal(nullptr, 
                                    aLowercaseLocalName, 
                                    kNameSpaceID_XHTML);
    }

    return startElementInternal(aPrefix, aLocalName, aNsID);
}

nsresult
txMozillaXMLOutput::startElement(nsIAtom* aPrefix,
                                 const nsSubstring& aLocalName,
                                 const int32_t aNsID)
{
    int32_t nsId = aNsID;
    nsCOMPtr<nsIAtom> lname;

    if (mOutputFormat.mMethod == eHTMLOutput && aNsID == kNameSpaceID_None) {
        nsId = kNameSpaceID_XHTML;

        nsAutoString lnameStr;
        nsContentUtils::ASCIIToLower(aLocalName, lnameStr);
        lname = do_GetAtom(lnameStr);
    }
    else {
        lname = do_GetAtom(aLocalName);
    }

    
    NS_ENSURE_TRUE(lname, NS_ERROR_OUT_OF_MEMORY);

    
    if (!nsContentUtils::IsValidNodeName(lname, aPrefix, nsId)) {
        
        aPrefix = nullptr;
        if (!nsContentUtils::IsValidNodeName(lname, aPrefix, nsId)) {
            return NS_ERROR_XSLT_BAD_NODE_NAME;
        }
    }

    return startElementInternal(aPrefix, lname, nsId);
}

nsresult
txMozillaXMLOutput::startElementInternal(nsIAtom* aPrefix,
                                         nsIAtom* aLocalName,
                                         int32_t aNsID)
{
    TX_ENSURE_CURRENTNODE;

    if (mBadChildLevel) {
        ++mBadChildLevel;
        PR_LOG(txLog::xslt, PR_LOG_DEBUG,
               ("startElement, mBadChildLevel = %d\n", mBadChildLevel));
        return NS_OK;
    }

    nsresult rv = closePrevious(true);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (mTreeDepth == MAX_REFLOW_DEPTH) {
        
        
        ++mBadChildLevel;
        PR_LOG(txLog::xslt, PR_LOG_DEBUG,
               ("startElement, mBadChildLevel = %d\n", mBadChildLevel));
        return NS_OK;
    }

    ++mTreeDepth;

    rv = mTableStateStack.push(NS_INT32_TO_PTR(mTableState));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mCurrentNodeStack.AppendObject(mCurrentNode)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    mTableState = NORMAL;
    mOpenedElementIsHTML = false;

    
    nsRefPtr<NodeInfo> ni =
        mNodeInfoManager->GetNodeInfo(aLocalName, aPrefix, aNsID,
                                      nsIDOMNode::ELEMENT_NODE);

    NS_NewElement(getter_AddRefs(mOpenedElement), ni.forget(),
                  mCreatingNewDocument ?
                  FROM_PARSER_XSLT : FROM_PARSER_FRAGMENT);

    
    if (!mNoFixup) {
        if (aNsID == kNameSpaceID_XHTML) {
            mOpenedElementIsHTML = (mOutputFormat.mMethod == eHTMLOutput);
            rv = startHTMLElement(mOpenedElement, mOpenedElementIsHTML);
            NS_ENSURE_SUCCESS(rv, rv);

        }
    }

    if (mCreatingNewDocument) {
        
        nsCOMPtr<nsIStyleSheetLinkingElement> ssle =
            do_QueryInterface(mOpenedElement);
        if (ssle) {
            ssle->InitStyleLinkElement(false);
            ssle->SetEnableUpdates(false);
        }
    }

    return NS_OK;
}

nsresult
txMozillaXMLOutput::closePrevious(bool aFlushText)
{
    TX_ENSURE_CURRENTNODE;

    nsresult rv;
    if (mOpenedElement) {
        bool currentIsDoc = mCurrentNode == mDocument;
        if (currentIsDoc && mRootContentCreated) {
            
            
            
            
            rv = createTxWrapper();
            NS_ENSURE_SUCCESS(rv, rv);
        }

        rv = mCurrentNode->AppendChildTo(mOpenedElement, true);
        NS_ENSURE_SUCCESS(rv, rv);

        if (currentIsDoc) {
            mRootContentCreated = true;
            nsContentSink::NotifyDocElementCreated(mDocument);
        }

        mCurrentNode = mOpenedElement;
        mOpenedElement = nullptr;
    }
    else if (aFlushText && !mText.IsEmpty()) {
        
        if (mDocument == mCurrentNode) {
            if (XMLUtils::isWhitespace(mText)) {
                mText.Truncate();
                
                return NS_OK;
            }

            rv = createTxWrapper();
            NS_ENSURE_SUCCESS(rv, rv);
        }
        nsRefPtr<nsTextNode> text = new nsTextNode(mNodeInfoManager);

        rv = text->SetText(mText, false);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mCurrentNode->AppendChildTo(text, true);
        NS_ENSURE_SUCCESS(rv, rv);

        mText.Truncate();
    }

    return NS_OK;
}

nsresult
txMozillaXMLOutput::createTxWrapper()
{
    NS_ASSERTION(mDocument == mCurrentNode,
                 "creating wrapper when document isn't parent");

    int32_t namespaceID;
    nsresult rv = nsContentUtils::NameSpaceManager()->
        RegisterNameSpace(NS_LITERAL_STRING(kTXNameSpaceURI), namespaceID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIContent> wrapper;
    rv = mDocument->CreateElem(nsDependentAtomString(nsGkAtoms::result),
                               nsGkAtoms::transformiix, namespaceID,
                               getter_AddRefs(wrapper));
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t i, j, childCount = mDocument->GetChildCount();
#ifdef DEBUG
    
    
    uint32_t rootLocation = 0;
#endif
    for (i = 0, j = 0; i < childCount; ++i) {
        nsCOMPtr<nsIContent> childContent = mDocument->GetChildAt(j);

#ifdef DEBUG
        if (childContent->IsElement()) {
            rootLocation = j;
        }
#endif

        if (childContent->NodeInfo()->NameAtom() == nsGkAtoms::documentTypeNodeName) {
#ifdef DEBUG
            
            
            
            rootLocation = std::max(rootLocation, j + 1);
#endif
            ++j;
        }
        else {
            mDocument->RemoveChildAt(j, true);

            rv = wrapper->AppendChildTo(childContent, true);
            NS_ENSURE_SUCCESS(rv, rv);
            break;
        }
    }

    if (!mCurrentNodeStack.AppendObject(wrapper)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    mCurrentNode = wrapper;
    mRootContentCreated = true;
    NS_ASSERTION(rootLocation == mDocument->GetChildCount(),
                 "Incorrect root location");
    return mDocument->AppendChildTo(wrapper, true);
}

nsresult
txMozillaXMLOutput::startHTMLElement(nsIContent* aElement, bool aIsHTML)
{
    nsresult rv = NS_OK;

    if ((!aElement->IsHTMLElement(nsGkAtoms::tr) || !aIsHTML) &&
        NS_PTR_TO_INT32(mTableStateStack.peek()) == ADDED_TBODY) {
        uint32_t last = mCurrentNodeStack.Count() - 1;
        NS_ASSERTION(last != (uint32_t)-1, "empty stack");

        mCurrentNode = mCurrentNodeStack.SafeObjectAt(last);
        mCurrentNodeStack.RemoveObjectAt(last);
        mTableStateStack.pop();
    }

    if (aElement->IsHTMLElement(nsGkAtoms::table) && aIsHTML) {
        mTableState = TABLE;
    }
    else if (aElement->IsHTMLElement(nsGkAtoms::tr) && aIsHTML &&
             NS_PTR_TO_INT32(mTableStateStack.peek()) == TABLE) {
        nsCOMPtr<nsIContent> tbody;
        rv = createHTMLElement(nsGkAtoms::tbody, getter_AddRefs(tbody));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mCurrentNode->AppendChildTo(tbody, true);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mTableStateStack.push(NS_INT32_TO_PTR(ADDED_TBODY));
        NS_ENSURE_SUCCESS(rv, rv);

        if (!mCurrentNodeStack.AppendObject(tbody)) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        mCurrentNode = tbody;
    }
    else if (aElement->IsHTMLElement(nsGkAtoms::head) &&
             mOutputFormat.mMethod == eHTMLOutput) {
        
        
        nsCOMPtr<nsIContent> meta;
        rv = createHTMLElement(nsGkAtoms::meta, getter_AddRefs(meta));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = meta->SetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv,
                           NS_LITERAL_STRING("Content-Type"), false);
        NS_ENSURE_SUCCESS(rv, rv);

        nsAutoString metacontent;
        metacontent.Append(mOutputFormat.mMediaType);
        metacontent.AppendLiteral("; charset=");
        metacontent.Append(mOutputFormat.mEncoding);
        rv = meta->SetAttr(kNameSpaceID_None, nsGkAtoms::content,
                           metacontent, false);
        NS_ENSURE_SUCCESS(rv, rv);

        
        NS_ASSERTION(!aElement->IsInDoc(), "should not be in doc");
        rv = aElement->AppendChildTo(meta, false);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

nsresult
txMozillaXMLOutput::endHTMLElement(nsIContent* aElement)
{
    if (mTableState == ADDED_TBODY) {
        NS_ASSERTION(aElement->IsHTMLElement(nsGkAtoms::tbody),
                     "Element flagged as added tbody isn't a tbody");
        uint32_t last = mCurrentNodeStack.Count() - 1;
        NS_ASSERTION(last != (uint32_t)-1, "empty stack");

        mCurrentNode = mCurrentNodeStack.SafeObjectAt(last);
        mCurrentNodeStack.RemoveObjectAt(last);
        mTableState = static_cast<TableState>
                                 (NS_PTR_TO_INT32(mTableStateStack.pop()));

        return NS_OK;
    }
    else if (mCreatingNewDocument && aElement->IsHTMLElement(nsGkAtoms::meta)) {
        
        nsAutoString httpEquiv;
        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv, httpEquiv);
        if (!httpEquiv.IsEmpty()) {
            nsAutoString value;
            aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::content, value);
            if (!value.IsEmpty()) {
                nsContentUtils::ASCIIToLower(httpEquiv);
                nsCOMPtr<nsIAtom> header = do_GetAtom(httpEquiv);
                processHTTPEquiv(header, value);
            }
        }
    }
    
    return NS_OK;
}

void txMozillaXMLOutput::processHTTPEquiv(nsIAtom* aHeader, const nsString& aValue)
{
    
    
    if (aHeader == nsGkAtoms::refresh)
        LossyCopyUTF16toASCII(aValue, mRefreshString);
}

nsresult
txMozillaXMLOutput::createResultDocument(const nsSubstring& aName, int32_t aNsID,
                                         nsIDOMDocument* aSourceDocument,
                                         bool aLoadedAsData)
{
    nsresult rv;

    
    if (mOutputFormat.mMethod == eHTMLOutput) {
        rv = NS_NewHTMLDocument(getter_AddRefs(mDocument),
                                aLoadedAsData);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        
        
        rv = NS_NewXMLDocument(getter_AddRefs(mDocument),
                               aLoadedAsData);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    
    MOZ_ASSERT(mDocument->GetReadyStateEnum() ==
               nsIDocument::READYSTATE_UNINITIALIZED, "Bad readyState");
    mDocument->SetReadyStateInternal(nsIDocument::READYSTATE_LOADING);
    nsCOMPtr<nsIDocument> source = do_QueryInterface(aSourceDocument);
    NS_ENSURE_STATE(source);
    bool hasHadScriptObject = false;
    nsIScriptGlobalObject* sgo =
      source->GetScriptHandlingObject(hasHadScriptObject);
    NS_ENSURE_STATE(sgo || !hasHadScriptObject);
    mDocument->SetScriptHandlingObject(sgo);

    mCurrentNode = mDocument;
    mNodeInfoManager = mDocument->NodeInfoManager();

    
    URIUtils::ResetWithSource(mDocument, aSourceDocument);

    
    if (!mOutputFormat.mEncoding.IsEmpty()) {
        nsAutoCString canonicalCharset;
        if (EncodingUtils::FindEncodingForLabel(mOutputFormat.mEncoding,
                                                canonicalCharset)) {
            mDocument->SetDocumentCharacterSetSource(kCharsetFromOtherComponent);
            mDocument->SetDocumentCharacterSet(canonicalCharset);
        }
    }

    
    if (!mOutputFormat.mMediaType.IsEmpty()) {
        mDocument->SetContentType(mOutputFormat.mMediaType);
    }
    else if (mOutputFormat.mMethod == eHTMLOutput) {
        mDocument->SetContentType(NS_LITERAL_STRING("text/html"));
    }
    else {
        mDocument->SetContentType(NS_LITERAL_STRING("application/xml"));
    }

    if (mOutputFormat.mMethod == eXMLOutput &&
        mOutputFormat.mOmitXMLDeclaration != eTrue) {
        int32_t standalone;
        if (mOutputFormat.mStandalone == eNotSet) {
          standalone = -1;
        }
        else if (mOutputFormat.mStandalone == eFalse) {
          standalone = 0;
        }
        else {
          standalone = 1;
        }

        
        
        static const char16_t kOneDotZero[] = { '1', '.', '0', '\0' };
        mDocument->SetXMLDeclaration(kOneDotZero, mOutputFormat.mEncoding.get(),
                                     standalone);
    }

    
    nsScriptLoader *loader = mDocument->ScriptLoader();
    if (mNotifier) {
        loader->AddObserver(mNotifier);
    }
    else {
        
        loader->SetEnabled(false);
    }

    if (mNotifier) {
        rv = mNotifier->SetOutputDocument(mDocument);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mDocument);
    if (htmlDoc) {
        htmlDoc->SetCompatibilityMode(eCompatibility_FullStandards);
    }

    
    if (!mOutputFormat.mSystemId.IsEmpty()) {
        nsAutoString qName;
        if (mOutputFormat.mMethod == eHTMLOutput) {
            qName.AssignLiteral("html");
        }
        else {
            qName.Assign(aName);
        }

        nsCOMPtr<nsIDOMDocumentType> documentType;

        nsresult rv = nsContentUtils::CheckQName(qName);
        if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIAtom> doctypeName = do_GetAtom(qName);
            if (!doctypeName) {
                return NS_ERROR_OUT_OF_MEMORY;
            }

            
            rv = NS_NewDOMDocumentType(getter_AddRefs(documentType),
                                       mNodeInfoManager,
                                       doctypeName,
                                       mOutputFormat.mPublicId,
                                       mOutputFormat.mSystemId,
                                       NullString());
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIContent> docType = do_QueryInterface(documentType);
            rv = mDocument->AppendChildTo(docType, true);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    return NS_OK;
}

nsresult
txMozillaXMLOutput::createHTMLElement(nsIAtom* aName,
                                      nsIContent** aResult)
{
    NS_ASSERTION(mOutputFormat.mMethod == eHTMLOutput,
                 "need to adjust createHTMLElement");

    *aResult = nullptr;

    nsRefPtr<NodeInfo> ni;
    ni = mNodeInfoManager->GetNodeInfo(aName, nullptr,
                                       kNameSpaceID_XHTML,
                                       nsIDOMNode::ELEMENT_NODE);

    nsCOMPtr<Element> el;
    nsresult rv =
        NS_NewHTMLElement(getter_AddRefs(el), ni.forget(),
                          mCreatingNewDocument ?
                            FROM_PARSER_XSLT : FROM_PARSER_FRAGMENT);
    el.forget(aResult);
    return rv;
}

txTransformNotifier::txTransformNotifier()
    : mPendingStylesheetCount(0),
      mInTransform(false)      
{
}

txTransformNotifier::~txTransformNotifier()
{
}

NS_IMPL_ISUPPORTS(txTransformNotifier,
                  nsIScriptLoaderObserver,
                  nsICSSLoaderObserver)

NS_IMETHODIMP
txTransformNotifier::ScriptAvailable(nsresult aResult, 
                                     nsIScriptElement *aElement, 
                                     bool aIsInline,
                                     nsIURI *aURI, 
                                     int32_t aLineNo)
{
    if (NS_FAILED(aResult) &&
        mScriptElements.RemoveObject(aElement)) {
        SignalTransformEnd();
    }

    return NS_OK;
}

NS_IMETHODIMP 
txTransformNotifier::ScriptEvaluated(nsresult aResult, 
                                     nsIScriptElement *aElement,
                                     bool aIsInline)
{
    if (mScriptElements.RemoveObject(aElement)) {
        SignalTransformEnd();
    }

    return NS_OK;
}

NS_IMETHODIMP 
txTransformNotifier::StyleSheetLoaded(CSSStyleSheet* aSheet,
                                      bool aWasAlternate,
                                      nsresult aStatus)
{
    if (mPendingStylesheetCount == 0) {
        
        
        
        return NS_OK;
    }

    
    if (!aWasAlternate) {
        --mPendingStylesheetCount;
        SignalTransformEnd();
    }
    
    return NS_OK;
}

void
txTransformNotifier::Init(nsITransformObserver* aObserver)
{
    mObserver = aObserver;
}

nsresult
txTransformNotifier::AddScriptElement(nsIScriptElement* aElement)
{
    return mScriptElements.AppendObject(aElement) ? NS_OK :
                                                    NS_ERROR_OUT_OF_MEMORY;
}

void
txTransformNotifier::AddPendingStylesheet()
{
    ++mPendingStylesheetCount;
}

void
txTransformNotifier::OnTransformEnd(nsresult aResult)
{
    mInTransform = false;
    SignalTransformEnd(aResult);
}

void
txTransformNotifier::OnTransformStart()
{
    mInTransform = true;
}

nsresult
txTransformNotifier::SetOutputDocument(nsIDocument* aDocument)
{
    mDocument = aDocument;

    
    return mObserver->OnDocumentCreated(mDocument);
}

void
txTransformNotifier::SignalTransformEnd(nsresult aResult)
{
    if (mInTransform ||
        (NS_SUCCEEDED(aResult) &&
         (mScriptElements.Count() > 0 || mPendingStylesheetCount > 0))) {
        return;
    }

    
    
    
    mPendingStylesheetCount = 0;
    mScriptElements.Clear();

    
    
    nsCOMPtr<nsIScriptLoaderObserver> kungFuDeathGrip(this);

    if (mDocument) {
        mDocument->ScriptLoader()->RemoveObserver(this);
        

        if (NS_FAILED(aResult)) {
            mDocument->CSSLoader()->Stop();
        }
    }

    if (NS_SUCCEEDED(aResult)) {
        mObserver->OnTransformDone(aResult, mDocument);
    }
}
