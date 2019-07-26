





#if !defined(CUBEB_c2f983e9_c96f_e71c_72c3_bbf62992a382)
#define CUBEB_c2f983e9_c96f_e71c_72c3_bbf62992a382

#include <cubeb/cubeb-stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif































































typedef struct cubeb cubeb;               
typedef struct cubeb_stream cubeb_stream; 


typedef enum {
  
  CUBEB_SAMPLE_S16LE,
  
  CUBEB_SAMPLE_S16BE,
  
  CUBEB_SAMPLE_FLOAT32LE,
  
  CUBEB_SAMPLE_FLOAT32BE,
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
  
  CUBEB_SAMPLE_S16NE = CUBEB_SAMPLE_S16BE,
  
  CUBEB_SAMPLE_FLOAT32NE = CUBEB_SAMPLE_FLOAT32BE
#else
  
  CUBEB_SAMPLE_S16NE = CUBEB_SAMPLE_S16LE,
  
  CUBEB_SAMPLE_FLOAT32NE = CUBEB_SAMPLE_FLOAT32LE
#endif
} cubeb_sample_format;

#if defined(__ANDROID__)
typedef enum {
    CUBEB_STREAM_TYPE_VOICE_CALL = 0,
    CUBEB_STREAM_TYPE_SYSTEM = 1,
    CUBEB_STREAM_TYPE_RING = 2,
    CUBEB_STREAM_TYPE_MUSIC = 3,
    CUBEB_STREAM_TYPE_ALARM = 4,
    CUBEB_STREAM_TYPE_NOTIFICATION = 5,
    CUBEB_STREAM_TYPE_BLUETOOTH_SCO = 6,
    CUBEB_STREAM_TYPE_ENFORCED_AUDIBLE = 7,
    CUBEB_STREAM_TYPE_DTMF = 8,
    CUBEB_STREAM_TYPE_TTS = 9,
    CUBEB_STREAM_TYPE_FM = 10,

    CUBEB_STREAM_TYPE_MAX
} cubeb_stream_type;
#endif


typedef struct {
  cubeb_sample_format format; 

  unsigned int rate;          
  unsigned int channels;      
#if defined(__ANDROID__)
  cubeb_stream_type stream_type; 
#endif
} cubeb_stream_params;


typedef enum {
  CUBEB_STATE_STARTED, 
  CUBEB_STATE_STOPPED, 
  CUBEB_STATE_DRAINED, 
  CUBEB_STATE_ERROR    
} cubeb_state;


enum {
  CUBEB_OK = 0,                       
  CUBEB_ERROR = -1,                   
  CUBEB_ERROR_INVALID_FORMAT = -2,    
  CUBEB_ERROR_INVALID_PARAMETER = -3  
};









typedef long (* cubeb_data_callback)(cubeb_stream * stream,
                                     void * user_ptr,
                                     void * buffer,
                                     long nframes);





typedef void (* cubeb_state_callback)(cubeb_stream * stream,
                                      void * user_ptr,
                                      cubeb_state state);







int cubeb_init(cubeb ** context, char const * context_name);




char const * cubeb_get_backend_id(cubeb * context);



void cubeb_destroy(cubeb * context);














int cubeb_stream_init(cubeb * context, cubeb_stream ** stream, char const * stream_name,
                      cubeb_stream_params stream_params, unsigned int latency,
                      cubeb_data_callback data_callback,
                      cubeb_state_callback state_callback,
                      void * user_ptr);



void cubeb_stream_destroy(cubeb_stream * stream);





int cubeb_stream_start(cubeb_stream * stream);





int cubeb_stream_stop(cubeb_stream * stream);






int cubeb_stream_get_position(cubeb_stream * stream, uint64_t * position);

#if defined(__cplusplus)
}
#endif

#endif
