































































































#ifndef _FILESTREAM_H_
#define _FILESTREAM_H_

#include "xpcomobsolete.h"
#include "nsStringFwd.h"

#include "prio.h"

#include "nsCOMPtr.h"
#include "nsIFileStream.h"


class nsFileSpec;
class nsIInputStream;
class nsIOutputStream;
class nsIFileSpec;




#if !defined(NS_USING_NAMESPACE) && (defined(__MWERKS__) || defined(_MSC_VER))
#define NS_USING_NAMESPACE
#endif

#ifdef NS_USING_NAMESPACE

#define NS_NAMESPACE_PROTOTYPE
#define NS_NAMESPACE namespace
#define NS_NAMESPACE_END
	
#else

#define NS_NAMESPACE_PROTOTYPE static
#define NS_NAMESPACE struct
#define NS_NAMESPACE_END ;

#endif 

#ifndef __KCC













#define NS_USE_PR_STDIO
#endif

#ifdef NS_USE_PR_STDIO
class istream;
class ostream;
#define CONSOLE_IN 0
#define CONSOLE_OUT 0
#else
#include <iostream>
using std::istream;
using std::ostream;
#define CONSOLE_IN &std::cin
#define CONSOLE_OUT &std::cout
#endif




class NS_COM_OBSOLETE nsInputStream





{
public:
                                      nsInputStream(nsIInputStream* inStream)
                                      :   mInputStream(do_QueryInterface(inStream))
                                      ,   mEOF(PR_FALSE)
                                      {}
    virtual                           ~nsInputStream();
 
    nsCOMPtr<nsIInputStream>          GetIStream() const
                                      {
                                          return mInputStream;
                                      }
    PRBool                            eof() const { return get_at_eof(); }
    char                              get();
    nsresult                          close()
                                      {
    					NS_ASSERTION(mInputStream, "mInputStream is null!");
					if (mInputStream) {
						return mInputStream->Close();                        
					}
                    return NS_OK;
                                      }
    PRInt32                           read(void* s, PRInt32 n);

    
    
    nsInputStream&                    operator >> (char& ch);

    
    nsInputStream&                    operator >> (nsInputStream& (*pf)(nsInputStream&))
                                      {
                                           return pf(*this);
                                      }

protected:

   
   
   virtual void                       set_at_eof(PRBool atEnd)
                                      {
                                         mEOF = atEnd;
                                      }
   virtual PRBool                     get_at_eof() const
                                      {
                                          return mEOF;
                                      }
private:

    nsInputStream&                    operator >> (char* buf); 

    
                                      nsInputStream(const nsInputStream& rhs);
    nsInputStream&                    operator=(const nsInputStream& rhs);


protected:
    nsCOMPtr<nsIInputStream>          mInputStream;
    PRBool                            mEOF;
}; 

typedef nsInputStream nsBasicInStream; 


class NS_COM_OBSOLETE nsOutputStream





{
public:
                                      nsOutputStream() {}
                                      nsOutputStream(nsIOutputStream* inStream)
                                      :   mOutputStream(do_QueryInterface(inStream))
                                          {}

    virtual                          ~nsOutputStream();

    nsCOMPtr<nsIOutputStream>         GetIStream() const
                                      {
                                          return mOutputStream;
                                      }
    nsresult                          close()
                                      {
                                          if (mOutputStream)
                                            return mOutputStream->Close();
                                          return NS_OK;
                                      }
    void                              put(char c);
    PRInt32                           write(const void* s, PRInt32 n);
    virtual nsresult                  flush();
    nsresult                          lastWriteStatus();

    
    
    nsOutputStream&                   operator << (const char* buf);
    nsOutputStream&                   operator << (char ch);
    nsOutputStream&                   operator << (short val);
    nsOutputStream&                   operator << (unsigned short val);
    nsOutputStream&                   operator << (long val);
    nsOutputStream&                   operator << (unsigned long val);
    nsOutputStream&                   operator << (int val);
    nsOutputStream&                   operator << (unsigned int val);

    
    nsOutputStream&                   operator << (nsOutputStream& (*pf)(nsOutputStream&))
                                      {
                                           return pf(*this);
                                      }

private:

    
                                      nsOutputStream(const nsOutputStream& rhs);
    nsOutputStream&                   operator=(const nsOutputStream& rhs);

    nsresult                          mWriteStatus;


protected:
    nsCOMPtr<nsIOutputStream>         mOutputStream;
}; 

typedef nsOutputStream nsBasicOutStream; 


class NS_COM_OBSOLETE nsErrorProne


{
public:
                                      nsErrorProne() 
                                      :   mResult(NS_OK)
                                      {
                                      }
    PRBool                            failed() const
                                      {
                                          return NS_FAILED(mResult);
                                      }
    nsresult                          error() const
                                      {
                                          return mResult;
                                      }


protected:
    nsresult                          mResult;
}; 


class NS_COM_OBSOLETE nsFileClient




:    public virtual nsErrorProne
{
public:
                                      nsFileClient(const nsCOMPtr<nsIOpenFile>& inFile)
                                      :   mFile(do_QueryInterface(inFile))
                                      {
                                      }
    virtual                           ~nsFileClient() {}

    void                              open(
                                          const nsFileSpec& inFile,
                                          int nsprMode,
                                          PRIntn accessMode)
                                      {
                                          if (mFile)
                                              mResult = mFile->Open(inFile, nsprMode, accessMode);
                                      }
    PRBool                            is_open() const
                                      {
                                          PRBool result = PR_FALSE;
                                          if (mFile)
                                              mFile->GetIsOpen(&result);
                                          return result;
                                      }
    PRBool                            is_file() const
                                      {
                                          return mFile ? PR_TRUE : PR_FALSE;
                                      }

protected:

                                      nsFileClient() 
                                      {
                                      }

protected:
    nsCOMPtr<nsIOpenFile>                 mFile;
}; 


class NS_COM_OBSOLETE nsRandomAccessStoreClient




:    public virtual nsErrorProne
{
public:
                                      nsRandomAccessStoreClient() 
                                      {
                                      }
                                      nsRandomAccessStoreClient(const nsCOMPtr<nsIRandomAccessStore>& inStore)
                                      :   mStore(do_QueryInterface(inStore))
                                      {
                                      }
    virtual                           ~nsRandomAccessStoreClient() {}
    
    void                              seek(PRInt64 offset)
                                      {
                                          seek(PR_SEEK_SET, offset);
                                      }

    void                              seek(PRSeekWhence whence, PRInt64 offset)
                                      {
                                          set_at_eof(PR_FALSE);
                                          if (mStore)
                                              mResult = mStore->Seek(whence, offset);
                                      }
    PRInt64                           tell()
                                      {
                                          PRInt64 result;
                                          LL_I2L(result, -1);
                                          if (mStore)
                                              mResult = mStore->Tell(&result);
                                          return result;
                                      }

protected:

   virtual PRBool                     get_at_eof() const
                                      {
                                          PRBool result = PR_TRUE;
                                          if (mStore)
                                              mStore->GetAtEOF(&result);
                                          return result;
                                      }

   virtual void                       set_at_eof(PRBool atEnd)
                                      {
                                          if (mStore)
                                              mStore->SetAtEOF(atEnd);
                                      }

private:

    
                                      nsRandomAccessStoreClient(const nsRandomAccessStoreClient& rhs);
    nsRandomAccessStoreClient&        operator=(const nsRandomAccessStoreClient& rhs);


protected:
    nsCOMPtr<nsIRandomAccessStore>    mStore;
}; 


class NS_COM_OBSOLETE nsRandomAccessInputStream


:	public nsRandomAccessStoreClient
,	public nsInputStream
{
public:
                                      nsRandomAccessInputStream(nsIInputStream* inStream)
                                      :   nsRandomAccessStoreClient(do_QueryInterface(inStream))
                                      ,   nsInputStream(inStream)
                                      {
                                      }
    PRBool                            readline(char* s,  PRInt32 n);
                                          
                                          
                                          
                                          

    
    nsInputStream&                    operator >> (char& ch)
                                         { return nsInputStream::operator >>(ch); }
    nsInputStream&                    operator >> (nsInputStream& (*pf)(nsInputStream&))
                                         { return nsInputStream::operator >>(pf); }

protected:
                                      nsRandomAccessInputStream()
                                      :  nsInputStream(nsnull)
                                      {
                                      }

   virtual PRBool                     get_at_eof() const
                                      {
                                          return nsRandomAccessStoreClient::get_at_eof();
                                      }

   virtual void                       set_at_eof(PRBool atEnd)
                                      {
                                          nsRandomAccessStoreClient::set_at_eof(atEnd);
                                      }

private:

    
                                      nsRandomAccessInputStream(const nsRandomAccessInputStream& rhs);
    nsRandomAccessInputStream&        operator=(const nsRandomAccessInputStream& rhs);

}; 


class NS_COM_OBSOLETE nsInputFileStream


:	public nsRandomAccessInputStream
,   public nsFileClient
{
public:
	enum  { kDefaultMode = PR_RDONLY };
                                      nsInputFileStream(nsIInputStream* inStream)
                                      :   nsRandomAccessInputStream(inStream)
                                      ,   nsFileClient(do_QueryInterface(inStream))
                                      ,   mFileInputStream(do_QueryInterface(inStream))
                                      {
                                      }
                                      nsInputFileStream(
                                          const nsFileSpec& inFile,
                                          int nsprMode = kDefaultMode,
                                          PRIntn accessMode = 00666);
                                      nsInputFileStream(nsIFileSpec* inFile);
    virtual                           ~nsInputFileStream();

    void                              Open(
                                          const nsFileSpec& inFile,
                                          int nsprMode = kDefaultMode,
                                          PRIntn accessMode = 00666)
                                      {
                                          if (mFile)
                                              mFile->Open(inFile, nsprMode, accessMode);
                                      }

    
    nsInputStream&                    operator >> (char& ch)
                                         { return nsInputStream::operator >>(ch); }
    nsInputStream&                    operator >> (nsInputStream& (*pf)(nsInputStream&))
                                         { return nsInputStream::operator >>(pf); }

protected:
    void                              AssignFrom(nsISupports* stream);

private:

    
                                      nsInputFileStream(const nsInputFileStream& rhs);
    nsInputFileStream&                operator=(const nsInputFileStream& rhs);


protected:
    nsCOMPtr<nsIFileSpecInputStream>      mFileInputStream;
}; 


class NS_COM_OBSOLETE nsRandomAccessOutputStream


:	public nsRandomAccessStoreClient
,	public nsOutputStream
{
public:
                                      nsRandomAccessOutputStream(nsIOutputStream* inStream)
                                      :   nsRandomAccessStoreClient(do_QueryInterface(inStream))
                                      ,   nsOutputStream(inStream)
                                      {
                                      }

    
    nsOutputStream&                   operator << (const char* buf)
                                        { return nsOutputStream::operator << (buf); }
    nsOutputStream&                   operator << (char ch)
                                        { return nsOutputStream::operator << (ch); }
    nsOutputStream&                   operator << (short val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (unsigned short val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (long val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (unsigned long val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (int val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (unsigned int val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (nsOutputStream& (*pf)(nsOutputStream&))
                                        { return nsOutputStream::operator << (pf); }

protected:
                                      nsRandomAccessOutputStream()
                                      :  nsOutputStream(nsnull)
                                      {
                                      }

private:

    
                                      nsRandomAccessOutputStream(const nsRandomAccessOutputStream& rhs);
    nsRandomAccessOutputStream&       operator=(const nsRandomAccessOutputStream& rhs);

}; 


class NS_COM_OBSOLETE nsOutputFileStream


:	public nsRandomAccessOutputStream
,	public nsFileClient
{
public:
	enum  { kDefaultMode = (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE) };

                                      nsOutputFileStream() {}
                                      nsOutputFileStream(nsIOutputStream* inStream)
                                      {
                                          AssignFrom(inStream);
                                      }
                                      nsOutputFileStream(
                                           const nsFileSpec& inFile,
                                           int nsprMode = kDefaultMode,
                                           PRIntn accessMode = 00666) 
                                      {
                                          nsISupports* stream;
                                          if (NS_FAILED(NS_NewIOFileStream(
                                              &stream,
                                              inFile, nsprMode, accessMode)))
                                              return;
                                          AssignFrom(stream);
                                          NS_RELEASE(stream);
                                      }
                                      nsOutputFileStream(nsIFileSpec* inFile);
    virtual                           ~nsOutputFileStream();
 
    virtual nsresult                  flush();
    virtual void					  abort();

    
    nsOutputStream&                   operator << (const char* buf)
                                        { return nsOutputStream::operator << (buf); }
    nsOutputStream&                   operator << (char ch)
                                        { return nsOutputStream::operator << (ch); }
    nsOutputStream&                   operator << (short val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (unsigned short val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (long val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (unsigned long val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (int val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (unsigned int val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (nsOutputStream& (*pf)(nsOutputStream&))
                                        { return nsOutputStream::operator << (pf); }

protected:
    void                              AssignFrom(nsISupports* stream);

private:
    
    
                                      nsOutputFileStream(const nsOutputFileStream& rhs);
    nsOutputFileStream&               operator=(const nsOutputFileStream& rhs);


protected:
    nsCOMPtr<nsIFileSpecOutputStream>     mFileOutputStream;
}; 



class nsIOFileStream


:	public nsInputFileStream
,	public nsOutputStream
{
public:
	enum  { kDefaultMode = (PR_RDWR | PR_CREATE_FILE) };

                                      nsIOFileStream(
                                          nsIInputStream* inInputStream
                                      ,   nsIOutputStream* inOutputStream)
                                      :   nsInputFileStream(inInputStream)
                                      ,   nsOutputStream(inOutputStream)
                                      ,   mFileOutputStream(do_QueryInterface(inOutputStream))
                                      {
                                      }
                                      nsIOFileStream(
                                           const nsFileSpec& inFile,
                                           int nsprMode = kDefaultMode,
                                           PRIntn accessMode = 00666) 
                                      :  nsInputFileStream((nsIInputStream*)nsnull)
                                      ,  nsOutputStream(nsnull)
                                      {
                                          nsISupports* stream;
                                          if (NS_FAILED(NS_NewIOFileStream(
                                              &stream,
                                              inFile, nsprMode, accessMode)))
                                              return;
                                          mFile = do_QueryInterface(stream);
                                          mStore = do_QueryInterface(stream);
                                          mInputStream = do_QueryInterface(stream);
                                          mOutputStream = do_QueryInterface(stream);
                                          mFileInputStream = do_QueryInterface(stream);
                                          mFileOutputStream = do_QueryInterface(stream);
                                          NS_RELEASE(stream);
                                      }
 
    virtual nsresult                  close()
                                      {
                                          
                                          
                                          return nsInputFileStream::close();
                                      }

     
    nsOutputStream&                   operator << (const char* buf)
                                        { return nsOutputStream::operator << (buf); }
    nsOutputStream&                   operator << (char ch)
                                        { return nsOutputStream::operator << (ch); }
    nsOutputStream&                   operator << (short val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (unsigned short val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (long val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (unsigned long val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (int val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (unsigned int val)
                                        { return nsOutputStream::operator << (val); }
    nsOutputStream&                   operator << (nsOutputStream& (*pf)(nsOutputStream&))
                                        { return nsOutputStream::operator << (pf); }

    
    nsInputStream&                    operator >> (char& ch)
                                         { return nsInputStream::operator >>(ch); }
    nsInputStream&                    operator >> (nsInputStream& (*pf)(nsInputStream&))
                                         { return nsInputStream::operator >>(pf); }

	virtual nsresult flush() {if (mFileOutputStream) mFileOutputStream->Flush(); return error(); }


private:

    
                                      nsIOFileStream(const nsIOFileStream& rhs);
    nsIOFileStream&                   operator=(const nsIOFileStream& rhs);

    
protected:
    nsCOMPtr<nsIFileSpecOutputStream>     mFileOutputStream;
}; 
 




NS_COM_OBSOLETE nsOutputStream&     nsEndl(nsOutputStream& os); 


#endif 
