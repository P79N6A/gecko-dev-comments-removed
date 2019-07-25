






































#include "nsFileControlFrame.h"

#include "nsIContent.h"
#include "prtypes.h"
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
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMMouseListener.h"
#include "nsIPresShell.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsPIDOMWindow.h"
#include "nsIFilePicker.h"
#include "nsIDOMMouseEvent.h"
#include "nsINodeInfo.h"
#include "nsIDOMEventTarget.h"
#include "nsILocalFile.h"
#include "nsHTMLInputElement.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMEventGroup.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMHTMLInputElement.h"
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

#include "nsInterfaceHashtable.h"
#include "nsURIHashKey.h"
#include "nsILocalFile.h"
#include "nsNetCID.h"
#include "nsWeakReference.h"
#include "nsIVariant.h"
#include "mozilla/Services.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsHTMLInputElement.h"
#include "nsICapturePicker.h"
#include "nsIFileURL.h"
#include "nsDOMFile.h"
#include "nsEventStates.h"

#include "nsIDOMDOMStringList.h"
#include "nsIDOMDragEvent.h"

namespace dom = mozilla::dom;

#define SYNC_TEXT 0x1
#define SYNC_BUTTON 0x2

nsIFrame*
NS_NewFileControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFileControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsFileControlFrame)

nsFileControlFrame::nsFileControlFrame(nsStyleContext* aContext):
  nsBlockFrame(aContext),
  mTextFrame(nsnull)
{
  AddStateBits(NS_BLOCK_FLOAT_MGR);
}


NS_IMETHODIMP
nsFileControlFrame::Init(nsIContent* aContent,
                         nsIFrame*   aParent,
                         nsIFrame*   aPrevInFlow)
{
  nsresult rv = nsBlockFrame::Init(aContent, aParent, aPrevInFlow);
  NS_ENSURE_SUCCESS(rv, rv);

  mMouseListener = new BrowseMouseListener(this);
  NS_ENSURE_TRUE(mMouseListener, NS_ERROR_OUT_OF_MEMORY);
  mCaptureMouseListener = new CaptureMouseListener(this);
  NS_ENSURE_TRUE(mCaptureMouseListener, NS_ERROR_OUT_OF_MEMORY);

  return rv;
}

void
nsFileControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  mTextFrame = nsnull;
  ENSURE_TRUE(mContent);

  
  nsCOMPtr<nsIDOMEventTarget> dragTarget = do_QueryInterface(mContent);
  if (dragTarget) {
    dragTarget->RemoveEventListener(NS_LITERAL_STRING("drop"),
                                    mMouseListener, PR_FALSE);
    dragTarget->RemoveEventListener(NS_LITERAL_STRING("dragover"),
                                    mMouseListener, PR_FALSE);
  }

  
  NS_NAMED_LITERAL_STRING(click, "click");

  nsCOMPtr<nsIDOMEventGroup> systemGroup;
  mContent->GetSystemEventGroup(getter_AddRefs(systemGroup));

  nsCOMPtr<nsIDOM3EventTarget> dom3Capture = do_QueryInterface(mCapture);
  if (dom3Capture) {
    nsContentUtils::DestroyAnonymousContent(&mCapture);
  }

  nsCOMPtr<nsIDOM3EventTarget> dom3Browse = do_QueryInterface(mBrowse);
  if (dom3Browse) {
    dom3Browse->RemoveGroupedEventListener(click, mMouseListener, PR_FALSE,
                                           systemGroup);
    nsContentUtils::DestroyAnonymousContent(&mBrowse);
  }
  nsCOMPtr<nsIDOM3EventTarget> dom3TextContent =
    do_QueryInterface(mTextContent);
  if (dom3TextContent) {
    dom3TextContent->RemoveGroupedEventListener(click, mMouseListener, PR_FALSE,
                                                systemGroup);
    nsContentUtils::DestroyAnonymousContent(&mTextContent);
  }

  mCaptureMouseListener->ForgetFrame();
  mMouseListener->ForgetFrame();
  nsBlockFrame::DestroyFrom(aDestructRoot);
}

struct CaptureCallbackData {
  nsICapturePicker* picker;
  PRUint32* mode;
};

typedef struct CaptureCallbackData CaptureCallbackData;

PRBool CapturePickerAcceptCallback(const nsAString& aAccept, void* aClosure)
{
  nsresult rv;
  PRBool captureEnabled;
  CaptureCallbackData* closure = (CaptureCallbackData*)aClosure;

  if (StringBeginsWith(aAccept,
                       NS_LITERAL_STRING("image/"))) {
    rv = closure->picker->ModeMayBeAvailable(nsICapturePicker::MODE_STILL,
                                             &captureEnabled);
    NS_ENSURE_SUCCESS(rv, rv);
    if (captureEnabled) {
      *closure->mode = nsICapturePicker::MODE_STILL;
      return PR_FALSE;
    }
  } else if (StringBeginsWith(aAccept,
                              NS_LITERAL_STRING("audio/"))) {
    rv = closure->picker->ModeMayBeAvailable(nsICapturePicker::MODE_AUDIO_CLIP,
                                             &captureEnabled);
    NS_ENSURE_SUCCESS(rv, rv);
    if (captureEnabled) {
      *closure->mode = nsICapturePicker::MODE_AUDIO_CLIP;
      return PR_FALSE;
    }
  } else if (StringBeginsWith(aAccept,
                              NS_LITERAL_STRING("video/"))) {
    rv = closure->picker->ModeMayBeAvailable(nsICapturePicker::MODE_VIDEO_CLIP,
                                             &captureEnabled);
    NS_ENSURE_SUCCESS(rv, rv);
    if (captureEnabled) {
      *closure->mode = nsICapturePicker::MODE_VIDEO_CLIP;
      return PR_FALSE;
    }
    rv = closure->picker->ModeMayBeAvailable(nsICapturePicker::MODE_VIDEO_NO_SOUND_CLIP,
                                             &captureEnabled);
    NS_ENSURE_SUCCESS(rv, rv);
    if (captureEnabled) {
      *closure->mode = nsICapturePicker::MODE_VIDEO_NO_SOUND_CLIP;
      return PR_FALSE;;
    }
  }
  return PR_TRUE;
}

nsresult
nsFileControlFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::input, nsnull,
                                                 kNameSpaceID_XHTML,
                                                 nsIDOMNode::ELEMENT_NODE);

  
  NS_NewHTMLElement(getter_AddRefs(mTextContent), nodeInfo.forget(),
                    dom::NOT_FROM_PARSER);
  if (!mTextContent)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mTextContent->SetNativeAnonymous();

  mTextContent->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                        NS_LITERAL_STRING("text"), PR_FALSE);

  nsHTMLInputElement* inputElement =
    nsHTMLInputElement::FromContent(mContent);
  NS_ASSERTION(inputElement, "Why is our content not a <input>?");

  
  
  nsAutoString value;
  inputElement->GetDisplayFileName(value);

  nsCOMPtr<nsIDOMHTMLInputElement> textControl = do_QueryInterface(mTextContent);
  NS_ASSERTION(textControl, "Why is the <input> we created not a <input>?");
  textControl->SetValue(value);

  textControl->SetTabIndex(-1);
  textControl->SetReadOnly(PR_TRUE);

  if (!aElements.AppendElement(mTextContent))
    return NS_ERROR_OUT_OF_MEMORY;

  
  nsCOMPtr<nsIDOMEventTarget> dragTarget = do_QueryInterface(mContent);
  NS_ENSURE_STATE(dragTarget);
  dragTarget->AddEventListener(NS_LITERAL_STRING("drop"),
                               mMouseListener, PR_FALSE);
  dragTarget->AddEventListener(NS_LITERAL_STRING("dragover"),
                               mMouseListener, PR_FALSE);

  NS_NAMED_LITERAL_STRING(click, "click");
  nsCOMPtr<nsIDOMEventGroup> systemGroup;
  mContent->GetSystemEventGroup(getter_AddRefs(systemGroup));
  nsCOMPtr<nsIDOM3EventTarget> dom3TextContent =
    do_QueryInterface(mTextContent);
  NS_ENSURE_STATE(dom3TextContent);
  
  
  dom3TextContent->AddGroupedEventListener(click, mMouseListener, PR_FALSE,
                                           systemGroup);

  
  nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::input, nsnull,
                                                 kNameSpaceID_XHTML,
                                                 nsIDOMNode::ELEMENT_NODE);
  NS_NewHTMLElement(getter_AddRefs(mBrowse), nodeInfo.forget(),
                    dom::NOT_FROM_PARSER);
  if (!mBrowse)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mBrowse->SetNativeAnonymous();

  mBrowse->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                   NS_LITERAL_STRING("button"), PR_FALSE);

  
  nsCOMPtr<nsICapturePicker> capturePicker;
  capturePicker = do_GetService("@mozilla.org/capturepicker;1");
  if (capturePicker) {
    PRUint32 mode = 0;

    CaptureCallbackData data;
    data.picker = capturePicker;
    data.mode = &mode;
    ParseAcceptAttribute(&CapturePickerAcceptCallback, (void*)&data);

    if (mode != 0) {
      mCaptureMouseListener->mMode = mode;
      nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::input, nsnull,
                                                     kNameSpaceID_XHTML,
                                                     nsIDOMNode::ELEMENT_NODE);
      NS_NewHTMLElement(getter_AddRefs(mCapture), nodeInfo.forget(),
                        dom::NOT_FROM_PARSER);
      if (!mCapture)
        return NS_ERROR_OUT_OF_MEMORY;

      
      mCapture->SetNativeAnonymous();

      mCapture->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                        NS_LITERAL_STRING("button"), PR_FALSE);

      mCapture->SetAttr(kNameSpaceID_None, nsGkAtoms::value,
                        NS_LITERAL_STRING("capture"), PR_FALSE);

      nsCOMPtr<nsIDOMEventTarget> captureEventTarget =
        do_QueryInterface(mCapture);
      captureEventTarget->AddEventListener(click, mCaptureMouseListener, PR_FALSE);
    }
  }
  nsCOMPtr<nsIDOMHTMLInputElement> fileContent = do_QueryInterface(mContent);
  nsCOMPtr<nsIDOMHTMLInputElement> browseControl = do_QueryInterface(mBrowse);
  if (fileContent && browseControl) {
    PRInt32 tabIndex;
    nsAutoString accessKey;

    fileContent->GetAccessKey(accessKey);
    browseControl->SetAccessKey(accessKey);
    fileContent->GetTabIndex(&tabIndex);
    browseControl->SetTabIndex(tabIndex);
  }

  if (!aElements.AppendElement(mBrowse))
    return NS_ERROR_OUT_OF_MEMORY;

  if (mCapture && !aElements.AppendElement(mCapture))
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIDOM3EventTarget> dom3Browse = do_QueryInterface(mBrowse);
  NS_ENSURE_STATE(dom3Browse);
  
  
  dom3Browse->AddGroupedEventListener(click, mMouseListener, PR_FALSE,
                                      systemGroup);

  SyncAttr(kNameSpaceID_None, nsGkAtoms::size,     SYNC_TEXT);
  SyncDisabledState();

  return NS_OK;
}

void
nsFileControlFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                             PRUint32 aFilter)
{
  aElements.MaybeAppendElement(mTextContent);
  aElements.MaybeAppendElement(mBrowse);
  aElements.MaybeAppendElement(mCapture);
}

NS_QUERYFRAME_HEAD(nsFileControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

void 
nsFileControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
}

PRBool ShouldProcessMouseClick(nsIDOMEvent* aMouseEvent)
{
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  nsCOMPtr<nsIDOMNSUIEvent> uiEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_STATE(uiEvent);
  PRBool defaultPrevented = PR_FALSE;
  uiEvent->GetPreventDefault(&defaultPrevented);
  if (defaultPrevented) {
    return PR_FALSE;
  }

  PRUint16 whichButton;
  if (NS_FAILED(mouseEvent->GetButton(&whichButton)) || whichButton != 0) {
    return PR_FALSE;
  }

  PRInt32 clickCount;
  if (NS_FAILED(mouseEvent->GetDetail(&clickCount)) || clickCount > 1) {
    return PR_FALSE;
  }

  return PR_TRUE;
}




NS_IMETHODIMP
nsFileControlFrame::CaptureMouseListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  nsresult rv;

  NS_ASSERTION(mFrame, "We should have been unregistered");
  if (!ShouldProcessMouseClick(aMouseEvent))
    return NS_OK;

  
  nsIContent* content = mFrame->GetContent();
  if (!content)
    return NS_ERROR_FAILURE;

  nsHTMLInputElement* inputElement = nsHTMLInputElement::FromContent(content);
  if (!inputElement)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc = content->GetDocument();
  if (!doc)
    return NS_ERROR_FAILURE;

  
  nsXPIDLString title;
  nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                     "FileUpload", title);

  nsPIDOMWindow* win = doc->GetWindow();
  if (!win) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsICapturePicker> capturePicker;
  capturePicker = do_CreateInstance("@mozilla.org/capturepicker;1");
  if (!capturePicker)
    return NS_ERROR_FAILURE;

  rv = capturePicker->Init(win, title, mMode);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mFrame->mTextFrame->InitFocusedValue();

  
  PRUint32 result;
  rv = capturePicker->Show(&result);
  NS_ENSURE_SUCCESS(rv, rv);
  if (result == nsICapturePicker::RETURN_CANCEL)
    return NS_OK;

  if (!mFrame) {
    
    
    
    
    return NS_OK;
  }

  nsCOMPtr<nsIDOMFile> domFile;
  rv = capturePicker->GetFile(getter_AddRefs(domFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMArray<nsIDOMFile> newFiles;
  if (domFile) {
    newFiles.AppendObject(domFile);
  } else {
    return NS_ERROR_FAILURE;
  }

  
  
  
  if (newFiles.Count()) {
    
    
    
    PRBool oldState = mFrame->mTextFrame->GetFireChangeEventState();
    mFrame->mTextFrame->SetFireChangeEventState(PR_TRUE);
    inputElement->SetFiles(newFiles, true);

    mFrame->mTextFrame->SetFireChangeEventState(oldState);
    
    mFrame->mTextFrame->CheckFireOnChange();
  }

  return NS_OK;
}




NS_IMETHODIMP
nsFileControlFrame::BrowseMouseListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(mFrame, "We should have been unregistered");
  if (!ShouldProcessMouseClick(aMouseEvent))
    return NS_OK;
  
  nsHTMLInputElement* input =
    nsHTMLInputElement::FromContent(mFrame->GetContent());
  return input ? input->FireAsyncClickHandler() : NS_OK;
}






NS_IMETHODIMP
nsFileControlFrame::BrowseMouseListener::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ASSERTION(mFrame, "We should have been unregistered");
  nsCOMPtr<nsIDOMNSUIEvent> uiEvent = do_QueryInterface(aEvent);
  NS_ENSURE_STATE(uiEvent);
  PRBool defaultPrevented = PR_FALSE;
  uiEvent->GetPreventDefault(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }
  
  nsCOMPtr<nsIDOMDragEvent> dragEvent = do_QueryInterface(aEvent);
  if (!dragEvent || !IsValidDropData(dragEvent)) {
    return NS_OK;
  }

  nsAutoString eventType;
  aEvent->GetType(eventType);
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

    PRBool oldState = mFrame->mTextFrame->GetFireChangeEventState();
    mFrame->mTextFrame->SetFireChangeEventState(PR_TRUE);
    inputElement->SetFiles(fileList, true);
    mFrame->mTextFrame->SetFireChangeEventState(oldState);
    mFrame->mTextFrame->CheckFireOnChange();
  }

  return NS_OK;
}

 PRBool
nsFileControlFrame::BrowseMouseListener::IsValidDropData(nsIDOMDragEvent* aEvent)
{
  nsCOMPtr<nsIDOMDataTransfer> dataTransfer;
  aEvent->GetDataTransfer(getter_AddRefs(dataTransfer));
  NS_ENSURE_TRUE(dataTransfer, PR_FALSE);

  nsCOMPtr<nsIDOMDOMStringList> types;
  dataTransfer->GetTypes(getter_AddRefs(types));
  NS_ENSURE_TRUE(types, PR_FALSE);

  
  PRBool typeSupported;
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

NS_IMETHODIMP nsFileControlFrame::Reflow(nsPresContext*          aPresContext, 
                                         nsHTMLReflowMetrics&     aDesiredSize,
                                         const nsHTMLReflowState& aReflowState, 
                                         nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsFileControlFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  aStatus = NS_FRAME_COMPLETE;

  if (mState & NS_FRAME_FIRST_REFLOW) {
    mTextFrame = GetTextControlFrame(aPresContext, this);
    NS_ENSURE_TRUE(mTextFrame, NS_ERROR_UNEXPECTED);
  }

  
  return nsBlockFrame::Reflow(aPresContext, aDesiredSize, aReflowState,
                             aStatus);
}

nsNewFrame*
nsFileControlFrame::GetTextControlFrame(nsPresContext* aPresContext, nsIFrame* aStart)
{
  nsNewFrame* result = nsnull;
#ifndef DEBUG_NEWFRAME
  
  nsIFrame* childFrame = aStart->GetFirstChild(nsnull);

  while (childFrame) {
    
    nsCOMPtr<nsIFormControl> formCtrl =
      do_QueryInterface(childFrame->GetContent());

    if (formCtrl && formCtrl->GetType() == NS_FORM_INPUT_TEXT) {
      result = (nsNewFrame*)childFrame;
    }

    
    nsNewFrame* frame = GetTextControlFrame(aPresContext, childFrame);
    if (frame)
       result = frame;
     
    childFrame = childFrame->GetNextSibling();
  }

  return result;
#else
  return nsnull;
#endif
}

PRIntn
nsFileControlFrame::GetSkipSides() const
{
  return 0;
}

void
nsFileControlFrame::SyncAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRInt32 aWhichControls)
{
  nsAutoString value;
  if (mContent->GetAttr(aNameSpaceID, aAttribute, value)) {
    if (aWhichControls & SYNC_TEXT && mTextContent) {
      mTextContent->SetAttr(aNameSpaceID, aAttribute, value, PR_TRUE);
    }
    if (aWhichControls & SYNC_BUTTON && mBrowse) {
      mBrowse->SetAttr(aNameSpaceID, aAttribute, value, PR_TRUE);
    }
  } else {
    if (aWhichControls & SYNC_TEXT && mTextContent) {
      mTextContent->UnsetAttr(aNameSpaceID, aAttribute, PR_TRUE);
    }
    if (aWhichControls & SYNC_BUTTON && mBrowse) {
      mBrowse->UnsetAttr(aNameSpaceID, aAttribute, PR_TRUE);
    }
  }
}

void
nsFileControlFrame::SyncDisabledState()
{
  nsEventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
    mTextContent->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled, EmptyString(),
                          PR_TRUE);
    mBrowse->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled, EmptyString(),
                     PR_TRUE);
  } else {
    mTextContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, PR_TRUE);
    mBrowse->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, PR_TRUE);
  }
}

NS_IMETHODIMP
nsFileControlFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::size) {
      SyncAttr(aNameSpaceID, aAttribute, SYNC_TEXT);
    } else if (aAttribute == nsGkAtoms::tabindex) {
      SyncAttr(aNameSpaceID, aAttribute, SYNC_BUTTON);
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

PRBool
nsFileControlFrame::IsLeaf() const
{
  return PR_TRUE;
}

#ifdef NS_DEBUG
NS_IMETHODIMP
nsFileControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("FileControl"), aResult);
}
#endif

nsresult
nsFileControlFrame::SetFormProperty(nsIAtom* aName,
                                    const nsAString& aValue)
{
  if (nsGkAtoms::value == aName) {
    nsCOMPtr<nsIDOMHTMLInputElement> textControl =
      do_QueryInterface(mTextContent);
    NS_ASSERTION(textControl,
                 "The text control should exist and be an input element");
    textControl->SetValue(aValue);
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

NS_IMETHODIMP
nsFileControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  if (GetStyleBorder()->mBoxShadow) {
    nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBoxShadowOuter(aBuilder, this));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  nsDisplayListCollection tempList;
  nsresult rv = nsBlockFrame::BuildDisplayList(aBuilder, aDirtyRect, tempList);
  if (NS_FAILED(rv))
    return rv;

  tempList.BorderBackground()->DeleteAll();

  
  nsRect clipRect(aBuilder->ToReferenceFrame(this), GetSize());
  clipRect.width = GetVisualOverflowRect().XMost();
  nscoord radii[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  rv = OverflowClip(aBuilder, tempList, aLists, clipRect, radii);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsEventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED) && IsVisibleForPainting(aBuilder)) {
    rv = aLists.Content()->AppendNewToTop(
        new (aBuilder) nsDisplayEventReceiver(aBuilder, this));
    if (NS_FAILED(rv))
      return rv;
  }

  return DisplaySelectionOverlay(aBuilder, aLists.Content());
}

#ifdef ACCESSIBILITY
already_AddRefed<nsAccessible>
nsFileControlFrame::CreateAccessible()
{
  
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (!accService)
    return nsnull;

  return accService->CreateHyperTextAccessible(mContent,
                                               PresContext()->PresShell());
}
#endif

void 
nsFileControlFrame::ParseAcceptAttribute(AcceptAttrCallback aCallback,
                                         void* aClosure) const
{
  nsAutoString accept;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accept, accept);

  nsCharSeparatedTokenizerTemplate<nsContentUtils::IsHTMLWhitespace>
    tokenizer(accept, ',');
  
  while (tokenizer.hasMoreTokens() &&
         (*aCallback)(tokenizer.nextToken(), aClosure));
}




NS_IMPL_ISUPPORTS2(nsFileControlFrame::MouseListener,
                   nsIDOMMouseListener,
                   nsIDOMEventListener)
