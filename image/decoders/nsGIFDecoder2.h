





#ifndef mozilla_image_decoders_nsGIFDecoder2_h
#define mozilla_image_decoders_nsGIFDecoder2_h

#include "Decoder.h"

#include "GIF2.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace image {
class RasterImage;




class nsGIFDecoder2 : public Decoder
{
public:

  explicit nsGIFDecoder2(RasterImage* aImage);
  ~nsGIFDecoder2();

  virtual void WriteInternal(const char* aBuffer, uint32_t aCount) override;
  virtual void FinishInternal() override;
  virtual Telemetry::ID SpeedHistogram() override;

private:
  
  

  void      BeginGIF();
  void      BeginImageFrame(uint16_t aDepth);
  void      EndImageFrame();
  void      FlushImageData();
  void      FlushImageData(uint32_t fromRow, uint32_t rows);

  nsresult  GifWrite(const uint8_t* buf, uint32_t numbytes);
  uint32_t  OutputRow();
  bool      DoLzw(const uint8_t* q);
  bool      SetHold(const uint8_t* buf, uint32_t count,
                    const uint8_t* buf2 = nullptr, uint32_t count2 = 0);

  inline int ClearCode() const { return 1 << mGIFStruct.datasize; }

  int32_t mCurrentRow;
  int32_t mLastFlushedRow;

  uint32_t mOldColor;        

  
  
  int32_t mCurrentFrameIndex;

  uint8_t mCurrentPass;
  uint8_t mLastFlushedPass;
  uint8_t mColorMask;        
  bool mGIFOpen;
  bool mSawTransparency;

  gif_struct mGIFStruct;
};

} 
} 

#endif 
