




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



class AVCSampleEntry : public VisualSampleEntry {
public:
  
  nsRefPtr<AVCConfigurationBox> avcConfigBox;

  
  nsresult Generate(uint32_t* aBoxSize) MOZ_OVERRIDE;
  nsresult Write() MOZ_OVERRIDE;

  
  AVCSampleEntry(ISOControl* aControl);
  ~AVCSampleEntry();
};

}

#endif 
