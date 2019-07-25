
















typedef struct highlevel_byblocktype {
  double tone_mask_setting;
  double tone_peaklimit_setting;
  double noise_bias_setting;
  double noise_compand_setting;
} highlevel_byblocktype;

typedef struct highlevel_encode_setup {
  int   set_in_stone;
  const void *setup;
  double base_setting;

  double impulse_noisetune;

  
  float  req;
  int    managed;
  long   bitrate_min;
  long   bitrate_av;
  double bitrate_av_damp;
  long   bitrate_max;
  long   bitrate_reservoir;
  double bitrate_reservoir_bias;

  int impulse_block_p;
  int noise_normalize_p;
  int coupling_p;

  double stereo_point_setting;
  double lowpass_kHz;
  int    lowpass_altered;

  double ath_floating_dB;
  double ath_absolute_dB;

  double amplitude_track_dBpersec;
  double trigger_setting;

  highlevel_byblocktype block[4]; 

} highlevel_encode_setup;
