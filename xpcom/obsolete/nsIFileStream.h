



































#ifndef nsIFileStream_h___
#define nsIFileStream_h___

#include "xpcomobsolete.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsISeekableStream.h"
#include "prio.h"

class nsFileSpec;


#define NS_IOPENFILE_IID \
{ 0xa6cf90e8, 0x15b3, 0x11d2, \
    {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }


class nsIOpenFile


: public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IOPENFILE_IID)

	NS_IMETHOD                         Open(
                                           const nsFileSpec& inFile,
                                           int nsprMode,
                                           PRIntn accessMode) = 0;
                                           
                                           
                                           
    NS_IMETHOD                         GetIsOpen(PRBool* outOpen) = 0;

}; 

NS_DEFINE_STATIC_IID_ACCESSOR(nsIOpenFile, NS_IOPENFILE_IID)


#define NS_IRANDOMACCESS_IID \
{  0xa6cf90eb, 0x15b3, 0x11d2, \
    {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }


class nsIRandomAccessStore


: public nsISeekableStream
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRANDOMACCESS_IID)


    NS_IMETHOD                         GetAtEOF(PRBool* outAtEOF) = 0;
    NS_IMETHOD                         SetAtEOF(PRBool inAtEOF) = 0;
}; 

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRandomAccessStore, NS_IRANDOMACCESS_IID)

#ifndef NO_XPCOM_FILE_STREAMS   
                                


#define NS_IFILESPECINPUTSTREAM_IID \
{ 0xa6cf90e6, 0x15b3, 0x11d2, \
    {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }
    

class nsIFileSpecInputStream




: public nsIInputStream
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFILESPECINPUTSTREAM_IID)
}; 

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFileSpecInputStream,
                              NS_IFILESPECINPUTSTREAM_IID)


#define NS_IFILESPECOUTPUTSTREAM_IID \
{ 0xa6cf90e7, 0x15b3, 0x11d2, \
    {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }


class nsIFileSpecOutputStream




: public nsIOutputStream
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFILESPECOUTPUTSTREAM_IID)
}; 

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFileSpecOutputStream,
                              NS_IFILESPECOUTPUTSTREAM_IID)

#endif


nsresult NS_NewTypicalInputFileStream(
    nsISupports** aStreamResult,
    const nsFileSpec& inFile
    
    );



nsresult NS_NewTypicalOutputFileStream(
    nsISupports** aStreamResult, 
    const nsFileSpec& inFile
    
    );
    


extern "C" NS_COM_OBSOLETE nsresult NS_NewIOFileStream(
    nsISupports** aStreamResult, 
    const nsFileSpec& inFile,
    PRInt32 nsprMode,
    PRInt32 accessMode);
    
    

#endif
