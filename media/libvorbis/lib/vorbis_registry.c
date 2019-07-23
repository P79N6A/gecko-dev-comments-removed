
















#include "vorbis/codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "misc.h"



extern const vorbis_func_floor     floor0_exportbundle;
extern const vorbis_func_floor     floor1_exportbundle;
extern const vorbis_func_residue   residue0_exportbundle;
extern const vorbis_func_residue   residue1_exportbundle;
extern const vorbis_func_residue   residue2_exportbundle;
extern const vorbis_func_mapping   mapping0_exportbundle;

const vorbis_func_floor     *const _floor_P[]={
  &floor0_exportbundle,
  &floor1_exportbundle,
};

const vorbis_func_residue   *const _residue_P[]={
  &residue0_exportbundle,
  &residue1_exportbundle,
  &residue2_exportbundle,
};

const vorbis_func_mapping   *const _mapping_P[]={
  &mapping0_exportbundle,
};

