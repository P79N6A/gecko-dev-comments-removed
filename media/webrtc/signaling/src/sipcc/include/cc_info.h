






































#ifndef _CC_INFO_H_
#define _CC_INFO_H_

#include "cc_constants.h"





#define FAST_PIC_INFO_PACK  ""
#define FAST_PIC_CTYPE  "application/media_control+xml"
#define FAST_PIC_MSG_BODY "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n\
        <media_control>\n\
          <vc_primitive>\n\
            <to_encoder>\n\
              <picture_fast_update/>\n\
            </to_encoder>\n\
          </vc_primitive>\n\
        </media_control>"










void CC_Info_sendInfo(cc_call_handle_t call_handle,
		cc_string_t info_package,
		cc_string_t info_type,
		cc_string_t info_body);

#endif 
