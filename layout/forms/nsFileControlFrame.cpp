






































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
#include "nsIDOMEventTarget.h"
#include "nsILocalFile.h"
#include "nsIFileControlElement.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMEventGroup.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMNSHTMLInputElement.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif

#include "nsInterfaceHashtable.h"
#include "nsURIHashKey.h"
#include "nsILocalFile.h"
#include "nsIPrivateBrowsingService.h"
#include "nsNetCID.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsWeakReference.h"
#include "nsIVariant.h"
#include "nsIContentPrefService.h"
#include "nsIContentURIGrouper.h"
#include "mozilla/Services.h"
#include "nsDirectoryServiceDefs.h"

#define SYNC_TEXT 0x1
#define SYNC_BUTTON 0x2
#define SYNC_BOTH 0x3

#define CPS_PREF_NAME NS_LITERAL_STRING("browser.upload.lastDir")

class UploadLastDir : public nsIObserver, public nsSupportsWeakReference {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  UploadLastDir();

  






  nsresult FetchLastUsedDirectory(nsIURI* aURI, nsILocalFile** aFile);

  






  nsresult StoreLastUsedDirectory(nsIURI* aURI, nsILocalFile* aFile);
private:
  
  nsInterfaceHashtable<nsStringHashKey, nsILocalFile> mUploadLastDirStore;
  PRBool mInPrivateBrowsing;
};

NS_IMPL_ISUPPORTS2(UploadLastDir, nsIObserver, nsISupportsWeakReference)
UploadLastDir* gUploadLastDir = nsnull;

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

  mMouseListener = new MouseListener(this);
  NS_ENSURE_TRUE(mMouseListener, NS_ERROR_OUT_OF_MEMORY);

  if (!gUploadLastDir)
    nsFileControlFrame::InitUploadLastDir();

  return rv;
}

void
nsFileControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  mTextFrame = nsnull;
  ENSURE_TRUE(mContent);

  
  NS_NAMED_LITERAL_STRING(click, "click");

  nsCOMPtr<nsIDOMEventGroup> systemGroup;
  mContent->GetSystemEventGroup(getter_AddRefs(systemGroup));

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

  mMouseListener->ForgetFrame();
  nsBlockFrame::DestroyFrom(aDestructRoot);
}

nsresult
nsFileControlFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::input, nsnull,
                                                 kNameSpaceID_XHTML);

  
  NS_NewHTMLElement(getter_AddRefs(mTextContent), nodeInfo, PR_FALSE);
  if (!mTextContent)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mTextContent->SetNativeAnonymous();

  mTextContent->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                        NS_LITERAL_STRING("text"), PR_FALSE);

  nsCOMPtr<nsIDOMHTMLInputElement> textControl = do_QueryInterface(mTextContent);
  if (textControl) {
    nsCOMPtr<nsIFileControlElement> fileControl = do_QueryInterface(mContent);
    if (fileControl) {
      
      
      nsAutoString value;
      fileControl->GetDisplayFileName(value);
      textControl->SetValue(value);
    }

    textControl->SetTabIndex(-1);
    textControl->SetReadOnly(PR_TRUE);
  }

  if (!aElements.AppendElement(mTextContent))
    return NS_ERROR_OUT_OF_MEMORY;

  NS_NAMED_LITERAL_STRING(click, "click");
  nsCOMPtr<nsIDOMEventGroup> systemGroup;
  mContent->GetSystemEventGroup(getter_AddRefs(systemGroup));
  nsCOMPtr<nsIDOM3EventTarget> dom3TextContent =
    do_QueryInterface(mTextContent);
  NS_ENSURE_STATE(dom3TextContent);
  
  
  dom3TextContent->AddGroupedEventListener(click, mMouseListener, PR_FALSE,
                                           systemGroup);

  
  NS_NewHTMLElement(getter_AddRefs(mBrowse), nodeInfo, PR_FALSE);
  if (!mBrowse)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mBrowse->SetNativeAnonymous();

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

  nsCOMPtr<nsIDOM3EventTarget> dom3Browse = do_QueryInterface(mBrowse);
  NS_ENSURE_STATE(dom3Browse);
  
  
  dom3Browse->AddGroupedEventListener(click, mMouseListener, PR_FALSE,
                                      systemGroup);

  SyncAttr(kNameSpaceID_None, nsGkAtoms::size,     SYNC_TEXT);
  SyncAttr(kNameSpaceID_None, nsGkAtoms::disabled, SYNC_BOTH);

  return NS_OK;
}

void
nsFileControlFrame::AppendAnonymousContentTo(nsBaseContentList& aElements)
{
  aElements.MaybeAppendElement(mTextContent);
  aElements.MaybeAppendElement(mBrowse);
}

NS_QUERYFRAME_HEAD(nsFileControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

void 
nsFileControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
}




NS_IMETHODIMP
nsFileControlFrame::MouseListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(mFrame, "We should have been unregistered");

  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  nsCOMPtr<nsIDOMNSUIEvent> uiEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_STATE(uiEvent);
  PRBool defaultPrevented = PR_FALSE;
  uiEvent->GetPreventDefault(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }

  PRUint16 whichButton;
  if (NS_FAILED(mouseEvent->GetButton(&whichButton)) || whichButton != 0) {
    return NS_OK;
  }

  PRInt32 clickCount;
  if (NS_FAILED(mouseEvent->GetDetail(&clickCount)) || clickCount > 1) {
    return NS_OK;
  }

  nsresult result;

  
  nsIContent* content = mFrame->GetContent();
  nsCOMPtr<nsIDOMNSHTMLInputElement> inputElem = do_QueryInterface(content);
  nsCOMPtr<nsIFileControlElement> fileControl = do_QueryInterface(content);
  if (!content || !inputElem || !fileControl)
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

  nsPIDOMWindow* win = doc->GetWindow();
  if (!win) {
    return NS_ERROR_FAILURE;
  }

  PRBool multi;
  result = inputElem->GetMultiple(&multi);
  NS_ENSURE_SUCCESS(result, result);

  result = filePicker->Init(win, title, multi ?
                            (PRInt16)nsIFilePicker::modeOpenMultiple :
                            (PRInt16)nsIFilePicker::modeOpen);
  if (NS_FAILED(result))
    return result;

  
  
  PRUint32 filter = mFrame->GetFileFilterFromAccept();
  filePicker->AppendFilters(filter | nsIFilePicker::filterAll);

  
  if (filter) {
    
    
    
    filePicker->SetFilterIndex(1);
  }

  
  nsAutoString defaultName;

  nsCOMArray<nsIFile> oldFiles;
  fileControl->GetFileArray(oldFiles);

  if (oldFiles.Count()) {
    
    nsCOMPtr<nsIFile> parentFile;
    oldFiles[0]->GetParent(getter_AddRefs(parentFile));
    if (parentFile) {
      nsCOMPtr<nsILocalFile> parentLocalFile = do_QueryInterface(parentFile, &result);
      if (parentLocalFile) {
        filePicker->SetDisplayDirectory(parentLocalFile);
      }
    }

    
    
    
    if (oldFiles.Count() == 1) {
      nsAutoString leafName;
      oldFiles[0]->GetLeafName(leafName);
      if (!leafName.IsEmpty()) {
        filePicker->SetDefaultString(leafName);
      }
    }
  } else {
    
    nsCOMPtr<nsILocalFile> localFile;
    gUploadLastDir->FetchLastUsedDirectory(doc->GetDocumentURI(),
                                           getter_AddRefs(localFile));
    if (!localFile) {
      
      nsCOMPtr<nsIFile> homeDir;
      NS_GetSpecialDirectory(NS_OS_DESKTOP_DIR, getter_AddRefs(homeDir));
      localFile = do_QueryInterface(homeDir);
    }
    filePicker->SetDisplayDirectory(localFile);
  }

  
  mFrame->mTextFrame->InitFocusedValue();

  
  PRInt16 mode;
  result = filePicker->Show(&mode);
  if (NS_FAILED(result))
    return result;
  if (mode == nsIFilePicker::returnCancel)
    return NS_OK;

  if (!mFrame) {
    
    
    
    
    return NS_OK;
  }
  
  
  nsTArray<nsString> newFileNames;
  if (multi) {
    nsCOMPtr<nsISimpleEnumerator> iter;
    result = filePicker->GetFiles(getter_AddRefs(iter));
    NS_ENSURE_SUCCESS(result, result);

    nsCOMPtr<nsISupports> tmp;
    PRBool prefSaved = PR_FALSE;
    while (NS_SUCCEEDED(iter->GetNext(getter_AddRefs(tmp)))) {
      nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(tmp);
      if (localFile) {
        nsString unicodePath;
        result = localFile->GetPath(unicodePath);
        if (!unicodePath.IsEmpty()) {
          newFileNames.AppendElement(unicodePath);
        }
        if (!prefSaved) {
          
          result = gUploadLastDir->StoreLastUsedDirectory(doc->GetDocumentURI(), 
                                                          localFile);
          NS_ENSURE_SUCCESS(result, result);
          prefSaved = PR_TRUE;
        }
      }
    }
  }
  else {
    nsCOMPtr<nsILocalFile> localFile;
    result = filePicker->GetFile(getter_AddRefs(localFile));
    if (localFile) {
      nsString unicodePath;
      result = localFile->GetPath(unicodePath);
      if (!unicodePath.IsEmpty()) {
        newFileNames.AppendElement(unicodePath);
      }
      
      result = gUploadLastDir->StoreLastUsedDirectory(doc->GetDocumentURI(),
                                                      localFile);
      NS_ENSURE_SUCCESS(result, result);
    }
  }

  
  if (!newFileNames.IsEmpty()) {
    
    
    
    PRBool oldState = mFrame->mTextFrame->GetFireChangeEventState();
    mFrame->mTextFrame->SetFireChangeEventState(PR_TRUE);
    fileControl->SetFileNames(newFileNames);

    mFrame->mTextFrame->SetFireChangeEventState(oldState);
    
    mFrame->mTextFrame->CheckFireOnChange();
  }

  return NS_OK;
}

void nsFileControlFrame::InitUploadLastDir() {
  gUploadLastDir = new UploadLastDir();
  NS_IF_ADDREF(gUploadLastDir);

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService && gUploadLastDir) {
    observerService->AddObserver(gUploadLastDir, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_TRUE);
    observerService->AddObserver(gUploadLastDir, "browser:purge-session-history", PR_TRUE);
  }
}

void nsFileControlFrame::DestroyUploadLastDir() {
  if (gUploadLastDir)
    NS_RELEASE(gUploadLastDir);
}

UploadLastDir::UploadLastDir():
  mInPrivateBrowsing(PR_FALSE)
{
  nsCOMPtr<nsIPrivateBrowsingService> pbService =
    do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
  if (pbService) {
    pbService->GetPrivateBrowsingEnabled(&mInPrivateBrowsing);
  }

  mUploadLastDirStore.Init();
}

nsresult
UploadLastDir::FetchLastUsedDirectory(nsIURI* aURI, nsILocalFile** aFile)
{
  NS_PRECONDITION(aURI, "aURI is null");
  NS_PRECONDITION(aFile, "aFile is null");
  
  
  if (mInPrivateBrowsing) {
    nsCOMPtr<nsIContentURIGrouper> hostnameGrouperService =
      do_GetService(NS_HOSTNAME_GROUPER_SERVICE_CONTRACTID);
    if (!hostnameGrouperService)
      return NS_ERROR_NOT_AVAILABLE;
    nsString group;
    hostnameGrouperService->Group(aURI, group);

    if (mUploadLastDirStore.Get(group, aFile)) {
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIContentPrefService> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  if (!contentPrefService)
    return NS_ERROR_NOT_AVAILABLE;
  nsCOMPtr<nsIWritableVariant> uri = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!uri)
    return NS_ERROR_OUT_OF_MEMORY;
  uri->SetAsISupports(aURI);

  
  PRBool hasPref;
  if (NS_SUCCEEDED(contentPrefService->HasPref(uri, CPS_PREF_NAME, &hasPref)) && hasPref) {
    nsCOMPtr<nsIVariant> pref;
    contentPrefService->GetPref(uri, CPS_PREF_NAME, nsnull, getter_AddRefs(pref));
    nsString prefStr;
    pref->GetAsAString(prefStr);

    nsCOMPtr<nsILocalFile> localFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
    if (!localFile)
      return NS_ERROR_OUT_OF_MEMORY;
    localFile->InitWithPath(prefStr);

    *aFile = localFile;
    NS_ADDREF(*aFile);
  }
  return NS_OK;
}

nsresult
UploadLastDir::StoreLastUsedDirectory(nsIURI* aURI, nsILocalFile* aFile)
{
  NS_PRECONDITION(aURI, "aURI is null");
  NS_PRECONDITION(aFile, "aFile is null");
  nsCOMPtr<nsIFile> parentFile;
  aFile->GetParent(getter_AddRefs(parentFile));
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(parentFile);

  
  if (mInPrivateBrowsing) {
    nsCOMPtr<nsIContentURIGrouper> hostnameGrouperService =
      do_GetService(NS_HOSTNAME_GROUPER_SERVICE_CONTRACTID);
    if (!hostnameGrouperService)
      return NS_ERROR_NOT_AVAILABLE;
    nsString group;
    hostnameGrouperService->Group(aURI, group);

    return mUploadLastDirStore.Put(group, localFile);
  }

  
  nsCOMPtr<nsIContentPrefService> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  if (!contentPrefService)
    return NS_ERROR_NOT_AVAILABLE;
  nsCOMPtr<nsIWritableVariant> uri = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!uri)
    return NS_ERROR_OUT_OF_MEMORY;
  uri->SetAsISupports(aURI);
 
  
  nsString unicodePath;
  parentFile->GetPath(unicodePath);
  if (unicodePath.IsEmpty()) 
    return NS_OK;
  nsCOMPtr<nsIWritableVariant> prefValue = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!prefValue)
    return NS_ERROR_OUT_OF_MEMORY;
  prefValue->SetAsAString(unicodePath);
  return contentPrefService->SetPref(uri, CPS_PREF_NAME, prefValue);
}

NS_IMETHODIMP
UploadLastDir::Observe(nsISupports *aSubject, char const *aTopic, PRUnichar const *aData)
{
  if (strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC) == 0) {
    if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).Equals(aData)) {
      mInPrivateBrowsing = PR_TRUE;
    } else if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(aData)) {
      mInPrivateBrowsing = PR_FALSE;
      if (mUploadLastDirStore.IsInitialized()) {
        mUploadLastDirStore.Clear();
      }
    }
  } else if (strcmp(aTopic, "browser:purge-session-history") == 0) {
    if (mUploadLastDirStore.IsInitialized()) {
      mUploadLastDirStore.Clear();
    }
    nsCOMPtr<nsIContentPrefService> contentPrefService =
      do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
    if (contentPrefService)
      contentPrefService->RemovePrefsByName(CPS_PREF_NAME);
  }
  return NS_OK;
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

NS_IMETHODIMP
nsFileControlFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::disabled) {
      SyncAttr(aNameSpaceID, aAttribute, SYNC_BOTH);
    
    } else if (aAttribute == nsGkAtoms::size) {
      SyncAttr(aNameSpaceID, aAttribute, SYNC_TEXT);
    } else if (aAttribute == nsGkAtoms::tabindex) {
      SyncAttr(aNameSpaceID, aAttribute, SYNC_BUTTON);
    }
  }

  return nsBlockFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
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
    nsCOMPtr<nsIFileControlElement> fileControl =
      do_QueryInterface(mContent);
    if (fileControl) {
      fileControl->GetDisplayFileName(aValue);
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
        nsDisplayBoxShadowOuter(this));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  nsDisplayListCollection tempList;
  nsresult rv = nsBlockFrame::BuildDisplayList(aBuilder, aDirtyRect, tempList);
  if (NS_FAILED(rv))
    return rv;

  tempList.BorderBackground()->DeleteAll();

  
  nsRect clipRect(aBuilder->ToReferenceFrame(this), GetSize());
  clipRect.width = GetOverflowRect().XMost();
  rv = OverflowClip(aBuilder, tempList, aLists, clipRect);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled) && 
      IsVisibleForPainting(aBuilder)) {
    nsDisplayItem* item = new (aBuilder) nsDisplayEventReceiver(this);
    if (!item)
      return NS_ERROR_OUT_OF_MEMORY;
    aLists.Content()->AppendToTop(item);
  }

  return DisplaySelectionOverlay(aBuilder, aLists);
}

#ifdef ACCESSIBILITY
already_AddRefed<nsAccessible>
nsFileControlFrame::CreateAccessible()
{
  
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");
  if (!accService)
    return nsnull;

  return accService->CreateHyperTextAccessible(mContent,
                                               PresContext()->PresShell());
}
#endif

PRInt32
nsFileControlFrame::GetFileFilterFromAccept() const
{
  nsAutoString accept;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accept, accept);

  if (accept.EqualsLiteral("image/*")) {
    return nsIFilePicker::filterImages;
  } else if (accept.EqualsLiteral("audio/*")) {
    return nsIFilePicker::filterAudio;
  } else if (accept.EqualsLiteral("video/*")) {
    return nsIFilePicker::filterVideo;
  }

  return 0;
}



NS_IMPL_ISUPPORTS2(nsFileControlFrame::MouseListener,
                   nsIDOMMouseListener,
                   nsIDOMEventListener)
