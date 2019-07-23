





































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
#include "nsIDOMHTMLInputElement.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsPIDOMWindow.h"
#include "nsIFilePicker.h"
#include "nsIDOMMouseEvent.h"
#include "nsINodeInfo.h"
#include "nsIDOMEventReceiver.h"
#include "nsILocalFile.h"
#include "nsIFileControlElement.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"

#define SYNC_TEXT 0x1
#define SYNC_BUTTON 0x2
#define SYNC_BOTH 0x3

nsIFrame*
NS_NewFileControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFileControlFrame(aContext);
}

nsFileControlFrame::nsFileControlFrame(nsStyleContext* aContext):
  nsAreaFrame(aContext),
  mTextFrame(nsnull), 
  mCachedState(nsnull)
{
}

nsFileControlFrame::~nsFileControlFrame()
{
  if (mCachedState) {
    delete mCachedState;
    mCachedState = nsnull;
  }
}

NS_IMETHODIMP
nsFileControlFrame::Init(nsIContent* aContent,
                         nsIFrame*   aParent,
                         nsIFrame*   aPrevInFlow)
{
  nsresult rv = nsAreaFrame::Init(aContent, aParent, aPrevInFlow);
  NS_ENSURE_SUCCESS(rv, rv);

  mMouseListener = new MouseListener(this);
  NS_ENSURE_TRUE(mMouseListener, NS_ERROR_OUT_OF_MEMORY);

  return rv;
}

void
nsFileControlFrame::Destroy()
{
  mTextFrame = nsnull;
  
  if (mBrowse) {
    nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(mBrowse));
    receiver->RemoveEventListenerByIID(mMouseListener,
                                       NS_GET_IID(nsIDOMMouseListener));
    nsContentUtils::DestroyAnonymousContent(&mBrowse);
  }
  if (mTextContent) {
    nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(mTextContent));
    receiver->RemoveEventListenerByIID(mMouseListener,
                                       NS_GET_IID(nsIDOMMouseListener));
    nsContentUtils::DestroyAnonymousContent(&mTextContent);
  }

  mMouseListener->ForgetFrame();
  nsAreaFrame::Destroy();
}

nsresult
nsFileControlFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();

  nsCOMPtr<nsINodeInfo> nodeInfo;
  doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::input, nsnull,
                                      kNameSpaceID_None,
                                      getter_AddRefs(nodeInfo));

  
  NS_NewHTMLElement(getter_AddRefs(mTextContent), nodeInfo);
  if (!mTextContent)
    return NS_ERROR_OUT_OF_MEMORY;

  mTextContent->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                        NS_LITERAL_STRING("text"), PR_FALSE);

  nsCOMPtr<nsIDOMHTMLInputElement> textControl = do_QueryInterface(mTextContent);
  if (textControl) {
    nsCOMPtr<nsIFileControlElement> fileControl = do_QueryInterface(mContent);
    if (fileControl) {
      
      
      nsAutoString value;
      fileControl->GetFileName(value);
      textControl->SetValue(value);
    }

    textControl->SetTabIndex(-1);
    textControl->SetDisabled(PR_TRUE);
    textControl->SetReadOnly(PR_TRUE);
  }

  if (!aElements.AppendElement(mTextContent))
    return NS_ERROR_OUT_OF_MEMORY;

  
  nsCOMPtr<nsIDOMEventReceiver> receiver = do_QueryInterface(mTextContent);
  receiver->AddEventListenerByIID(mMouseListener,
                                  NS_GET_IID(nsIDOMMouseListener));

  
  NS_NewHTMLElement(getter_AddRefs(mBrowse), nodeInfo);
  if (!mBrowse)
    return NS_ERROR_OUT_OF_MEMORY;

  mBrowse->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                   NS_LITERAL_STRING("button"), PR_FALSE);
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

  
  receiver = do_QueryInterface(mBrowse);
  receiver->AddEventListenerByIID(mMouseListener,
                                  NS_GET_IID(nsIDOMMouseListener));

  SyncAttr(kNameSpaceID_None, nsGkAtoms::size,     SYNC_TEXT);
  SyncAttr(kNameSpaceID_None, nsGkAtoms::disabled, SYNC_BOTH);

  return NS_OK;
}


NS_IMETHODIMP
nsFileControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null ptr");
  if (NS_UNLIKELY(!aInstancePtr)) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIAnonymousContentCreator))) {
    *aInstancePtr = NS_STATIC_CAST(nsIAnonymousContentCreator*, this);
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIFormControlFrame))) {
    *aInstancePtr = NS_STATIC_CAST(nsIFormControlFrame*, this);
    return NS_OK;
  }
  return nsAreaFrame::QueryInterface(aIID, aInstancePtr);
}

void 
nsFileControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
  
  if (mTextFrame) {
    nsIContent* content = mTextFrame->GetContent();
    if (content) {
      content->SetFocus(GetPresContext());
    }
  }
}




nsresult 
nsFileControlFrame::MouseClick(nsIDOMEvent* aMouseEvent)
{
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  if (mouseEvent) {
    PRUint16 whichButton;
    if (NS_SUCCEEDED(mouseEvent->GetButton(&whichButton))) {
      if (whichButton != 0) {
        return NS_OK;
      }
    }
  }


  nsresult result;

  
  nsIContent* content = GetContent();
  if (!content)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc = content->GetDocument();
  if (!doc)
    return NS_ERROR_FAILURE;

  
  nsXPIDLString title;
  nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                     "FileUpload", title);

  nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1");
  if (!filePicker)
    return NS_ERROR_FAILURE;

  result = filePicker->Init(doc->GetWindow(), title, nsIFilePicker::modeOpen);
  if (NS_FAILED(result))
    return result;

  
  filePicker->AppendFilters(nsIFilePicker::filterAll);

  
  nsAutoString defaultName;
  GetFormProperty(nsGkAtoms::value, defaultName);

  nsCOMPtr<nsILocalFile> currentFile = do_CreateInstance("@mozilla.org/file/local;1");
  if (currentFile && !defaultName.IsEmpty()) {
    result = currentFile->InitWithPath(defaultName);
    if (NS_SUCCEEDED(result)) {
      nsAutoString leafName;
      currentFile->GetLeafName(leafName);
      if (!leafName.IsEmpty()) {
        filePicker->SetDefaultString(leafName);
      }

      
      nsCOMPtr<nsIFile> parentFile;
      currentFile->GetParent(getter_AddRefs(parentFile));
      if (parentFile) {
        nsCOMPtr<nsILocalFile> parentLocalFile = do_QueryInterface(parentFile, &result);
        if (parentLocalFile)
          filePicker->SetDisplayDirectory(parentLocalFile);
      }
    }
  }

  
  mTextFrame->InitFocusedValue();

  
  PRInt16 mode;
  result = filePicker->Show(&mode);
  if (NS_FAILED(result))
    return result;
  if (mode == nsIFilePicker::returnCancel)
    return NS_OK;

  if (!mTextFrame) {
    
    return NS_OK;
  }
  
  
  nsCOMPtr<nsILocalFile> localFile;
  result = filePicker->GetFile(getter_AddRefs(localFile));
  if (localFile) {
    nsAutoString unicodePath;
    result = localFile->GetPath(unicodePath);
    if (!unicodePath.IsEmpty()) {
      
      
      
      PRBool oldState = mTextFrame->GetFireChangeEventState();
      mTextFrame->SetFireChangeEventState(PR_TRUE);
      mTextFrame->SetFormProperty(nsGkAtoms::value, unicodePath);
      mTextFrame->SetFireChangeEventState(oldState);
      nsCOMPtr<nsIFileControlElement> fileControl = do_QueryInterface(mContent);
      if (fileControl) {
        fileControl->SetFileName(unicodePath);
      }
      
      
      mTextFrame->CheckFireOnChange();
      return NS_OK;
    }
  }

  return NS_FAILED(result) ? result : NS_ERROR_FAILURE;
}

nscoord
nsFileControlFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
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
    if (mCachedState) {
      mTextFrame->SetFormProperty(nsGkAtoms::value, *mCachedState);
      delete mCachedState;
      mCachedState = nsnull;
    }
  }

  
  
  
  
  nsresult rv = nsAreaFrame::Reflow(aPresContext, aDesiredSize, aReflowState,
                                    aStatus);
  if (NS_SUCCEEDED(rv) && mTextFrame != nsnull) {
    nsIFrame* child = GetFirstChild(nsnull);
    if (child == mTextFrame) {
      child = child->GetNextSibling();
    }
    if (child) {
      nsRect buttonRect = child->GetRect();
      nsRect txtRect = mTextFrame->GetRect();

      
      
      
      
      if (txtRect.width + buttonRect.width != aDesiredSize.width ||
          txtRect.height != aDesiredSize.height) {
        nsHTMLReflowMetrics txtKidSize;
        nsSize txtAvailSize(aReflowState.availableWidth, aDesiredSize.height);
        nsHTMLReflowState   txtKidReflowState(aPresContext,
                                              *aReflowState.parentReflowState,
                                              this, txtAvailSize);
        txtKidReflowState.mComputedHeight = aDesiredSize.height;
        rv = nsAreaFrame::WillReflow(aPresContext);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Should have succeeded");
        rv = nsAreaFrame::Reflow(aPresContext, txtKidSize, txtKidReflowState, aStatus);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Should have succeeded");
        rv = nsAreaFrame::DidReflow(aPresContext, &txtKidReflowState, aStatus);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Should have succeeded");

        
        txtRect        = mTextFrame->GetRect();
        txtRect.y      = aReflowState.mComputedBorderPadding.top;
        txtRect.height = aDesiredSize.height;
        mTextFrame->SetRect(txtRect);
      }
    }
  }
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
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

NS_IMETHODIMP
nsFileControlFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::disabled) {
    SyncAttr(aNameSpaceID, aAttribute, SYNC_BOTH);
  
  } else if (aNameSpaceID == kNameSpaceID_None &&
             aAttribute == nsGkAtoms::size) {
    SyncAttr(aNameSpaceID, aAttribute, SYNC_TEXT);
  }

  return nsAreaFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
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
    if (mTextFrame) {
      mTextFrame->SetValue(aValue);
    } else {
      if (mCachedState) delete mCachedState;
      mCachedState = new nsString(aValue);
      NS_ENSURE_TRUE(mCachedState, NS_ERROR_OUT_OF_MEMORY);
    }
  }
  return NS_OK;
}      

nsresult
nsFileControlFrame::GetFormProperty(nsIAtom* aName, nsAString& aValue) const
{
  aValue.Truncate();  

  if (nsGkAtoms::value == aName) {
    NS_ASSERTION(!mCachedState || !mTextFrame,
                 "If we have a cached state, we better have no mTextFrame");
    if (mCachedState) {
      aValue.Assign(*mCachedState);
    } else if (mTextContent) {
      nsCOMPtr<nsIFileControlElement> fileControl = do_QueryInterface(mTextContent);
      if (fileControl) {
        fileControl->GetFileName(aValue);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFileControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  
  
  
  nsDisplayListCollection tempList;
  nsresult rv = nsAreaFrame::BuildDisplayList(aBuilder, aDirtyRect, tempList);
  if (NS_FAILED(rv))
    return rv;

  tempList.BorderBackground()->DeleteAll();
  tempList.MoveTo(aLists);
  
  
  
  
  
  
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled) && 
      IsVisibleForPainting(aBuilder)) {
    nsDisplayItem* item = new (aBuilder) nsDisplayEventReceiver(this);
    if (!item)
      return NS_ERROR_OUT_OF_MEMORY;
    aLists.Content()->AppendToTop(item);
  }

  return DisplaySelectionOverlay(aBuilder, aLists);
}




NS_IMPL_ISUPPORTS1(nsFileControlFrame::MouseListener, nsIDOMMouseListener)

NS_IMETHODIMP
nsFileControlFrame::MouseListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  if (mFrame) {
    return mFrame->MouseClick(aMouseEvent);
  }

  return NS_OK;
}
