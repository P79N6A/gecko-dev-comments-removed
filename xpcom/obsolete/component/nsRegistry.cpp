




































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "nsIGenericFactory.h"

#include "nsRegistry.h"
#include "nsIEnumerator.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "NSReg.h"
#include "prmem.h"
#include "prlock.h"
#include "prlog.h"
#include "prprf.h"
#include "nsCRT.h"
#include "nsMemory.h"

#include "nsCOMPtr.h"
#include "nsILocalFile.h"
#include "nsIServiceManager.h"
#include "nsTextFormatter.h"

#ifdef XP_BEOS
#include <FindDirectory.h>
#include <Path.h>
#endif



#ifndef EXTRA_THREADSAFE
#define PR_Lock(x)           (void)0
#define PR_Unlock(x)         (void)0
#endif


extern NS_COM PRLogModuleInfo *nsComponentManagerLog;

PRUnichar widestrFormat[] = { PRUnichar('%'),PRUnichar('s'),PRUnichar(0)};













#define NS_MOZILLA_DIR_PERMISSION 00700

#include "nsRegistry.h"


























#include "nsIFactory.h"



struct nsRegistryFactory : public nsIFactory {
    
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD CreateInstance(nsISupports *,const nsIID &,void **);
    NS_IMETHOD LockFactory(PRBool aLock);

    
    nsRegistryFactory();
};






struct nsRegSubtreeEnumerator : public nsIRegistryEnumerator {
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIENUMERATOR

    
    NS_DECL_NSIREGISTRYENUMERATOR

    
    nsRegSubtreeEnumerator( HREG hReg, RKEY rKey, PRBool all );
    
    virtual ~nsRegSubtreeEnumerator();

protected:
    NS_IMETHOD advance(); 
    HREG    mReg;   
    RKEY    mKey;   
    char    mName[MAXREGPATHLEN]; 
    REGENUM mEnum;  
    REGENUM mNext;  
    PRUint32  mStyle; 
    PRBool  mDone;  
#ifdef EXTRA_THREADSAFE
    PRLock *mregLock;
#endif
}; 








struct nsRegValueEnumerator : public nsRegSubtreeEnumerator {
    
    NS_IMETHOD CurrentItem( nsISupports **result );

    
    NS_IMETHOD advance();

    
    nsRegValueEnumerator( HREG hReg, RKEY rKey );
}; 





struct nsRegistryNode : public nsIRegistryNode {
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIREGISTRYNODE

    
    nsRegistryNode( HREG hReg, char *name, RKEY childKey );
    
private:
    ~nsRegistryNode();

protected:
    HREG    mReg;  
    char    mName[MAXREGPATHLEN]; 
    RKEY    mChildKey;    
#ifdef EXTRA_THREADSAFE
    PRLock *mregLock;
#endif
}; 






struct nsRegistryValue : public nsIRegistryValue {
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIREGISTRYVALUE

    
    nsRegistryValue( HREG hReg, RKEY key, REGENUM slot );

private:
    ~nsRegistryValue();

protected:
    nsresult getInfo(); 
    HREG    mReg;  
    RKEY    mKey;  
    REGENUM mEnum; 
    REGINFO mInfo; 
    char    mName[MAXREGNAMELEN]; 
    REGERR  mErr; 
#ifdef EXTRA_THREADSAFE
    PRLock *mregLock;
#endif
}; 






static nsresult regerr2nsresult( REGERR err ) {
    nsresult rv = NS_ERROR_UNEXPECTED;
    switch( err ) {
        case REGERR_OK:
            rv = NS_OK;
            break;

        case REGERR_FAIL:
            rv = NS_ERROR_FAILURE;
            break;

        case REGERR_NOMORE:
            rv = NS_ERROR_REG_NO_MORE;
            break;
    
        case REGERR_NOFIND:
            rv = NS_ERROR_REG_NOT_FOUND;
            break;
    
        case REGERR_PARAM:
        case REGERR_BADTYPE:
        case REGERR_BADNAME:
            rv = NS_ERROR_INVALID_ARG;
            break;
    
        case REGERR_NOFILE:
            rv = NS_ERROR_REG_NOFILE;
            break;
    
        case REGERR_MEMORY:
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
    
        case REGERR_BUFTOOSMALL:
            rv = NS_ERROR_REG_BUFFER_TOO_SMALL;
            break;
    
        case REGERR_NAMETOOLONG:
            rv = NS_ERROR_REG_NAME_TOO_LONG;
            break;
    
        case REGERR_NOPATH:
            rv = NS_ERROR_REG_NO_PATH;
            break;
    
        case REGERR_READONLY:
            rv = NS_ERROR_REG_READ_ONLY;
            break;
    
        case REGERR_BADUTF8:
            rv = NS_ERROR_REG_BAD_UTF8;
            break;
    
    }
    return rv;
}





static void reginfo2DataType( const REGINFO &in, PRUint32 &out ) {
    
    switch( in.entryType ) {
        case REGTYPE_ENTRY_STRING_UTF:
            out = nsIRegistry::String;
            
            break;

        case REGTYPE_ENTRY_INT32_ARRAY:
            out = nsIRegistry::Int32;
            
            
            break;

        case REGTYPE_ENTRY_BYTES:
            out = nsIRegistry::Bytes;
            
            break;

        case REGTYPE_ENTRY_FILE:
            out = nsIRegistry::File;
            
            break;
    }
}





static void reginfo2Length( const REGINFO &in, PRUint32 &out ) {
    
    switch( in.entryType ) {
        case REGTYPE_ENTRY_STRING_UTF:
            out = in.entryLength;
            break;

        case REGTYPE_ENTRY_INT32_ARRAY:
            
            out = in.entryLength / sizeof(PRInt32);
            break;

        case REGTYPE_ENTRY_BYTES:
            out = in.entryLength;
            break;

        case REGTYPE_ENTRY_FILE:
            out = in.entryLength;
            break;
    }
}





NS_IMPL_THREADSAFE_ISUPPORTS2(nsRegistry,  nsIRegistry, nsIRegistryGetter)
NS_IMPL_ISUPPORTS2( nsRegSubtreeEnumerator, nsIEnumerator,
                    nsIRegistryEnumerator)
NS_IMPL_ISUPPORTS1( nsRegistryNode,         nsIRegistryNode  )
NS_IMPL_ISUPPORTS1( nsRegistryValue,        nsIRegistryValue )




nsRegistry::nsRegistry() 
    : mReg(0), mCurRegID(0) {
#ifdef EXTRA_THREADSAFE
    mregLock = PR_NewLock();
#endif
    NR_StartupRegistry();
    return;
}




nsRegistry::~nsRegistry() {
    if( mReg ) {
        Close();
    }
#ifdef EXTRA_THREADSAFE
    if (mregLock) {
        PR_DestroyLock(mregLock);
    }
#endif
    NR_ShutdownRegistry();
    return;
}






NS_IMETHODIMP nsRegistry::Open( nsIFile *regFile ) {
    REGERR err = REGERR_OK;

    
    if( !regFile ) {
        return OpenWellKnownRegistry(nsIRegistry::ApplicationRegistry);
    }

    nsCAutoString regPath;
    nsresult rv = regFile->GetNativePath(regPath);
    if (NS_FAILED(rv)) return rv;

#ifdef DEBUG_dp
    printf("nsRegistry: Opening registry %s\n", regPath.get());
#endif 
   
    if (mCurRegID != nsIRegistry::None && mCurRegID != nsIRegistry::ApplicationCustomRegistry)
    {
        
        return NS_ERROR_INVALID_ARG;
    }

    
    if (mCurRegID != nsIRegistry::None)
    {
        PRBool equals;
        if (mCurRegFile && NS_SUCCEEDED(mCurRegFile->Equals(regFile, &equals)) && equals)
        {
            
            return NS_OK;
        }
        else
        {
            
            
            return NS_ERROR_FAILURE;
        }
    }

    
    PR_Lock(mregLock);
    err = NR_RegOpen(const_cast<char*>(regPath.get()), &mReg);
    PR_Unlock(mregLock);

    mCurRegID = nsIRegistry::ApplicationCustomRegistry;

    
    if (NS_FAILED(regFile->Clone(getter_AddRefs(mCurRegFile))))
        mCurRegFile = nsnull; 

    
    return regerr2nsresult( err );
}

static void
EnsureDefaultRegistryDirectory() {
#if defined(XP_UNIX) && !defined(XP_MACOSX)
    

    















    char *home = getenv("HOME");
    if (home != NULL)
    {
        char dotMozillaDir[1024];
        PR_snprintf(dotMozillaDir, sizeof(dotMozillaDir),
                    "%s/" MOZ_USER_DIR, home);
        if (PR_Access(dotMozillaDir, PR_ACCESS_EXISTS) != PR_SUCCESS)
        {
            PR_MkDir(dotMozillaDir, NS_MOZILLA_DIR_PERMISSION);
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Creating Directory %s", dotMozillaDir));
        }
    }
#endif 

#ifdef XP_BEOS
    BPath p;
    const char *settings = "/boot/home/config/settings";
    if(find_directory(B_USER_SETTINGS_DIRECTORY, &p) == B_OK)
        settings = p.Path();
    char settingsMozillaDir[1024];
    PR_snprintf(settingsMozillaDir, sizeof(settingsMozillaDir),
                "%s/" MOZ_USER_DIR, settings);
    if (PR_Access(settingsMozillaDir, PR_ACCESS_EXISTS) != PR_SUCCESS) {
        PR_MkDir(settingsMozillaDir, NS_MOZILLA_DIR_PERMISSION);
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: Creating Directory %s", settingsMozillaDir));
    }
#endif
}





NS_IMETHODIMP nsRegistry::OpenWellKnownRegistry( nsWellKnownRegistry regid ) 
{
    REGERR err = REGERR_OK;

    if (mCurRegID != nsIRegistry::None && mCurRegID != regid)
    {
        
        return NS_ERROR_INVALID_ARG;
    }

    if (mCurRegID == regid)
    {
        
        return NS_OK;
    }

    nsresult rv;
    nsCOMPtr<nsIFile> registryLocation;

    PRBool foundReg = PR_FALSE;
    nsCAutoString regFile;
    
    switch ( (nsWellKnownRegistry) regid ) {
      case ApplicationComponentRegistry:
        NS_WARNING("ApplicationComponentRegistry is unsupported!");
        break;
      case ApplicationRegistry:
        {
            EnsureDefaultRegistryDirectory();
            nsCOMPtr<nsIProperties> directoryService = do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
            if (NS_FAILED(rv)) return rv;
            directoryService->Get(NS_APP_APPLICATION_REGISTRY_FILE, NS_GET_IID(nsIFile), 
                                  getter_AddRefs(registryLocation));

            if (registryLocation)
            {
                foundReg = PR_TRUE;
                rv = registryLocation->GetNativePath(regFile);  
                
                
                if (NS_FAILED(rv))
                  return rv;
            }
        }
        break;

      default:
        break;
    }

    if (foundReg == PR_FALSE) {
        return NS_ERROR_REG_BADTYPE;
    }
   
#ifdef DEBUG_dp
    printf("nsRegistry: Opening std registry %s\n", regFile.get());
#endif 

    PR_Lock(mregLock);
    err = NR_RegOpen(const_cast<char*>(regFile.get()), &mReg );
    PR_Unlock(mregLock);

    
    mCurRegID = regid;

    
    return regerr2nsresult( err );
}

#if 0




NS_IMETHODIMP nsRegistry::OpenDefault() {
    return OpenWellKnownRegistry(nsIRegistry::ApplicationRegistry);
}
#endif




NS_IMETHODIMP nsRegistry::Close() {
    REGERR err = REGERR_OK;
    if( mReg ) {
        PR_Lock(mregLock);
        err = NR_RegClose( mReg );
        PR_Unlock(mregLock);
        mReg = 0;
        mCurRegFile = nsnull;
        mCurRegID = 0;
    }
    return regerr2nsresult( err );
}




NS_IMETHODIMP nsRegistry::Flush() {
    REGERR err = REGERR_FAIL;
    if( mReg ) {
        PR_Lock(mregLock);
        err = NR_RegFlush( mReg );
        PR_Unlock(mregLock);
    }
    return regerr2nsresult( err );
}




NS_IMETHODIMP nsRegistry::IsOpen( PRBool *result ) {
    *result = ( mReg != 0 );
    return NS_OK;
}






NS_IMETHODIMP nsRegistry::AddKey( nsRegistryKey baseKey, const PRUnichar *keyname, nsRegistryKey *_retval)
{
    if ( !keyname ) 
        return NS_ERROR_NULL_POINTER;

    return AddSubtree( baseKey, NS_ConvertUTF16toUTF8(keyname).get(), _retval );
}




NS_IMETHODIMP nsRegistry::GetKey(nsRegistryKey baseKey, const PRUnichar *keyname, nsRegistryKey *_retval)
{
    if ( !keyname || !_retval ) 
        return NS_ERROR_NULL_POINTER;

    return GetSubtree( baseKey, NS_ConvertUTF16toUTF8(keyname).get(), _retval );
}




NS_IMETHODIMP nsRegistry::RemoveKey(nsRegistryKey baseKey, const PRUnichar *keyname)
{
    if ( !keyname ) 
        return NS_ERROR_NULL_POINTER;

    return RemoveSubtree( baseKey, NS_ConvertUTF16toUTF8(keyname).get() );
}

NS_IMETHODIMP nsRegistry::GetString(nsRegistryKey baseKey, const PRUnichar *valname, PRUnichar **_retval)
{
    
    if ( !valname || !_retval )
        return NS_ERROR_NULL_POINTER;

    
    *_retval = nsnull;
    nsXPIDLCString tmpstr;

    nsresult rv = GetStringUTF8( baseKey, NS_ConvertUTF16toUTF8(valname).get(), getter_Copies(tmpstr) );

    if (NS_SUCCEEDED(rv))
    {
        *_retval = nsTextFormatter::smprintf( widestrFormat, tmpstr.get() );
        if ( *_retval == nsnull )
            rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}

NS_IMETHODIMP nsRegistry::SetString(nsRegistryKey baseKey, const PRUnichar *valname, const PRUnichar *value)
{
    if ( !valname || ! value )
        return NS_ERROR_NULL_POINTER;

    return SetStringUTF8( baseKey,
                          NS_ConvertUTF16toUTF8(valname).get(),
                          NS_ConvertUTF16toUTF8(value).get() );
}





NS_IMETHODIMP nsRegistry::GetStringUTF8( nsRegistryKey baseKey, const char *path, char **result ) {
    nsresult rv = NS_OK;
    REGERR   err = REGERR_OK;

    
    if ( !result )
        return NS_ERROR_NULL_POINTER;

    char   regStr[MAXREGPATHLEN];

    
    *result = 0;

    
    PR_Lock(mregLock);
    err = NR_RegGetEntryString( mReg,(RKEY)baseKey,(char*)path, regStr,
                                sizeof(regStr) );
    PR_Unlock(mregLock);

    if ( err == REGERR_OK )
    {
        *result = nsCRT::strdup(regStr);
        if (!*result)
            rv = NS_ERROR_OUT_OF_MEMORY;
    }
    else if ( err == REGERR_BUFTOOSMALL ) 
    {
        
        PRUint32 length;
        rv = GetValueLength( baseKey, path, &length );
        
        if( rv == NS_OK ) 
        {
            *result =(char*)nsMemory::Alloc( length + 1 );
            if( *result ) 
            {
                
                PR_Lock(mregLock);
                err = NR_RegGetEntryString( mReg,(RKEY)baseKey,(char*)path, *result, length+1 );
                PR_Unlock(mregLock);

                
                rv = regerr2nsresult( err );
                if ( rv != NS_OK )
                {
                    
                    nsCRT::free( *result );
                    *result = 0;
                }
            }
            else
            {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
        }
    }
    else
    {
        
        rv = regerr2nsresult( err );
        NS_ASSERTION(NS_FAILED(rv), "returning success code on failure");
    }

   return rv;
}

NS_IMETHODIMP
nsRegistry::GetStringUTF8IntoBuffer( nsRegistryKey baseKey, const char *path,
                                     char *buf, PRUint32 *length )
{
    REGERR   err = REGERR_OK;

    
    PR_Lock(mregLock);
    err = NR_RegGetEntryString( mReg,(RKEY)baseKey,(char*)path, buf, *length );
    PR_Unlock(mregLock);

    
    nsresult rv = regerr2nsresult( err );

    if (rv == NS_ERROR_REG_BUFFER_TOO_SMALL) {
      
      nsresult rv1 = GetValueLength( baseKey, path, length );
      if(NS_FAILED(rv1))
        return rv1;
    }

    return rv;
}




NS_IMETHODIMP nsRegistry::SetStringUTF8( nsRegistryKey baseKey, const char *path, const char *value ) {
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);
    err = NR_RegSetEntryString( mReg,(RKEY)baseKey,(char*)path,(char*)value );
    PR_Unlock(mregLock);
    
    return regerr2nsresult( err );
}





NS_IMETHODIMP nsRegistry::GetBytesUTF8( nsRegistryKey baseKey, const char *path, PRUint32* length, PRUint8** result) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    
    if ( !result )
        return NS_ERROR_NULL_POINTER;

    char   regStr[MAXREGPATHLEN];

    
    *length = 0;
    *result = 0;

    
    PRUint32 type;
    rv = GetValueType( baseKey, path, &type );
    
    if( rv == NS_OK ) 
    {
            
        if( type == Bytes ) 
        {
            
            PR_Lock(mregLock);
            uint32 length2 = sizeof regStr;
            err = NR_RegGetEntry( mReg,(RKEY)baseKey,const_cast<char*>(path), regStr, &length2);
            PR_Unlock(mregLock);

            if ( err == REGERR_OK )
            {
                *length = length2;
                *result = (PRUint8*)(nsCRT::strdup(regStr));
                if (!*result)
                {
                    rv = NS_ERROR_OUT_OF_MEMORY;
                    *length = 0;
                }
                else
                {
                    *length = length2;
                }
            }
            else if ( err == REGERR_BUFTOOSMALL ) 
            {
            
                rv = GetValueLength( baseKey, path, length );
                
                if( rv == NS_OK ) 
                {
                    *result = reinterpret_cast<PRUint8*>(nsMemory::Alloc( *length ));
                    if( *result ) 
                    {
                        
                        PR_Lock(mregLock);
                        length2 = *length;
                        err = NR_RegGetEntry( mReg,(RKEY)baseKey,const_cast<char*>(path), *result, &length2);
                        *length = length2;
                        PR_Unlock(mregLock);
                        
                        rv = regerr2nsresult( err );
                        if ( rv != NS_OK )
                        {
                            
                            nsCRT::free( reinterpret_cast<char*>(*result) );
                            *result = 0;
                            *length = 0;
                        }
                    }
                    else
                    {
                        rv = NS_ERROR_OUT_OF_MEMORY;
                    }
                }
            }
        } 
        else 
        {
            
            rv = NS_ERROR_REG_BADTYPE;
        }
    }
    return rv;
}

NS_IMETHODIMP
nsRegistry::GetBytesUTF8IntoBuffer( nsRegistryKey baseKey, const char *path,
                                    PRUint8 *buf, PRUint32* length )
{
    REGERR err = REGERR_OK;

    
    PRUint32 type;
    nsresult rv = GetValueType( baseKey, path, &type );
    
    if(NS_FAILED(rv)) 
      return rv;
    
    if (type != Bytes)
      return NS_ERROR_REG_BADTYPE;

    
    PR_Lock(mregLock);
    err = NR_RegGetEntry( mReg,(RKEY)baseKey,const_cast<char*>(path),
                          buf, (uint32 *)length );
    PR_Unlock(mregLock);

    rv = regerr2nsresult(rv);

    if (rv == NS_ERROR_REG_BUFFER_TOO_SMALL) {
      
      nsresult rv1 = GetValueLength( baseKey, path, length );
      if(NS_FAILED(rv1))
        return rv1;
    }


    return rv;
}





NS_IMETHODIMP nsRegistry::GetInt( nsRegistryKey baseKey, const char *path, PRInt32 *result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;

    
    if( result ) {
        
        PRUint32 type;
        rv = GetValueType( baseKey, path, &type );
        
        if( rv == NS_OK ) {
            
            if( type == Int32 ) {
                uint32 len = sizeof *result;
                
                PR_Lock(mregLock);
                err = NR_RegGetEntry( mReg,(RKEY)baseKey,(char*)path, result, &len );
                PR_Unlock(mregLock);
                
                rv = regerr2nsresult( err );
            } else {
                
                rv = NS_ERROR_REG_BADTYPE;
            }
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}






NS_IMETHODIMP nsRegistry::GetLongLong( nsRegistryKey baseKey, const char *path, PRInt64 *result ) {
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);
    
    uint32 length = sizeof(PRInt64);
    err = NR_RegGetEntry( mReg,(RKEY)baseKey,(char*)path,(void*)result,&length);
    
    PR_Unlock(mregLock);
    
    
    return regerr2nsresult( err );
}



NS_IMETHODIMP nsRegistry::SetBytesUTF8( nsRegistryKey baseKey, const char *path, PRUint32 length, PRUint8* value) {
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);
    err = NR_RegSetEntry( mReg,
                (RKEY)baseKey,
                (char*)path,
                           REGTYPE_ENTRY_BYTES,
                           (char*)value,
                           length);
    PR_Unlock(mregLock);
    
    return regerr2nsresult( err );
}




NS_IMETHODIMP nsRegistry::SetInt( nsRegistryKey baseKey, const char *path, PRInt32 value ) {
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);
    err = NR_RegSetEntry( mReg,
                (RKEY)baseKey,
                (char*)path,
                           REGTYPE_ENTRY_INT32_ARRAY,
                           &value,
                           sizeof value );
    PR_Unlock(mregLock);
    
    return regerr2nsresult( err );
}






NS_IMETHODIMP nsRegistry::SetLongLong( nsRegistryKey baseKey, const char *path, PRInt64* value ) {
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);

    err = NR_RegSetEntry( mReg,
                        (RKEY)baseKey,
                        (char*)path,
                        REGTYPE_ENTRY_BYTES,
                        (void*)value,
                        sizeof(PRInt64) );

    PR_Unlock(mregLock);
    
    return regerr2nsresult( err );
}




NS_IMETHODIMP nsRegistry::AddSubtree( nsRegistryKey baseKey, const char *path, nsRegistryKey *result ) {
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);
    err = NR_RegAddKey( mReg,(RKEY)baseKey,(char*)path,(RKEY*)result );
    PR_Unlock(mregLock);
    
    return regerr2nsresult( err );
}




NS_IMETHODIMP nsRegistry::AddSubtreeRaw( nsRegistryKey baseKey, const char *path, nsRegistryKey *result ) {
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);
    err = NR_RegAddKeyRaw( mReg,(RKEY)baseKey,(char*)path,(RKEY*)result );
    PR_Unlock(mregLock);
    
    return regerr2nsresult( err );
}





NS_IMETHODIMP nsRegistry::RemoveSubtree( nsRegistryKey baseKey, const char *path ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;

    
    

    RKEY key;

    PR_Lock(mregLock);
    err = NR_RegGetKey(mReg, baseKey, (char *)path, &key);
    PR_Unlock(mregLock);
    if (err != REGERR_OK)
    {
        rv = regerr2nsresult( err );
        return rv;
    }

    
    
    char subkeyname[MAXREGPATHLEN+1];
    REGENUM state = 0;
    subkeyname[0] = '\0';
    while (NR_RegEnumSubkeys(mReg, key, &state, subkeyname, sizeof(subkeyname),
           REGENUM_NORMAL) == REGERR_OK)
    {
#ifdef DEBUG_dp
        printf("...recursing into %s\n", subkeyname);
#endif 
        
        
        
        
        err = RemoveSubtreeRaw(key, subkeyname);
        if (err != REGERR_OK) break;
    }

    
    if (err == REGERR_OK)
    {
#ifdef DEBUG_dp
        printf("...deleting %s\n", path);
#endif 
        PR_Lock(mregLock);
        err = NR_RegDeleteKey(mReg, baseKey, (char *)path);
        PR_Unlock(mregLock);
    }

    
      rv = regerr2nsresult( err );
    return rv;
}





NS_IMETHODIMP nsRegistry::RemoveSubtreeRaw( nsRegistryKey baseKey, const char *keyname ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;

    
    

    RKEY key;
    char subkeyname[MAXREGPATHLEN+1];
    int n = sizeof(subkeyname);
    REGENUM state = 0;

    PR_Lock(mregLock);
    err = NR_RegGetKeyRaw(mReg, baseKey, (char *)keyname, &key);
    PR_Unlock(mregLock);
    if (err != REGERR_OK)
    {
        rv = regerr2nsresult( err );
        return rv;
    }

    
    
    subkeyname[0] = '\0';
    while (NR_RegEnumSubkeys(mReg, key, &state, subkeyname, n, REGENUM_NORMAL) == REGERR_OK)
    {
#ifdef DEBUG_dp
        printf("...recursing into %s\n", subkeyname);
#endif 
        err = RemoveSubtreeRaw(key, subkeyname);
        if (err != REGERR_OK) break;
    }

    
    if (err == REGERR_OK)
    {
#ifdef DEBUG_dp
        printf("...deleting %s\n", keyname);
#endif 
        PR_Lock(mregLock);
        err = NR_RegDeleteKeyRaw(mReg, baseKey, (char *)keyname);
        PR_Unlock(mregLock);
    }

    
      rv = regerr2nsresult( err );
    return rv;
}




NS_IMETHODIMP nsRegistry::GetSubtree( nsRegistryKey baseKey, const char *path, nsRegistryKey *result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    
    if( result ) {
        
        PR_Lock(mregLock);
        err = NR_RegGetKey( mReg,(RKEY)baseKey,(char*)path,(RKEY*)result );
        PR_Unlock(mregLock);
        
        rv = regerr2nsresult( err );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}





NS_IMETHODIMP nsRegistry::GetSubtreeRaw( nsRegistryKey baseKey, const char *path, nsRegistryKey *result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    
    if( result ) {
        
        PR_Lock(mregLock);
        err = NR_RegGetKeyRaw( mReg,(RKEY)baseKey,(char*)path,(RKEY*)result );
        PR_Unlock(mregLock);
        
        rv = regerr2nsresult( err );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}









NS_IMETHODIMP nsRegistry::EnumerateSubtrees( nsRegistryKey baseKey, nsIEnumerator **result ) {
    nsresult rv = NS_OK;
    
    if( result ) {
        *result = new nsRegSubtreeEnumerator( mReg,(RKEY)baseKey, PR_FALSE );
        
        if( *result ) {
            
          NS_ADDREF(*result);
        } else {
            
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}





NS_IMETHODIMP nsRegistry::EnumerateAllSubtrees( nsRegistryKey baseKey, nsIEnumerator **result ) {
    nsresult rv = NS_OK;
    
    if( result ) {
        *result = new nsRegSubtreeEnumerator( mReg,(RKEY)baseKey, PR_TRUE );
        
        if( *result ) {
            
          NS_ADDREF(*result);
        } else {
            
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}






NS_IMETHODIMP nsRegistry::GetValueType( nsRegistryKey baseKey, const char *path, PRUint32 *result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    
    if( result ) {
        
        REGINFO info = { sizeof info, 0, 0 };
        PR_Lock(mregLock);
        err = NR_RegGetEntryInfo( mReg,(RKEY)baseKey,(char*)path, &info );
        PR_Unlock(mregLock);
        if( err == REGERR_OK ) {
            
            reginfo2DataType( info, *result );
        } else {
            rv = regerr2nsresult( err );
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}





NS_IMETHODIMP nsRegistry::GetValueLength( nsRegistryKey baseKey, const char *path, PRUint32 *result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    
    if( result ) {
        
        REGINFO info = { sizeof info, 0, 0 };
        PR_Lock(mregLock);
        err = NR_RegGetEntryInfo( mReg,(RKEY)baseKey,(char*)path, &info );
        PR_Unlock(mregLock);
        if( err == REGERR_OK ) {
            
            reginfo2Length( info, *result );
        } else {
            rv = regerr2nsresult( err );
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}




NS_IMETHODIMP nsRegistry::DeleteValue( nsRegistryKey baseKey, const char *path)
{
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);
    err = NR_RegDeleteEntry( mReg,(RKEY)baseKey,(char*)path );
    PR_Unlock(mregLock);
    
    return regerr2nsresult( err );
}






NS_IMETHODIMP nsRegistry::EnumerateValues( nsRegistryKey baseKey, nsIEnumerator **result ) {
    nsresult rv = NS_OK;
    
    if( result ) {
        *result = new nsRegValueEnumerator( mReg,(RKEY)baseKey );
        
        if( *result ) {
            
            NS_ADDREF(*result);
        } else {
            
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}




NS_IMETHODIMP nsRegistry::GetCurrentUserName( char **result ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    
    if( result ) {
        
        PR_Lock(mregLock);
        err = NR_RegGetUsername( result );
        PR_Unlock(mregLock);
        
        rv = regerr2nsresult( err );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}




NS_IMETHODIMP nsRegistry::SetCurrentUserName( const char *name ) {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);
    err = NR_RegSetUsername( name );
    PR_Unlock(mregLock);
    
    rv = regerr2nsresult( err );
    return rv;
}




NS_IMETHODIMP nsRegistry::Pack() {
    nsresult rv = NS_OK;
    REGERR err = REGERR_OK;
    
    PR_Lock(mregLock);
    err = NR_RegPack( mReg, 0, 0 );
    PR_Unlock(mregLock);
    
    rv = regerr2nsresult( err );
    return rv;
}







static const char sEscapeKeyHex[] = "0123456789abcdef0123456789ABCDEF";
NS_IMETHODIMP nsRegistry::EscapeKey(PRUint8* key, PRUint32 termination, PRUint32* length, PRUint8** escaped)
{
    nsresult rv = NS_OK;
    char* value = (char*)key;
    char* b = value;
    char* e = b + *length;
    int escapees = 0;
    while (b < e)    
    {
        int c = *b++;
        if (c <= ' '
            || c > '~'
            || c == '/'
            || c == '%')
        {
            escapees++;
        }
    }
    if (escapees == 0)    
    {
        *length = 0;
        *escaped = nsnull;
        return NS_OK;
    }
    
    *length += escapees * 2;
    *escaped = (PRUint8*)nsMemory::Alloc(*length + termination);
    if (*escaped == nsnull)
    {
        *length = 0;
        *escaped = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    char* n = (char*)*escaped;
    b = value;
    while (escapees && b < e)
    {
        char c = *b++;
        if (c < ' '
            || c > '~'
            || c == '/'
            || c == '%')
        {
            *(n++) = '%';
            *(n++) = sEscapeKeyHex[ 0xF & (c >> 4) ];
            *(n++) = sEscapeKeyHex[ 0xF & c ];
            escapees--;
        }
        else
        {
            *(n++) = c;
        }
    }
    e += termination;
    if (b < e)
    {
        strncpy(n, b, e - b);
    }
    return rv;
}







NS_IMETHODIMP nsRegistry::UnescapeKey(PRUint8* escaped, PRUint32 termination, PRUint32* length, PRUint8** key)
{
    nsresult rv = NS_OK;
    char* value = (char*)escaped;
    char* b = value;
    char* e = b + *length;
    int escapees = 0;
    while (b < e)    
    {
        if (*b++ == '%')
        {
            escapees++;
        }
    }
    if (escapees == 0)    
    {
        *length = 0;
        *key = nsnull;
        return NS_OK;
    }
    
    *length -= escapees * 2;
    *key = (PRUint8*)nsMemory::Alloc(*length + termination);
    if (*key == nsnull)
    {
        *length = 0;
        *key = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    char* n = (char*)*key;
    b = value;
    while (escapees && b < e)
    {
        char c = *(b++);
        if (c == '%')
        {
            if (e - b >= 2)
            {
                const char* c1 = strchr(sEscapeKeyHex, *(b++));
                const char* c2 = strchr(sEscapeKeyHex, *(b++));
                if (c1 != nsnull
                    && c2 != nsnull)
                {
                    *(n++) = ((c2 - sEscapeKeyHex) & 0xF)
                        | (((c1 - sEscapeKeyHex) & 0xF) << 4);
                }
                else
                {
                    escapees = -1;
                }
            }
            else
            {
                escapees = -1;
            }
            escapees--;
        }
        else
        {
            *(n++) = c;
        }
    }
    if (escapees < 0)
    {
        nsMemory::Free(*key);
        *length = 0;
        *key = nsnull;
        return NS_ERROR_INVALID_ARG;
    }
    e += termination;
    if (b < e)
    {
        strncpy(n, b, e - b);
    }
    return rv;
}





int nsRegistry::SetBufferSize( int bufsize )
{
    int newSize;
    
    PR_Lock(mregLock);
    newSize = NR_RegSetBufferSize( mReg, bufsize );
    PR_Unlock(mregLock);
    return newSize;
}






nsRegSubtreeEnumerator::nsRegSubtreeEnumerator( HREG hReg, RKEY rKey, PRBool all )
    : mReg( hReg ), mKey( rKey ), mEnum( 0 ), mNext( 0 ),
      mStyle( all ? REGENUM_DESCEND : REGENUM_CHILDREN ), mDone( PR_FALSE ) {

    mName[0] = '\0';

#ifdef EXTRA_THREADSAFE
    
    mregLock = PR_NewLock();
#endif
    return;
}

nsRegSubtreeEnumerator::~nsRegSubtreeEnumerator()
{
#ifdef EXTRA_THREADSAFE
    if (mregLock) {
        PR_DestroyLock(mregLock);
    }
#endif
}






NS_IMETHODIMP
nsRegSubtreeEnumerator::First() {
    nsresult rv = NS_OK;
    
    mDone = PR_FALSE;
    
    mName[0] = '\0';
    
    mEnum = mNext = 0;
    
    rv = Next();
    return rv;
}









NS_IMETHODIMP
nsRegSubtreeEnumerator::Next() {
    nsresult rv = NS_OK;
    
    if ( !mDone ) {
        
        mEnum = mNext;
        
        rv = advance();
    } else {
        
        rv = regerr2nsresult( REGERR_NOMORE );
    }
    return rv;
}





NS_IMETHODIMP nsRegSubtreeEnumerator::advance() {
    REGERR err = REGERR_OK;
    PR_Lock(mregLock);
    err = NR_RegEnumSubkeys( mReg, mKey, &mNext, mName, sizeof mName, mStyle );
    
    if( err == REGERR_NOMORE ) {
        
        mDone = PR_TRUE;
    }
    PR_Unlock(mregLock);
    
    nsresult rv = regerr2nsresult( err );
    return rv;
}






NS_IMETHODIMP
nsRegSubtreeEnumerator::CurrentItem( nsISupports **result) {
    nsresult rv = NS_OK;
    
    if( result ) {
        *result = new nsRegistryNode( mReg, mName, (RKEY) mNext );
        if( *result ) {
            NS_ADDREF(*result);
        } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}





NS_IMETHODIMP
nsRegSubtreeEnumerator::CurrentItemInPlaceUTF8(  nsRegistryKey *childKey ,
                                                 const char **name )
{
  *childKey = mNext;
  
  *name = mName;
  return NS_OK;
}




NS_IMETHODIMP
nsRegSubtreeEnumerator::IsDone() {
    nsresult rv = mDone ? NS_OK : NS_ENUMERATOR_FALSE;
    return rv;
}





nsRegValueEnumerator::nsRegValueEnumerator( HREG hReg, RKEY rKey )
    : nsRegSubtreeEnumerator( hReg, rKey, PR_FALSE ) {
    return;
}






NS_IMETHODIMP
nsRegValueEnumerator::CurrentItem( nsISupports **result ) {
    nsresult rv = NS_OK;
    
    if( result ) {
        *result = new nsRegistryValue( mReg, mKey, mEnum );
        if( *result ) {
            NS_ADDREF(*result);
        } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}





NS_IMETHODIMP nsRegValueEnumerator::advance() {
    REGERR err = REGERR_OK;
    char name[MAXREGNAMELEN];
    PRUint32 len = sizeof name;
    REGINFO info = { sizeof info, 0, 0 };
    PR_Lock(mregLock);
    err = NR_RegEnumEntries( mReg, mKey, &mNext, name, len, &info );
    
    if( err == REGERR_NOMORE ) {
        
        mDone = PR_TRUE;
    }
    PR_Unlock(mregLock);
    
    nsresult rv = regerr2nsresult( err );
    return rv;
}







nsRegistryNode::nsRegistryNode( HREG hReg, char *name, RKEY childKey )
    : mReg( hReg ), mChildKey( childKey ) {

    PR_ASSERT(name != nsnull);
    strcpy(mName, name);

#ifdef EXTRA_THREADSAFE
    mregLock = PR_NewLock();
#endif
    
    return;
}

nsRegistryNode::~nsRegistryNode()
{
#ifdef EXTRA_THREADSAFE
    if (mregLock) {
        PR_DestroyLock(mregLock);
    }
#endif
}





NS_IMETHODIMP nsRegistryNode::GetName( PRUnichar **result ) {
    if (result == nsnull) return NS_ERROR_NULL_POINTER;
    
    *result = nsTextFormatter::smprintf( widestrFormat, mName );
    if ( !*result ) return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}





NS_IMETHODIMP nsRegistryNode::GetNameUTF8( char **result ) {
    if (result == nsnull) return NS_ERROR_NULL_POINTER;
    
    *result = nsCRT::strdup( mName );
    if ( !*result ) return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}





NS_IMETHODIMP nsRegistryNode::GetKey( nsRegistryKey *r_key ) {
    nsresult rv = NS_OK;
    if (r_key == nsnull) return NS_ERROR_NULL_POINTER;
    *r_key = mChildKey;
    return rv;
}
    





nsRegistryValue::nsRegistryValue( HREG hReg, RKEY key, REGENUM slot )
    : mReg( hReg ), mKey( key ), mEnum( slot ), mErr( -1 ) {
#ifdef EXTRA_THREADSAFE
    mregLock = PR_NewLock();
#endif
    mInfo.size = sizeof(REGINFO);
}

nsRegistryValue::~nsRegistryValue()
{
#ifdef EXTRA_THREADSAFE
    if (mregLock) {
        PR_DestroyLock(mregLock);
    }
#endif
}




NS_IMETHODIMP nsRegistryValue::GetName( PRUnichar **result ) {
    nsresult rv = NS_OK;
    
    if( result ) {
        
        rv = getInfo();            
        if( rv == NS_OK || rv == NS_ERROR_REG_NO_MORE ) {
            
            *result = nsTextFormatter::smprintf( widestrFormat, mName );
            if ( *result ) {
                rv = NS_OK;
            } else {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}




NS_IMETHODIMP nsRegistryValue::GetNameUTF8( char **result ) {
    nsresult rv = NS_OK;
    
    if( result ) {
        
        rv = getInfo();            
        if( rv == NS_OK || rv == NS_ERROR_REG_NO_MORE ) {
            
            *result = nsCRT::strdup( mName );
            if ( *result ) {
                rv = NS_OK;
            } else {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}






NS_IMETHODIMP nsRegistryValue::GetType( PRUint32 *result ) {
    nsresult rv = NS_OK;
    
    if( result ) {
        
        rv = getInfo();
        
        if( rv == NS_OK ) {
            
            reginfo2DataType( mInfo, *result );
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}






NS_IMETHODIMP nsRegistryValue::GetLength( PRUint32 *result ) {
    nsresult rv = NS_OK;
    
    if( result ) {
        
        rv = getInfo();
        
        if( rv == NS_OK ) {
            
            reginfo2Length( mInfo, *result );
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}




nsresult nsRegistryValue::getInfo() {
    nsresult rv = NS_OK;
    
    if( mErr == -1 ) {
        REGENUM temp = mEnum;
        
        PR_Lock(mregLock);
        mErr = NR_RegEnumEntries( mReg, mKey, &temp, mName, sizeof mName, &mInfo );
        
        rv = regerr2nsresult( mErr );            
        PR_Unlock(mregLock);
    }
    return rv;
}


nsRegistryFactory::nsRegistryFactory() {
}

NS_IMPL_ISUPPORTS1(nsRegistryFactory, nsIFactory)

NS_IMETHODIMP
nsRegistryFactory::CreateInstance(nsISupports *aOuter,
                                   const nsIID &aIID,
                                   void **aResult) {
    nsresult rv = NS_OK;
    nsRegistry* newRegistry;

    if(aResult == nsnull) {
        return NS_ERROR_NULL_POINTER;
    } else {
        *aResult = nsnull;
    }

    if(0 != aOuter) {
        return NS_ERROR_NO_AGGREGATION;
    }

    NS_NEWXPCOM(newRegistry, nsRegistry);

    if(newRegistry == nsnull) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(newRegistry);
    rv = newRegistry->QueryInterface(aIID, aResult);
    NS_RELEASE(newRegistry);

    return rv;
}

nsresult
nsRegistryFactory::LockFactory(PRBool aLock)
{
  
  return NS_OK;
}



extern "C" NS_EXPORT nsresult
NS_RegistryGetFactory(nsIFactory** aFactory ) {
    nsresult rv = NS_OK;

    if( aFactory == 0 ) {
        return NS_ERROR_NULL_POINTER;
    } else {
        *aFactory = 0;
    }

    nsIFactory* inst = new nsRegistryFactory();
    if(0 == inst) {
        rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
        NS_ADDREF(inst);
        *aFactory = inst;
    }

    return rv;
}
