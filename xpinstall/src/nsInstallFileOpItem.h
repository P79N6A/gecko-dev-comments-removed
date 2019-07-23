




































#ifndef nsInstallFileOpItem_h__
#define nsInstallFileOpItem_h__

#include "prtypes.h"

#include "nsIFile.h"
#include "nsSoftwareUpdate.h"
#include "nsInstallObject.h"
#include "nsInstall.h"

class nsInstallFileOpItem : public nsInstallObject
{
  public:
    
    enum 
    {
      ACTION_NONE                 = -401,
      ACTION_SUCCESS              = -402,
      ACTION_FAILED               = -403
    };


    
    
    
    nsInstallFileOpItem(nsInstall*    installObj,
                        PRInt32       aCommand,
                        nsIFile*      aTarget,
                        PRInt32       aFlags,
                        PRInt32*      aReturn);

    
    
    
    
    
    nsInstallFileOpItem(nsInstall*    installObj,
                        PRInt32       aCommand,
                        nsIFile*      aSrc,
                        nsIFile*      aTarget,
                        PRInt32*      aReturn);

    
    
    nsInstallFileOpItem(nsInstall*    aInstallObj,
                        PRInt32       aCommand,
                        nsIFile*      aTarget,
                        PRInt32*      aReturn);

    
    
    
    
    nsInstallFileOpItem(nsInstall*    aInstallObj,
                        PRInt32       aCommand,
                        nsIFile*      a1,
                        nsString&     a2,
                        PRBool        aBlocking,
                        PRInt32*      aReturn);

    
    
    nsInstallFileOpItem(nsInstall*    aInstallObj,
                        PRInt32       aCommand,
                        nsIFile*      aTarget,
                        nsIFile*      aShortcutPath,
                        nsString&     aDescription,
                        nsIFile*      aWorkingPath,
                        nsString&     aParams,
                        nsIFile*      aIcon,
                        PRInt32       aIconId,
                        PRInt32*      aReturn);

    virtual ~nsInstallFileOpItem();

    PRInt32       Prepare(void);
    PRInt32       Complete();
    char*         toString();
    void          Abort();
    
  
    PRBool        CanUninstall();
    PRBool        RegisterPackageNode();
      
  private:
    
    
    
    nsInstall*          mIObj;        
    nsCOMPtr<nsIFile>   mSrc;
    nsCOMPtr<nsIFile>   mTarget;
    nsCOMPtr<nsIFile>   mShortcutPath;
    nsCOMPtr<nsIFile>   mWorkingPath;
    nsCOMPtr<nsIFile>   mIcon;
    nsString            mDescription;
    nsString*           mStrTarget;
    nsString            mParams;
    long                mFStat;
    PRInt32             mFlags;
    PRInt32             mIconId;
    PRInt32             mCommand;
    PRInt32             mAction;
    PRBool              mBlocking;
    
    

    PRInt32       NativeFileOpDirCreatePrepare();
    PRInt32       NativeFileOpDirCreateAbort();
    PRInt32       NativeFileOpDirRemovePrepare();
    PRInt32       NativeFileOpDirRemoveComplete();
    PRInt32       NativeFileOpDirRenamePrepare();
    PRInt32       NativeFileOpDirRenameComplete();
    PRInt32       NativeFileOpDirRenameAbort();
    PRInt32       NativeFileOpFileCopyPrepare();
    PRInt32       NativeFileOpFileCopyComplete();
    PRInt32       NativeFileOpFileCopyAbort();
    PRInt32       NativeFileOpFileDeletePrepare();
    PRInt32       NativeFileOpFileDeleteComplete(nsIFile *aTarget);
    PRInt32       NativeFileOpFileExecutePrepare();
    PRInt32       NativeFileOpFileExecuteComplete();
    PRInt32       NativeFileOpFileMovePrepare();
    PRInt32       NativeFileOpFileMoveComplete();
    PRInt32       NativeFileOpFileMoveAbort();
    PRInt32       NativeFileOpFileRenamePrepare();
    PRInt32       NativeFileOpFileRenameComplete();
    PRInt32       NativeFileOpFileRenameAbort();
    PRInt32       NativeFileOpWindowsShortcutPrepare();
    PRInt32       NativeFileOpWindowsShortcutComplete();
    PRInt32       NativeFileOpWindowsShortcutAbort();
    PRInt32       NativeFileOpMacAliasPrepare();
    PRInt32       NativeFileOpMacAliasComplete();
    PRInt32       NativeFileOpMacAliasAbort();
    PRInt32       NativeFileOpUnixLink();
    PRInt32       NativeFileOpWindowsRegisterServerPrepare();
    PRInt32       NativeFileOpWindowsRegisterServerComplete();
    PRInt32       NativeFileOpWindowsRegisterServerAbort();

};

#endif

