



#ifndef _CC_BLF_H_
#define _CC_BLF_H_
#include "cc_constants.h"





int CC_BLF_init();










void CC_BLF_subscribe(int request_id,
		int duration,
		const char *watcher,
        const char *presentity,
        int app_id,
        cc_blf_feature_mask_t feature_mask);





void CC_BLF_unsubscribe(int request_id);





void CC_BLF_unsubscribe_All();

#endif 

