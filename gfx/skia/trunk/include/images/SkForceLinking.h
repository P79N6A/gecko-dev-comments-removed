















int SkForceLinking(bool doNotPassTrue);

#define __SK_FORCE_IMAGE_DECODER_LINKING       \
static int linking_forced = SkForceLinking(false)
