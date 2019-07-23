





































#ifndef nsFileSystemDataSource_h__
#define nsFileSystemDataSource_h__

#include "nsIRDFDataSource.h"
#include "nsIRDFLiteral.h"
#include "nsIRDFResource.h"
#include "nsIRDFService.h"
#include "nsISupportsArray.h"
#include "nsCOMPtr.h"
#include "nsString.h"

#if defined(XP_UNIX) || defined(XP_OS2) || defined(XP_WIN) || defined(XP_BEOS)
#define USE_NC_EXTENSION
#endif

class FileSystemDataSource : public nsIRDFDataSource
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRDFDATASOURCE

    static NS_METHOD Create(nsISupports* aOuter,
                            const nsIID& aIID, void **aResult);

    ~FileSystemDataSource() { }
    nsresult Init();

private:
    FileSystemDataSource() { }

    
    PRBool   isFileURI(nsIRDFResource* aResource);
    PRBool   isDirURI(nsIRDFResource* aSource);
    nsresult GetVolumeList(nsISimpleEnumerator **aResult);
    nsresult GetFolderList(nsIRDFResource *source, PRBool allowHidden, PRBool onlyFirst, nsISimpleEnumerator **aResult);
    nsresult GetName(nsIRDFResource *source, nsIRDFLiteral** aResult);
    nsresult GetURL(nsIRDFResource *source, PRBool *isFavorite, nsIRDFLiteral** aResult);
    nsresult GetFileSize(nsIRDFResource *source, nsIRDFInt** aResult);
    nsresult GetLastMod(nsIRDFResource *source, nsIRDFDate** aResult);

    nsCOMPtr<nsIRDFService>    mRDFService;

    
    nsCOMPtr<nsIRDFResource>       mNC_FileSystemRoot;
    nsCOMPtr<nsIRDFResource>       mNC_Child;
    nsCOMPtr<nsIRDFResource>       mNC_Name;
    nsCOMPtr<nsIRDFResource>       mNC_URL;
    nsCOMPtr<nsIRDFResource>       mNC_Icon;
    nsCOMPtr<nsIRDFResource>       mNC_Length;
    nsCOMPtr<nsIRDFResource>       mNC_IsDirectory;
    nsCOMPtr<nsIRDFResource>       mWEB_LastMod;
    nsCOMPtr<nsIRDFResource>       mNC_FileSystemObject;
    nsCOMPtr<nsIRDFResource>       mNC_pulse;
    nsCOMPtr<nsIRDFResource>       mRDF_InstanceOf;
    nsCOMPtr<nsIRDFResource>       mRDF_type;

    nsCOMPtr<nsIRDFLiteral>        mLiteralTrue;
    nsCOMPtr<nsIRDFLiteral>        mLiteralFalse;

#ifdef USE_NC_EXTENSION
    nsresult GetExtension(nsIRDFResource *source, nsIRDFLiteral** aResult);
    nsCOMPtr<nsIRDFResource>       mNC_extension;
#endif

#ifdef  XP_WIN
    PRBool   isValidFolder(nsIRDFResource *source);
    nsresult getIEFavoriteURL(nsIRDFResource *source, nsString aFileURL, nsIRDFLiteral **urlLiteral);
    nsCOMPtr<nsIRDFResource>       mNC_IEFavoriteObject;
    nsCOMPtr<nsIRDFResource>       mNC_IEFavoriteFolder;
    nsCString                      ieFavoritesDir;
#endif

#ifdef  XP_BEOS
    nsresult getNetPositiveURL(nsIRDFResource *source, nsString aFileURL, nsIRDFLiteral **urlLiteral);
    nsCString netPositiveDir;
#endif
};

#endif 
