



#include "timecard.h"
#include "cc_call_feature.h"
#include "CCProvider.h"
#include "sessionConstants.h"
#include "sessionTypes.h"
#include "lsm.h"
#include "phone_debug.h"
#include "text_strings.h"
#include "ccapi.h"
#include "ccapp_task.h"
#include "sessionHash.h"
#include "cpr_rand.h"

extern cpr_status_e ccappTaskPostMsg(unsigned int msgId, void * data, uint16_t len, int appId);






cc_call_handle_t cc_createCallHandle(cc_lineid_t line, cc_callid_t call_id)
{
        return (CREATE_CALL_HANDLE(line, call_id));
}







void cc_getLineIdAndCallId (cc_lineid_t *line_id, cc_callid_t *call_id)
{
    
    if ((*line_id) == 0 || (*line_id) == CC_ALL_LINES) {
        




        (*line_id) = lsm_get_available_line(FALSE);
    }

    if ((*call_id) == 0) {
        (*call_id) = cc_get_new_call_id();
    }
}




cc_return_t cc_invokeFeature(cc_call_handle_t call_handle, group_cc_feature_t featureId, cc_sdp_direction_t video_pref, string_t data) {
	session_feature_t callFeature;
    callFeature.session_id = (SESSIONTYPE_CALLCONTROL << CC_SID_TYPE_SHIFT) + call_handle;
    callFeature.featureID = featureId;
    callFeature.featData.ccData.state = video_pref;

    CCAPP_DEBUG(DEB_F_PREFIX"cc_invokeFeature:sid=%d, line=%d, cid=%d, fid=%d, video_pref=%s data=%s",
                        DEB_F_PREFIX_ARGS("cc_call_feature", "cc_invokeFeature"),
                        callFeature.session_id,
                        GET_LINE_ID(call_handle),
                        GET_CALL_ID(call_handle),
                        featureId, SDP_DIRECTION_PRINT(video_pref),
                        ((featureId == CC_FEATURE_KEYPRESS) ? "...": data));

    switch (featureId) {
    case CC_FEATURE_KEYPRESS:
    case CC_FEATURE_DIALSTR:
    case CC_FEATURE_SPEEDDIAL:
    case CC_FEATURE_BLIND_XFER_WITH_DIALSTRING:
    case CC_FEATURE_END_CALL:
    case CC_FEATURE_B2BCONF:
    case CC_FEATURE_CONF:
    case CC_FEATURE_XFER:
    case CC_FEATURE_HOLD:
        callFeature.featData.ccData.info = strlib_malloc(data, strlen(data));
        callFeature.featData.ccData.info1 = NULL;
        break;

    default:
        callFeature.featData.ccData.info = NULL;
        callFeature.featData.ccData.info1 = NULL;
        break;
    }

    if (ccappTaskPostMsg(CCAPP_INVOKE_FEATURE, &callFeature, sizeof(session_feature_t), CCAPP_CCPROVIER) == CPR_FAILURE) {
            CCAPP_DEBUG(DEB_F_PREFIX"ccappTaskSendMsg failed",
            		DEB_F_PREFIX_ARGS("cc_call_feature", "cc_invokeFeature"));
            return CC_FAILURE;
	}
	return CC_SUCCESS;
}













cc_call_handle_t CC_createCall(cc_lineid_t line) {
	static const char fname[] = "CC_CreateCall";
	
	cc_call_handle_t call_handle = CC_EMPTY_CALL_HANDLE;
	cc_lineid_t lineid = line;
	cc_callid_t callid = CC_NO_CALL_ID;


	
	cc_getLineIdAndCallId(&lineid, &callid);
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, callid, lineid, fname));

	if (lineid == CC_NO_LINE) {
        lsm_ui_display_notify_str_index(STR_INDEX_ERROR_PASS_LIMIT);
        return call_handle;
    }

    call_handle = cc_createCallHandle(lineid, callid);

	return call_handle;
}






 
cc_return_t CC_CallFeature_originateCall(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref) {
	static const char fname[] = "CC_CallFeature_originateCall:";
	
	
	CCAPP_DEBUG(DEB_F_PREFIX"CC_CallFeature_originateCall:cHandle=%d",
	                        DEB_F_PREFIX_ARGS("cc_call_feature", fname),
	                        call_handle);
    return cc_invokeFeature(call_handle, CC_FEATURE_OFFHOOK, video_pref, NULL);
}






cc_return_t CC_CallFeature_terminateCall(cc_call_handle_t call_handle) {
	static const char fname[] = "CC_CallFeature_TerminateCall";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

    return cc_invokeFeature(call_handle, CC_FEATURE_ONHOOK, CC_SDP_MAX_QOS_DIRECTIONS, NULL);
}






cc_return_t CC_CallFeature_answerCall(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref) {
	static const char fname[] = "CC_CallFeature_AnswerCall";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

    return cc_invokeFeature(call_handle, CC_FEATURE_ANSWER, video_pref, NULL);
}







cc_return_t CC_CallFeature_sendDigit(cc_call_handle_t call_handle, cc_digit_t cc_digit) {
	static const char fname[] = "CC_CallFeature_SendDigit";
    char digit;
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));
	
	digit = cc_digit;
    return cc_invokeFeature(call_handle, CC_FEATURE_KEYPRESS, CC_SDP_MAX_QOS_DIRECTIONS, (string_t)&digit);
}






cc_return_t CC_CallFeature_backSpace(cc_call_handle_t call_handle) {
	static const char fname[] = "CC_CallFeature_BackSpace";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

    return cc_invokeFeature(call_handle, CC_FEATURE_BKSPACE, CC_SDP_MAX_QOS_DIRECTIONS, NULL);
}







cc_return_t CC_CallFeature_dial(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref, const string_t numbers) {
	static const char fname[] = "CC_CallFeature_Dial";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

    if (cpr_strcasecmp(numbers, "DIAL") == 0) {
	    return cc_invokeFeature(call_handle, CC_FEATURE_DIAL, video_pref, numbers);
    }

	return cc_invokeFeature(call_handle, CC_FEATURE_DIALSTR, video_pref, numbers);
}








cc_return_t CC_CallFeature_speedDial(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref, const string_t speed_dial_number) {
	static const char fname[] = "CC_CallFeature_SpeedDial";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

	return cc_invokeFeature(call_handle, CC_FEATURE_SPEEDDIAL, video_pref, speed_dial_number);
}







cc_return_t CC_CallFeature_blfCallPickup(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref, const string_t speed_dial_number) {
	static const char fname[] = "CC_CallFeature_BLFCallPickup";
	cc_return_t ret = CC_SUCCESS;
    string_t blf_sd = strlib_malloc(CISCO_BLFPICKUP_STRING, sizeof(CISCO_BLFPICKUP_STRING));
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

    blf_sd = strlib_append(blf_sd, "-");
    blf_sd = strlib_append(blf_sd, speed_dial_number);

	ret = cc_invokeFeature(call_handle, CC_FEATURE_SPEEDDIAL, video_pref, blf_sd);
	
	strlib_free(blf_sd);
	return ret;
}








cc_return_t CC_CallFeature_redial(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref) {
	static const char fname[] = "CC_CallFeature_Redial";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

	return cc_invokeFeature(call_handle, CC_FEATURE_REDIAL, video_pref, NULL);
}







cc_return_t CC_CallFeature_updateCallMediaCapability(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref) {
	static const char fname[] = "CC_CallFeature_updateCallMediaCapability";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

	return cc_invokeFeature(call_handle, CC_FEATURE_UPD_SESSION_MEDIA_CAP, video_pref, NULL);
}






cc_return_t CC_CallFeature_callForwardAll(cc_call_handle_t call_handle) {
	static const char fname[] = "CC_CallFeature_CallForwardAll";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

	return cc_invokeFeature(call_handle, CC_FEATURE_CFWD_ALL, CC_SDP_MAX_QOS_DIRECTIONS, NULL);
}






cc_return_t CC_CallFeature_resume(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref) {
	static const char fname[] = "CC_CallFeature_Resume";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

	return cc_invokeFeature(call_handle, CC_FEATURE_RESUME, video_pref, NULL);
}






cc_return_t CC_CallFeature_endConsultativeCall(cc_call_handle_t call_handle) {
	static const char fname[] = "CC_CallFeature_EndConsultativeCall";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

	return cc_invokeFeature(call_handle, CC_FEATURE_END_CALL, CC_SDP_MAX_QOS_DIRECTIONS, "ACTIVECALLS");
}





























cc_return_t CC_CallFeature_conference(cc_call_handle_t call_handle,
		boolean is_local,
		cc_call_handle_t parent_call_handle, cc_sdp_direction_t video_pref) {
	static const char fname[] = "CC_CallFeature_Conference";
	char call_handle_str[10];
	cc_return_t ret = CC_SUCCESS;
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));
	if (parent_call_handle == CC_EMPTY_CALL_HANDLE) {
		if (is_local == FALSE) {
		    return cc_invokeFeature(call_handle, CC_FEATURE_B2BCONF, video_pref, "");
		} else {
		    return cc_invokeFeature(call_handle, CC_FEATURE_CONF, video_pref, "");
		}
	} else {
		cc_call_handle_t parent = (SESSIONTYPE_CALLCONTROL << CC_SID_TYPE_SHIFT) + parent_call_handle;
        string_t parent_call_handle_str;
		snprintf(call_handle_str, sizeof(call_handle_str), "%d", parent);
		parent_call_handle_str = strlib_malloc(call_handle_str, strlen(call_handle_str));

		if (is_local == FALSE) {
		    ret = cc_invokeFeature(call_handle, CC_FEATURE_B2BCONF, video_pref, parent_call_handle_str);
		} else {
		    ret = cc_invokeFeature(call_handle, CC_FEATURE_CONF, video_pref, parent_call_handle_str);
		}
		strlib_free(parent_call_handle_str);
		return ret;
	}
}











cc_return_t CC_CallFeature_transfer(cc_call_handle_t call_handle, cc_call_handle_t parent_call_handle, cc_sdp_direction_t video_pref) {
	static const char fname[] = "CC_CallFeature_transfer";
	char call_handle_str[10];
	cc_return_t ret = CC_SUCCESS;
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));
	if (parent_call_handle == CC_EMPTY_CALL_HANDLE) {
		return cc_invokeFeature(call_handle, CC_FEATURE_XFER, video_pref, "");
	} else {
		cc_call_handle_t parent = (SESSIONTYPE_CALLCONTROL << CC_SID_TYPE_SHIFT) + parent_call_handle;
        string_t parent_call_handle_str;
		snprintf(call_handle_str, sizeof(call_handle_str), "%d", parent);
		parent_call_handle_str = strlib_malloc(call_handle_str, strlen(call_handle_str));

		ret = cc_invokeFeature(call_handle, CC_FEATURE_XFER, video_pref, parent_call_handle_str);
		strlib_free(parent_call_handle_str);
		return ret;
	}
}










cc_return_t CC_CallFeature_holdCall(cc_call_handle_t call_handle, cc_hold_reason_t reason) {
	static const char fname[] = "CC_CallFeature_HoldCall";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));
	switch (reason) {
	case CC_HOLD_REASON_XFER:
		return cc_invokeFeature(call_handle, CC_FEATURE_HOLD, CC_SDP_MAX_QOS_DIRECTIONS, "TRANSFER");
	case CC_HOLD_REASON_CONF:
		return cc_invokeFeature(call_handle, CC_FEATURE_HOLD, CC_SDP_MAX_QOS_DIRECTIONS, "CONFERENCE");
	case CC_HOLD_REASON_SWAP:
		return cc_invokeFeature(call_handle, CC_FEATURE_HOLD, CC_SDP_MAX_QOS_DIRECTIONS, "SWAP");
	default:
		break;
	}

	return cc_invokeFeature(call_handle, CC_FEATURE_HOLD, CC_SDP_MAX_QOS_DIRECTIONS, "");
}












cc_return_t CC_CallFeature_b2bJoin(cc_call_handle_t call_handle) {
	static const char fname[] = "CC_CallFeature_b2bJoin";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

	return cc_invokeFeature(call_handle, CC_FEATURE_B2B_JOIN, CC_SDP_MAX_QOS_DIRECTIONS, NULL);
}







cc_return_t CC_CallFeature_directTransfer(cc_call_handle_t call_handle,
        cc_call_handle_t target_call_handle) {
    static const char fname[] = "CC_CallFeature_directTransfer";
    CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
                GET_LINE_ID(call_handle), fname));
    if (target_call_handle == CC_EMPTY_CALL_HANDLE) {
        CCAPP_DEBUG(DEB_L_C_F_PREFIX"target call handle is empty.", DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
                    GET_LINE_ID(call_handle), fname));
        return CC_FAILURE;
    }
    return CC_CallFeature_transfer(call_handle, target_call_handle, CC_SDP_MAX_QOS_DIRECTIONS);
}






cc_return_t CC_CallFeature_joinAcrossLine(cc_call_handle_t call_handle, cc_call_handle_t target_call_handle) {
    static const char fname[] = "CC_CallFeature_joinAcrossLine";
    CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
                GET_LINE_ID(call_handle), fname));
    if (target_call_handle == CC_EMPTY_CALL_HANDLE) {
        CCAPP_DEBUG(DEB_L_C_F_PREFIX"target call handle is empty.", DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
                    GET_LINE_ID(call_handle), fname));
        return CC_FAILURE;
    }
    return CC_CallFeature_conference(call_handle, TRUE, target_call_handle, CC_SDP_MAX_QOS_DIRECTIONS);
}






cc_return_t CC_CallFeature_select(cc_call_handle_t call_handle) {
	static const char fname[] = "CC_CallFeature_select";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

	return cc_invokeFeature(call_handle, CC_FEATURE_SELECT, CC_SDP_MAX_QOS_DIRECTIONS, NULL);
}







cc_return_t CC_CallFeature_cancelXfrerCnf(cc_call_handle_t call_handle) {
	static const char fname[] = "CC_CallFeature_cancelXfrerCnf";
	CCAPP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(SIP_CC_PROV, GET_CALL_ID(call_handle),
			GET_LINE_ID(call_handle), fname));

	return cc_invokeFeature(call_handle, CC_FEATURE_CANCEL, CC_SDP_MAX_QOS_DIRECTIONS, NULL);
}

void CC_CallFeature_mute(boolean mute) {
}

void CC_CallFeature_speaker(boolean mute) {
}

cc_call_handle_t CC_CallFeature_getConnectedCall() {
    return ccappGetConnectedCall();
}
