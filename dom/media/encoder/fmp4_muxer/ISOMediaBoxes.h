




#ifndef ISOMediaBoxes_h_
#define ISOMediaBoxes_h_

#include <bitset>
#include "nsString.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "MuxerOperation.h"

#define WRITE_FULLBOX(_compositor, _size)       \
  BoxSizeChecker checker(_compositor, _size);   \
  FullBox::Write();

#define FOURCC(a, b, c, d) ( ((a) << 24) | ((b) << 16) | ((c) << 8) | (d) )

namespace mozilla {




#define Audio_Track 0x01
#define Video_Track 0x02

class AudioTrackMetadata;
class VideoTrackMetadata;
class ISOControl;










class Box : public MuxerOperation {
protected:
  
  uint32_t size;     
  nsCString boxType; 
                     

public:
  
  nsresult Write() override;
  nsresult Find(const nsACString& aType,
                nsTArray<nsRefPtr<MuxerOperation>>& aOperations) override;

  
  
  class BoxSizeChecker {
  public:
    BoxSizeChecker(ISOControl* aControl, uint32_t aSize);
    ~BoxSizeChecker();

    uint32_t ori_size;
    uint32_t box_size;
    ISOControl* mControl;
  };

protected:
  Box() = delete;
  Box(const nsACString& aType, ISOControl* aControl);

  ISOControl* mControl;
  nsRefPtr<AudioTrackMetadata> mAudioMeta;
  nsRefPtr<VideoTrackMetadata> mVideoMeta;
};








class FullBox : public Box {
public:
  
  uint8_t version;       
  std::bitset<24> flags; 

  
  nsresult Write() override;

protected:
  
  FullBox(const nsACString& aType, uint8_t aVersion, uint32_t aFlags,
          ISOControl* aControl);
  FullBox() = delete;
};











class DefaultContainerImpl : public Box {
public:
  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;
  nsresult Find(const nsACString& aType,
                nsTArray<nsRefPtr<MuxerOperation>>& aOperations) override;

protected:
  
  DefaultContainerImpl(const nsACString& aType, ISOControl* aControl);
  DefaultContainerImpl() = delete;

  nsTArray<nsRefPtr<MuxerOperation>> boxes;
};



class FileTypeBox : public Box {
public:
  
  nsCString major_brand; 
  uint32_t minor_version;
  nsTArray<nsCString> compatible_brands;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  FileTypeBox(ISOControl* aControl);
  ~FileTypeBox();
};




class MovieBox : public DefaultContainerImpl {
public:
  MovieBox(ISOControl* aControl);
  ~MovieBox();
};



class MovieHeaderBox : public FullBox {
public:
  
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t timescale;
  uint32_t duration;
  uint32_t rate;
  uint16_t volume;
  uint16_t reserved16;
  uint32_t reserved32[2];
  uint32_t matrix[9];
  uint32_t pre_defined[6];
  uint32_t next_track_ID;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  MovieHeaderBox(ISOControl* aControl);
  ~MovieHeaderBox();
  uint32_t GetTimeScale();
};



class MediaHeaderBox : public FullBox {
public:
  
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t timescale;
  uint32_t duration;
  std::bitset<1> pad;
  std::bitset<5> lang1;
  std::bitset<5> lang2;
  std::bitset<5> lang3;
  uint16_t pre_defined;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  MediaHeaderBox(uint32_t aType, ISOControl* aControl);
  ~MediaHeaderBox();
  uint32_t GetTimeScale();

protected:
  uint32_t mTrackType;
};




class TrackBox : public DefaultContainerImpl {
public:
  TrackBox(uint32_t aTrackType, ISOControl* aControl);
  ~TrackBox();
};



class MediaDataBox : public Box {
public:
  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  uint32_t GetAllSampleSize() { return mAllSampleSize; }
  uint32_t FirstSampleOffsetInMediaDataBox() { return mFirstSampleOffset; }
  MediaDataBox(uint32_t aTrackType, ISOControl* aControl);
  ~MediaDataBox();

protected:
  uint32_t mAllSampleSize;      
  uint32_t mFirstSampleOffset;  
                                
  uint32_t mTrackType;
};


#define flags_data_offset_present                     0x000001
#define flags_first_sample_flags_present              0x000002
#define flags_sample_duration_present                 0x000100
#define flags_sample_size_present                     0x000200
#define flags_sample_flags_present                    0x000400
#define flags_sample_composition_time_offsets_present 0x000800



uint32_t set_sample_flags(bool aSync);



class TrackRunBox : public FullBox {
public:
  
  typedef struct {
    uint32_t sample_duration;
    uint32_t sample_size;
    uint32_t sample_flags;
    uint32_t sample_composition_time_offset;
  } tbl;

  uint32_t sample_count;
  
  uint32_t data_offset; 
  uint32_t first_sample_flags;
  nsAutoArrayPtr<tbl> sample_info_table;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  uint32_t GetAllSampleSize() { return mAllSampleSize; }
  nsresult SetDataOffset(uint32_t aOffset);

  TrackRunBox(uint32_t aType, uint32_t aFlags, ISOControl* aControl);
  ~TrackRunBox();

protected:
  uint32_t fillSampleTable();

  uint32_t mAllSampleSize;
  uint32_t mTrackType;
};


#define base_data_offset_present         0x000001
#define sample_description_index_present 0x000002
#define default_sample_duration_present  0x000008
#define default_sample_size_present      0x000010
#define default_sample_flags_present     0x000020
#define duration_is_empty                0x010000
#define default_base_is_moof             0x020000



class TrackFragmentHeaderBox : public FullBox {
public:
  
  uint32_t track_ID;
  uint64_t base_data_offset;
  uint32_t default_sample_duration;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  nsresult UpdateBaseDataOffset(uint64_t aOffset); 
                                                   

  TrackFragmentHeaderBox(uint32_t aType, uint32_t aFlags, ISOControl* aControl);
  ~TrackFragmentHeaderBox();

protected:
  uint32_t mTrackType;
};




class TrackFragmentBox : public DefaultContainerImpl {
public:
  TrackFragmentBox(uint32_t aType, ISOControl* aControl);
  ~TrackFragmentBox();

protected:
  uint32_t mTrackType;
};



class MovieFragmentHeaderBox : public FullBox {
public:
  
  uint32_t sequence_number;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  MovieFragmentHeaderBox(uint32_t aType, ISOControl* aControl);
  ~MovieFragmentHeaderBox();

protected:
  uint32_t mTrackType;
};




class MovieFragmentBox : public DefaultContainerImpl {
public:
  
  nsresult Generate(uint32_t* aBoxSize) override;

  
  MovieFragmentBox(uint32_t aType, ISOControl* aControl);
  ~MovieFragmentBox();

protected:
  uint32_t mTrackType;
};



class TrackExtendsBox : public FullBox {
public:
  
  uint32_t track_ID;
  uint32_t default_sample_description_index;
  uint32_t default_sample_duration;
  uint32_t default_sample_size;
  uint32_t default_sample_flags;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  TrackExtendsBox(uint32_t aType, ISOControl* aControl);
  ~TrackExtendsBox();

protected:
  uint32_t mTrackType;
};




class MovieExtendsBox : public DefaultContainerImpl {
public:
  MovieExtendsBox(ISOControl* aControl);
  ~MovieExtendsBox();
};



class ChunkOffsetBox : public FullBox {
public:
  
  typedef struct {
    uint32_t chunk_offset;
  } tbl;

  uint32_t entry_count;
  nsAutoArrayPtr<tbl> sample_tbl;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  ChunkOffsetBox(uint32_t aType, ISOControl* aControl);
  ~ChunkOffsetBox();

protected:
  uint32_t mTrackType;
};



class SampleToChunkBox : public FullBox {
public:
  
  typedef struct {
    uint32_t first_chunk;
    uint32_t sample_per_chunk;
    uint32_t sample_description_index;
  } tbl;

  uint32_t entry_count;
  nsAutoArrayPtr<tbl> sample_tbl;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  SampleToChunkBox(uint32_t aType, ISOControl* aControl);
  ~SampleToChunkBox();

protected:
  uint32_t mTrackType;
};



class TimeToSampleBox : public FullBox {
public:
  
  typedef struct {
    uint32_t sample_count;
    uint32_t sample_delta;
  } tbl;

  uint32_t entry_count;
  nsAutoArrayPtr<tbl> sample_tbl;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  TimeToSampleBox(uint32_t aType, ISOControl* aControl);
  ~TimeToSampleBox();

protected:
  uint32_t mTrackType;
};




















class SampleEntryBox : public Box {
public:
  
  uint8_t reserved[6];
  uint16_t data_reference_index;

  
  SampleEntryBox(const nsACString& aFormat, ISOControl* aControl);

  
  nsresult Write() override;

protected:
  SampleEntryBox() = delete;
};



class SampleDescriptionBox : public FullBox {
public:
  
  uint32_t entry_count;
  nsRefPtr<SampleEntryBox> sample_entry_box;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  SampleDescriptionBox(uint32_t aType, ISOControl* aControl);
  ~SampleDescriptionBox();

protected:
  nsresult CreateAudioSampleEntry(nsRefPtr<SampleEntryBox>& aSampleEntry);
  nsresult CreateVideoSampleEntry(nsRefPtr<SampleEntryBox>& aSampleEntry);

  uint32_t mTrackType;
};




class AudioSampleEntry : public SampleEntryBox {
public:
  
  uint16_t sound_version;
  uint8_t reserved2[6];
  uint16_t channels;
  uint16_t sample_size;
  uint16_t compressionId;
  uint16_t packet_size;
  uint32_t timeScale;  

  
  nsresult Write() override;

  ~AudioSampleEntry();

protected:
  AudioSampleEntry(const nsACString& aFormat, ISOControl* aControl);
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

  
  nsresult Write() override;

  
  ~VisualSampleEntry();

protected:
  VisualSampleEntry(const nsACString& aFormat, ISOControl* aControl);
};



class SampleSizeBox : public FullBox {
public:
  
  uint32_t sample_size;
  uint32_t sample_count;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  SampleSizeBox(ISOControl* aControl);
  ~SampleSizeBox();
};









class SampleTableBox : public DefaultContainerImpl {
public:
  SampleTableBox(uint32_t aType, ISOControl* aControl);
  ~SampleTableBox();
};



class DataEntryUrlBox : public FullBox {
public:
  
  
  const static uint16_t flags_media_at_the_same_file = 0x0001;

  nsCString location;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  DataEntryUrlBox();
  DataEntryUrlBox(ISOControl* aControl);
  DataEntryUrlBox(const DataEntryUrlBox& aBox);
  ~DataEntryUrlBox();
};



class DataReferenceBox : public FullBox {
public:
  
  uint32_t entry_count;
  nsTArray<nsAutoPtr<DataEntryUrlBox>> urls;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  DataReferenceBox(ISOControl* aControl);
  ~DataReferenceBox();
};




class DataInformationBox : public DefaultContainerImpl {
public:
  DataInformationBox(ISOControl* aControl);
  ~DataInformationBox();
};



class VideoMediaHeaderBox : public FullBox {
public:
  
  uint16_t graphicsmode;
  uint16_t opcolor[3];

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  VideoMediaHeaderBox(ISOControl* aControl);
  ~VideoMediaHeaderBox();
};



class SoundMediaHeaderBox : public FullBox {
public:
  
  uint16_t balance;
  uint16_t reserved;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  SoundMediaHeaderBox(ISOControl* aControl);
  ~SoundMediaHeaderBox();
};





class MediaInformationBox : public DefaultContainerImpl {
public:
  MediaInformationBox(uint32_t aType, ISOControl* aControl);
  ~MediaInformationBox();

protected:
  uint32_t mTrackType;
};


#define flags_track_enabled    0x000001
#define flags_track_in_movie   0x000002
#define flags_track_in_preview 0x000004



class TrackHeaderBox : public FullBox {
public:
  
  
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t track_ID;
  uint32_t reserved;
  uint32_t duration;

  uint32_t reserved2[2];
  uint16_t layer;
  uint16_t alternate_group;
  uint16_t volume;
  uint16_t reserved3;
  uint32_t matrix[9];
  uint32_t width;
  uint32_t height;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  TrackHeaderBox(uint32_t aType, ISOControl* aControl);
  ~TrackHeaderBox();

protected:
  uint32_t mTrackType;
};



class HandlerBox : public FullBox {
public:
  
  uint32_t pre_defined;
  uint32_t handler_type;
  uint32_t reserved[3];
  nsCString name;

  
  nsresult Generate(uint32_t* aBoxSize) override;
  nsresult Write() override;

  
  HandlerBox(uint32_t aType, ISOControl* aControl);
  ~HandlerBox();

protected:
  uint32_t mTrackType;
};




class MediaBox : public DefaultContainerImpl {
public:
  MediaBox(uint32_t aType, ISOControl* aControl);
  ~MediaBox();

protected:
  uint32_t mTrackType;
};

}
#endif 
