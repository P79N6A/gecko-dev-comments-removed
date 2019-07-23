



































#if !defined(nsChannelReader_h_)
#define nsChannelReader_h_

#include "nsAutoPtr.h"
#include "nsMediaStream.h"

#include "oggplay/oggplay.h"

class nsIURI;
class nsIChannel;
class nsIStreamListener;
class nsMediaDecoder;

class nsChannelReader : public OggPlayReader
{
public:
  nsChannelReader();
  ~nsChannelReader();

  



  void Init(nsMediaStream* aStream);

  nsMediaStream* Stream() { return mStream; }

  
  
  
  void SetLastFrameTime(PRInt64 aTime);

  nsIPrincipal* GetCurrentPrincipal();
  
  
  OggPlayErrorCode initialise(int aBlock);
  OggPlayErrorCode destroy();
  size_t io_read(char* aBuffer, size_t aCount);
  int io_seek(long aOffset, int aWhence);
  long io_tell();
  ogg_int64_t duration();
  
public:
  nsAutoPtr<nsMediaStream> mStream;

  
  
  PRInt64 mLastFrameTime;
};

#endif
