







































#ifndef nsFilePicker_h__
#define nsFilePicker_h__

#include <Navigation.h>

#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsIFileChannel.h"
#include "nsILocalFile.h"
#include "nsCOMArray.h"

class nsILocalFileMac;

#define	kMaxTypeListCount	10
#define kMaxTypesPerFilter	9





class nsFilePicker : public nsBaseFilePicker
{
public:
  nsFilePicker(); 
  virtual ~nsFilePicker();

  NS_DECL_ISUPPORTS
   
    
  NS_IMETHOD GetDefaultString(nsAString& aDefaultString);
  NS_IMETHOD SetDefaultString(const nsAString& aDefaultString);
  NS_IMETHOD GetDefaultExtension(nsAString& aDefaultExtension);
  NS_IMETHOD SetDefaultExtension(const nsAString& aDefaultExtension);
  NS_IMETHOD GetFilterIndex(PRInt32 *aFilterIndex);
  NS_IMETHOD SetFilterIndex(PRInt32 aFilterIndex);
  NS_IMETHOD GetFile(nsILocalFile * *aFile);
  NS_IMETHOD GetFileURL(nsIFileURL * *aFileURL);
  NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles);
  NS_IMETHOD Show(PRInt16 *_retval); 
  NS_IMETHOD AppendFilter(const nsAString& aTitle, const nsAString& aFilter);

protected:

  virtual void InitNative(nsIWidget *aParent, const nsAString& aTitle,
                          PRInt16 aMode);

    
  PRInt16 GetLocalFiles(const nsString& inTitle, PRBool inAllowMultiple, nsCOMArray<nsILocalFile>& outFiles);
  PRInt16 GetLocalFolder(const nsString& inTitle, nsILocalFile** outFile);
  PRInt16 PutLocalFile(const nsString& inTitle, const nsString& inDefaultName, nsILocalFile** outFile);
  
  void MapFilterToFileTypes ( ) ;
  void SetupFormatMenuItems (NavDialogCreationOptions* dialogCreateOptions) ;
  Boolean IsTypeInFilterList ( ResType inType ) ;
  Boolean IsExtensionInFilterList ( StrFileName & inFileName ) ;
  void HandleShowPopupMenuSelect( NavCBRecPtr callBackParms ) ;
  
    
  static pascal void FileDialogEventHandlerProc( NavEventCallbackMessage msg,
                                                 NavCBRecPtr cbRec,
                                                 NavCallBackUserData callbackUD ) ;
  
    
  static pascal Boolean FileDialogFilterProc ( AEDesc* theItem, void* info,
                                                NavCallBackUserData callbackUD,
                                                NavFilterModes filterMode ) ;
                                                
  PRPackedBool           mAllFilesDisplayed;
  PRPackedBool           mApplicationsDisplayed;
  nsString               mTitle;
  PRInt16                mMode;
  nsCOMArray<nsILocalFile> mFiles;
  nsString               mDefault;

  nsStringArray          mFilters; 
  nsStringArray          mTitles;
  nsCStringArray         mFlatFilters;        
  
  NavTypeListPtr         mTypeLists[kMaxTypeListCount];

  static OSType          sCurrentProcessSignature;
  
  PRInt32                mSelectedType;
  PRInt32                mTypeOffset;       
};

#endif 
