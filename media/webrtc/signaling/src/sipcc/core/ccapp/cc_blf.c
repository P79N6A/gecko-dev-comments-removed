






































#include "cc_blf.h"
#include "pres_sub_not_handler.h"





int CC_BLF_init() {
	pres_sub_handler_initialized();
	return CC_SUCCESS;
}










void CC_BLF_subscribe(int request_id,
		int duration,
		const char *watcher,
        const char *presentity,
        int app_id,
        cc_blf_feature_mask_t feature_mask) {
	pres_get_state(request_id, duration, watcher, presentity, app_id, feature_mask);
}





void CC_BLF_unsubscribe(int request_id) {
	pres_terminate_req(request_id);
}





void CC_BLF_unsubscribe_All() {
	pres_terminate_req_all();
}

