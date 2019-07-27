




#ifndef MP4ESDS_h_
#define MP4ESDS_h_

#include "nsTArray.h"
#include "MuxerOperation.h"

namespace mozilla {

class ISOControl;




#define ESDescrTag        0x03






class ES_Descriptor : public MuxerOperation {
public:
  
  uint8_t tag;      
  uint8_t length;
  uint16_t ES_ID;
  std::bitset<1> streamDependenceFlag;
  std::bitset<1> URL_Flag;
  std::bitset<1> reserved;
  std::bitset<5> streamPriority;

  nsTArray<uint8_t> DecodeSpecificInfo;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;
  nsresult Find(const nsACString& aType,
                nsTArray<nsRefPtr<MuxerOperation>>& aOperations) override;

  
  ES_Descriptor(ISOControl* aControl);
  ~ES_Descriptor();

protected:
  ISOControl* mControl;
};



class ESDBox : public FullBox {
public:
  
  nsRefPtr<ES_Descriptor> es_descriptor;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  ESDBox(ISOControl* aControl);
  ~ESDBox();
};



class MP4AudioSampleEntry : public AudioSampleEntry {
public:
  
  nsRefPtr<ESDBox> es;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  MP4AudioSampleEntry(ISOControl* aControl);
  ~MP4AudioSampleEntry();
};

}

#endif 
