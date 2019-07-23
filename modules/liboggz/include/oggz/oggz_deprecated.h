































#ifndef __OGGZ_DEPRECATED_H__
#define __OGGZ_DEPRECATED_H__















#define OGGZ_ERR_USER_STOPPED OGGZ_ERR_STOP_OK









#define OGGZ_ERR_READ_STOP_OK  OGGZ_ERR_STOP_OK









#define OGGZ_ERR_READ_STOP_ERR OGGZ_ERR_STOP_ERR

















int oggz_set_metric_linear (OGGZ * oggz, long serialno,
			    ogg_int64_t granule_rate_numerator,
			    ogg_int64_t granule_rate_denominator);































ogg_packet *
oggz_comment_generate(OGGZ * oggz, long serialno,
		      OggzStreamContent packet_type,
		      int FLAC_final_metadata_block);

#endif 
