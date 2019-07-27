


















#ifndef AVFORMAT_AVIO_H
#define AVFORMAT_AVIO_H







#include <stdint.h>

#include "libavutil/common.h"
#include "libavutil/dict.h"
#include "libavutil/log.h"

#include "libavformat/version.h"


#define AVIO_SEEKABLE_NORMAL 0x0001 /**< Seeking works like for a local file */












typedef struct {
    int (*callback)(void*);
    void *opaque;
} AVIOInterruptCB;













typedef struct {
#if !FF_API_OLD_AVIO
    











    AVClass *av_class;
#endif
    unsigned char *buffer;  
    int buffer_size;        
    unsigned char *buf_ptr; 
    unsigned char *buf_end; 



    void *opaque;           

    int (*read_packet)(void *opaque, uint8_t *buf, int buf_size);
    int (*write_packet)(void *opaque, uint8_t *buf, int buf_size);
    int64_t (*seek)(void *opaque, int64_t offset, int whence);
    int64_t pos;            
    int must_flush;         
    int eof_reached;        
    int write_flag;         
#if FF_API_OLD_AVIO
    attribute_deprecated int is_streamed;
#endif
    int max_packet_size;
    unsigned long checksum;
    unsigned char *checksum_ptr;
    unsigned long (*update_checksum)(unsigned long checksum, const uint8_t *buf, unsigned int size);
    int error;              
    


    int (*read_pause)(void *opaque, int pause);
    




    int64_t (*read_seek)(void *opaque, int stream_index,
                         int64_t timestamp, int flags);
    


    int seekable;
} AVIOContext;



#if FF_API_OLD_AVIO








typedef struct URLContext {
    const AVClass *av_class; 
    struct URLProtocol *prot;
    int flags;
    int is_streamed;  
    int max_packet_size;  
    void *priv_data;
    char *filename; 
    int is_connected;
    AVIOInterruptCB interrupt_callback;
} URLContext;

#define URL_PROTOCOL_FLAG_NESTED_SCHEME 1 /*< The protocol name can be the first part of a nested protocol scheme */
#define URL_PROTOCOL_FLAG_NETWORK       2 /*< The protocol uses network */





typedef struct URLProtocol {
    const char *name;
    int (*url_open)(URLContext *h, const char *url, int flags);
    int (*url_read)(URLContext *h, unsigned char *buf, int size);
    int (*url_write)(URLContext *h, const unsigned char *buf, int size);
    int64_t (*url_seek)(URLContext *h, int64_t pos, int whence);
    int (*url_close)(URLContext *h);
    struct URLProtocol *next;
    int (*url_read_pause)(URLContext *h, int pause);
    int64_t (*url_read_seek)(URLContext *h, int stream_index,
                             int64_t timestamp, int flags);
    int (*url_get_file_handle)(URLContext *h);
    int priv_data_size;
    const AVClass *priv_data_class;
    int flags;
    int (*url_check)(URLContext *h, int mask);
} URLProtocol;

typedef struct URLPollEntry {
    URLContext *handle;
    int events;
    int revents;
} URLPollEntry;


attribute_deprecated int url_poll(URLPollEntry *poll_table, int n, int timeout);







#define URL_RDONLY 1  /**< read-only */
#define URL_WRONLY 2  /**< write-only */
#define URL_RDWR   (URL_RDONLY|URL_WRONLY)  /**< read-write */
















#define URL_FLAG_NONBLOCK 8

typedef int URLInterruptCB(void);
extern URLInterruptCB *url_interrupt_cb;







attribute_deprecated int url_open_protocol (URLContext **puc, struct URLProtocol *up,
                                            const char *url, int flags);
attribute_deprecated int url_alloc(URLContext **h, const char *url, int flags);
attribute_deprecated int url_connect(URLContext *h);
attribute_deprecated int url_open(URLContext **h, const char *url, int flags);
attribute_deprecated int url_read(URLContext *h, unsigned char *buf, int size);
attribute_deprecated int url_read_complete(URLContext *h, unsigned char *buf, int size);
attribute_deprecated int url_write(URLContext *h, const unsigned char *buf, int size);
attribute_deprecated int64_t url_seek(URLContext *h, int64_t pos, int whence);
attribute_deprecated int url_close(URLContext *h);
attribute_deprecated int64_t url_filesize(URLContext *h);
attribute_deprecated int url_get_file_handle(URLContext *h);
attribute_deprecated int url_get_max_packet_size(URLContext *h);
attribute_deprecated void url_get_filename(URLContext *h, char *buf, int buf_size);
attribute_deprecated int av_url_read_pause(URLContext *h, int pause);
attribute_deprecated int64_t av_url_read_seek(URLContext *h, int stream_index,
                                              int64_t timestamp, int flags);
attribute_deprecated void url_set_interrupt_cb(int (*interrupt_cb)(void));





attribute_deprecated URLProtocol *av_protocol_next(URLProtocol *p);





attribute_deprecated int av_register_protocol2(URLProtocol *protocol, int size);





typedef attribute_deprecated AVIOContext ByteIOContext;

attribute_deprecated int init_put_byte(AVIOContext *s,
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence));
attribute_deprecated AVIOContext *av_alloc_put_byte(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence));







attribute_deprecated int          get_buffer(AVIOContext *s, unsigned char *buf, int size);
attribute_deprecated int          get_partial_buffer(AVIOContext *s, unsigned char *buf, int size);
attribute_deprecated int          get_byte(AVIOContext *s);
attribute_deprecated unsigned int get_le16(AVIOContext *s);
attribute_deprecated unsigned int get_le24(AVIOContext *s);
attribute_deprecated unsigned int get_le32(AVIOContext *s);
attribute_deprecated uint64_t     get_le64(AVIOContext *s);
attribute_deprecated unsigned int get_be16(AVIOContext *s);
attribute_deprecated unsigned int get_be24(AVIOContext *s);
attribute_deprecated unsigned int get_be32(AVIOContext *s);
attribute_deprecated uint64_t     get_be64(AVIOContext *s);

attribute_deprecated void         put_byte(AVIOContext *s, int b);
attribute_deprecated void         put_nbyte(AVIOContext *s, int b, int count);
attribute_deprecated void         put_buffer(AVIOContext *s, const unsigned char *buf, int size);
attribute_deprecated void         put_le64(AVIOContext *s, uint64_t val);
attribute_deprecated void         put_be64(AVIOContext *s, uint64_t val);
attribute_deprecated void         put_le32(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_be32(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_le24(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_be24(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_le16(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_be16(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_tag(AVIOContext *s, const char *tag);




attribute_deprecated int     av_url_read_fpause(AVIOContext *h,    int pause);
attribute_deprecated int64_t av_url_read_fseek (AVIOContext *h,    int stream_index,
                                                int64_t timestamp, int flags);







attribute_deprecated int url_fopen( AVIOContext **s, const char *url, int flags);
attribute_deprecated int url_fclose(AVIOContext *s);
attribute_deprecated int64_t url_fseek(AVIOContext *s, int64_t offset, int whence);
attribute_deprecated int url_fskip(AVIOContext *s, int64_t offset);
attribute_deprecated int64_t url_ftell(AVIOContext *s);
attribute_deprecated int64_t url_fsize(AVIOContext *s);
#define URL_EOF (-1)
attribute_deprecated int url_fgetc(AVIOContext *s);
attribute_deprecated int url_setbufsize(AVIOContext *s, int buf_size);
attribute_deprecated int url_fprintf(AVIOContext *s, const char *fmt, ...) av_printf_format(2, 3);
attribute_deprecated void put_flush_packet(AVIOContext *s);
attribute_deprecated int url_open_dyn_buf(AVIOContext **s);
attribute_deprecated int url_open_dyn_packet_buf(AVIOContext **s, int max_packet_size);
attribute_deprecated int url_close_dyn_buf(AVIOContext *s, uint8_t **pbuffer);
attribute_deprecated int url_fdopen(AVIOContext **s, URLContext *h);







attribute_deprecated int url_feof(AVIOContext *s);
attribute_deprecated int url_ferror(AVIOContext *s);

attribute_deprecated int udp_set_remote_url(URLContext *h, const char *uri);
attribute_deprecated int udp_get_local_port(URLContext *h);

attribute_deprecated void init_checksum(AVIOContext *s,
                   unsigned long (*update_checksum)(unsigned long c, const uint8_t *p, unsigned int len),
                   unsigned long checksum);
attribute_deprecated unsigned long get_checksum(AVIOContext *s);
attribute_deprecated void put_strz(AVIOContext *s, const char *buf);


attribute_deprecated char *url_fgets(AVIOContext *s, char *buf, int buf_size);



attribute_deprecated char *get_strz(AVIOContext *s, char *buf, int maxlen);



attribute_deprecated static inline int url_is_streamed(AVIOContext *s)
{
    return !s->seekable;
}
attribute_deprecated URLContext *url_fileno(AVIOContext *s);




attribute_deprecated int url_fget_max_packet_size(AVIOContext *s);

attribute_deprecated int url_open_buf(AVIOContext **s, uint8_t *buf, int buf_size, int flags);


attribute_deprecated int url_close_buf(AVIOContext *s);






attribute_deprecated int url_exist(const char *url);
#endif 













int avio_check(const char *url, int flags);

#if FF_API_OLD_INTERRUPT_CB








attribute_deprecated void avio_set_interrupt_cb(int (*interrupt_cb)(void));
#endif


















AVIOContext *avio_alloc_context(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence));

void avio_w8(AVIOContext *s, int b);
void avio_write(AVIOContext *s, const unsigned char *buf, int size);
void avio_wl64(AVIOContext *s, uint64_t val);
void avio_wb64(AVIOContext *s, uint64_t val);
void avio_wl32(AVIOContext *s, unsigned int val);
void avio_wb32(AVIOContext *s, unsigned int val);
void avio_wl24(AVIOContext *s, unsigned int val);
void avio_wb24(AVIOContext *s, unsigned int val);
void avio_wl16(AVIOContext *s, unsigned int val);
void avio_wb16(AVIOContext *s, unsigned int val);





int avio_put_str(AVIOContext *s, const char *str);





int avio_put_str16le(AVIOContext *s, const char *str);






#define AVSEEK_SIZE 0x10000







#define AVSEEK_FORCE 0x20000





int64_t avio_seek(AVIOContext *s, int64_t offset, int whence);





static av_always_inline int64_t avio_skip(AVIOContext *s, int64_t offset)
{
    return avio_seek(s, offset, SEEK_CUR);
}





static av_always_inline int64_t avio_tell(AVIOContext *s)
{
    return avio_seek(s, 0, SEEK_CUR);
}





int64_t avio_size(AVIOContext *s);


int avio_printf(AVIOContext *s, const char *fmt, ...) av_printf_format(2, 3);

void avio_flush(AVIOContext *s);






int avio_read(AVIOContext *s, unsigned char *buf, int size);








int          avio_r8  (AVIOContext *s);
unsigned int avio_rl16(AVIOContext *s);
unsigned int avio_rl24(AVIOContext *s);
unsigned int avio_rl32(AVIOContext *s);
uint64_t     avio_rl64(AVIOContext *s);
unsigned int avio_rb16(AVIOContext *s);
unsigned int avio_rb24(AVIOContext *s);
unsigned int avio_rb32(AVIOContext *s);
uint64_t     avio_rb64(AVIOContext *s);
















int avio_get_str(AVIOContext *pb, int maxlen, char *buf, int buflen);







int avio_get_str16le(AVIOContext *pb, int maxlen, char *buf, int buflen);
int avio_get_str16be(AVIOContext *pb, int maxlen, char *buf, int buflen);








#define AVIO_FLAG_READ  1                                      /**< read-only */
#define AVIO_FLAG_WRITE 2                                      /**< write-only */
#define AVIO_FLAG_READ_WRITE (AVIO_FLAG_READ|AVIO_FLAG_WRITE)  /**< read-write pseudo flag */
















#define AVIO_FLAG_NONBLOCK 8














int avio_open(AVIOContext **s, const char *url, int flags);


















int avio_open2(AVIOContext **s, const char *url, int flags,
               const AVIOInterruptCB *int_cb, AVDictionary **options);







int avio_close(AVIOContext *s);







int avio_open_dyn_buf(AVIOContext **s);










int avio_close_dyn_buf(AVIOContext *s, uint8_t **pbuffer);












const char *avio_enum_protocols(void **opaque, int output);






int     avio_pause(AVIOContext *h, int pause);


















int64_t avio_seek_time(AVIOContext *h, int stream_index,
                       int64_t timestamp, int flags);

#endif 
