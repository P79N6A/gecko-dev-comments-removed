



















#ifndef AVFORMAT_AVFORMAT_H
#define AVFORMAT_AVFORMAT_H



















































































































#include <time.h>
#include <stdio.h>  
#include "libavcodec/avcodec.h"
#include "libavutil/dict.h"
#include "libavutil/log.h"

#include "avio.h"
#include "libavformat/version.h"

struct AVFormatContext;











































































#if FF_API_OLD_METADATA2







#define AV_METADATA_MATCH_CASE      AV_DICT_MATCH_CASE
#define AV_METADATA_IGNORE_SUFFIX   AV_DICT_IGNORE_SUFFIX
#define AV_METADATA_DONT_STRDUP_KEY AV_DICT_DONT_STRDUP_KEY
#define AV_METADATA_DONT_STRDUP_VAL AV_DICT_DONT_STRDUP_VAL
#define AV_METADATA_DONT_OVERWRITE  AV_DICT_DONT_OVERWRITE

typedef attribute_deprecated AVDictionary AVMetadata;
typedef attribute_deprecated AVDictionaryEntry  AVMetadataTag;

typedef struct AVMetadataConv AVMetadataConv;









attribute_deprecated AVDictionaryEntry *
av_metadata_get(AVDictionary *m, const char *key, const AVDictionaryEntry *prev, int flags);











attribute_deprecated int av_metadata_set2(AVDictionary **pm, const char *key, const char *value, int flags);




attribute_deprecated void av_metadata_conv(struct AVFormatContext *ctx, const AVMetadataConv *d_conv,
                                                                        const AVMetadataConv *s_conv);









attribute_deprecated void av_metadata_copy(AVDictionary **dst, AVDictionary *src, int flags);




attribute_deprecated void av_metadata_free(AVDictionary **m);



#endif













int av_get_packet(AVIOContext *s, AVPacket *pkt, int size);















int av_append_packet(AVIOContext *s, AVPacket *pkt, int size);








typedef struct AVFrac {
    int64_t val, num, den;
} AVFrac;




struct AVCodecTag;




typedef struct AVProbeData {
    const char *filename;
    unsigned char *buf; 
    int buf_size;       
} AVProbeData;

#define AVPROBE_SCORE_MAX 100               ///< maximum score, half of that is used for file-extension-based detection
#define AVPROBE_PADDING_SIZE 32             ///< extra allocated bytes at the end of the probe buffer

typedef struct AVFormatParameters {
#if FF_API_FORMAT_PARAMETERS
    attribute_deprecated AVRational time_base;
    attribute_deprecated int sample_rate;
    attribute_deprecated int channels;
    attribute_deprecated int width;
    attribute_deprecated int height;
    attribute_deprecated enum PixelFormat pix_fmt;
    attribute_deprecated int channel; 
    attribute_deprecated const char *standard; 
    attribute_deprecated unsigned int mpeg2ts_raw:1;  
    
    attribute_deprecated unsigned int mpeg2ts_compute_pcr:1;
    attribute_deprecated unsigned int initial_pause:1;       

    attribute_deprecated unsigned int prealloced_context:1;
#endif
} AVFormatParameters;


#define AVFMT_NOFILE        0x0001
#define AVFMT_NEEDNUMBER    0x0002 /**< Needs '%d' in filename. */
#define AVFMT_SHOW_IDS      0x0008 /**< Show format stream IDs numbers. */
#define AVFMT_RAWPICTURE    0x0020 /**< Format wants AVPicture structure for
                                      raw picture data. */
#define AVFMT_GLOBALHEADER  0x0040 /**< Format wants global header. */
#define AVFMT_NOTIMESTAMPS  0x0080 /**< Format does not need / have any timestamps. */
#define AVFMT_GENERIC_INDEX 0x0100 /**< Use generic index building code. */
#define AVFMT_TS_DISCONT    0x0200 /**< Format allows timestamp discontinuities. Note, muxers always require valid (monotone) timestamps */
#define AVFMT_VARIABLE_FPS  0x0400 /**< Format allows variable fps. */
#define AVFMT_NODIMENSIONS  0x0800 /**< Format does not need width/height */
#define AVFMT_NOSTREAMS     0x1000 /**< Format does not require any streams */
#define AVFMT_NOBINSEARCH   0x2000 /**< Format does not allow to fallback to binary search via read_timestamp */
#define AVFMT_NOGENSEARCH   0x4000 /**< Format does not allow to fallback to generic search */
#define AVFMT_NO_BYTE_SEEK  0x8000 /**< Format does not allow seeking by bytes */





typedef struct AVOutputFormat {
    const char *name;
    




    const char *long_name;
    const char *mime_type;
    const char *extensions; 
    


    int priv_data_size;
    
    enum CodecID audio_codec; 
    enum CodecID video_codec; 
    int (*write_header)(struct AVFormatContext *);
    int (*write_packet)(struct AVFormatContext *, AVPacket *pkt);
    int (*write_trailer)(struct AVFormatContext *);
    




    int flags;
    


    int (*set_parameters)(struct AVFormatContext *, AVFormatParameters *);
    int (*interleave_packet)(struct AVFormatContext *, AVPacket *out,
                             AVPacket *in, int flush);

    



    const struct AVCodecTag * const *codec_tag;

    enum CodecID subtitle_codec; 

#if FF_API_OLD_METADATA2
    const AVMetadataConv *metadata_conv;
#endif

    const AVClass *priv_class; 

    





    int (*query_codec)(enum CodecID id, int std_compliance);

    
    struct AVOutputFormat *next;
} AVOutputFormat;








typedef struct AVInputFormat {
    



    const char *name;

    




    const char *long_name;

    


    int priv_data_size;

    




    int (*read_probe)(AVProbeData *);

    





    int (*read_header)(struct AVFormatContext *,
                       AVFormatParameters *ap);

    








    int (*read_packet)(struct AVFormatContext *, AVPacket *pkt);

    



    int (*read_close)(struct AVFormatContext *);

    







    int (*read_seek)(struct AVFormatContext *,
                     int stream_index, int64_t timestamp, int flags);

    



    int64_t (*read_timestamp)(struct AVFormatContext *s, int stream_index,
                              int64_t *pos, int64_t pos_limit);

    




    int flags;

    




    const char *extensions;

    


    int value;

    



    int (*read_play)(struct AVFormatContext *);

    



    int (*read_pause)(struct AVFormatContext *);

    const struct AVCodecTag * const *codec_tag;

    





    int (*read_seek2)(struct AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);

#if FF_API_OLD_METADATA2
    const AVMetadataConv *metadata_conv;
#endif

    const AVClass *priv_class; 

    
    struct AVInputFormat *next;
} AVInputFormat;




enum AVStreamParseType {
    AVSTREAM_PARSE_NONE,
    AVSTREAM_PARSE_FULL,       
    AVSTREAM_PARSE_HEADERS,    
    AVSTREAM_PARSE_TIMESTAMPS, 
    AVSTREAM_PARSE_FULL_ONCE,  
};

typedef struct AVIndexEntry {
    int64_t pos;
    int64_t timestamp;
#define AVINDEX_KEYFRAME 0x0001
    int flags:2;
    int size:30; 
    int min_distance;         
} AVIndexEntry;

#define AV_DISPOSITION_DEFAULT   0x0001
#define AV_DISPOSITION_DUB       0x0002
#define AV_DISPOSITION_ORIGINAL  0x0004
#define AV_DISPOSITION_COMMENT   0x0008
#define AV_DISPOSITION_LYRICS    0x0010
#define AV_DISPOSITION_KARAOKE   0x0020






#define AV_DISPOSITION_FORCED    0x0040
#define AV_DISPOSITION_HEARING_IMPAIRED  0x0080  /**< stream for hearing impaired audiences */
#define AV_DISPOSITION_VISUAL_IMPAIRED   0x0100  /**< stream for visual impaired audiences */
#define AV_DISPOSITION_CLEAN_EFFECTS     0x0200  /**< stream without voice */








typedef struct AVStream {
    int index;    
    int id;       
    AVCodecContext *codec; 
    







    AVRational r_frame_rate;
    void *priv_data;

#if FF_API_REORDER_PRIVATE
    
    int64_t first_dts;
#endif

    


    struct AVFrac pts;

    






    AVRational time_base;
#if FF_API_REORDER_PRIVATE
    int pts_wrap_bits; 
#endif
#if FF_API_STREAM_COPY
    
    attribute_deprecated int stream_copy; 
#endif
    enum AVDiscard discard; 

#if FF_API_AVSTREAM_QUALITY
    
    



    attribute_deprecated float quality;
#endif

    





    int64_t start_time;

    




    int64_t duration;

#if FF_API_REORDER_PRIVATE
    
    enum AVStreamParseType need_parsing;
    struct AVCodecParserContext *parser;

    int64_t cur_dts;
    int last_IP_duration;
    int64_t last_IP_pts;
    
    AVIndexEntry *index_entries; 

    int nb_index_entries;
    unsigned int index_entries_allocated_size;
#endif

    int64_t nb_frames;                 

    int disposition; 

#if FF_API_REORDER_PRIVATE
    AVProbeData probe_data;
#define MAX_REORDER_DELAY 16
    int64_t pts_buffer[MAX_REORDER_DELAY+1];
#endif

    




    AVRational sample_aspect_ratio;

    AVDictionary *metadata;

#if FF_API_REORDER_PRIVATE
    
    
    const uint8_t *cur_ptr;
    int cur_len;
    AVPacket cur_pkt;

    
    






    int64_t reference_dts;

    



#define MAX_PROBE_PACKETS 2500
    int probe_packets;

    




    struct AVPacketList *last_in_packet_buffer;
#endif

    


    AVRational avg_frame_rate;

    







    


    int codec_info_nb_frames;

    


#define MAX_STD_TIMEBASES (60*12+5)
    struct {
        int64_t last_dts;
        int64_t duration_gcd;
        int duration_count;
        double duration_error[MAX_STD_TIMEBASES];
        int64_t codec_info_duration;
        int nb_decoded_frames;
    } *info;
#if !FF_API_REORDER_PRIVATE
    const uint8_t *cur_ptr;
    int cur_len;
    AVPacket cur_pkt;

    
    






    int64_t reference_dts;
    int64_t first_dts;
    int64_t cur_dts;
    int last_IP_duration;
    int64_t last_IP_pts;

    


#define MAX_PROBE_PACKETS 2500
    int probe_packets;

    


    struct AVPacketList *last_in_packet_buffer;
    AVProbeData probe_data;
#define MAX_REORDER_DELAY 16
    int64_t pts_buffer[MAX_REORDER_DELAY+1];
    
    enum AVStreamParseType need_parsing;
    struct AVCodecParserContext *parser;

    AVIndexEntry *index_entries; 

    int nb_index_entries;
    unsigned int index_entries_allocated_size;

    int pts_wrap_bits; 
#endif
} AVStream;

#define AV_PROGRAM_RUNNING 1







typedef struct AVProgram {
    int            id;
    int            flags;
    enum AVDiscard discard;        
    unsigned int   *stream_index;
    unsigned int   nb_stream_indexes;
    AVDictionary *metadata;
} AVProgram;

#define AVFMTCTX_NOHEADER      0x0001 /**< signal that no header is present
                                         (streams are added dynamically) */

typedef struct AVChapter {
    int id;                 
    AVRational time_base;   
    int64_t start, end;     
    AVDictionary *metadata;
} AVChapter;









typedef struct AVFormatContext {
    



    const AVClass *av_class;

    





    struct AVInputFormat *iformat;
    struct AVOutputFormat *oformat;

    



    void *priv_data;

    










    AVIOContext *pb;

    








    unsigned int nb_streams;
    AVStream **streams;

    char filename[1024]; 
    
#if FF_API_TIMESTAMP
    


    attribute_deprecated int64_t timestamp;
#endif

    int ctx_flags; 
#if FF_API_REORDER_PRIVATE
    
    




    struct AVPacketList *packet_buffer;
#endif

    




    int64_t start_time;

    





    int64_t duration;

#if FF_API_FILESIZE
    


    attribute_deprecated int64_t file_size;
#endif

    




    int bit_rate;

#if FF_API_REORDER_PRIVATE
    
    AVStream *cur_st;

    
    int64_t data_offset; 
#endif

#if FF_API_MUXRATE
    


    attribute_deprecated int mux_rate;
#endif
    unsigned int packet_size;
#if FF_API_PRELOAD
    attribute_deprecated int preload;
#endif
    int max_delay;

#if FF_API_LOOP_OUTPUT
#define AVFMT_NOOUTPUTLOOP -1
#define AVFMT_INFINITEOUTPUTLOOP 0
    




    attribute_deprecated int loop_output;
#endif

    int flags;
#define AVFMT_FLAG_GENPTS       0x0001 ///< Generate missing pts even if it requires parsing future frames.
#define AVFMT_FLAG_IGNIDX       0x0002 ///< Ignore index.
#define AVFMT_FLAG_NONBLOCK     0x0004 ///< Do not block when reading packets from input.
#define AVFMT_FLAG_IGNDTS       0x0008 ///< Ignore DTS on frames that contain both DTS & PTS
#define AVFMT_FLAG_NOFILLIN     0x0010 ///< Do not infer any values from other values, just return what is stored in the container
#define AVFMT_FLAG_NOPARSE      0x0020 ///< Do not use AVParsers, you also must set AVFMT_FLAG_NOFILLIN as the fillin code works on frames and no parsing -> no frames. Also seeking to frames can not work if parsing to find frame boundaries has been disabled
#if FF_API_FLAG_RTP_HINT
#define AVFMT_FLAG_RTP_HINT     0x0040 ///< Deprecated, use the -movflags rtphint muxer specific AVOption instead
#endif
#define AVFMT_FLAG_CUSTOM_IO    0x0080 ///< The caller has supplied a custom AVIOContext, don't avio_close() it.
#define AVFMT_FLAG_DISCARD_CORRUPT  0x0100 ///< Discard frames marked corrupted

#if FF_API_LOOP_INPUT
    


    attribute_deprecated int loop_input;
#endif

    


    unsigned int probesize;

    



    int max_analyze_duration;

    const uint8_t *key;
    int keylen;

    unsigned int nb_programs;
    AVProgram **programs;

    



    enum CodecID video_codec_id;

    



    enum CodecID audio_codec_id;

    



    enum CodecID subtitle_codec_id;

    









    unsigned int max_index_size;

    



    unsigned int max_picture_buffer;

    unsigned int nb_chapters;
    AVChapter **chapters;

    


    int debug;
#define FF_FDEBUG_TS        0x0001

#if FF_API_REORDER_PRIVATE
    





    struct AVPacketList *raw_packet_buffer;
    struct AVPacketList *raw_packet_buffer_end;

    struct AVPacketList *packet_buffer_end;
#endif

    AVDictionary *metadata;

#if FF_API_REORDER_PRIVATE
    



#define RAW_PACKET_BUFFER_SIZE 2500000
    int raw_packet_buffer_remaining_size;
#endif

    






    int64_t start_time_realtime;

    


    int fps_probe_size;

    





    int error_recognition;

    








    AVIOInterruptCB interrupt_callback;

    






#if !FF_API_REORDER_PRIVATE
    





    struct AVPacketList *raw_packet_buffer;
    struct AVPacketList *raw_packet_buffer_end;
    


#define RAW_PACKET_BUFFER_SIZE 2500000
    int raw_packet_buffer_remaining_size;

    




    struct AVPacketList *packet_buffer;
    struct AVPacketList *packet_buffer_end;

    
    AVStream *cur_st;

    
    int64_t data_offset; 
#endif
} AVFormatContext;

typedef struct AVPacketList {
    AVPacket pkt;
    struct AVPacketList *next;
} AVPacketList;














unsigned avformat_version(void);




const char *avformat_configuration(void);




const char *avformat_license(void);










void av_register_all(void);

void av_register_input_format(AVInputFormat *format);
void av_register_output_format(AVOutputFormat *format);









int avformat_network_init(void);




int avformat_network_deinit(void);






AVInputFormat  *av_iformat_next(AVInputFormat  *f);






AVOutputFormat *av_oformat_next(AVOutputFormat *f);






AVFormatContext *avformat_alloc_context(void);





void avformat_free_context(AVFormatContext *s);







const AVClass *avformat_get_class(void);
















AVStream *avformat_new_stream(AVFormatContext *s, AVCodec *c);

AVProgram *av_new_program(AVFormatContext *s, int id);






#if FF_API_GUESS_IMG2_CODEC
attribute_deprecated enum CodecID av_guess_image2_codec(const char *filename);
#endif

#if FF_API_PKT_DUMP
attribute_deprecated void av_pkt_dump(FILE *f, AVPacket *pkt, int dump_payload);
attribute_deprecated void av_pkt_dump_log(void *avcl, int level, AVPacket *pkt,
                                          int dump_payload);
#endif










AVInputFormat *av_find_input_format(const char *short_name);







AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened);












AVInputFormat *av_probe_input_format2(AVProbeData *pd, int is_opened, int *score_max);
















int av_probe_input_buffer(AVIOContext *pb, AVInputFormat **fmt,
                          const char *filename, void *logctx,
                          unsigned int offset, unsigned int max_probe_size);

#if FF_API_FORMAT_PARAMETERS





attribute_deprecated int av_open_input_stream(AVFormatContext **ic_ptr,
                         AVIOContext *pb, const char *filename,
                         AVInputFormat *fmt, AVFormatParameters *ap);















attribute_deprecated int av_open_input_file(AVFormatContext **ic_ptr, const char *filename,
                       AVInputFormat *fmt,
                       int buf_size,
                       AVFormatParameters *ap);
#endif




















int avformat_open_input(AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary **options);

#if FF_API_FORMAT_PARAMETERS















attribute_deprecated
int av_find_stream_info(AVFormatContext *ic);
#endif






















int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options);

























int av_find_best_stream(AVFormatContext *ic,
                        enum AVMediaType type,
                        int wanted_stream_nb,
                        int related_stream,
                        AVCodec **decoder_ret,
                        int flags);











int av_read_packet(AVFormatContext *s, AVPacket *pkt);

























int av_read_frame(AVFormatContext *s, AVPacket *pkt);












int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp,
                  int flags);



























int avformat_seek_file(AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);





int av_read_play(AVFormatContext *s);






int av_read_pause(AVFormatContext *s);

#if FF_API_FORMAT_PARAMETERS





attribute_deprecated
void av_close_input_stream(AVFormatContext *s);
#endif

#if FF_API_CLOSE_INPUT_FILE






attribute_deprecated
void av_close_input_file(AVFormatContext *s);
#endif





void avformat_close_input(AVFormatContext **s);




#if FF_API_NEW_STREAM










attribute_deprecated
AVStream *av_new_stream(AVFormatContext *s, int id);
#endif

#if FF_API_SET_PTS_INFO



attribute_deprecated
void av_set_pts_info(AVStream *s, int pts_wrap_bits,
                     unsigned int pts_num, unsigned int pts_den);
#endif

#define AVSEEK_FLAG_BACKWARD 1 ///< seek backward
#define AVSEEK_FLAG_BYTE     2 ///< seeking based on position in bytes
#define AVSEEK_FLAG_ANY      4 ///< seek to any frame, even non-keyframes
#define AVSEEK_FLAG_FRAME    8 ///< seeking based on frame number

#if FF_API_SEEK_PUBLIC
attribute_deprecated
int av_seek_frame_binary(AVFormatContext *s, int stream_index,
                         int64_t target_ts, int flags);
attribute_deprecated
void av_update_cur_dts(AVFormatContext *s, AVStream *ref_st, int64_t timestamp);
attribute_deprecated
int64_t av_gen_search(AVFormatContext *s, int stream_index,
                      int64_t target_ts, int64_t pos_min,
                      int64_t pos_max, int64_t pos_limit,
                      int64_t ts_min, int64_t ts_max,
                      int flags, int64_t *ts_ret,
                      int64_t (*read_timestamp)(struct AVFormatContext *, int , int64_t *, int64_t ));
#endif

#if FF_API_FORMAT_PARAMETERS



attribute_deprecated int av_set_parameters(AVFormatContext *s, AVFormatParameters *ap);
#endif




















int avformat_write_header(AVFormatContext *s, AVDictionary **options);

#if FF_API_FORMAT_PARAMETERS











attribute_deprecated int av_write_header(AVFormatContext *s);
#endif













int av_write_frame(AVFormatContext *s, AVPacket *pkt);

























int av_interleaved_write_frame(AVFormatContext *s, AVPacket *pkt);
















int av_interleave_packet_per_dts(AVFormatContext *s, AVPacket *out,
                                 AVPacket *pkt, int flush);










int av_write_trailer(AVFormatContext *s);













AVOutputFormat *av_guess_format(const char *short_name,
                                const char *filename,
                                const char *mime_type);




enum CodecID av_guess_codec(AVOutputFormat *fmt, const char *short_name,
                            const char *filename, const char *mime_type,
                            enum AVMediaType type);
























void av_hex_dump(FILE *f, uint8_t *buf, int size);













void av_hex_dump_log(void *avcl, int level, uint8_t *buf, int size);









void av_pkt_dump2(FILE *f, AVPacket *pkt, int dump_payload, AVStream *st);













void av_pkt_dump_log2(void *avcl, int level, AVPacket *pkt, int dump_payload,
                      AVStream *st);








enum CodecID av_codec_get_id(const struct AVCodecTag * const *tags, unsigned int tag);








unsigned int av_codec_get_tag(const struct AVCodecTag * const *tags, enum CodecID id);

int av_find_default_stream_index(AVFormatContext *s);









int av_index_search_timestamp(AVStream *st, int64_t timestamp, int flags);







int av_add_index_entry(AVStream *st, int64_t pos, int64_t timestamp,
                       int size, int distance, int flags);





















void av_url_split(char *proto,         int proto_size,
                  char *authorization, int authorization_size,
                  char *hostname,      int hostname_size,
                  int *port_ptr,
                  char *path,          int path_size,
                  const char *url);

#if FF_API_DUMP_FORMAT
attribute_deprecated void dump_format(AVFormatContext *ic,
                                      int index,
                                      const char *url,
                                      int is_output);
#endif

void av_dump_format(AVFormatContext *ic,
                    int index,
                    const char *url,
                    int is_output);

#if FF_API_PARSE_DATE







attribute_deprecated
int64_t parse_date(const char *datestr, int duration);
#endif




int64_t av_gettime(void);

#if FF_API_FIND_INFO_TAG



attribute_deprecated int find_info_tag(char *arg, int arg_size, const char *tag1, const char *info);
#endif













int av_get_frame_filename(char *buf, int buf_size,
                          const char *path, int number);







int av_filename_number_test(const char *filename);















int av_sdp_create(AVFormatContext *ac[], int n_files, char *buf, int size);

#if FF_API_SDP_CREATE
attribute_deprecated int avf_sdp_create(AVFormatContext *ac[], int n_files, char *buff, int size);
#endif







int av_match_ext(const char *filename, const char *extensions);









int avformat_query_codec(AVOutputFormat *ofmt, enum CodecID codec_id, int std_compliance);
















const struct AVCodecTag *avformat_get_riff_video_tags(void);



const struct AVCodecTag *avformat_get_riff_audio_tags(void);








#endif 
