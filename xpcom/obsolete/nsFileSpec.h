













































































































































#ifndef _FILESPEC_H_
#define _FILESPEC_H_

#include "xpcomobsolete.h"
#include "nsError.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "prtypes.h"




#if !defined(NS_USING_NAMESPACE) && (defined(__MWERKS__) || defined(_MSC_VER))
#define NS_USING_NAMESPACE
#endif

#ifdef NS_USING_NAMESPACE

#define NS_NAMESPACE_PROTOTYPE
#define NS_NAMESPACE namespace
#define NS_NAMESPACE_END
#define NS_EXPLICIT explicit
#else

#define NS_NAMESPACE_PROTOTYPE static
#define NS_NAMESPACE struct
#define NS_NAMESPACE_END ;
#define NS_EXPLICIT

#endif


#include "nsILocalFile.h"
#include "nsCOMPtr.h"

#if defined(XP_UNIX) || defined(XP_BEOS)
#include <dirent.h>
#elif defined(XP_WIN)






#ifdef CreateDirectory
#undef CreateDirectory
#endif

#include "prio.h"
#elif defined(XP_OS2)
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include "prio.h"
#endif





class nsFileSpec;             
class nsFilePath;
class nsFileURL;
class nsNSPRPath;             
class nsPersistentFileDescriptor; 

#define kFileURLPrefix "file://"
#define kFileURLPrefixLength (7)

class nsOutputStream;
class nsInputStream;
class nsIOutputStream;
class nsIInputStream;
class nsOutputFileStream;
class nsInputFileStream;
class nsOutputConsoleStream;

class nsIUnicodeEncoder;
class nsIUnicodeDecoder;








#define NS_FILE_RESULT(x) ns_file_convert_result((PRInt32)x)
nsresult ns_file_convert_result(PRInt32 nativeErr);
#define NS_FILE_FAILURE NS_FILE_RESULT(-1)


class nsSimpleCharString



{
public:
                                 nsSimpleCharString();
                                 nsSimpleCharString(const char*);
                                 nsSimpleCharString(const nsString&);
                                 nsSimpleCharString(const nsSimpleCharString&);
                                 nsSimpleCharString(const char* inData, PRUint32 inLength);
                                 
                                 ~nsSimpleCharString();
                                 
    void                         operator = (const char*);
    void                         operator = (const nsString&);
    void                         operator = (const nsSimpleCharString&);
                                 
                                 operator const char*() const { return mData ? mData->mString : 0; }
                                 operator char* ()
                                 {
                                     ReallocData(Length()); 
                                     return mData ? mData->mString : 0;
                                 }
    PRBool                       operator == (const char*);
    PRBool                       operator == (const nsString&);
    PRBool                       operator == (const nsSimpleCharString&);

    void                         operator += (const char* inString);
    nsSimpleCharString           operator + (const char* inString) const;
    
    char                         operator [](int i) const { return mData ? mData->mString[i] : '\0'; }
    char&                        operator [](int i)
                                 {
                                     if (i >= (int)Length())
                                         ReallocData((PRUint32)i + 1);
                                     return mData->mString[i]; 
                                 }
    char&                        operator [](unsigned int i) { return (*this)[(int)i]; }
    
    void                         Catenate(const char* inString1, const char* inString2);
   
    void                         SetToEmpty(); 
    PRBool                       IsEmpty() const { return Length() == 0; }
    
    PRUint32                     Length() const { return mData ? mData->mLength : 0; }
    void                         SetLength(PRUint32 inLength) { ReallocData(inLength); }
    void                         CopyFrom(const char* inData, PRUint32 inLength);
    void                         LeafReplace(char inSeparator, const char* inLeafName);
    char*                        GetLeaf(char inSeparator) const; 
    void                         Unescape();

protected:

    void                         AddRefData();
    void                         ReleaseData();
    void                         ReallocData(PRUint32 inLength);

    
    
    

protected:

    struct Data {
        int         mRefCount;
        PRUint32    mLength;
        char        mString[1];
        };
    Data*                        mData;
}; 


class NS_COM_OBSOLETE nsFileSpec



{
    public:
                                nsFileSpec();
                                
                                
        NS_EXPLICIT             nsFileSpec(const char* inNativePath, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFileSpec(const nsString& inNativePath, PRBool inCreateDirs = PR_FALSE);
                                
        
        NS_EXPLICIT             nsFileSpec(const nsFilePath& inPath);
        NS_EXPLICIT             nsFileSpec(const nsFileURL& inURL);
                                nsFileSpec(const nsFileSpec& inPath);
        virtual                 ~nsFileSpec();

                                
        void                    operator = (const char* inNativePath);

        void                    operator = (const nsFilePath& inPath);
        void                    operator = (const nsFileURL& inURL);
        void                    operator = (const nsFileSpec& inOther);
        void                    operator = (const nsPersistentFileDescriptor& inOther);

        PRBool                  operator ==(const nsFileSpec& inOther) const;
        PRBool                  operator !=(const nsFileSpec& inOther) const;


                                
                                
                                
                                
                                
                                
                                
                                
       const char*              GetCString() const;

                                
                                
                                operator const char* () const { return GetCString(); }

                                
                                
       const char*              GetNativePathCString() const { return GetCString(); }

       PRBool                   IsChildOf(nsFileSpec &possibleParent);

        PRBool                  Valid() const { return NS_SUCCEEDED(Error()); }
        nsresult                Error() const
                                {
                                    if (mPath.IsEmpty() && NS_SUCCEEDED(mError)) 
                                        ((nsFileSpec*)this)->mError = NS_ERROR_NOT_INITIALIZED; 
                                    return mError;
                                }
        PRBool                  Failed() const { return (PRBool)NS_FAILED(Error()); }

    
    
    

        char*                   GetLeafName() const; 
                                
                                
        void                    SetLeafName(const char* inLeafName);

                                
                                
                                
                                
                                
                                
        void                    GetParent(nsFileSpec& outSpec) const;


                                
                                
                                
                                
                                
                                
        typedef PRUint32        TimeStamp;

                                
                                
                                
                                
        void                    GetModDate(TimeStamp& outStamp) const;

        PRBool                  ModDateChanged(const TimeStamp& oldStamp) const
                                {
                                    TimeStamp newStamp;
                                    GetModDate(newStamp);
                                    return newStamp != oldStamp;
                                }
        
        PRUint32                GetFileSize() const;
        PRInt64                 GetDiskSpaceAvailable() const;
        
        nsFileSpec              operator + (const char* inRelativeUnixPath) const;

                                
                                
                                
                                
                                
                                
                                
                                
        void                    operator += (const char* inRelativeUnixPath);


                               
        PRBool                  IsDirectory() const;          
        PRBool                  IsFile() const;               
        PRBool                  Exists() const;

        PRBool                  IsHidden() const;
        
        PRBool                  IsSymlink() const;

    
    
    

    
        void                    MakeUnique(PRBool inCreateFile = PR_TRUE);
        void                    MakeUnique(const char* inSuggestedLeafName, PRBool inCreateFile = PR_TRUE);
    
                                
                                
        nsresult                ResolveSymlink(PRBool& wasSymlink);

        void                    CreateDirectory(int mode = 0775 );
        void                    CreateDir(int mode = 0775) { CreateDirectory(mode); }
                                   
        void                    Delete(PRBool inRecursive) const;
        nsresult                Truncate(PRInt32 aNewLength) const;
        void                    RecursiveCopy(nsFileSpec newDir) const;
        
        nsresult                Rename(const char* inNewName); 
        nsresult                CopyToDir(const nsFileSpec& inNewParentDirectory) const;
        nsresult                MoveToDir(const nsFileSpec& inNewParentDirectory);
        nsresult                Execute(const char* args) const;

    protected:

    
    
    

    protected:
    
                                
                                
       void                     Clear();
       
                                friend class nsFilePath;
                                friend class nsFileURL;
                                friend class nsDirectoryIterator;

        nsSimpleCharString      mPath;
        nsresult                mError;

private:
        NS_EXPLICIT             nsFileSpec(const nsPersistentFileDescriptor& inURL);
  
}; 



typedef nsFileSpec nsNativeFileSpec;


class NS_COM_OBSOLETE nsFileURL




{
    public:
                                nsFileURL(const nsFileURL& inURL);
        NS_EXPLICIT             nsFileURL(const char* inURLString, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFileURL(const nsString& inURLString, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFileURL(const nsFilePath& inPath);
        NS_EXPLICIT             nsFileURL(const nsFileSpec& inPath);
        virtual                 ~nsFileURL();


                                    
                                    

        void                    operator = (const nsFileURL& inURL);
        void                    operator = (const char* inURLString);
        void                    operator = (const nsString& inURLString)
                                {
                                    *this = NS_LossyConvertUTF16toASCII(inURLString).get();
                                }
        void                    operator = (const nsFilePath& inOther);
        void                    operator = (const nsFileSpec& inOther);

        void                    operator +=(const char* inRelativeUnixPath);
        nsFileURL               operator +(const char* inRelativeUnixPath) const;
                                operator const char* () const { return (const char*)mURL; } 
        const char*             GetURLString() const { return (const char*)mURL; }
        							
        const char*             GetAsString() const { return (const char*)mURL; }
        							

    
    
    

    protected:
                                friend class nsFilePath; 
        nsSimpleCharString      mURL;
}; 


class NS_COM_OBSOLETE nsFilePath




{
    public:
                                nsFilePath(const nsFilePath& inPath);
        NS_EXPLICIT             nsFilePath(const char* inUnixPathString, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFilePath(const nsString& inUnixPathString, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFilePath(const nsFileURL& inURL);
        NS_EXPLICIT             nsFilePath(const nsFileSpec& inPath);
        virtual                 ~nsFilePath();

                                
                                operator const char* () const { return mPath; }
                                    
                                    
                                    

        void                    operator = (const nsFilePath& inPath);
        void                    operator = (const char* inUnixPathString);
        void                    operator = (const nsString& inUnixPathString)
                                {
                                    *this = NS_LossyConvertUTF16toASCII(inUnixPathString).get();
                                }
        void                    operator = (const nsFileURL& inURL);
        void                    operator = (const nsFileSpec& inOther);

        void                    operator +=(const char* inRelativeUnixPath);
        nsFilePath              operator +(const char* inRelativeUnixPath) const;

    
    
    

    private:

        nsSimpleCharString       mPath;
}; 


class nsPersistentFileDescriptor





{
    public:
                                nsPersistentFileDescriptor() {}
                                    
                                nsPersistentFileDescriptor(const nsPersistentFileDescriptor& inEncodedData);
        virtual                 ~nsPersistentFileDescriptor();
        void					operator = (const nsPersistentFileDescriptor& inEncodedData);
        
        
        NS_EXPLICIT             nsPersistentFileDescriptor(const nsFileSpec& inSpec);
        void					operator = (const nsFileSpec& inSpec);
        
		
		
	
    	friend nsresult         Read(nsIInputStream* aStream, nsPersistentFileDescriptor&);
    	friend nsresult         Write(nsIOutputStream* aStream, const nsPersistentFileDescriptor&);
    	    
    	friend NS_COM_OBSOLETE nsInputStream& operator >> (nsInputStream&, nsPersistentFileDescriptor&);
    		
    	friend NS_COM_OBSOLETE nsOutputStream& operator << (nsOutputStream&, const nsPersistentFileDescriptor&);
    	    
        friend class nsFileSpec;

        void                    GetData(nsAFlatCString& outData) const;
        void                    SetData(const nsAFlatCString& inData);
        void                    SetData(const char* inData, PRInt32 inSize);

    
    
    

    protected:

        nsSimpleCharString      mDescriptorString;

}; 


class NS_COM_OBSOLETE nsDirectoryIterator
















{
	public:
	                            nsDirectoryIterator( const nsFileSpec& parent,
	                            	                 PRBool resoveSymLinks);
	    virtual                 ~nsDirectoryIterator();
	    PRBool                  Exists() const { return mExists; }
	    nsDirectoryIterator&    operator ++(); 
	    nsDirectoryIterator&    operator ++(int) { return ++(*this); } 
	    nsDirectoryIterator&    operator --(); 
	    nsDirectoryIterator&    operator --(int) { return --(*this); } 
	                            operator nsFileSpec&() { return mCurrent; }
	    
	    nsFileSpec&             Spec() { return mCurrent; }

    
    
    

	private:

	    nsFileSpec              mCurrent;
	    PRBool                  mExists;
        PRBool                  mResoveSymLinks;

#if (defined(XP_UNIX) || defined(XP_BEOS) || defined (XP_WIN) || defined(XP_OS2))
	    nsFileSpec		        mStarting;
#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
	    DIR*                    mDir;
#elif defined(XP_WIN) || defined(XP_OS2)
        PRDir*                  mDir; 
#endif
}; 


class NS_COM_OBSOLETE nsNSPRPath




{
public:
    NS_EXPLICIT                  nsNSPRPath(const nsFileSpec& inSpec)
                                     : mFilePath(inSpec), modifiedNSPRPath(nsnull) {}
    NS_EXPLICIT                  nsNSPRPath(const nsFileURL& inURL)
                                     : mFilePath(inURL), modifiedNSPRPath(nsnull) {}
    NS_EXPLICIT                  nsNSPRPath(const nsFilePath& inUnixPath)
                                     : mFilePath(inUnixPath), modifiedNSPRPath(nsnull) {}
    
    virtual                      ~nsNSPRPath();    
 
                                 operator const char*() const;
                                    
                                    
                                    
                                    

    
    
    

private:

    nsFilePath                   mFilePath;
    char*                        modifiedNSPRPath; 
}; 


NS_COM_OBSOLETE nsresult NS_FileSpecToIFile(nsFileSpec* fileSpec, nsILocalFile* *result);

#endif 
