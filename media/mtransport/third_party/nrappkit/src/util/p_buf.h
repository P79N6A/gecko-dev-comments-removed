






































#ifndef _p_buf_h
#define _p_buf_h

typedef struct nr_p_buf_ {
     UCHAR *data;     
     UINT4 size;      
     UINT4 r_offset;  

     UINT4 length;    

     STAILQ_ENTRY(nr_p_buf_) entry;
} nr_p_buf;

typedef STAILQ_HEAD(nr_p_buf_head_,nr_p_buf_) nr_p_buf_head;


typedef struct nr_p_buf_ctx_ {
     int buf_size;

     nr_p_buf_head free_list;
} nr_p_buf_ctx;

int nr_p_buf_ctx_create(int size,nr_p_buf_ctx **ctxp);
int nr_p_buf_ctx_destroy(nr_p_buf_ctx **ctxp);
int nr_p_buf_alloc(nr_p_buf_ctx *ctx,nr_p_buf **bufp);
int nr_p_buf_free(nr_p_buf_ctx *ctx,nr_p_buf *buf);
int nr_p_buf_free_chain(nr_p_buf_ctx *ctx,nr_p_buf_head *chain);
int nr_p_buf_write_to_chain(nr_p_buf_ctx *ctx,
                                       nr_p_buf_head *chain,
                                       UCHAR *data,UINT4 len);

#endif

