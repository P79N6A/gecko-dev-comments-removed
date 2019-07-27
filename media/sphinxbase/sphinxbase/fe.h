

















































#if defined(_WIN32) && !defined(GNUWINCE)
#define srand48(x) srand(x)
#define lrand48() rand()
#endif

#ifndef _NEW_FE_H_
#define _NEW_FE_H_


#include <sphinxbase/sphinxbase_export.h>

#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/fixpoint.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

#ifdef WORDS_BIGENDIAN
#define NATIVE_ENDIAN "big"
#else
#define NATIVE_ENDIAN "little"
#endif


#define DEFAULT_SAMPLING_RATE 16000

#define DEFAULT_FRAME_RATE 100


#define DEFAULT_FRAME_SHIFT 160

#define DEFAULT_WINDOW_LENGTH 0.025625 

#define DEFAULT_FFT_SIZE 512

#define DEFAULT_NUM_CEPSTRA 13

#define DEFAULT_NUM_FILTERS 40

#define DEFAULT_PRESPCH_STATE_LEN 10

#define DEFAULT_POSTSPCH_STATE_LEN 50

#define DEFAULT_LOWER_FILT_FREQ 133.33334

#define DEFAULT_UPPER_FILT_FREQ 6855.4976

#define DEFAULT_PRE_EMPHASIS_ALPHA 0.97

#define DEFAULT_WARP_TYPE "inverse_linear"

#define SEED  -1

#define waveform_to_cepstral_command_line_macro() \
  { "-logspec", \
    ARG_BOOLEAN, \
    "no", \
    "Write out logspectral files instead of cepstra" }, \
   \
  { "-smoothspec", \
    ARG_BOOLEAN, \
    "no", \
    "Write out cepstral-smoothed logspectral files" }, \
   \
  { "-transform", \
    ARG_STRING, \
    "legacy", \
    "Which type of transform to use to calculate cepstra (legacy, dct, or htk)" }, \
   \
  { "-alpha", \
    ARG_FLOAT32, \
    ARG_STRINGIFY(DEFAULT_PRE_EMPHASIS_ALPHA), \
    "Preemphasis parameter" }, \
   \
  { "-samprate", \
    ARG_FLOAT32, \
    ARG_STRINGIFY(DEFAULT_SAMPLING_RATE), \
    "Sampling rate" }, \
   \
  { "-frate", \
    ARG_INT32, \
    ARG_STRINGIFY(DEFAULT_FRAME_RATE), \
    "Frame rate" }, \
   \
  { "-wlen", \
    ARG_FLOAT32, \
    ARG_STRINGIFY(DEFAULT_WINDOW_LENGTH), \
    "Hamming window length" }, \
   \
  { "-nfft", \
    ARG_INT32, \
    ARG_STRINGIFY(DEFAULT_FFT_SIZE), \
    "Size of FFT" }, \
   \
  { "-nfilt", \
    ARG_INT32, \
    ARG_STRINGIFY(DEFAULT_NUM_FILTERS), \
    "Number of filter banks" }, \
   \
  { "-lowerf", \
    ARG_FLOAT32, \
    ARG_STRINGIFY(DEFAULT_LOWER_FILT_FREQ), \
    "Lower edge of filters" }, \
   \
  { "-upperf", \
    ARG_FLOAT32, \
    ARG_STRINGIFY(DEFAULT_UPPER_FILT_FREQ), \
    "Upper edge of filters" }, \
   \
  { "-unit_area", \
    ARG_BOOLEAN, \
    "yes", \
    "Normalize mel filters to unit area" }, \
   \
  { "-round_filters", \
    ARG_BOOLEAN, \
    "yes", \
    "Round mel filter frequencies to DFT points" }, \
   \
  { "-ncep", \
    ARG_INT32, \
    ARG_STRINGIFY(DEFAULT_NUM_CEPSTRA), \
    "Number of cep coefficients" }, \
   \
  { "-doublebw", \
    ARG_BOOLEAN, \
    "no", \
    "Use double bandwidth filters (same center freq)" }, \
   \
  { "-lifter", \
    ARG_INT32, \
    "0", \
    "Length of sin-curve for liftering, or 0 for no liftering." }, \
   \
  { "-vad_prespeech", \
    ARG_INT32, \
    ARG_STRINGIFY(DEFAULT_PRESPCH_STATE_LEN), \
    "Num of speech frames to trigger vad from silence to speech." }, \
   \
  { "-vad_postspeech", \
    ARG_INT32, \
    ARG_STRINGIFY(DEFAULT_POSTSPCH_STATE_LEN), \
    "Num of silence frames to trigger vad from speech to silence." }, \
   \
  { "-vad_threshold", \
    ARG_FLOAT32, \
    "2.0", \
    "Threshold for decision between noise and silence frames. Log-ratio between signal level and noise level." }, \
   \
  { "-input_endian", \
    ARG_STRING, \
    NATIVE_ENDIAN, \
    "Endianness of input data, big or little, ignored if NIST or MS Wav" }, \
   \
  { "-warp_type", \
    ARG_STRING, \
    DEFAULT_WARP_TYPE, \
    "Warping function type (or shape)" }, \
   \
  { "-warp_params", \
    ARG_STRING, \
    NULL, \
    "Parameters defining the warping function" }, \
   \
  { "-dither", \
    ARG_BOOLEAN, \
    "no", \
    "Add 1/2-bit noise" }, \
   \
  { "-seed", \
    ARG_INT32, \
    ARG_STRINGIFY(SEED), \
    "Seed for random number generator; if less than zero, pick our own" }, \
   \
  { "-remove_dc", \
    ARG_BOOLEAN, \
    "no", \
    "Remove DC offset from each frame" }, \
                                          \
  { "-remove_noise", \
    ARG_BOOLEAN, \
    "yes", \
    "Remove noise with spectral subtraction in mel-energies" }, \
                                                                \
  { "-remove_silence", \
    ARG_BOOLEAN, \
    "yes", \
    "Enables VAD, removes silence frames from processing" }, \
                                                             \
  { "-verbose", \
    ARG_BOOLEAN, \
    "no", \
    "Show input filenames" } \
  
  
#ifdef FIXED_POINT

typedef fixed32 mfcc_t;


#define FLOAT2MFCC(x) FLOAT2FIX(x)

#define MFCC2FLOAT(x) FIX2FLOAT(x)

#define MFCCMUL(a,b) FIXMUL(a,b)
#define MFCCLN(x,in,out) FIXLN_ANY(x,in,out)
#else 


typedef float32 mfcc_t;

#define FLOAT2MFCC(x) (x)

#define MFCC2FLOAT(x) (x)

#define MFCCMUL(a,b) ((a)*(b))
#define MFCCLN(x,in,out) log(x)
#endif 




typedef struct fe_s fe_t;




enum fe_error_e {
	FE_SUCCESS = 0,
	FE_OUTPUT_FILE_SUCCESS  = 0,
	FE_CONTROL_FILE_ERROR = -1,
	FE_START_ERROR = -2,
	FE_UNKNOWN_SINGLE_OR_BATCH = -3,
	FE_INPUT_FILE_OPEN_ERROR = -4,
	FE_INPUT_FILE_READ_ERROR = -5,
	FE_MEM_ALLOC_ERROR = -6,
	FE_OUTPUT_FILE_WRITE_ERROR = -7,
	FE_OUTPUT_FILE_OPEN_ERROR = -8,
	FE_ZERO_ENERGY_ERROR = -9,
	FE_INVALID_PARAM_ERROR =  -10
};








SPHINXBASE_EXPORT
fe_t* fe_init_auto(void);








SPHINXBASE_EXPORT
arg_t const *fe_get_args(void);











SPHINXBASE_EXPORT
fe_t *fe_init_auto_r(cmd_ln_t *config);








SPHINXBASE_EXPORT
const cmd_ln_t *fe_get_config(fe_t *fe);




SPHINXBASE_EXPORT
void fe_start_stream(fe_t *fe);





SPHINXBASE_EXPORT
int fe_start_utt(fe_t *fe);













SPHINXBASE_EXPORT
int fe_get_output_size(fe_t *fe);













SPHINXBASE_EXPORT
void fe_get_input_size(fe_t *fe, int *out_frame_shift,
                       int *out_frame_size);






SPHINXBASE_EXPORT
uint8 fe_get_vad_state(fe_t *fe);















SPHINXBASE_EXPORT
int fe_end_utt(fe_t *fe, mfcc_t *out_cepvector, int32 *out_nframes);






SPHINXBASE_EXPORT
fe_t *fe_retain(fe_t *fe);








SPHINXBASE_EXPORT
int fe_free(fe_t *fe);














SPHINXBASE_EXPORT
int fe_process_frames_ext(fe_t *fe,
                      int16 const **inout_spch,
                      size_t *inout_nsamps,
                      mfcc_t **buf_cep,
                      int32 *inout_nframes,
                      int16 **voiced_spch,
                      int32 *voiced_spch_nsamps,
                      int32 *out_frameidx);


















































SPHINXBASE_EXPORT
int fe_process_frames(fe_t *fe,
                      int16 const **inout_spch,
                      size_t *inout_nsamps,
                      mfcc_t **buf_cep,
                      int32 *inout_nframes,
                      int32 *out_frameidx);
















SPHINXBASE_EXPORT
int fe_process_utt(fe_t *fe,  
                   int16 const *spch, 
                   size_t nsamps, 
                   mfcc_t ***cep_block, 
                   int32 *nframes 
	);




SPHINXBASE_EXPORT
void fe_free_2d(void *arr);




SPHINXBASE_EXPORT
int fe_mfcc_to_float(fe_t *fe,
                     mfcc_t **input,
                     float32 **output,
                     int32 nframes);




SPHINXBASE_EXPORT
int fe_float_to_mfcc(fe_t *fe,
                     float32 **input,
                     mfcc_t **output,
                     int32 nframes);
























SPHINXBASE_EXPORT
int fe_logspec_to_mfcc(fe_t *fe,  
                       const mfcc_t *fr_spec, 
                       mfcc_t *fr_cep 
        );









SPHINXBASE_EXPORT
int fe_logspec_dct2(fe_t *fe,  
                    const mfcc_t *fr_spec, 
                    mfcc_t *fr_cep 
        );









SPHINXBASE_EXPORT
int fe_mfcc_dct3(fe_t *fe,  
                 const mfcc_t *fr_cep, 
                 mfcc_t *fr_spec 
        );

#ifdef __cplusplus
}
#endif


#endif
