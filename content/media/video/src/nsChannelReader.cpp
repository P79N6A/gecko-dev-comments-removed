




































#include "nsAString.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "prlog.h"
#include "nsOggDecoder.h"
#include "nsChannelReader.h"
#include "nsIScriptSecurityManager.h"

nsChannelToPipeListener::nsChannelToPipeListener(nsOggDecoder* aDecoder) :
  mDecoder(aDecoder),
  mIntervalStart(0),
  mIntervalEnd(0),
  mTotalBytes(0)
{
}

nsresult nsChannelToPipeListener::Init() 
{
  nsresult rv = NS_NewPipe(getter_AddRefs(mInput), 
                           getter_AddRefs(mOutput),
                           0, 
                           PR_UINT32_MAX);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void nsChannelToPipeListener::Stop()
{
  mDecoder = nsnull;
  mInput = nsnull;
  mOutput = nsnull;
  mDecoder = nsnull;
}

double nsChannelToPipeListener::BytesPerSecond() const
{
  return mTotalBytes / ((PR_IntervalToMilliseconds(mIntervalEnd-mIntervalStart)) / 1000.0);
}

void nsChannelToPipeListener::GetInputStream(nsIInputStream** aStream)
{
  if (aStream) {
    NS_IF_ADDREF(*aStream = mInput);
  }
}

nsresult nsChannelToPipeListener::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  mIntervalStart = PR_IntervalNow();
  mIntervalEnd = mIntervalStart;
  mTotalBytes = 0;
  mDecoder->UpdateBytesDownloaded(mTotalBytes);

  
  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  if (chan) {
    nsCOMPtr<nsIScriptSecurityManager> secMan =
      do_GetService("@mozilla.org/scriptsecuritymanager;1");
    if (secMan) {
      nsresult rv = secMan->GetChannelPrincipal(chan,
                                                getter_AddRefs(mPrincipal));
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }

  return NS_OK;
}

nsresult nsChannelToPipeListener::OnStopRequest(nsIRequest* aRequest, nsISupports* aContext, nsresult aStatus) 
{
  mOutput = nsnull;
  if (mDecoder) {
    mDecoder->ResourceLoaded();
  }
  return NS_OK;
}

nsresult nsChannelToPipeListener::OnDataAvailable(nsIRequest* aRequest, 
                                                nsISupports* aContext, 
                                                nsIInputStream* aStream,
                                                PRUint32 aOffset,
                                                PRUint32 aCount)
{
  if (mOutput) {
    PRUint32 bytes = 0;
  
    do {
      nsresult rv = mOutput->WriteFrom(aStream, aCount, &bytes);
      NS_ENSURE_SUCCESS(rv, rv);
      
      aCount -= bytes;
      mTotalBytes += bytes;
      mDecoder->UpdateBytesDownloaded(mTotalBytes);
    } while (aCount) ;
    
    nsresult rv = mOutput->Flush();
    NS_ENSURE_SUCCESS(rv, rv);
    mIntervalEnd = PR_IntervalNow();
  }
  return NS_OK;
}

nsIPrincipal*
nsChannelToPipeListener::GetCurrentPrincipal()
{
  return mPrincipal;
}

NS_IMPL_ISUPPORTS2(nsChannelToPipeListener, nsIRequestObserver, nsIStreamListener)

PRUint32 nsChannelReader::Available()
{
  PRUint32 available = 0;
  mInput->Available(&available);
  return available;
}

double nsChannelReader::BytesPerSecond() const
{
  return mListener->BytesPerSecond();
}

OggPlayErrorCode nsChannelReader::initialise(int aBlock)
{
  return E_OGGPLAY_OK;
}

OggPlayErrorCode nsChannelReader::destroy()
{
  mChannel->Cancel(NS_BINDING_ABORTED);
  mChannel = nsnull;
  mInput->Close();
  mInput = nsnull;
  mListener->Stop();
  mListener = nsnull;

  return E_OGGPLAY_OK;
}

size_t nsChannelReader::io_read(char* aBuffer, size_t aCount)
{
  PRUint32 bytes = 0;
  nsresult rv = mInput->Read(aBuffer, aCount, &bytes);
  if (!NS_SUCCEEDED(rv)) {
    bytes = 0;
  }

  mCurrentPosition += bytes;
  return bytes == 0 ? static_cast<size_t>(OGGZ_ERR_SYSTEM) : bytes;
}

int nsChannelReader::io_seek(long aOffset, int aWhence)
{
  
  

  return -1;
}

long nsChannelReader::io_tell()
{
  return mCurrentPosition;
}

static OggPlayErrorCode oggplay_channel_reader_initialise(OggPlayReader* aReader, int aBlock) 
{
  nsChannelReader * me = static_cast<nsChannelReader*>(aReader);

  if (me == NULL) {
    return E_OGGPLAY_BAD_READER;
  }
  return me->initialise(aBlock);
}

static OggPlayErrorCode oggplay_channel_reader_destroy(OggPlayReader* aReader) 
{
  nsChannelReader* me = static_cast<nsChannelReader*>(aReader);
  OggPlayErrorCode result = me->destroy();
  delete me;
  return result;
}

static size_t oggplay_channel_reader_io_read(void* aReader, void* aBuffer, size_t aCount) 
{
  nsChannelReader* me = static_cast<nsChannelReader*>(aReader);
  return me->io_read(static_cast<char*>(aBuffer), aCount);
}

static int oggplay_channel_reader_io_seek(void* aReader, long aOffset, int aWhence) 
{
  nsChannelReader* me = static_cast<nsChannelReader*>(aReader);
  return me->io_seek(aOffset, aWhence);
}

static long oggplay_channel_reader_io_tell(void* aReader) 
{
  nsChannelReader* me = static_cast<nsChannelReader*>(aReader);
  return me->io_tell();
}

nsresult nsChannelReader::Init(nsOggDecoder* aDecoder, nsIURI* aURI)
{
  nsresult rv;

  mCurrentPosition = 0;
  mListener = new nsChannelToPipeListener(aDecoder);
  NS_ENSURE_TRUE(mListener, NS_ERROR_OUT_OF_MEMORY);

  rv = mListener->Init();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = NS_NewChannel(getter_AddRefs(mChannel), 
                     aURI, 
                     nsnull,
                     nsnull,
                     nsnull,
                     nsIRequest::LOAD_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mChannel->AsyncOpen(mListener, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mListener->GetInputStream(getter_AddRefs(mInput));

  return NS_OK;
}

nsChannelReader::nsChannelReader() 
{
  OggPlayReader* reader = this;
  reader->initialise = &oggplay_channel_reader_initialise;
  reader->destroy = &oggplay_channel_reader_destroy;
  reader->seek = nsnull;
  reader->io_read = &oggplay_channel_reader_io_read;
  reader->io_seek = &oggplay_channel_reader_io_seek;
  reader->io_tell = &oggplay_channel_reader_io_tell;
}

nsIPrincipal*
nsChannelReader::GetCurrentPrincipal()
{
  if (!mListener)
    return nsnull;
  return mListener->GetCurrentPrincipal();
}
