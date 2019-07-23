




































#ifndef nsPrefMigration_h___
#define nsPrefMigration_h___


#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsFileSpec.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIDOMWindowInternal.h"
#include "nsIFileSpec.h"
#include "nsPrefMigrationCIDs.h"
#include "nsIPrefMigration.h"
#include "nsVoidArray.h"
#include "nsILocalFile.h"

#define MIGRATION_SUCCESS    0
#define MIGRATION_RETRY      1
#define MIGRATION_CANCEL     2
#define MIGRATION_CREATE_NEW 3

#define MAX_DRIVES 4



#if defined(XP_MAC) || defined(XP_MACOSX)
#define IMAP_MAIL_FILTER_FILE_NAME_FORMAT_IN_4x "%s Rules" 
#endif

#if defined(XP_UNIX) && !defined(XP_MACOSX)
#define NEED_TO_COPY_AND_RENAME_NEWSRC_FILES
#endif

class nsPrefConverter: public nsIPrefConverter
{
    public:
      NS_DEFINE_STATIC_CID_ACCESSOR(NS_PREFCONVERTER_CID) 

      nsPrefConverter();
      virtual ~nsPrefConverter();

      NS_DECL_ISUPPORTS
      NS_DECL_NSIPREFCONVERTER

      nsresult GetPlatformCharset(nsCString& aCharset);
};

class nsPrefMigration: public nsIPrefMigration
{
    public:
      NS_DEFINE_STATIC_CID_ACCESSOR(NS_PREFMIGRATION_CID) 

      static nsPrefMigration *GetInstance();

      nsPrefMigration();
      virtual ~nsPrefMigration();

      NS_DECL_ISUPPORTS

      NS_DECL_NSIPREFMIGRATION

      
      

      nsVoidArray mProfilesToMigrate;
      nsresult ProcessPrefsCallback(const char* oldProfilePathStr, const char * newProfilePathStr);
      void WaitForThread();

      nsresult mErrorCode;

    private:
      
      static nsPrefMigration* mInstance;
	
      nsresult ConvertPersistentStringToFileSpec(const char *str, nsIFileSpec *path);
      nsresult CreateNewUser5Tree(nsIFileSpec* oldProfilePath, 
                                  nsIFileSpec* newProfilePath);

      nsresult GetDirFromPref(nsIFileSpec* oldProfilePath,
                              nsIFileSpec* newProfilePath, 
                              const char* newDirName,
                              const char* pref, 
                              nsIFileSpec* newPath, 
                              nsIFileSpec* oldPath);

      nsresult GetSizes(nsFileSpec inputPath,
                        PRBool readSubdirs,
                        PRUint32* sizeTotal);

      nsresult ComputeSpaceRequirements(PRInt64 DriveArray[], 
                                        PRUint32 SpaceReqArray[], 
                                        PRInt64 Drive, 
                                        PRUint32 SpaceNeeded);

      nsresult DoTheCopy(nsIFileSpec *oldPath, 
                         nsIFileSpec *newPath,
                         PRBool readSubdirs); 
      nsresult DoTheCopy(nsIFileSpec *oldPath,
                         nsIFileSpec *newPath,
                         const char *fileOrDirName,
                         PRBool isDirectory = PR_FALSE);

      nsresult DoTheCopyAndRename(nsIFileSpec *oldPath, 
                              nsIFileSpec *newPath,
                              PRBool readSubdirs,
                              PRBool needToRenameFiles,
                              const char *oldName,
                              const char *newName); 
      nsresult DoTheCopyAndRename(nsIFileSpec *aPath, 
                              PRBool aReadSubdirs,
                              const char *aOldName,
                              const char *aNewName);
      nsresult CopyFilesByPattern(nsIFileSpec * oldPathSpec,
                              nsIFileSpec * newPathSpec,
                              const char *pattern);


#ifdef NEED_TO_COPY_AND_RENAME_NEWSRC_FILES
      nsresult CopyAndRenameNewsrcFiles(nsIFileSpec *newPath);
#endif 

      nsresult DoSpecialUpdates(nsIFileSpec * profilePath);
      nsresult Rename4xFileAfterMigration(nsIFileSpec *profilePath, const char *oldFileName, const char *newFileName);
#ifdef IMAP_MAIL_FILTER_FILE_NAME_FORMAT_IN_4x
      nsresult RenameAndMove4xImapFilterFile(nsIFileSpec *profilePath, const char *hostname);
      nsresult RenameAndMove4xImapFilterFiles(nsIFileSpec *profilePath);
#endif 
      nsresult RenameAndMove4xPopStateFile(nsIFileSpec *profilePath);
      nsresult RenameAndMove4xPopFilterFile(nsIFileSpec *profilePath);
      nsresult RenameAndMove4xPopFile(nsIFileSpec * profilePath, const char *fileNameIn4x, const char *fileNameIn5x);
  
      nsresult DetermineOldPath(nsIFileSpec *profilePath, const char *oldPathName, const char *oldPathEntityName, nsIFileSpec *oldPath);
      nsresult SetPremigratedFilePref(const char *pref_name, nsIFileSpec *filePath);
#ifdef NEED_TO_COPY_AND_RENAME_NEWSRC_FILES
      nsresult GetPremigratedFilePref(const char *pref_name, nsIFileSpec **filePath);
#endif 

      nsresult getPrefService();

      nsCOMPtr<nsIPrefBranch>    m_prefBranch;
      nsCOMPtr<nsILocalFile>     m_prefsFile; 
      nsCOMPtr<nsIDOMWindowInternal>    m_parentWindow;
      nsCOMPtr<nsIDOMWindow>    mPMProgressWindow;
};

#endif 
