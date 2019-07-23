































#ifndef __OGGZ_COMMENT_H__
#define __OGGZ_COMMENT_H__



















































#include <oggz/oggz.h>




typedef struct {
  
  char * name;

  
  char * value;
} OggzComment;

#ifdef __cplusplus
extern "C" {
#endif










const char *
oggz_comment_get_vendor (OGGZ * oggz, long serialno);













int
oggz_comment_set_vendor (OGGZ * oggz, long serialno,
			 const char * vendor_string);









const OggzComment *
oggz_comment_first (OGGZ * oggz, long serialno);











const OggzComment *
oggz_comment_next (OGGZ * oggz, long serialno, const OggzComment * comment);












const OggzComment *
oggz_comment_first_byname (OGGZ * oggz, long serialno, char * name);













const OggzComment *
oggz_comment_next_byname (OGGZ * oggz, long serialno,
                          const OggzComment * comment);










int
oggz_comment_add (OGGZ * oggz, long serialno, OggzComment * comment);











int
oggz_comment_add_byname (OGGZ * oggz, long serialno,
                         const char * name, const char * value);













int
oggz_comment_remove (OGGZ * oggz, long serialno, OggzComment * comment);












int
oggz_comment_remove_byname (OGGZ * oggz, long serialno, char * name);























ogg_packet *
oggz_comments_generate(OGGZ * oggz, long serialno,
                       int FLAC_final_metadata_block);
  









int
oggz_comments_copy (OGGZ * src, long src_serialno,
                    OGGZ * dest, long dest_serialno);






void oggz_packet_destroy (ogg_packet *packet);

#ifdef __cplusplus
}
#endif

#endif
