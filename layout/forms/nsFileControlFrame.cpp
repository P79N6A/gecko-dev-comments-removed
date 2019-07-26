




#include "nsFileControlFrame.h"

#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsWidgetsCID.h"
#include "nsIComponentManager.h"
#include "nsHTMLParts.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIFormControl.h"
#include "nsINameSpaceManager.h"
#include "nsCOMPtr.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsPIDOMWindow.h"
#include "nsIFilePicker.h"
#include "nsIDOMMouseEvent.h"
#include "nsINodeInfo.h"
#include "nsIDOMEventTarget.h"
#include "nsIFile.h"
#include "nsHTMLInputElement.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsEventListenerManager.h"

#include "nsInterfaceHashtable.h"
#include "nsURIHashKey.h"
#include "nsNetCID.h"
#include "nsWeakReference.h"
#include "nsIVariant.h"
#include "mozilla/Services.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDOMFile.h"
#include "nsEventStates.h"
#include "nsTextControlFrame.h"

#include "nsIDOMDOMStringList.h"
#include "nsIDOMDragEvent.h"
#include "nsContentList.h"
#include "nsIDOMMutationEvent.h"

using namespace mozilla;

#define SYNC_TEXT 0x1
#define SYNC_BUTTON 0x2

nsIFrame*
NS_NewFileControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFileControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsFileControlFrame)

nsFileControlFrame::nsFileControlFrame(nsStyleContext* aContext):
  nsBlockFrame(aContext)
{
  AddStateBits(NS_BLOCK_FLOAT_MGR);
}


void
nsFileControlFrame::Init(nsIContent* aContent,
                         nsIFrame*   aParent,
                         nsIFrame*   aPrevInFlow)
{
  nsBlockFrame::Init(aContent, aParent, aPrevInFlow);

  mMouseListener = new BrowseMouseListener(this);
}

void
nsFileControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  ENSURE_TRUE(mContent);

  
  if (mContent) {
    mContent->RemoveSystemEventListener(NS_LITERAL_STRING("drop"),
                                        mMouseListener, false);
    mContent->RemoveSystemEventListener(NS_LITERAL_STRING("dragover"),
                                        mMouseListener, false);
  }

  

  if (mBrowse) {
    mBrowse->RemoveSystemEventListener(NS_LITERAL_STRING("click"),
                                       mMouseListener, false);
  }
  nsContentUtils::DestroyAnonymousContent(&mBrowse);

  if (mTextContent) {
    mTextContent->RemoveSystemEventListener(NS_LITERAL_STRING("click"),
                                            mMouseListener, false);
  }
  nsContentUtils::DestroyAnonymousContent(&mTextContent);

  mMouseListener->ForgetFrame();
  nsBlockFrame::DestroyFrom(aDestructRoot);
}

nsresult
nsFileControlFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::label, nullptr,
                                                 kNameSpaceID_XUL,
                                                 nsIDOMNode::ELEMENT_NODE);

  
  NS_TrustedNewXULElement(getter_AddRefs(mTextContent), nodeInfo.forget());
  if (!mTextContent)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mTextContent->SetNativeAnonymous();

  mTextContent->SetAttr(kNameSpaceID_None, nsGkAtoms::crop,
                        NS_LITERAL_STRING("center"), false);

  nsHTMLInputElement* inputElement =
    nsHTMLInputElement::FromContent(mContent);
  NS_ASSERTION(inputElement, "Why is our content not a <input>?");

  
  
  nsAutoString value;
  inputElement->GetDisplayFileName(value);
  UpdateDisplayedValue(value, false);

  if (!aElements.AppendElement(mTextContent))
    return NS_ERROR_OUT_OF_MEMORY;

  
  mContent->AddSystemEventListener(NS_LITERAL_STRING("drop"),
                                   mMouseListener, false);
  mContent->AddSystemEventListener(NS_LITERAL_STRING("dragover"),
                                   mMouseListener, false);

  
  
  mTextContent->AddSystemEventListener(NS_LITERAL_STRING("click"),
                                       mMouseListener, false);

  
  nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::input, nullptr,
                                                 kNameSpaceID_XHTML,
                                                 nsIDOMNode::ELEMENT_NODE);
  NS_NewHTMLElement(getter_AddRefs(mBrowse), nodeInfo.forget(),
                    dom::NOT_FROM_PARSER);
  if (!mBrowse)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mBrowse->SetNativeAnonymous();

  mBrowse->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                   NS_LITERAL_STRING("button"), false);

  nsCOMPtr<nsIDOMHTMLInputElement> fileContent = do_QueryInterface(mContent);
  nsCOMPtr<nsIDOMHTMLInputElement> browseControl = do_QueryInterface(mBrowse);
  if (fileContent && browseControl) {
    int32_t tabIndex;
    nsAutoString accessKey;

    fileContent->GetAccessKey(accessKey);
    browseControl->SetAccessKey(accessKey);
    fileContent->GetTabIndex(&tabIndex);
    browseControl->SetTabIndex(tabIndex);
  }

  if (!aElements.AppendElement(mBrowse))
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  mBrowse->AddSystemEventListener(NS_LITERAL_STRING("click"),
                                  mMouseListener, false);

  SyncDisabledState();

  return NS_OK;
}

void
nsFileControlFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                             uint32_t aFilter)
{
  aElements.MaybeAppendElement(mTextContent);
  aElements.MaybeAppendElement(mBrowse);
}

NS_QUERYFRAME_HEAD(nsFileControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

void 
nsFileControlFrame::SetFocus(bool aOn, bool aRepaint)
{
}

bool ShouldProcessMouseClick(nsIDOMEvent* aMouseEvent)
{
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_TRUE(mouseEvent, false);
  bool defaultPrevented = false;
  aMouseEvent->GetPreventDefault(&defaultPrevented);
  if (defaultPrevented) {
    return false;
  }

  uint16_t whichButton;
  if (NS_FAILED(mouseEvent->GetButton(&whichButton)) || whichButton != 0) {
    return false;
  }

  int32_t clickCount;
  if (NS_FAILED(mouseEvent->GetDetail(&clickCount)) || clickCount > 1) {
    return false;
  }

  return true;
}





NS_IMETHODIMP
nsFileControlFrame::BrowseMouseListener::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ASSERTION(mFrame, "We should have been unregistered");

  nsAutoString eventType;
  aEvent->GetType(eventType);
  if (eventType.EqualsLiteral("click")) {
    if (!ShouldProcessMouseClick(aEvent))
      return NS_OK;
    
    nsHTMLInputElement* input =
      nsHTMLInputElement::FromContent(mFrame->GetContent());
    return input ? input->FireAsyncClickHandler() : NS_OK;
  }

  bool defaultPrevented = false;
  aEvent->GetPreventDefault(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }
  
  nsCOMPtr<nsIDOMDragEvent> dragEvent = do_QueryInterface(aEvent);
  if (!dragEvent || !IsValidDropData(dragEvent)) {
    return NS_OK;
  }

  if (eventType.EqualsLiteral("dragover")) {
    
    aEvent->PreventDefault();
    return NS_OK;
  }

  if (eventType.EqualsLiteral("drop")) {
    aEvent->StopPropagation();
    aEvent->PreventDefault();

    nsIContent* content = mFrame->GetContent();
    NS_ASSERTION(content, "The frame has no content???");

    nsHTMLInputElement* inputElement = nsHTMLInputElement::FromContent(content);
    NS_ASSERTION(inputElement, "No input element for this file upload control frame!");

    nsCOMPtr<nsIDOMDataTransfer> dataTransfer;
    dragEvent->GetDataTransfer(getter_AddRefs(dataTransfer));

    nsCOMPtr<nsIDOMFileList> fileList;
    dataTransfer->GetFiles(getter_AddRefs(fileList));

    inputElement->SetFiles(fileList, true);
    nsContentUtils::DispatchTrustedEvent(content->OwnerDoc(), content,
                                         NS_LITERAL_STRING("change"), true,
                                         false);
  }

  return NS_OK;
}

 bool
nsFileControlFrame::BrowseMouseListener::IsValidDropData(nsIDOMDragEvent* aEvent)
{
  nsCOMPtr<nsIDOMDataTransfer> dataTransfer;
  aEvent->GetDataTransfer(getter_AddRefs(dataTransfer));
  NS_ENSURE_TRUE(dataTransfer, false);

  nsCOMPtr<nsIDOMDOMStringList> types;
  dataTransfer->GetTypes(getter_AddRefs(types));
  NS_ENSURE_TRUE(types, false);

  
  bool typeSupported;
  types->Contains(NS_LITERAL_STRING("Files"), &typeSupported);
  return typeSupported;
}

nscoord
nsFileControlFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  
  result = GetPrefWidth(aRenderingContext);
  return result;
}

void
nsFileControlFrame::SyncDisabledState()
{
  nsEventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
    mBrowse->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled, EmptyString(),
                     true);
  } else {
    mBrowse->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, true);
  }
}

NS_IMETHODIMP
nsFileControlFrame::AttributeChanged(int32_t  aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     int32_t  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::tabindex) {
    if (aModType == nsIDOMMutationEvent::REMOVAL) {
      mBrowse->UnsetAttr(aNameSpaceID, aAttribute, true);
    } else {
      nsAutoString value;
      mContent->GetAttr(aNameSpaceID, aAttribute, value);
      mBrowse->SetAttr(aNameSpaceID, aAttribute, value, true);
    }
  }

  return nsBlockFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

void
nsFileControlFrame::ContentStatesChanged(nsEventStates aStates)
{
  if (aStates.HasState(NS_EVENT_STATE_DISABLED)) {
    nsContentUtils::AddScriptRunner(new SyncDisabledStateEvent(this));
  }
}

bool
nsFileControlFrame::IsLeaf() const
{
  return true;
}

#ifdef DEBUG
NS_IMETHODIMP
nsFileControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("FileControl"), aResult);
}
#endif

void
nsFileControlFrame::UpdateDisplayedValue(const nsAString& aValue, bool aNotify)
{
  nsXPIDLString value;
  if (aValue.IsEmpty()) {
    if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)) {
      nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                         "NoFilesSelected", value);
    } else {
      nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                         "NoFileSelected", value);
    }
  } else {
    value = aValue;
  }
  mTextContent->SetAttr(kNameSpaceID_None, nsGkAtoms::value, value, aNotify);
}

nsresult
nsFileControlFrame::SetFormProperty(nsIAtom* aName,
                                    const nsAString& aValue)
{
  if (nsGkAtoms::value == aName) {
    UpdateDisplayedValue(aValue, true);
  }
  return NS_OK;
}

nsresult
nsFileControlFrame::GetFormProperty(nsIAtom* aName, nsAString& aValue) const
{
  aValue.Truncate();  

  if (nsGkAtoms::value == aName) {
    nsHTMLInputElement* inputElement =
      nsHTMLInputElement::FromContent(mContent);

    if (inputElement) {
      inputElement->GetDisplayFileName(aValue);
    }
  }
  return NS_OK;
}

void
nsFileControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  if (StyleBorder()->mBoxShadow) {
    aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayBoxShadowOuter(aBuilder, this));
  }

  
  
  
  
  nsDisplayListCollection tempList;
  nsBlockFrame::BuildDisplayList(aBuilder, aDirtyRect, tempList);

  tempList.BorderBackground()->DeleteAll();

  
  nsRect clipRect(aBuilder->ToReferenceFrame(this), GetSize());
  clipRect.width = GetVisualOverflowRect().XMost();
  nscoord radii[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  OverflowClip(aBuilder, tempList, aLists, clipRect, radii);

  
  
  
  nsEventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED) && IsVisibleForPainting(aBuilder)) {
    aLists.Content()->AppendNewToTop(
      new (aBuilder) nsDisplayEventReceiver(aBuilder, this));
  }

  DisplaySelectionOverlay(aBuilder, aLists.Content());
}

#ifdef ACCESSIBILITY
a11y::AccType
nsFileControlFrame::AccessibleType()
{
  return a11y::eHTMLFileInputType;
}
#endif




NS_IMPL_ISUPPORTS1(nsFileControlFrame::MouseListener,
                   nsIDOMEventListener)
