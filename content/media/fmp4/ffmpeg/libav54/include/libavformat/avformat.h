



















#ifndef AVFORMAT_AVFORMAT_H
#define AVFORMAT_AVFORMAT_H


















































































































































































#include <time.h>
#include <stdio.h>  
#include "libavcodec/avcodec.h"
#include "libavutil/dict.h"
#include "libavutil/log.h"

#include "avio.h"
#include "libavformat/version.h"

#if FF_API_AV_GETTIME
#include "libavutil/time.h"
#endif

struct AVFormatContext;






















































































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
#define AVFMT_ALLOW_FLUSH  0x10000 /**< Format allows flushing. If not set, the muxer will not receive a NULL packet in the write_packet function. */
#define AVFMT_TS_NONSTRICT 0x20000 /**< Format does not require strictly
                                        increasing timestamps, but they must
                                        still be monotonic */





typedef struct AVOutputFormat {
    const char *name;
    




    const char *long_name;
    const char *mime_type;
    const char *extensions; 
    
    enum AVCodecID audio_codec;    
    enum AVCodecID video_codec;    
    enum AVCodecID subtitle_codec; 
    





    int flags;

    



    const struct AVCodecTag * const *codec_tag;


    const AVClass *priv_class; 

    






    struct AVOutputFormat *next;
    


    int priv_data_size;

    int (*write_header)(struct AVFormatContext *);
    






    int (*write_packet)(struct AVFormatContext *, AVPacket *pkt);
    int (*write_trailer)(struct AVFormatContext *);
    


    int (*interleave_packet)(struct AVFormatContext *, AVPacket *out,
                             AVPacket *in, int flush);
    





    int (*query_codec)(enum AVCodecID id, int std_compliance);
} AVOutputFormat;








typedef struct AVInputFormat {
    



    const char *name;

    




    const char *long_name;

    




    int flags;

    




    const char *extensions;

    const struct AVCodecTag * const *codec_tag;

    const AVClass *priv_class; 

    






    struct AVInputFormat *next;

    


    int raw_codec_id;

    


    int priv_data_size;

    




    int (*read_probe)(AVProbeData *);

    




    int (*read_header)(struct AVFormatContext *);

    








    int (*read_packet)(struct AVFormatContext *, AVPacket *pkt);

    



    int (*read_close)(struct AVFormatContext *);

    







    int (*read_seek)(struct AVFormatContext *,
                     int stream_index, int64_t timestamp, int flags);

    



    int64_t (*read_timestamp)(struct AVFormatContext *s, int stream_index,
                              int64_t *pos, int64_t pos_limit);

    



    int (*read_play)(struct AVFormatContext *);

    



    int (*read_pause)(struct AVFormatContext *);

    





    int (*read_seek2)(struct AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);
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






#define AV_DISPOSITION_ATTACHED_PIC      0x0400








typedef struct AVStream {
    int index;    
    




    int id;
    










    AVCodecContext *codec;
#if FF_API_R_FRAME_RATE
    







    AVRational r_frame_rate;
#endif
    void *priv_data;

    


    struct AVFrac pts;

    








    AVRational time_base;

    





    int64_t start_time;

    




    int64_t duration;

    int64_t nb_frames;                 

    int disposition; 

    enum AVDiscard discard; 

    




    AVRational sample_aspect_ratio;

    AVDictionary *metadata;

    


    AVRational avg_frame_rate;

    






    AVPacket attached_pic;

    







    


#define MAX_STD_TIMEBASES (60*12+5)
    struct {
#if FF_API_R_FRAME_RATE
        int64_t last_dts;
        int64_t duration_gcd;
        int duration_count;
        double duration_error[MAX_STD_TIMEBASES];
#endif
        int nb_decoded_frames;
        int found_decoder;

        


        int64_t fps_first_dts;
        int     fps_first_dts_idx;
        int64_t fps_last_dts;
        int     fps_last_dts_idx;

    } *info;

    int pts_wrap_bits; 

    
    






    int64_t reference_dts;
    int64_t first_dts;
    int64_t cur_dts;
    int64_t last_IP_pts;
    int last_IP_duration;

    


#define MAX_PROBE_PACKETS 2500
    int probe_packets;

    


    int codec_info_nb_frames;

    
    enum AVStreamParseType need_parsing;
    struct AVCodecParserContext *parser;

    


    struct AVPacketList *last_in_packet_buffer;
    AVProbeData probe_data;
#define MAX_REORDER_DELAY 16
    int64_t pts_buffer[MAX_REORDER_DELAY+1];

    AVIndexEntry *index_entries; 

    int nb_index_entries;
    unsigned int index_entries_allocated_size;
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

    
    int ctx_flags; 

    








    unsigned int nb_streams;
    AVStream **streams;

    char filename[1024]; 

    




    int64_t start_time;

    





    int64_t duration;

    




    int bit_rate;

    unsigned int packet_size;
    int max_delay;

    int flags;
#define AVFMT_FLAG_GENPTS       0x0001 ///< Generate missing pts even if it requires parsing future frames.
#define AVFMT_FLAG_IGNIDX       0x0002 ///< Ignore index.
#define AVFMT_FLAG_NONBLOCK     0x0004 ///< Do not block when reading packets from input.
#define AVFMT_FLAG_IGNDTS       0x0008 ///< Ignore DTS on frames that contain both DTS & PTS
#define AVFMT_FLAG_NOFILLIN     0x0010 ///< Do not infer any values from other values, just return what is stored in the container
#define AVFMT_FLAG_NOPARSE      0x0020 ///< Do not use AVParsers, you also must set AVFMT_FLAG_NOFILLIN as the fillin code works on frames and no parsing -> no frames. Also seeking to frames can not work if parsing to find frame boundaries has been disabled
#define AVFMT_FLAG_NOBUFFER     0x0040 ///< Do not buffer frames when possible
#define AVFMT_FLAG_CUSTOM_IO    0x0080 ///< The caller has supplied a custom AVIOContext, don't avio_close() it.
#define AVFMT_FLAG_DISCARD_CORRUPT  0x0100 ///< Discard frames marked corrupted

    


    unsigned int probesize;

    



    int max_analyze_duration;

    const uint8_t *key;
    int keylen;

    unsigned int nb_programs;
    AVProgram **programs;

    



    enum AVCodecID video_codec_id;

    



    enum AVCodecID audio_codec_id;

    



    enum AVCodecID subtitle_codec_id;

    









    unsigned int max_index_size;

    



    unsigned int max_picture_buffer;

    unsigned int nb_chapters;
    AVChapter **chapters;

    AVDictionary *metadata;

    






    int64_t start_time_realtime;

    


    int fps_probe_size;

    





    int error_recognition;

    








    AVIOInterruptCB interrupt_callback;

    


    int debug;
#define FF_FDEBUG_TS        0x0001
    







    




    struct AVPacketList *packet_buffer;
    struct AVPacketList *packet_buffer_end;

    
    int64_t data_offset; 

    





    struct AVPacketList *raw_packet_buffer;
    struct AVPacketList *raw_packet_buffer_end;
    


    struct AVPacketList *parse_queue;
    struct AVPacketList *parse_queue_end;
    


#define RAW_PACKET_BUFFER_SIZE 2500000
    int raw_packet_buffer_remaining_size;
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














AVInputFormat *av_find_input_format(const char *short_name);







AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened);












AVInputFormat *av_probe_input_format2(AVProbeData *pd, int is_opened, int *score_max);
















int av_probe_input_buffer(AVIOContext *pb, AVInputFormat **fmt,
                          const char *filename, void *logctx,
                          unsigned int offset, unsigned int max_probe_size);




















int avformat_open_input(AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary **options);






















int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options);

























int av_find_best_stream(AVFormatContext *ic,
                        enum AVMediaType type,
                        int wanted_stream_nb,
                        int related_stream,
                        AVCodec **decoder_ret,
                        int flags);

#if FF_API_READ_PACKET













attribute_deprecated
int av_read_packet(AVFormatContext *s, AVPacket *pkt);
#endif

























int av_read_frame(AVFormatContext *s, AVPacket *pkt);












int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp,
                  int flags);



























int avformat_seek_file(AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);





int av_read_play(AVFormatContext *s);






int av_read_pause(AVFormatContext *s);

#if FF_API_CLOSE_INPUT_FILE






attribute_deprecated
void av_close_input_file(AVFormatContext *s);
#endif





void avformat_close_input(AVFormatContext **s);




#define AVSEEK_FLAG_BACKWARD 1 ///< seek backward
#define AVSEEK_FLAG_BYTE     2 ///< seeking based on position in bytes
#define AVSEEK_FLAG_ANY      4 ///< seek to any frame, even non-keyframes
#define AVSEEK_FLAG_FRAME    8 ///< seeking based on frame number




















int avformat_write_header(AVFormatContext *s, AVDictionary **options);

















int av_write_frame(AVFormatContext *s, AVPacket *pkt);



























int av_interleaved_write_frame(AVFormatContext *s, AVPacket *pkt);

#if FF_API_INTERLEAVE_PACKET




attribute_deprecated
int av_interleave_packet_per_dts(AVFormatContext *s, AVPacket *out,
                                 AVPacket *pkt, int flush);
#endif










int av_write_trailer(AVFormatContext *s);













AVOutputFormat *av_guess_format(const char *short_name,
                                const char *filename,
                                const char *mime_type);




enum AVCodecID av_guess_codec(AVOutputFormat *fmt, const char *short_name,
                            const char *filename, const char *mime_type,
                            enum AVMediaType type);
























void av_hex_dump(FILE *f, const uint8_t *buf, int size);













void av_hex_dump_log(void *avcl, int level, const uint8_t *buf, int size);









void av_pkt_dump2(FILE *f, AVPacket *pkt, int dump_payload, AVStream *st);













void av_pkt_dump_log2(void *avcl, int level, AVPacket *pkt, int dump_payload,
                      AVStream *st);








enum AVCodecID av_codec_get_id(const struct AVCodecTag * const *tags, unsigned int tag);








unsigned int av_codec_get_tag(const struct AVCodecTag * const *tags, enum AVCodecID id);

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


void av_dump_format(AVFormatContext *ic,
                    int index,
                    const char *url,
                    int is_output);













int av_get_frame_filename(char *buf, int buf_size,
                          const char *path, int number);







int av_filename_number_test(const char *filename);


















int av_sdp_create(AVFormatContext *ac[], int n_files, char *buf, int size);







int av_match_ext(const char *filename, const char *extensions);









int avformat_query_codec(AVOutputFormat *ofmt, enum AVCodecID codec_id, int std_compliance);
















const struct AVCodecTag *avformat_get_riff_video_tags(void);



const struct AVCodecTag *avformat_get_riff_audio_tags(void);








#endif 
