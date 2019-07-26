



#include <cpr_types.h>
#include "ccapi.h"
#include "phone_debug.h"
#include "cc_debug.h"
#include "CCProvider.h"
#include "ccapi_snapshot.h"







cc_media_cap_table_t g_media_table = {
      1,
      {
        {CC_AUDIO_1,SDP_MEDIA_AUDIO,TRUE,TRUE,SDP_DIRECTION_SENDRECV},
        {CC_VIDEO_1,SDP_MEDIA_VIDEO,FALSE,TRUE,SDP_DIRECTION_SENDRECV},
        {CC_DATACHANNEL_1,SDP_MEDIA_APPLICATION,FALSE,TRUE,SDP_DIRECTION_SENDRECV},
      }
};

static boolean g_nativeVidSupported = FALSE;
static boolean g_vidCapEnabled = FALSE;
static boolean g_natve_txCap_enabled = FALSE;





void escalateDeescalate() {
    g_media_table.id++;
    if ( ccapp_get_state() != CC_INSERVICE ) {
        VCM_DEBUG(MED_F_PREFIX"Ignoring video cap update\n", "escalateDeescalate");
        return;
    }

    
    cc_int_feature(CC_SRC_UI, CC_SRC_GSM, CC_NO_CALL_ID,
         CC_NO_LINE, CC_FEATURE_UPD_MEDIA_CAP, NULL);
}

cc_boolean cc_media_isTxCapEnabled() {
   return g_natve_txCap_enabled;
}

cc_boolean cc_media_isVideoCapEnabled() {
    if ( g_nativeVidSupported ) {
       return g_vidCapEnabled;
    }
    return FALSE;
}







static void updateVidCapTbl(){

    if ( g_vidCapEnabled  ) {
        if ( g_media_table.cap[CC_VIDEO_1].enabled == FALSE ) {
            
            if ( g_nativeVidSupported ) {
                
                g_media_table.cap[CC_VIDEO_1].enabled = TRUE;
                g_media_table.cap[CC_VIDEO_1].support_direction =
                   g_natve_txCap_enabled?SDP_DIRECTION_SENDRECV:SDP_DIRECTION_RECVONLY;
                if ( g_natve_txCap_enabled == FALSE ) {

                }
                escalateDeescalate();
            } else {

            }
        }
    }  else {
        
        DEF_DEBUG(MED_F_PREFIX"video capability disabled \n", "updateVidCapTbl");

        if ( g_media_table.cap[CC_VIDEO_1].enabled ) {
            g_media_table.cap[CC_VIDEO_1].enabled = FALSE;
            escalateDeescalate();
        }
    }
}






void cc_media_update_native_video_support(boolean val) {
    DEF_DEBUG(MED_F_PREFIX"Setting native video support val=%d\n", "cc_media_update_native_video_support", val);
    g_nativeVidSupported = val;
    updateVidCapTbl();
}





void cc_media_update_video_cap(boolean val) {
    DEF_DEBUG(MED_F_PREFIX"Setting video cap val=%d\n", "cc_media_update_video_cap", val);
    g_vidCapEnabled = val;
    updateVidCapTbl();
    if ( g_nativeVidSupported ) {
        ccsnap_gen_deviceEvent(CCAPI_DEVICE_EV_VIDEO_CAP_ADMIN_CONFIG_CHANGED, CC_DEVICE_ID);
    }
}






void cc_media_update_native_video_txcap(boolean enable) {

    VCM_DEBUG(MED_F_PREFIX"Setting txcap val=%d\n", "cc_media_update_video_txcap", enable);

    if ( g_natve_txCap_enabled == enable ) {
        
        return;
    }

    g_natve_txCap_enabled = enable;
    ccsnap_gen_deviceEvent(CCAPI_DEVICE_EV_CAMERA_ADMIN_CONFIG_CHANGED, CC_DEVICE_ID);

    if ( g_nativeVidSupported && g_vidCapEnabled ) {
    
        if ( g_natve_txCap_enabled ) {

        } else if (g_media_table.cap[CC_VIDEO_1].enabled) {

        }

        g_media_table.cap[CC_VIDEO_1].support_direction  =
            g_natve_txCap_enabled?SDP_DIRECTION_SENDRECV:SDP_DIRECTION_RECVONLY;

        escalateDeescalate();
    }
}

static cc_boolean vidAutoTxPref=FALSE;
void cc_media_setVideoAutoTxPref(cc_boolean txPref){
	vidAutoTxPref = txPref;
}

cc_boolean cc_media_getVideoAutoTxPref(){
	return vidAutoTxPref;
}


