








































#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "nsIComponentManager.h"
#include "nsILocalFile.h"
#include "nsILocalFileMac.h"
#include "nsIURL.h"
#include "nsIFileURL.h"
#include "nsToolkit.h"
#include "nsIEventSink.h"
#include "nsArrayEnumerator.h"

#include <InternetConfig.h>

#include "nsCarbonHelpers.h"

#include "nsFilePicker.h"
#include "nsIInternetConfigService.h"
#include "nsIMIMEInfo.h"


NS_IMPL_ISUPPORTS1(nsFilePicker, nsIFilePicker)

OSType nsFilePicker::sCurrentProcessSignature = 0;






nsFilePicker::nsFilePicker()
  : mAllFilesDisplayed(PR_TRUE)
  , mApplicationsDisplayed(PR_FALSE)
  , mSelectedType(0)
  , mTypeOffset(0)
{
  
  
  for (int i = 0; i < kMaxTypeListCount; i++)
  	mTypeLists[i] = 0L;
  
  mSelectedType = 0;
  
  mTypeOffset = (NavLibraryVersion() < 0x02000000) ? 1 : 0;

  if (sCurrentProcessSignature == 0)
  {
    ProcessSerialNumber psn;
    ProcessInfoRec  info;
    
    psn.highLongOfPSN = 0;
    psn.lowLongOfPSN  = kCurrentProcess;

    info.processInfoLength = sizeof(ProcessInfoRec);
    info.processName = nil;
    info.processAppSpec = nil;
    OSErr err = ::GetProcessInformation(&psn, &info);
    if (err == noErr)
        sCurrentProcessSignature = info.processSignature;
    
  }
}







nsFilePicker::~nsFilePicker()
{
	
	if ( mFilters.Count() ) {
	  for (int i = 0; i < kMaxTypeListCount; i++) {
	  	if (mTypeLists[i])
	  		DisposePtr((Ptr)mTypeLists[i]);
	  }
	}	
  mFilters.Clear();
  mTitles.Clear();

}


void
nsFilePicker::InitNative(nsIWidget *aParent, const nsAString& aTitle,
                         PRInt16 aMode)
{
  mTitle = aTitle;
  mMode = aMode;
}







NS_IMETHODIMP nsFilePicker::Show(PRInt16 *retval)
{
  NS_ENSURE_ARG_POINTER(retval);
  *retval = returnCancel;
      
  PRInt16 userClicksOK = returnCancel;
  
  mFiles.Clear();
  
  
  
  mFiles.Clear();
  nsCOMPtr<nsILocalFile> theFile;

  switch (mMode)
  {
    case modeOpen:
      userClicksOK = GetLocalFiles(mTitle, PR_FALSE, mFiles);
      break;
    
    case modeOpenMultiple:
      userClicksOK = GetLocalFiles(mTitle, PR_TRUE, mFiles);
      break;
      
    case modeSave:
      userClicksOK = PutLocalFile(mTitle, mDefault, getter_AddRefs(theFile));
      break;
      
    case modeGetFolder:
      userClicksOK = GetLocalFolder(mTitle, getter_AddRefs(theFile));
      break;
    
    default:
      NS_ASSERTION(0, "Unknown file picker mode");
      break;
  }
  if (theFile)
    mFiles.AppendObject(theFile);
  
  *retval = userClicksOK;
  return NS_OK;
}











void nsFilePicker::HandleShowPopupMenuSelect(NavCBRecPtr callBackParms)
{
  if (callBackParms)
  {
    NavMenuItemSpec menuItemSpec = *(NavMenuItemSpec*)callBackParms->eventData.eventDataParms.param;
    PRUint32        numMenuItems = mTitles.Count();
    if (mTypeOffset && (numMenuItems != 0))
    { 
      
      if ((menuItemSpec.menuType != menuItemSpec.menuCreator) ||
          ((PRInt32)menuItemSpec.menuType < mTypeOffset) ||
          (menuItemSpec.menuType > numMenuItems))
      { 
        NavMenuItemSpec  menuItem;
        menuItem.version = kNavMenuItemSpecVersion;
        menuItem.menuType = mSelectedType + mTypeOffset;
        menuItem.menuCreator = mSelectedType + mTypeOffset;
        menuItem.menuItemName[0] = 0;
        (void)::NavCustomControl(callBackParms->context, kNavCtlSelectCustomType, &menuItem);
      }
      else
        mSelectedType = menuItemSpec.menuType - mTypeOffset;
    }
    else
      mSelectedType = menuItemSpec.menuType;
  }
}







pascal void nsFilePicker::FileDialogEventHandlerProc(NavEventCallbackMessage msg, NavCBRecPtr cbRec, NavCallBackUserData callbackUD)
{
  nsFilePicker* self = reinterpret_cast<nsFilePicker*>(callbackUD);
  switch (msg)
  {
    case kNavCBEvent:
      switch (cbRec->eventData.eventDataParms.event->what)
      {
        case updateEvt:
        {
          WindowPtr window = reinterpret_cast<WindowPtr>(cbRec->eventData.eventDataParms.event->message);
          nsCOMPtr<nsIEventSink> sink;
          nsToolkit::GetWindowEventSink (window, getter_AddRefs(sink));
          if (sink) {
            ::BeginUpdate(window);
            PRBool handled = PR_FALSE;
            sink->DispatchEvent(cbRec->eventData.eventDataParms.event, &handled);
            ::EndUpdate(window);	        
          }        
        }
        break;
      }
      break;
    
    case kNavCBStart:
    {
      NavMenuItemSpec  menuItem;
      menuItem.version = kNavMenuItemSpecVersion;
      menuItem.menuType = self->mSelectedType + self->mTypeOffset;
      menuItem.menuCreator = self->mSelectedType + self->mTypeOffset;
      menuItem.menuItemName[0] = 0;
      (void)::NavCustomControl(cbRec->context, kNavCtlSelectCustomType, &menuItem);

      
      if (self->mDisplayDirectory != nsnull) {
        nsCOMPtr<nsILocalFileMac> localDisplay = do_QueryInterface(self->mDisplayDirectory);
        if (localDisplay) {
          FSRef displayFSRef;
          if (NS_SUCCEEDED(localDisplay->GetFSRef(&displayFSRef))) {
            AEDesc desc;
            OSErr status = ::AECreateDesc(typeFSRef, &displayFSRef,
                                          sizeof(displayFSRef), &desc);
            if (status == noErr) {
              (void)::NavCustomControl(cbRec->context, kNavCtlSetLocation, &desc);
              (void)::AEDisposeDesc(&desc);
            }
          }
        }
      }
    }
    break;
    
    case kNavCBPopupMenuSelect:
      
      if (self)
        self->HandleShowPopupMenuSelect(cbRec);
      break;
  }
}







Boolean
nsFilePicker::IsTypeInFilterList ( ResType inType )
{
  for ( int i = 0; i < mFilters.Count(); ++i ) {
    for ( int j = 0; j < mTypeLists[i]->osTypeCount; ++j ) {
      if ( mTypeLists[i]->osType[j] == inType ) 
        return true;
    }  
  } 
  
  return false;
  
} 


Boolean
nsFilePicker::IsExtensionInFilterList ( StrFileName & inFileName )
{
  char extension[256];
  
  
  unsigned char* curr = &inFileName[inFileName[0]];
  while ( curr != inFileName && *curr-- != '.' ) ;
  if ( curr == inFileName )                         
    return false;
  ++curr;                                           
  short extensionLen = (inFileName + inFileName[0]) - curr + 1;
  strncpy ( extension, (char*)curr, extensionLen);
  extension[extensionLen] = '\0';
  
  
  for ( int i = 0; i < mFlatFilters.Count(); ++i ) {
    if ( mFlatFilters[i]->Equals(extension) )
      return true;
  }
  return false;
}








pascal
Boolean 
nsFilePicker::FileDialogFilterProc(AEDesc* theItem, void* theInfo,
                                        NavCallBackUserData callbackUD, NavFilterModes filterMode)
{
  Boolean shouldDisplay = true;
  nsFilePicker* self = reinterpret_cast<nsFilePicker*>(callbackUD);
  if ( self && !self->mAllFilesDisplayed ) {
    if ( theItem->descriptorType == typeFSS ) {
      NavFileOrFolderInfo* info = reinterpret_cast<NavFileOrFolderInfo*>(theInfo);
      if ( !info->isFolder ) {
        
        if ( ! self->IsTypeInFilterList(info->fileAndFolder.fileInfo.finderInfo.fdType) ) {
          FSSpec fileSpec;
          if ( ::AEGetDescData(theItem, &fileSpec, sizeof(FSSpec)) == noErr )
            if ( ! self->IsExtensionInFilterList(fileSpec.name) )
              shouldDisplay = false;
        }
      } 
    } 
  }
  
  return shouldDisplay;
  
} 













PRInt16
nsFilePicker::GetLocalFiles(const nsString& inTitle, PRBool inAllowMultiple, nsCOMArray<nsILocalFile>& outFiles)
{
  PRInt16 retVal = returnCancel;
  NavEventUPP eventProc = NewNavEventUPP(FileDialogEventHandlerProc);  
  NavObjectFilterUPP filterProc = NewNavObjectFilterUPP(FileDialogFilterProc);  
  NavDialogRef dialog;
  NavDialogCreationOptions dialogCreateOptions;
  
  OSErr anErr = ::NavGetDefaultDialogCreationOptions(&dialogCreateOptions);
  if (anErr != noErr)
    return retVal;
    
  
  dialogCreateOptions.optionFlags |= kNavNoTypePopup;
  dialogCreateOptions.optionFlags |= kNavDontAutoTranslate;
  dialogCreateOptions.optionFlags |= kNavDontAddTranslateItems;
  if (inAllowMultiple)
    dialogCreateOptions.optionFlags |= kNavAllowMultipleFiles;
  dialogCreateOptions.modality = kWindowModalityAppModal;
  dialogCreateOptions.parentWindow = NULL;
  CFStringRef titleRef = CFStringCreateWithCharacters(NULL, 
                                                     (const UniChar *) inTitle.get(), inTitle.Length());
  dialogCreateOptions.windowTitle = titleRef;

  
  MapFilterToFileTypes();
	
  
  if (mAllFilesDisplayed || mApplicationsDisplayed)
    dialogCreateOptions.optionFlags |= kNavSupportPackages;		

  
  
  anErr = ::NavCreateGetFileDialog(
                                  &dialogCreateOptions,
                                  NULL, 
                                  eventProc,
                                  NULL, 
                                  mFilters.Count() ? filterProc : NULL,
                                  this, 
                                  &dialog);
  if (anErr == noErr)
  {
    anErr = ::NavDialogRun(dialog);

    if (anErr == noErr)
    {
    	NavReplyRecord reply;
      anErr = ::NavDialogGetReply(dialog, &reply);
      if (anErr == noErr && reply.validRecord)
      {
        long numItems;
        anErr = ::AECountItems((const AEDescList *)&reply.selection, &numItems);
        if (anErr == noErr)
        {
          for (long i = 1; i <= numItems; i ++)
          {
            AEKeyword   theKeyword;
            DescType    actualType;
            Size        actualSize;
            FSRef       theFSRef;

            
            anErr = ::AEGetNthPtr(&(reply.selection), i, typeFSRef, &theKeyword, &actualType,
                                  &theFSRef, sizeof(theFSRef), &actualSize);
            if (anErr == noErr)
            {
              nsCOMPtr<nsILocalFile> localFile;
              NS_NewLocalFile(EmptyString(), PR_TRUE, getter_AddRefs(localFile));
              nsCOMPtr<nsILocalFileMac> macLocalFile = do_QueryInterface(localFile);
              if (macLocalFile && NS_SUCCEEDED(macLocalFile->InitWithFSRef(&theFSRef)))
                outFiles.AppendObject(localFile);
            }
          }

          if (outFiles.Count() > 0)
            retVal = returnOK;
        }
        
        
        ::NavDisposeReply(&reply);
      }
    }
    ::NavDialogDispose(dialog);
  }
  
  
  if (titleRef)
    CFRelease(titleRef);

  if (filterProc)
    ::DisposeNavObjectFilterUPP(filterProc);
	
  if ( eventProc )
    ::DisposeNavEventUPP(eventProc);
		
  return retVal;
} 










PRInt16
nsFilePicker::GetLocalFolder(const nsString& inTitle, nsILocalFile** outFile)
{
  NS_ENSURE_ARG_POINTER(outFile);
  *outFile = nsnull;
  
  PRInt16 retVal = returnCancel;
  NavEventUPP eventProc = NewNavEventUPP(FileDialogEventHandlerProc);  
  NavDialogRef dialog;
  NavDialogCreationOptions dialogCreateOptions;

  
  OSErr anErr = ::NavGetDefaultDialogCreationOptions(&dialogCreateOptions);
  if (anErr != noErr)
    return retVal;

  
  dialogCreateOptions.optionFlags |= kNavNoTypePopup;
  dialogCreateOptions.optionFlags |= kNavDontAutoTranslate;
  dialogCreateOptions.optionFlags |= kNavDontAddTranslateItems;
  dialogCreateOptions.optionFlags ^= kNavAllowMultipleFiles;
  dialogCreateOptions.modality = kWindowModalityAppModal;
  dialogCreateOptions.parentWindow = NULL;
  CFStringRef titleRef = CFStringCreateWithCharacters(NULL, (const UniChar *) inTitle.get(), inTitle.Length());
  dialogCreateOptions.windowTitle = titleRef;

  anErr = ::NavCreateChooseFolderDialog(
                                        &dialogCreateOptions,
                                        eventProc,
                                        NULL, 
                                        this, 
                                        &dialog);

  if (anErr == noErr)
  {
    anErr = ::NavDialogRun(dialog);
    if (anErr == noErr)
    {
    	NavReplyRecord reply;
      anErr = ::NavDialogGetReply(dialog, &reply);
      if (anErr == noErr && reply.validRecord) {
        AEKeyword   theKeyword;
        DescType    actualType;
        Size        actualSize;
        FSRef       theFSRef;
         
        
        anErr = ::AEGetNthPtr(&(reply.selection), 1, typeFSRef, &theKeyword, &actualType,
                              &theFSRef, sizeof(theFSRef), &actualSize);
        if (anErr == noErr)
        {
          nsCOMPtr<nsILocalFile> localFile;
          NS_NewLocalFile(EmptyString(), PR_TRUE, getter_AddRefs(localFile));
          nsCOMPtr<nsILocalFileMac> macLocalFile = do_QueryInterface(localFile);
          if (macLocalFile && NS_SUCCEEDED(macLocalFile->InitWithFSRef(&theFSRef)))
          {
            *outFile = localFile;
            NS_ADDREF(*outFile);
            retVal = returnOK;
          }
        }
        
        ::NavDisposeReply(&reply);
      }
    }
    ::NavDialogDispose(dialog);
  }
  
  
  if (dialogCreateOptions.windowTitle)
    CFRelease(dialogCreateOptions.windowTitle);
	
  if (eventProc)
    ::DisposeNavEventUPP(eventProc);
		
  return retVal;
} 

PRInt16
nsFilePicker::PutLocalFile(const nsString& inTitle, const nsString& inDefaultName, nsILocalFile** outFile)
{
  NS_ENSURE_ARG_POINTER(outFile);
  *outFile = nsnull;
  
  PRInt16 retVal = returnCancel;
  NavEventUPP eventProc = NewNavEventUPP(FileDialogEventHandlerProc);  
  OSType typeToSave = 'TEXT';
  OSType creatorToSave = (sCurrentProcessSignature == 0) ? 'MOZZ' : sCurrentProcessSignature;
  NavDialogRef dialog;
  NavDialogCreationOptions dialogCreateOptions;

  
  OSErr anErr = ::NavGetDefaultDialogCreationOptions(&dialogCreateOptions);
  if (anErr != noErr)
    return retVal;

  
  if (mTitles.Count() == 0)
  { 
    dialogCreateOptions.optionFlags |= kNavNoTypePopup;
  }
  else
  {
    dialogCreateOptions.optionFlags &= ~kNavAllowStationery;  
    creatorToSave = kNavGenericSignature;   
    SetupFormatMenuItems(&dialogCreateOptions);
  }
  
  dialogCreateOptions.optionFlags |= kNavDontAutoTranslate;
  dialogCreateOptions.optionFlags |= kNavDontAddTranslateItems;
  dialogCreateOptions.optionFlags ^= kNavAllowMultipleFiles;
  dialogCreateOptions.modality = kWindowModalityAppModal;
  dialogCreateOptions.parentWindow = NULL;
  CFStringRef titleRef = CFStringCreateWithCharacters(NULL, 
                                                      (const UniChar *) inTitle.get(), inTitle.Length());
  dialogCreateOptions.windowTitle = titleRef;
  CFStringRef defaultFileNameRef = CFStringCreateWithCharacters(NULL, 
                                                                (const UniChar *) inDefaultName.get(), 
                                                                inDefaultName.Length());
  dialogCreateOptions.saveFileName = defaultFileNameRef;

  anErr = ::NavCreatePutFileDialog(
                                   &dialogCreateOptions,
                                   typeToSave,
                                   creatorToSave,
                                   eventProc,
                                   this, 
                                   &dialog);

  if (anErr == noErr)
  {
    anErr = ::NavDialogRun(dialog);

    if (anErr == noErr)
    {
    	NavReplyRecord reply;
      anErr = ::NavDialogGetReply(dialog, &reply);
      if (anErr == noErr && reply.validRecord)
      {
        AEKeyword   theKeyword;
        DescType    actualType;
        Size        actualSize;
        FSRef       theFSRef;

        
        
        
        anErr = ::AEGetNthPtr(&(reply.selection), 1, typeFSRef, &theKeyword, &actualType,
                              &theFSRef, sizeof(theFSRef), &actualSize);
        if (anErr == noErr)
        {
          CFURLRef fileURL;
          CFURLRef parentURL = ::CFURLCreateFromFSRef(NULL, &theFSRef);
          if (parentURL)
          {
            fileURL = ::CFURLCreateCopyAppendingPathComponent(NULL, parentURL, reply.saveFileName, PR_FALSE);
            if (fileURL)
            {
              nsCOMPtr<nsILocalFile> localFile;
              NS_NewLocalFile(EmptyString(), PR_TRUE, getter_AddRefs(localFile));
              nsCOMPtr<nsILocalFileMac> macLocalFile = do_QueryInterface(localFile);
              if (macLocalFile && NS_SUCCEEDED(macLocalFile->InitWithCFURL(fileURL)))
              {
                *outFile = localFile;
                NS_ADDREF(*outFile);
                retVal = reply.replacing ? returnReplace : returnOK;
              }
              ::CFRelease(fileURL);
            }
            ::CFRelease(parentURL);
          }
        }  			  

        
        ::NavCompleteSave(&reply, kNavTranslateInPlace);
        ::NavDisposeReply(&reply);
      }
    }
    ::NavDialogDispose(dialog);
  }  
  
  
  if (dialogCreateOptions.windowTitle)
    CFRelease(dialogCreateOptions.windowTitle);
  if (dialogCreateOptions.saveFileName)
    CFRelease(dialogCreateOptions.saveFileName);
  if (dialogCreateOptions.popupExtension)
    CFRelease(dialogCreateOptions.popupExtension);
	
  if (eventProc)
    ::DisposeNavEventUPP(eventProc);
	
  return retVal;	
}







void
nsFilePicker::MapFilterToFileTypes ( )
{
  nsCOMPtr<nsIInternetConfigService> icService ( do_GetService(NS_INTERNETCONFIGSERVICE_CONTRACTID) );
  NS_ASSERTION(icService, "Can't get InternetConfig Service, bailing out");
  if ( !icService ) {
    
    
    return;
  }
  
  if (mFilters.Count())
  {
    
    for (PRInt32 loop1 = 0; loop1 < mFilters.Count() && loop1 < kMaxTypeListCount; loop1++)
    {
      mTypeLists[loop1] =
        (NavTypeListPtr)NewPtrClear(sizeof(NavTypeList) + kMaxTypesPerFilter * sizeof(OSType));     
      if ( !mTypeLists[loop1] )
        return;                     
    }
    
    
    for (PRInt32 loop1 = 0; loop1 < mFilters.Count() && loop1 < kMaxTypeListCount; loop1++)
    {
      const nsString& filterWide = *mFilters[loop1];
      char* filter = ToNewCString(filterWide);

      NS_ASSERTION ( filterWide.Length(), "Oops. filepicker.properties not correctly installed");       

      
      if (filterWide.EqualsLiteral("..apps"))
        mApplicationsDisplayed = PR_TRUE;

      if ( filterWide.Length() && filter )
      {
        PRUint32 filterIndex = 0;         
        PRUint32 typeTempIndex = 0;       
        PRUint32 typesInThisFilter = 0;   
        bool finishedThisFilter = false;  
        char typeTemp[256];
        char tempChar;           

        
        
        
        
        do
        {
          tempChar = filter[filterIndex];
          if ((tempChar == ';') || (tempChar == 0)) {   
            typeTemp[typeTempIndex] = '\0';             
            
            
            
            
            if ( strlen(typeTemp) > 1 )
              mFlatFilters.AppendCString ( nsCString((char*)&typeTemp[1]) );  
            
            
            if ( !(typeTemp[1] == '\0' && typeTemp[0] == '*') ) {
              mAllFilesDisplayed = PR_FALSE;
              
              nsCOMPtr<nsIMIMEInfo> icEntry;
              icService->GetMIMEInfoFromExtension(typeTemp, getter_AddRefs(icEntry));
              if ( icEntry )
              {
                bool addToList = true;
                OSType tempOSType;
                icEntry->GetMacType(reinterpret_cast<PRUint32*>((&tempOSType)));
                for (PRUint32 typeIndex = 0; typeIndex < typesInThisFilter; typeIndex++)
                {
                  if (mTypeLists[loop1]->osType[typeIndex] == tempOSType)
                  {
                    addToList = false;
                    break;
                  }
                }
                if (addToList && typesInThisFilter < kMaxTypesPerFilter)
                  mTypeLists[loop1]->osType[typesInThisFilter++] = tempOSType;
              }
            } 
            
            typeTempIndex = 0;      
            if (tempChar == '\0')
              finishedThisFilter = true;
          }
          else
          {
            
            if ( tempChar != ' ' )
              typeTemp[typeTempIndex++] = tempChar;
          }
          
          filterIndex++;
        } while (!finishedThisFilter);
        
        
        mTypeLists[loop1]->osTypeCount = typesInThisFilter;
        
        nsMemory::Free ( reinterpret_cast<void*>(filter) );
      }
    }
  }

} 







void nsFilePicker::SetupFormatMenuItems (NavDialogCreationOptions* dialogCreateOptions)
{
  PRInt32   numMenuItems = mTitles.Count();
  PRInt32   index;
  CFStringRef      itemStr = NULL;
  CFArrayCallBacks callBacks = kCFTypeArrayCallBacks;
  
  
  dialogCreateOptions->popupExtension = CFArrayCreateMutable(kCFAllocatorDefault, numMenuItems, &callBacks);
  
  if (dialogCreateOptions->popupExtension)
  {
    for (index = 0; index < numMenuItems; ++index)
    {
      const nsString& titleWide = *mTitles[index];
      itemStr = CFStringCreateWithCharacters(NULL, (const unsigned short *)titleWide.get(), titleWide.Length());
      CFArrayInsertValueAtIndex((CFMutableArrayRef)dialogCreateOptions->popupExtension, index, (void*)itemStr);
      CFRelease(itemStr);
    }
  }
}



NS_IMETHODIMP nsFilePicker::GetFile(nsILocalFile **aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);
  *aFile = nsnull;
  
  
  if (mFiles.Count() > 0)
  {
    *aFile = mFiles.ObjectAt(0);
    NS_IF_ADDREF(*aFile);
  }

  return NS_OK;
}


NS_IMETHODIMP nsFilePicker::GetFileURL(nsIFileURL **aFileURL)
{
  NS_ENSURE_ARG_POINTER(aFileURL);
  *aFileURL = nsnull;

  if (mFiles.Count() == 0)
    return NS_OK;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewFileURI(getter_AddRefs(uri), mFiles.ObjectAt(0));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIFileURL> fileURL(do_QueryInterface(uri));
  NS_ENSURE_TRUE(fileURL, NS_ERROR_FAILURE);
  
  NS_ADDREF(*aFileURL = fileURL);
  return NS_OK;
}


NS_IMETHODIMP nsFilePicker::GetFiles(nsISimpleEnumerator **aFiles)
{
  return NS_NewArrayEnumerator(aFiles, mFiles);
}






NS_IMETHODIMP nsFilePicker::SetDefaultString(const nsAString& aString)
{
  mDefault = aString;
  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::GetDefaultString(nsAString& aString)
{
  return NS_ERROR_FAILURE;
}






NS_IMETHODIMP nsFilePicker::GetDefaultExtension(nsAString& aExtension)
{
  aExtension.Truncate();
  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::SetDefaultExtension(const nsAString& aExtension)
{
  return NS_OK;
}



NS_IMETHODIMP
nsFilePicker::AppendFilter(const nsAString& aTitle, const nsAString& aFilter)
{
  mFilters.AppendString(aFilter);
  mTitles.AppendString(aTitle);
  
  return NS_OK;
}







NS_IMETHODIMP nsFilePicker::GetFilterIndex(PRInt32 *aFilterIndex)
{
  *aFilterIndex = mSelectedType;
  return NS_OK;
}






NS_IMETHODIMP nsFilePicker::SetFilterIndex(PRInt32 aFilterIndex)
{
  mSelectedType = aFilterIndex;
  return NS_OK;
}

