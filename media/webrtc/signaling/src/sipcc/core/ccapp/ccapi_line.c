



#include "cpr_stdlib.h"
#include "string_lib.h"
#include "cc_call_feature.h"
#include "ccapi_snapshot.h"
#include "ccapi_line.h"

extern cc_line_info_t lineInfo[MAX_CONFIG_LINES+1];





cc_lineinfo_ref_t CCAPI_Line_getLineInfo(cc_uint32_t lineID)
{
  cc_line_info_t *line_info = NULL;
  int i;

  for (i=1;i<=MAX_CONFIG_LINES;i++) {
      if ( (cc_uint32_t)lineInfo[i].button == lineID ) {
          line_info = (cc_line_info_t*)cpr_malloc(sizeof(cc_line_info_t));

          if (line_info) {
              *line_info = lineInfo[i];
              line_info->ref_count = 1;
              line_info->name = strlib_copy(lineInfo[i].name);
              line_info->dn = strlib_copy(lineInfo[i].dn);
              line_info->cfwd_dest = strlib_copy(lineInfo[i].cfwd_dest);
              line_info->externalNumber =
                  strlib_copy(lineInfo[i].externalNumber);
          }
      }
  }
  return line_info;
}






cc_call_handle_t CCAPI_Line_CreateCall(cc_lineid_t line)
{
 
  return CC_createCall(line);
}






void CCAPI_Line_retainLineInfo(cc_lineinfo_ref_t ref){
    cc_line_info_t *line_info = ref;

    line_info->ref_count++;
}





void CCAPI_Line_releaseLineInfo(cc_lineinfo_ref_t ref){
    cc_line_info_t *line_info = ref;

    if (line_info) {
	line_info->ref_count--;
	if ( line_info->ref_count == 0) {
            strlib_free(line_info->name);
            strlib_free(line_info->dn);
            strlib_free(line_info->cfwd_dest);
            strlib_free(line_info->externalNumber);
            cpr_free(line_info);
	}
    }
}


