




#ifndef AMRBOX_h_
#define AMRBOX_h_

#include "nsTArray.h"
#include "MuxerOperation.h"

namespace mozilla {

class ISOControl;



class AMRSpecificBox : public Box {
public:
  
  nsTArray<uint8_t> amrDecSpecInfo;

  
  nsresult Generate(uint32_t* aBoxSize) MOZ_OVERRIDE;
  nsresult Write() MOZ_OVERRIDE;

  
  AMRSpecificBox(ISOControl* aControl);
  ~AMRSpecificBox();
};



class AMRSampleEntry : public AudioSampleEntry {
public:
  
  nsRefPtr<AMRSpecificBox> amr_special_box;

  
  nsresult Generate(uint32_t* aBoxSize) MOZ_OVERRIDE;
  nsresult Write() MOZ_OVERRIDE;

  
  AMRSampleEntry(ISOControl* aControl);
  ~AMRSampleEntry();
};

}

#endif 
