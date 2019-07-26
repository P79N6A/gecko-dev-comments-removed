






































#ifndef _HTTPISH_H_
#define _HTTPISH_H_

#include "cpr_types.h"
#include "pmhdefs.h"
#include "httpish_protocol.h"
#include "pmhutils.h"
#include "util_ios_queue.h"

#define HTTPISH_MIN_STATUS_CODE 100
#define HTTPISH_HEADER_CACHE_SIZE  12
#define HTTPISH_HEADER_NAME_SIZE   256

typedef struct h_header
{
    struct h_header *next;
    char *header;
} httpish_header;

typedef struct {
    char *hdr_start;
    char *val_start;
} httpish_cache_t;

#define HTTPISH_MAX_BODY_PARTS 6
typedef struct {
    uint8_t  msgContentDisp;
    boolean  msgRequiredHandling;
    uint8_t  msgContentTypeValue;
    char    *msgContentType;
    char    *msgBody;
    uint32_t msgLength;
    char    *msgContentId;
    uint8_t  msgContentEnc;
} msgBody_t;

typedef struct _httpMsg
{
    struct _httpMsg *next;
    boolean         retain_flag;    
    char           *mesg_line;
    queuetype      *headers;
    msgBody_t       mesg_body[HTTPISH_MAX_BODY_PARTS];
    char           *raw_body;
    int32_t         content_length;
    uint8_t         num_body_parts;
    boolean         is_complete;
    boolean         headers_read;
    
    httpish_cache_t hdr_cache[HTTPISH_HEADER_CACHE_SIZE];
    
    char           *complete_message;
} httpishMsg_t;

typedef struct
{
    char *method;
    char *url;
    char *version;
} httpishReqLine_t;


typedef struct
{
    char    *reason_phrase;
    uint16_t status_code;
    char    *version;
} httpishRespLine_t;

typedef enum
{
    STATUS_SUCCESS = 0,
    STATUS_FAILURE,
    STATUS_UNKNOWN,
    HSTATUS_SUCCESS = STATUS_SUCCESS,
    HSTATUS_FAILURE = STATUS_FAILURE,
    HSTATUS_UNKNOWN = STATUS_UNKNOWN
} hStatus_t;

typedef enum
{
    codeClassInvalid = 0,
    codeClass1xx = 1,
    codeClass2xx = 2,
    codeClass3xx = 3,
    codeClass4xx = 4,
    codeClass5xx = 5,
    codeClass6xx = 6
} httpishStatusCodeClass_t;



PMH_EXTERN httpishMsg_t *httpish_msg_create(void);


PMH_EXTERN void httpish_msg_free(httpishMsg_t *);






PMH_EXTERN boolean httpish_msg_is_request(httpishMsg_t *, const char *, int);






PMH_EXTERN boolean httpish_msg_is_complete(httpishMsg_t *);







PMH_EXTERN httpishReqLine_t *httpish_msg_get_reqline(httpishMsg_t *);


PMH_EXTERN void httpish_msg_free_reqline(httpishReqLine_t *);







PMH_EXTERN httpishRespLine_t *httpish_msg_get_respline(httpishMsg_t *);


PMH_EXTERN void httpish_msg_free_respline(httpishRespLine_t *);







PMH_EXTERN hStatus_t httpish_msg_add_reqline(httpishMsg_t *,
                                             const char *method,
                                             const char *url,
                                             const char *version);








PMH_EXTERN hStatus_t httpish_msg_add_respline(httpishMsg_t *,
                                              const char *version,
                                              uint16_t status_code,
                                              const char *reason_phrase);








PMH_EXTERN hStatus_t httpish_msg_add_text_header(httpishMsg_t *msg,
                                                 const char *hname,
                                                 const char *hval);









PMH_EXTERN hStatus_t httpish_msg_add_int_header(httpishMsg_t *msg,
                                                const char *hname,
                                                int32_t hvalue);






PMH_EXTERN hStatus_t httpish_msg_remove_header(httpishMsg_t *msg,
                                               const char *hname);








PMH_EXTERN const char *httpish_msg_get_header_val(httpishMsg_t *,
                                                  const char *hname,
                                                  const char *c_hname);

PMH_EXTERN hStatus_t httpish_msg_get_header_vals(httpishMsg_t *,
                                                 const char *hname,
                                                 const char *c_hname,
                                                 uint16_t *nheaders,
                                                 char **header_vals);

PMH_EXTERN const char *httpish_msg_get_cached_header_val(httpishMsg_t *, int);











PMH_EXTERN char **httpish_msg_get_all_headers(httpishMsg_t *msg, uint32_t *num_headers);

PMH_EXTERN uint32_t httpish_msg_get_num_headers(httpishMsg_t *msg);


PMH_EXTERN uint16_t httpish_msg_get_num_particular_headers(httpishMsg_t *msg,
                                                           const char *hname,
                                                           const char *c_hname,
                                                           char *header_val[],
                                                           uint16_t max_headers);





PMH_EXTERN int32_t httpish_msg_get_content_length(httpishMsg_t *);






PMH_EXTERN hStatus_t httpish_msg_add_body(httpishMsg_t *msg,
                                          char *body,
                                          uint32_t nbytes,
                                          const char *content_type,
                                          uint8_t msg_disposition,
                                          boolean required,
                                          char *content_id);

PMH_EXTERN boolean httpish_msg_header_present(httpishMsg_t *,
                                              const char *hname);









PMH_EXTERN char *httpish_msg_write_to_buf(httpishMsg_t * msg, uint32_t *nbytes);


PMH_EXTERN char *httpish_msg_write_to_string(httpishMsg_t * msg);









PMH_EXTERN hStatus_t httpish_msg_write(httpishMsg_t *msg,
                                       char *buf,
                                       uint32_t *nbytes);













PMH_EXTERN hStatus_t httpish_msg_process_network_msg(httpishMsg_t *msg,
                                                     char *nmsg,
                                                     uint32_t *bytes_read);





PMH_EXTERN httpishStatusCodeClass_t httpish_msg_get_code_class(uint16_t statusCode);


#endif 
