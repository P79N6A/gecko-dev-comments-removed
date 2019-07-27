



#ifndef MP4_DEMUXER_H264_H_
#define MP4_DEMUXER_H264_H_

#include "mp4_demuxer/DecoderData.h"

namespace mp4_demuxer
{

class BitReader;

struct SPSData
{
  
  




  uint32_t pic_width;
  




  uint32_t pic_height;

  bool interlaced;

  




  uint32_t display_width;
  uint32_t display_height;

  float sample_ratio;

  uint32_t crop_left;
  uint32_t crop_right;
  uint32_t crop_top;
  uint32_t crop_bottom;

  




  bool constraint_set0_flag;
  bool constraint_set1_flag;
  bool constraint_set2_flag;
  bool constraint_set3_flag;
  bool constraint_set4_flag;
  bool constraint_set5_flag;

  




  uint8_t profile_idc;
  uint8_t level_idc;

  




  uint8_t seq_parameter_set_id;

  







  uint8_t chroma_format_idc;

  










  bool separate_colour_plane_flag;

  







  uint8_t log2_max_frame_num;

  




  uint8_t pic_order_cnt_type;

  










  uint8_t log2_max_pic_order_cnt_lsb;

  





  bool delta_pic_order_always_zero_flag;

  





  int8_t offset_for_non_ref_pic;

  





  int8_t offset_for_top_to_bottom_field;

  









  uint32_t max_num_ref_frames;

  





  bool gaps_in_frame_num_allowed_flag;

  



  uint32_t pic_width_in_mbs;

  




  uint32_t pic_height_in_map_units;

  






  bool frame_mbs_only_flag;

  







  bool mb_adaptive_frame_field_flag;

  





  bool frame_cropping_flag;
  uint32_t frame_crop_left_offset;;
  uint32_t frame_crop_right_offset;
  uint32_t frame_crop_top_offset;
  uint32_t frame_crop_bottom_offset;

  

  






  bool vui_parameters_present_flag;

  




  bool aspect_ratio_info_present_flag;

  








  uint8_t aspect_ratio_idc;
  uint32_t sar_width;
  uint32_t sar_height;

  





  bool video_signal_type_present_flag;

  





  bool overscan_info_present_flag;
  






  bool overscan_appropriate_flag;

  






  uint8_t video_format;

  






  bool video_full_range_flag;

  





  bool colour_description_present_flag;

  







  uint8_t colour_primaries;

  









  uint8_t transfer_characteristics;

  uint8_t matrix_coefficients;
  bool chroma_loc_info_present_flag;
  uint32_t chroma_sample_loc_type_top_field;
  uint32_t chroma_sample_loc_type_bottom_field;
  bool timing_info_present_flag;
  uint32_t num_units_in_tick;
  uint32_t time_scale;
  bool fixed_frame_rate_flag;

  SPSData();
};

class H264
{
public:
  static bool DecodeSPSFromExtraData(const mozilla::DataBuffer* aExtraData, SPSData& aDest);
  



  static already_AddRefed<mozilla::DataBuffer> DecodeNALUnit(const mozilla::DataBuffer* aNAL);
  
  static bool DecodeSPS(const mozilla::DataBuffer* aSPS, SPSData& aDest);
  
  
  static bool EnsureSPSIsSane(SPSData& aSPS);

private:
  static void vui_parameters(BitReader& aBr, SPSData& aDest);
  
  static void hrd_parameters(BitReader& aBr);
};

} 

#endif 
