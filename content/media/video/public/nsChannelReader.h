



































#if !defined(nsChannelReader_h_)
#define nsChannelReader_h_

#include "nsAutoPtr.h"
#include "nsMediaStream.h"
#include "nsMediaDecoder.h"
#include "nsIPrincipal.h"

#include "oggplay/oggplay.h"

class nsIChannel;
class nsIStreamListener;

class nsChannelReader : public OggPlayReader
{
public:
  nsChannelReader();
  ~nsChannelReader();

  






  nsresult Init(nsMediaDecoder* aDecoder, nsIURI* aURI, nsIChannel* aChannel,
                nsIStreamListener** aStreamListener);

  
  
  void Cancel();

  
  
  PRUint32 Available();

  
  
  float DownloadRate();

  
  
  float PlaybackRate();

  nsIPrincipal* GetCurrentPrincipal();
  
  
  OggPlayErrorCode initialise(int aBlock);
  OggPlayErrorCode destroy();
  size_t io_read(char* aBuffer, size_t aCount);
  int io_seek(long aOffset, int aWhence);
  long io_tell();  
  
public:
  nsMediaStream mStream;
  unsigned long mCurrentPosition;
};

#endif
