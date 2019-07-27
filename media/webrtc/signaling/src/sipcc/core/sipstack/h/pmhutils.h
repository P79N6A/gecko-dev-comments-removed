



#ifndef _PMH_UTILS_H_
#define _PMH_UTILS_H_

#include "cpr_types.h"
#include "pmhdefs.h"








typedef struct
{
    char   *buff;
    char   *loc;  
    int32_t nbytes;
    int32_t bytes_read;
    boolean eof;
    boolean error;
} pmhRstream_t;








typedef struct
{
    char *buff;
    int32_t nbytes;
    int32_t total_bytes;
    boolean growable;
} pmhWstream_t;

typedef struct
{
    uint16_t num_tokens;
    char **tokens;
} pmhTokens_t;








PMH_EXTERN pmhRstream_t *pmhutils_rstream_create(char *buf, uint32_t nbytes);







PMH_EXTERN void pmhutils_rstream_delete(pmhRstream_t *rs, boolean freebuf);






PMH_EXTERN char pmhutils_rstream_read_byte(pmhRstream_t *);









PMH_EXTERN char *pmhutils_rstream_read_bytes(pmhRstream_t *rs, int32_t nbytes);









PMH_EXTERN char *pmhutils_rstream_read_line(pmhRstream_t *);


PMH_EXTERN pmhWstream_t *pmhutils_wstream_create(void);


PMH_EXTERN pmhWstream_t *pmhutils_wstream_create_with_buf(char *buf,
                                                          uint32_t nbytes);





PMH_EXTERN boolean pmhutils_wstream_grow(pmhWstream_t *);







PMH_EXTERN void pmhutils_wstream_delete(pmhWstream_t *ws, boolean freebuf);











PMH_EXTERN boolean pmhutils_wstream_write_line(pmhWstream_t *ws, char *line);





PMH_EXTERN boolean pmhutils_wstream_write_byte(pmhWstream_t *, char);







PMH_EXTERN boolean pmhutils_wstream_write_bytes(pmhWstream_t *ws, char *buf,
                                                uint32_t len);






PMH_EXTERN char *pmhutils_wstream_getbuf(pmhWstream_t *, uint32_t *nbytes);

PMH_EXTERN uint32_t pmhutils_wstream_get_length(pmhWstream_t *ws);







PMH_EXTERN pmhTokens_t *pmh_tokenize(const char *str, const char *tokens);




PMH_EXTERN void pmh_tokens_free(pmhTokens_t *tokens);

#endif 
