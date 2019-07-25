
















#include "ivorbiscodec.h"
#include "codec_internal.h"
#include "registry.h"
#include "misc.h"





extern vorbis_func_floor     floor0_exportbundle;
extern vorbis_func_floor     floor1_exportbundle;
extern vorbis_func_residue   residue0_exportbundle;
extern vorbis_func_residue   residue1_exportbundle;
extern vorbis_func_residue   residue2_exportbundle;
extern vorbis_func_mapping   mapping0_exportbundle;

vorbis_func_floor     *_floor_P[]={
  &floor0_exportbundle,
  &floor1_exportbundle,
};

vorbis_func_residue   *_residue_P[]={
  &residue0_exportbundle,
  &residue1_exportbundle,
  &residue2_exportbundle,
};

vorbis_func_mapping   *_mapping_P[]={
  &mapping0_exportbundle,
};



