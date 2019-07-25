





#ifndef   NESTEGG_671cac2a_365d_ed69_d7a3_4491d3538d79
#define   NESTEGG_671cac2a_365d_ed69_d7a3_4491d3538d79

#include <nestegg/nestegg-stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


















































#define NESTEGG_TRACK_VIDEO 0 /**< Track is of type video. */
#define NESTEGG_TRACK_AUDIO 1 /**< Track is of type audio. */

#define NESTEGG_CODEC_VP8    0 /**< Track uses Google On2 VP8 codec. */
#define NESTEGG_CODEC_VORBIS 1 /**< Track uses Xiph Vorbis codec. */

#define NESTEGG_SEEK_SET 0 /**< Seek offset relative to beginning of stream. */
#define NESTEGG_SEEK_CUR 1 /**< Seek offset relative to current position in stream. */
#define NESTEGG_SEEK_END 2 /**< Seek offset relative to end of stream. */

#define NESTEGG_LOG_DEBUG    1     /**< Debug level log message. */
#define NESTEGG_LOG_INFO     10    /**< Informational level log message. */
#define NESTEGG_LOG_WARNING  100   /**< Warning level log message. */
#define NESTEGG_LOG_ERROR    1000  /**< Error level log message. */
#define NESTEGG_LOG_CRITICAL 10000 /**< Critical level log message. */

typedef struct nestegg nestegg;               
typedef struct nestegg_packet nestegg_packet; 


typedef struct {
  






  int (* read)(void * buffer, size_t length, void * userdata);

  






  int (* seek)(int64_t offset, int whence, void * userdata);

  



  int64_t (* tell)(void * userdata);

  
  void * userdata;
} nestegg_io;


typedef struct {
  unsigned int width;          
  unsigned int height;         
  unsigned int display_width;  
  unsigned int display_height; 
  unsigned int crop_bottom;    
  unsigned int crop_top;       
  unsigned int crop_left;      
  unsigned int crop_right;     
} nestegg_video_params;


typedef struct {
  double rate;           
  unsigned int channels; 
  unsigned int depth;    
} nestegg_audio_params;


typedef void (* nestegg_log)(nestegg * context, unsigned int severity, char const * format, ...);









int nestegg_init(nestegg ** context, nestegg_io io, nestegg_log callback);



void nestegg_destroy(nestegg * context);






int nestegg_duration(nestegg * context, uint64_t * duration);






int nestegg_track_count(nestegg * context, unsigned int * tracks);









int nestegg_track_seek(nestegg * context, unsigned int track, uint64_t tstamp);







int nestegg_track_type(nestegg * context, unsigned int track);







int nestegg_track_codec_id(nestegg * context, unsigned int track);









int nestegg_track_codec_data_count(nestegg * context, unsigned int track,
                                   unsigned int * count);











int nestegg_track_codec_data(nestegg * context, unsigned int track, unsigned int item,
                             unsigned char ** data, size_t * length);







int nestegg_track_video_params(nestegg * context, unsigned int track,
                               nestegg_video_params * params);







int nestegg_track_audio_params(nestegg * context, unsigned int track,
                               nestegg_audio_params * params);










int nestegg_read_packet(nestegg * context, nestegg_packet ** packet);



void nestegg_free_packet(nestegg_packet * packet);






int nestegg_packet_track(nestegg_packet * packet, unsigned int * track);






int nestegg_packet_tstamp(nestegg_packet * packet, uint64_t * tstamp);






int nestegg_packet_count(nestegg_packet * packet, unsigned int * count);









int nestegg_packet_data(nestegg_packet * packet, unsigned int item,
                        unsigned char ** data, size_t * length);

#ifdef __cplusplus
}
#endif

#endif
