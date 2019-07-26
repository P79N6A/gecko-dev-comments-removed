







































static char *RCSSTRING __UNUSED__ ="Id: p_buf.c,v 1.3 2004/01/03 22:13:53 ekr Exp $";

#include <string.h>
#include <stddef.h>
#include "nr_common.h"
#include "p_buf.h"


static int nr_p_buf_destroy_chain(nr_p_buf_head *head);
static int nr_p_buf_destroy(nr_p_buf *buf);

int nr_p_buf_ctx_create(size,ctxp)
  int size;
  nr_p_buf_ctx **ctxp;
  {
    int _status;
    nr_p_buf_ctx *ctx=0;

    if(!(ctx=(nr_p_buf_ctx *)RCALLOC(sizeof(nr_p_buf_ctx))))
      ABORT(R_NO_MEMORY);

    ctx->buf_size=size;
    STAILQ_INIT(&ctx->free_list);

    *ctxp=ctx;
    _status=0;
  abort:
    if(_status){
      nr_p_buf_ctx_destroy(&ctx);
    }
    return(_status);
  }

int nr_p_buf_ctx_destroy(ctxp)
  nr_p_buf_ctx **ctxp;
  {
    nr_p_buf_ctx *ctx;

    if(!ctxp || !*ctxp)
      return(0);

    ctx=*ctxp;

    nr_p_buf_destroy_chain(&ctx->free_list);

    RFREE(ctx);
    *ctxp=0;

    return(0);
  }

int nr_p_buf_alloc(ctx,bufp)
  nr_p_buf_ctx *ctx;
  nr_p_buf **bufp;
  {
    int _status;
    nr_p_buf *buf=0;

    if(!STAILQ_EMPTY(&ctx->free_list)){
      buf=STAILQ_FIRST(&ctx->free_list);
      STAILQ_REMOVE_HEAD(&ctx->free_list,entry);
      goto ok;
    }
    else {
      if(!(buf=(nr_p_buf *)RCALLOC(sizeof(nr_p_buf))))
        ABORT(R_NO_MEMORY);
      if(!(buf->data=(UCHAR *)RMALLOC(ctx->buf_size)))
        ABORT(R_NO_MEMORY);
      buf->size=ctx->buf_size;
    }

  ok:
     buf->r_offset=0;
     buf->length=0;

     *bufp=buf;
    _status=0;
  abort:
     if(_status){
       nr_p_buf_destroy(buf);
     }
    return(_status);
  }

int nr_p_buf_free(ctx,buf)
  nr_p_buf_ctx *ctx;
  nr_p_buf *buf;
  {
    STAILQ_INSERT_TAIL(&ctx->free_list,buf,entry);

    return(0);
  }

int nr_p_buf_free_chain(ctx,head)
  nr_p_buf_ctx *ctx;
  nr_p_buf_head *head;
  {
    nr_p_buf *n1,*n2;

    n1=STAILQ_FIRST(head);
    while(n1){
      n2=STAILQ_NEXT(n1,entry);

      nr_p_buf_free(ctx,n1);

      n1=n2;
    }

    return(0);
  }


int nr_p_buf_write_to_chain(ctx,chain,data,len)
  nr_p_buf_ctx *ctx;
  nr_p_buf_head *chain;
  UCHAR *data;
  UINT4 len;
  {
    int r,_status;
    nr_p_buf *buf;

    buf=STAILQ_LAST(chain,nr_p_buf_,entry);
    while(len){
      int towrite;

      if(!buf){
        if(r=nr_p_buf_alloc(ctx,&buf))
          ABORT(r);
        STAILQ_INSERT_TAIL(chain,buf,entry);
      }

      towrite=MIN(len,(buf->size-(buf->length+buf->r_offset)));

      memcpy(buf->data+buf->length+buf->r_offset,data,towrite);
      len-=towrite;
      data+=towrite;
      buf->length+=towrite;

      r_log(LOG_COMMON,LOG_DEBUG,"Wrote %d bytes to buffer %d",towrite,buf);
      buf=0;
    }

    _status=0;
  abort:
    return(_status);
  }

static int nr_p_buf_destroy_chain(head)
  nr_p_buf_head *head;
  {
    nr_p_buf *n1,*n2;

    n1=STAILQ_FIRST(head);
    while(n1){
      n2=STAILQ_NEXT(n1,entry);

      nr_p_buf_destroy(n1);

      n1=n2;
    }

    return(0);
  }

static int nr_p_buf_destroy(buf)
  nr_p_buf *buf;
  {
    if(!buf)
      return(0);

    RFREE(buf->data);
    RFREE(buf);

    return(0);
  }


