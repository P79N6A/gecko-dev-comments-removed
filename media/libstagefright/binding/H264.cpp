



#include "mozilla/ArrayUtils.h"
#include "mozilla/PodOperations.h"
#include "mp4_demuxer/AnnexB.h"
#include "mp4_demuxer/ByteReader.h"
#include "mp4_demuxer/ByteWriter.h"
#include "mp4_demuxer/H264.h"
#include <media/stagefright/foundation/ABitReader.h>

using namespace mozilla;

namespace mp4_demuxer
{

class BitReader
{
public:
  explicit BitReader(const ByteBuffer& aBuffer)
  : mBitReader(aBuffer.Elements(), aBuffer.Length())
  {
  }

  uint32_t ReadBits(size_t aNum)
  {
    MOZ_ASSERT(mBitReader.numBitsLeft());
    MOZ_ASSERT(aNum <= 32);
    if (mBitReader.numBitsLeft() < aNum) {
      return 0;
    }
    return mBitReader.getBits(aNum);
  }

  uint32_t ReadBit()
  {
    return ReadBits(1);
  }

  
  uint32_t ReadUE()
  {
    uint32_t i = 0;

    while (ReadBit() == 0 && i < 32) {
      i++;
    }
    if (i == 32) {
      MOZ_ASSERT(false);
      return 0;
    }
    uint32_t r = ReadBits(i);
    r += (1 << i) - 1;
    return r;
  }

  
  int32_t ReadSE()
  {
    int32_t r = ReadUE();
    if (r & 1) {
      return (r+1) / 2;
    } else {
      return -r / 2;
    }
  }

private:
  stagefright::ABitReader mBitReader;
};

SPSData::SPSData()
{
  PodZero(this);
  
  video_format = 5;
  colour_primaries = 2;
  transfer_characteristics = 2;
  sample_ratio = 1.0;
}

 already_AddRefed<ByteBuffer>
H264::DecodeNALUnit(const ByteBuffer* aNAL)
{
  MOZ_ASSERT(aNAL);

  if (aNAL->Length() < 4) {
    return nullptr;
  }

  nsRefPtr<ByteBuffer> rbsp = new ByteBuffer;
  ByteReader reader(*aNAL);
  uint8_t nal_unit_type = reader.ReadU8() & 0x1f;
  uint32_t nalUnitHeaderBytes = 1;
  if (nal_unit_type == 14 || nal_unit_type == 20 || nal_unit_type == 21) {
    bool svc_extension_flag = false;
    bool avc_3d_extension_flag = false;
    if (nal_unit_type != 21) {
      svc_extension_flag = reader.PeekU8() & 0x80;
    } else {
      avc_3d_extension_flag = reader.PeekU8() & 0x80;
    }
    if (svc_extension_flag) {
      nalUnitHeaderBytes += 3;
    } else if (avc_3d_extension_flag) {
      nalUnitHeaderBytes += 2;
    } else {
      nalUnitHeaderBytes += 3;
    }
  }
  if (!reader.Read(nalUnitHeaderBytes - 1)) {
    return nullptr;
  }
  uint32_t lastbytes = 0xffff;
  while (reader.Remaining()) {
    uint8_t byte = reader.ReadU8();
    if ((lastbytes & 0xffff) == 0 && byte == 0x03) {
      
      lastbytes = 0xffff;
    } else {
      rbsp->AppendElement(byte);
    }
    lastbytes = (lastbytes << 8) | byte;
  }
  return rbsp.forget();
}

static int32_t
ConditionDimension(float aValue)
{
  
  if (aValue > 1.0 && aValue <= INT32_MAX)
    return int32_t(aValue);
  return 0;
}

 bool
H264::DecodeSPS(const ByteBuffer* aSPS, SPSData& aDest)
{
  MOZ_ASSERT(aSPS);
  BitReader br(*aSPS);

  int32_t lastScale;
  int32_t nextScale;
  int32_t deltaScale;

  aDest.profile_idc = br.ReadBits(8);
  aDest.constraint_set0_flag = br.ReadBit();
  aDest.constraint_set1_flag = br.ReadBit();
  aDest.constraint_set2_flag = br.ReadBit();
  aDest.constraint_set3_flag = br.ReadBit();
  aDest.constraint_set4_flag = br.ReadBit();
  aDest.constraint_set5_flag = br.ReadBit();
  br.ReadBits(2); 
  aDest.level_idc = br.ReadBits(8);
  aDest.seq_parameter_set_id = br.ReadUE();
  if (aDest.profile_idc == 100 || aDest.profile_idc == 110 ||
      aDest.profile_idc == 122 || aDest.profile_idc == 244 ||
      aDest.profile_idc == 44 || aDest.profile_idc == 83 ||
     aDest.profile_idc == 86 || aDest.profile_idc == 118 ||
      aDest.profile_idc == 128 || aDest.profile_idc == 138 ||
      aDest.profile_idc == 139 || aDest.profile_idc == 134) {
    if ((aDest.chroma_format_idc = br.ReadUE()) == 3) {
      aDest.separate_colour_plane_flag = br.ReadBit();
    }
    br.ReadUE();  
    br.ReadUE();  
    br.ReadBit(); 
    if (br.ReadBit()) { 
      for (int idx = 0; idx < ((aDest.chroma_format_idc != 3) ? 8 : 12); ++idx) {
        if (br.ReadBit()) { 
          lastScale = nextScale = 8;
          int sl_n = (idx < 6) ? 16 : 64;
          for (int sl_i = 0; sl_i < sl_n; sl_i++) {
            if (nextScale) {
              deltaScale = br.ReadSE();
              nextScale = (lastScale + deltaScale + 256) % 256;
            }
            lastScale = (nextScale == 0) ? lastScale : nextScale;
          }
        }
      }
    }
  }
  aDest.log2_max_frame_num = br.ReadUE() + 4;
  aDest.pic_order_cnt_type = br.ReadUE();
  if (aDest.pic_order_cnt_type == 0) {
    aDest.log2_max_pic_order_cnt_lsb = br.ReadUE() + 4;
  } else if (aDest.pic_order_cnt_type == 1) {
    aDest.delta_pic_order_always_zero_flag = br.ReadBit();
    aDest.offset_for_non_ref_pic = br.ReadSE();
    aDest.offset_for_top_to_bottom_field = br.ReadSE();
    uint32_t num_ref_frames_in_pic_order_cnt_cycle = br.ReadUE();
    for (uint32_t i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
      br.ReadSE(); 
    }
  }
  aDest.max_num_ref_frames = br.ReadUE();
  aDest.gaps_in_frame_num_allowed_flag = br.ReadBit();
  aDest.pic_width_in_mbs = br.ReadUE() + 1;
  aDest.pic_height_in_map_units = br.ReadUE() + 1;
  aDest.frame_mbs_only_flag = br.ReadBit();
  if (!aDest.frame_mbs_only_flag) {
    aDest.pic_height_in_map_units *= 2;
    aDest.mb_adaptive_frame_field_flag = br.ReadBit();
  }
  br.ReadBit(); 
  aDest.frame_cropping_flag = br.ReadBit();
  if (aDest.frame_cropping_flag) {
    aDest.frame_crop_left_offset = br.ReadUE();
    aDest.frame_crop_right_offset = br.ReadUE();
    aDest.frame_crop_top_offset = br.ReadUE();
    aDest.frame_crop_bottom_offset = br.ReadUE();
  }

  aDest.sample_ratio = 1.0f;
  aDest.vui_parameters_present_flag = br.ReadBit();
  if (aDest.vui_parameters_present_flag) {
    vui_parameters(br, aDest);
  }

  

  

  uint8_t ChromaArrayType =
    aDest.separate_colour_plane_flag ? 0 : aDest.chroma_format_idc;
  
  uint32_t CropUnitX = 1;
  uint32_t SubWidthC = aDest.chroma_format_idc == 3 ? 1 : 2;
  if (ChromaArrayType != 0) {
    CropUnitX = SubWidthC;
  }
  uint32_t cropX = CropUnitX * aDest.frame_crop_right_offset;
  aDest.pic_width = aDest.pic_width_in_mbs * 16 - cropX;

  
  uint32_t CropUnitY = 2 - aDest.frame_mbs_only_flag;
  uint32_t SubHeightC = aDest.chroma_format_idc <= 1 ? 2 : 1;
  if (ChromaArrayType != 0)
    CropUnitY *= SubHeightC;
  uint32_t cropY = CropUnitY * aDest.frame_crop_bottom_offset;
  aDest.pic_height = aDest.pic_height_in_map_units * 16 - cropY;

  aDest.interlaced = !aDest.frame_mbs_only_flag;

  
  if (aDest.sample_ratio > 1.0) {
    
    aDest.display_width =
      ConditionDimension(aDest.pic_width * aDest.sample_ratio);
    aDest.display_height = aDest.pic_height;
  } else {
    
    aDest.display_width = aDest.pic_width;
    aDest.display_height =
      ConditionDimension(aDest.pic_height / aDest.sample_ratio);
  }

  return true;
}

 void
H264::vui_parameters(BitReader& aBr, SPSData& aDest)
{
  aDest.aspect_ratio_info_present_flag = aBr.ReadBit();
  if (aDest.aspect_ratio_info_present_flag) {
    aDest.aspect_ratio_idc = aBr.ReadBits(8);
    aDest.sar_width = aDest.sar_height = 0;

    
    switch (aDest.aspect_ratio_idc)  {
      case 0:
        
        break;
      case 1:
        







        aDest.sample_ratio = 1.0f;
        break;
      case 2:
        




        aDest.sample_ratio = 12.0 / 11.0;
        break;
      case 3:
        




        aDest.sample_ratio = 10.0 / 11.0;
        break;
      case 4:
        




        aDest.sample_ratio = 16.0 / 11.0;
        break;
      case 5:
        




        aDest.sample_ratio = 40.0 / 33.0;
        break;
      case 6:
        




        aDest.sample_ratio = 24.0 / 11.0;
        break;
      case 7:
        




        aDest.sample_ratio = 20.0 / 11.0;
        break;
      case 8:
        



        aDest.sample_ratio = 32.0 / 11.0;
        break;
      case 9:
        



        aDest.sample_ratio = 80.0 / 33.0;
        break;
      case 10:
        



        aDest.sample_ratio = 18.0 / 11.0;
        break;
      case 11:
        



        aDest.sample_ratio = 15.0 / 11.0;
        break;
      case 12:
        



        aDest.sample_ratio = 64.0 / 33.0;
        break;
      case 13:
        



        aDest.sample_ratio = 160.0 / 99.0;
        break;
      case 14:
        



        aDest.sample_ratio = 4.0 / 3.0;
        break;
      case 15:
        



        aDest.sample_ratio = 3.2 / 2.0;
        break;
      case 16:
        



        aDest.sample_ratio = 2.0 / 1.0;
        break;
      case 255:
        
        aDest.sar_width  = aBr.ReadBits(16);
        aDest.sar_height = aBr.ReadBits(16);
        if (aDest.sar_width && aDest.sar_height) {
          aDest.sample_ratio = float(aDest.sar_width) / float(aDest.sar_height);
        }
        break;
      default:
        break;
    }
  }

  if (aBr.ReadBit()) { 
    aDest.overscan_appropriate_flag = aBr.ReadBit();
  }

  if (aBr.ReadBit()) { 
    aDest.video_format = aBr.ReadBits(3);
    aDest.video_full_range_flag = aBr.ReadBit();
    aDest.colour_description_present_flag = aBr.ReadBit();
    if (aDest.colour_description_present_flag) {
      aDest.colour_primaries = aBr.ReadBits(8);
      aDest.transfer_characteristics = aBr.ReadBits(8);
      aDest.matrix_coefficients = aBr.ReadBits(8);
    }
  }

  aDest.chroma_loc_info_present_flag = aBr.ReadBit();
  if (aDest.chroma_loc_info_present_flag) {
    aDest.chroma_sample_loc_type_top_field = aBr.ReadUE();
    aDest.chroma_sample_loc_type_bottom_field = aBr.ReadUE();
  }

  aDest.timing_info_present_flag = aBr.ReadBit();
  if (aDest.timing_info_present_flag ) {
    aDest.num_units_in_tick = aBr.ReadBits(32);
    aDest.time_scale = aBr.ReadBits(32);
    aDest.fixed_frame_rate_flag = aBr.ReadBit();
  }
}

 bool
H264::DecodeSPSFromExtraData(const ByteBuffer* aExtraData, SPSData& aDest)
{
  if (!AnnexB::HasSPS(aExtraData)) {
    return false;
  }
  ByteReader reader(*aExtraData);

  if (!reader.Read(5)) {
    return false;
  }

  if (!(reader.ReadU8() & 0x1f)) {
    
    reader.DiscardRemaining();
    return false;
  }
  uint16_t length = reader.ReadU16();

  if ((reader.PeekU8() & 0x1f) != 7) {
    
    reader.DiscardRemaining();
    return false;
  }

  const uint8_t* ptr = reader.Read(length);
  if (!ptr) {
    return false;
  }

  nsRefPtr<ByteBuffer> rawNAL = new ByteBuffer;
  rawNAL->AppendElements(ptr, length);

  nsRefPtr<ByteBuffer> sps = DecodeNALUnit(rawNAL);

  reader.DiscardRemaining();

  return DecodeSPS(sps, aDest);
}

 bool
H264::EnsureSPSIsSane(SPSData& aSPS)
{
  bool valid = true;
  static const float default_aspect = 4.0f / 3.0f;
  if (aSPS.sample_ratio <= 0.0f || aSPS.sample_ratio > 6.0f) {
    if (aSPS.pic_width && aSPS.pic_height) {
      aSPS.sample_ratio =
        (float) aSPS.pic_width / (float) aSPS.pic_height;
    } else {
      aSPS.sample_ratio = default_aspect;
    }
    aSPS.display_width = aSPS.pic_width;
    aSPS.display_height = aSPS.pic_height;
    valid = false;
  }
  if (aSPS.max_num_ref_frames > 16) {
    aSPS.max_num_ref_frames = 16;
    valid = false;
  }
  return valid;
}

} 
