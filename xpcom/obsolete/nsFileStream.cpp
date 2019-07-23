



































 




#include "nsFileStream.h"
#include "nsFileSpec.h"
#include "nsIFileSpec.h"
#include "nsIStringStream.h"
#include "nsInt64.h"
#include <string.h>
#include <stdio.h>







nsInputStream::~nsInputStream()

{
}


char nsInputStream::get()

{
	char c;
    if (read(&c, sizeof(c)) == sizeof(c))
        return c;
    return 0;
}


PRInt32 nsInputStream::read(void* s, PRInt32 n)

{
  if (!mInputStream)
      return 0;
  PRInt32 result = 0;
  PRInt32 status = mInputStream->Read((char*)s, n, (PRUint32*)&result);
  if (result == 0)
      set_at_eof(PR_TRUE);
  if (status < 0) 
      return (status); 
  return result;
} 


static void TidyEndOfLine(char*& cp)




{
    char ch = *cp;
    *cp++ = '\0'; 
    if ((ch == '\n' && *cp == '\r') || (ch == '\r' && *cp == '\n'))
        cp++; 
}


nsInputStream& nsInputStream::operator >> (char& c)

{
	c = get();
	return *this;
}






nsOutputStream::~nsOutputStream()

{
}


void nsOutputStream::put(char c)

{
    write(&c, sizeof(c));
}


PRInt32 nsOutputStream::write(const void* s, PRInt32 n)

{
  if (!mOutputStream)
      return 0;
  PRInt32 result = 0;
  mWriteStatus = mOutputStream->Write((char*)s, n, (PRUint32*)&result);
  return result;
} 


nsresult nsOutputStream::flush()

{
  return NS_OK;
}


nsresult nsOutputStream::lastWriteStatus()

{
  return mWriteStatus;
}


nsOutputStream& nsOutputStream::operator << (char c)

{
	put(c);
	return *this;
}


nsOutputStream& nsOutputStream::operator << (const char* s)

{
	if (s)
		write(s, strlen(s));
	return *this;
}


nsOutputStream& nsOutputStream::operator << (short val)

{
	char buf[30];
	sprintf(buf, "%hd", val);
	return (*this << buf);
}


nsOutputStream& nsOutputStream::operator << (unsigned short val)

{
	char buf[30];
	sprintf(buf, "%hu", val);
	return (*this << buf);
}


nsOutputStream& nsOutputStream::operator << (long val)

{
	char buf[30];
	sprintf(buf, "%ld", val);
	return (*this << buf);
}


nsOutputStream& nsOutputStream::operator << (unsigned long val)

{
	char buf[30];
	sprintf(buf, "%lu", val);
	return (*this << buf);
}


nsOutputStream& nsOutputStream::operator << (int val)

{
	char buf[30];
	sprintf(buf, "%d", val);
	return (*this << buf);
}


nsOutputStream& nsOutputStream::operator << (unsigned int val)

{
	char buf[30];
	sprintf(buf, "%u", val);
	return (*this << buf);
}






PRBool nsRandomAccessInputStream::readline(char* s, PRInt32 n)


{
    PRBool bufferLargeEnough = PR_TRUE; 
    if (!s || !n)
        return PR_TRUE;

    nsInt64 position = tell();
    const nsInt64 zero(0);
    if (position < zero)
        return PR_FALSE;
    PRInt32 bytesRead = read(s, n - 1);
    if (failed())
        return PR_FALSE;
    s[bytesRead] = '\0'; 
    char* tp = strpbrk(s, "\n\r");
    if (tp)
    {
        TidyEndOfLine(tp);
        bytesRead = (tp - s);
    }
    else if (!eof() && n-1 == bytesRead)
        bufferLargeEnough = PR_FALSE;
    position += bytesRead;
    seek(position);
    return bufferLargeEnough;
} 






nsInputFileStream::nsInputFileStream(
	const nsFileSpec& inFile,
	int nsprMode,
	PRIntn accessMode)

{
	nsISupports* stream;
	if (NS_FAILED(NS_NewIOFileStream(&stream, inFile, nsprMode, accessMode)))
        return;
	AssignFrom(stream);
	NS_RELEASE(stream);
} 


nsInputFileStream::nsInputFileStream(nsIFileSpec* inSpec)

{
	nsIInputStream* stream;
	if (NS_FAILED(inSpec->GetInputStream(&stream)))
        return;
	AssignFrom(stream);
	NS_RELEASE(stream);
} 


nsInputFileStream::~nsInputFileStream()

{


}


void nsInputFileStream::AssignFrom(nsISupports* stream)

{
	mFile = do_QueryInterface(stream);
	mInputStream = do_QueryInterface(stream);
	mStore = do_QueryInterface(stream);
	mFileInputStream = do_QueryInterface(stream);
}






nsOutputFileStream::nsOutputFileStream(nsIFileSpec* inSpec)

{
	if (!inSpec)
		return;
	nsIOutputStream* stream;
	if (NS_FAILED(inSpec->GetOutputStream(&stream)))
	  return;
	AssignFrom(stream);
	NS_RELEASE(stream);
}


nsOutputFileStream::~nsOutputFileStream()

{


}

void nsOutputFileStream::AssignFrom(nsISupports* stream)

{
    mFile = do_QueryInterface(stream);
    mOutputStream = do_QueryInterface(stream);
    mStore = do_QueryInterface(stream);
    mFileOutputStream = do_QueryInterface(stream);
}


nsresult nsOutputFileStream::flush()

{
	if (mFileOutputStream)
		mFileOutputStream->Flush();
    return error();
}


void nsOutputFileStream::abort()

{
	mResult = NS_FILE_FAILURE;
	close();
}






nsOutputStream& nsEndl(nsOutputStream& os)

{
#if defined(XP_WIN) || defined(XP_OS2)
    os.write("\r\n", 2);
#else
    os.put('\n');
#endif
    
    return os;
} 
