






































#ifndef _CC_DEVICE_LISTENER_H_
#define _CC_DEVICE_LISTENER_H_
#include "cc_constants.h"










void CC_DeviceListener_callForwardAllStateChanged(cc_lineid_t line, cc_boolean is_fwd, cc_boolean is_local, cc_string_t cfa_num);












void CC_DeviceListener_mwiStateUpdated(cc_lineid_t line, cc_boolean state,
		cc_message_type_t message_type,
		int new_count,
		int old_count,
		int high_priority_new_count,
		int high_priority_old_count);






void CC_DeviceListener_mwiLampUpdated(cc_lamp_state_t lamp_state);







void CC_DeviceListener_mncReached(cc_lineid_t line, cc_boolean state);









void CC_DeviceListener_displayNotify(int time, cc_boolean display_progress, char priority, cc_string_t prompt);









void CC_DeviceListener_labelNSpeedUpdated(cc_lineid_t line,
		int button_no,
		cc_string_t speed_dial_number,
		cc_string_t label);

#endif 
