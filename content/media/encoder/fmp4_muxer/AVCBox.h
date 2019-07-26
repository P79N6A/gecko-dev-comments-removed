




#ifndef AVCBox_h_
#define AVCBox_h_

#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "ISOMediaBoxes.h"

namespace mozilla {

class ISOControl;


#define resolution_72_dpi 0x00480000
#define video_depth 0x0018



class AVCConfigurationBox : public Box {
public:
  

  
  
  
  
  nsTArray<uint8_t> avcConfig;

  
  nsresult Generate(uint32_t* aBoxSize) MOZ_OVERRIDE;
  nsresult Write() MOZ_OVERRIDE;

  
  AVCConfigurationBox(ISOControl* aControl);
  ~AVCConfigurationBox();
};



class VisualSampleEntry : public SampleEntryBox {
public:
  
  uint8_t reserved[16];
  uint16_t width;
  uint16_t height;

  uint32_t horizresolution; 
  uint32_t vertresolution;  
  uint32_t reserved2;
  uint16_t frame_count;     

  uint8_t compressorName[32];
  uint16_t depth;       
  uint16_t pre_defined; 

  nsRefPtr<AVCConfigurationBox> avcConfigBox;

  
  nsresult Generate(uint32_t* aBoxSize) MOZ_OVERRIDE;
  nsresult Write() MOZ_OVERRIDE;

  
  VisualSampleEntry(ISOControl* aControl);
  ~VisualSampleEntry();
};

}

#endif 
