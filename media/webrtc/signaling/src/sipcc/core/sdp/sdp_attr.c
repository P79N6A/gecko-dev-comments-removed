






































#include <errno.h>
#include <limits.h>

#include "plstr.h"
#include "sdp_os_defs.h"
#include "sdp.h"
#include "sdp_private.h"
#include "sdp_base64.h"
#include "mozilla/Assertions.h"











sdp_result_e sdp_parse_attribute (sdp_t *sdp_p, u16 level, const char *ptr)
{
    int           i;
    u8            xcpar_flag = FALSE;
    sdp_result_e  result;
    sdp_mca_t    *mca_p=NULL;
    sdp_attr_t   *attr_p;
    sdp_attr_t   *next_attr_p;
    sdp_attr_t   *prev_attr_p = NULL;
    char          tmp[SDP_MAX_STRING_LEN];

    
    if (level != SDP_SESSION_LEVEL) {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_FAILURE);
        }
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), ": \t", &result);
    if (ptr == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s No attribute type specified, parse failed.",
                      sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    if (ptr[0] == ':') {
        
        ptr++;
    }
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s No attribute type specified, parse failed.",
                      sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    attr_p = (sdp_attr_t *)SDP_MALLOC(sizeof(sdp_attr_t));
    if (attr_p == NULL) {
        sdp_p->conf_p->num_no_resource++;
        return (SDP_NO_RESOURCE);
    }
    attr_p->type = SDP_ATTR_INVALID;
    attr_p->next_p = NULL;
    for (i=0; i < SDP_MAX_ATTR_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_attr[i].name, sdp_attr[i].strlen) == 0) {
            attr_p->type = (sdp_attr_e)i;
            break;
        }
    }
    if (attr_p->type == SDP_ATTR_INVALID) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Unrecognized attribute (%s) ", 
                     sdp_p->debug_str, tmp);
        }
        sdp_free_attr(attr_p);
        return (SDP_SUCCESS);
    }

    

    if ((attr_p->type == SDP_ATTR_X_CPAR) ||
	(attr_p->type == SDP_ATTR_CPAR)) {
        xcpar_flag = TRUE;
    }

    
    result = sdp_attr[attr_p->type].parse_func(sdp_p, attr_p, ptr);
    if (result != SDP_SUCCESS) {
        sdp_free_attr(attr_p);
        


        return (SDP_SUCCESS);
    }

    


    if (xcpar_flag == TRUE) {
        return (result);
    }

    
    if (level == SDP_SESSION_LEVEL) {
        for (next_attr_p = sdp_p->sess_attrs_p; next_attr_p != NULL;
             prev_attr_p = next_attr_p, 
                 next_attr_p = next_attr_p->next_p) {
            ; 
        }
        if (prev_attr_p == NULL) {
            sdp_p->sess_attrs_p = attr_p;
        } else {
            prev_attr_p->next_p = attr_p;
        }
    } else {  
        for (next_attr_p = mca_p->media_attrs_p; next_attr_p != NULL;
             prev_attr_p = next_attr_p, 
                 next_attr_p = next_attr_p->next_p) {
            ; 
        }
        if (prev_attr_p == NULL) {
            mca_p->media_attrs_p = attr_p;
        } else {
            prev_attr_p->next_p = attr_p;
        }
    }

    return (result);
}


sdp_result_e sdp_build_attribute (sdp_t *sdp_p, u16 level, char **ptr, u16 len)
{
    sdp_attr_t   *attr_p;
    sdp_mca_t    *mca_p=NULL;
    sdp_result_e  result;
    char          *endbuf_p;

    if (level == SDP_SESSION_LEVEL) {
        attr_p = sdp_p->sess_attrs_p;
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_FAILURE);
        }
        attr_p = mca_p->media_attrs_p;
    }
    
    sdp_p->cur_cap_num = 1;

    
    endbuf_p = *ptr + len;

    

    while (attr_p != NULL) {
        if (attr_p->type >= SDP_MAX_ATTR_TYPES) {
            if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
                SDP_WARN("%s Invalid attribute type to build (%u)", 
                         sdp_p->debug_str, attr_p->type);
            }
        } else {
            result = sdp_attr[attr_p->type].build_func(sdp_p, attr_p, 
                                                       ptr, (u16)(endbuf_p - *ptr));
            
            
            
            if (endbuf_p - *ptr <= 0)
                return (SDP_POTENTIAL_SDP_OVERFLOW);

            if (result == SDP_SUCCESS) {
                if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
                    SDP_PRINT("%s Built a=%s attribute line", sdp_p->debug_str,
                              sdp_get_attr_name(attr_p->type));
                }
            }
        }
        attr_p = attr_p->next_p;
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_simple_string (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                           const char *ptr)
{
    sdp_result_e  result;

    ptr = sdp_getnextstrtok(ptr, attr_p->attr.string_val, sizeof(attr_p->attr.string_val), " \t", &result);

    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No string token found for %s attribute",
                     sdp_p->debug_str, sdp_get_attr_name(attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Parsed a=%s, %s", sdp_p->debug_str,
                      sdp_get_attr_name(attr_p->type), 
                      attr_p->attr.string_val);
        }
        return (SDP_SUCCESS);
    }
}

sdp_result_e sdp_build_attr_simple_string (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                           char **ptr, u16 len)
{
    char *endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr),0), "a=%s:%s\r\n", sdp_attr[attr_p->type].name,
                     attr_p->attr.string_val);

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_simple_u32 (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                        const char *ptr)
{
    sdp_result_e  result;

    attr_p->attr.u32_val = sdp_getnextnumtok(ptr, &ptr, " \t", &result);

    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Numeric token for %s attribute not found",
                     sdp_p->debug_str, sdp_get_attr_name(attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Parsed a=%s, %lu", sdp_p->debug_str,
                      sdp_get_attr_name(attr_p->type), attr_p->attr.u32_val);
        }
        return (SDP_SUCCESS);
    }
}

sdp_result_e sdp_build_attr_simple_u32 (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                        char **ptr, u16 len)
{
    char *endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr),0), "a=%s:%u\r\n", sdp_attr[attr_p->type].name,
                     attr_p->attr.u32_val);

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_simple_bool (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                         const char *ptr)
{
    sdp_result_e  result;

    if (sdp_getnextnumtok(ptr, &ptr, " \t", &result) == 0) {
        attr_p->attr.boolean_val = FALSE;
    } else {
        attr_p->attr.boolean_val= TRUE;
    }

    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Boolean token for %s attribute not found",
                     sdp_p->debug_str, sdp_get_attr_name(attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            if (attr_p->attr.boolean_val) {
                SDP_PRINT("%s Parsed a=%s, boolean is TRUE", sdp_p->debug_str,
                          sdp_get_attr_name(attr_p->type));
            } else {
                SDP_PRINT("%s Parsed a=%s, boolean is FALSE", sdp_p->debug_str,
                          sdp_get_attr_name(attr_p->type));
            }
        }
        return (SDP_SUCCESS);
    }
}

sdp_result_e sdp_build_attr_simple_bool (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                         char **ptr, u16 len)
{
    char         *endbuf_p;

    
    endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr),0), "a=%s:", sdp_attr[attr_p->type].name);
    len = endbuf_p - *ptr;

    if (attr_p->attr.boolean_val == TRUE) {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr),0), "1\r\n");
    } else {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr),0), "0\r\n");
    }
    return (SDP_SUCCESS);
}
















sdp_result_e sdp_parse_attr_maxprate (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                      const char *ptr)
{
    sdp_result_e  result;

    ptr = sdp_getnextstrtok(ptr, attr_p->attr.string_val, sizeof(attr_p->attr.string_val), " \t", &result);

    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No string token found for %s attribute",
                     sdp_p->debug_str, sdp_get_attr_name(attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        if (!sdp_validate_maxprate(attr_p->attr.string_val)) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                SDP_ERROR("%s is not a valid maxprate value.", 
                          attr_p->attr.string_val);
            }
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }

        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Parsed a=%s, %s", sdp_p->debug_str,
                      sdp_get_attr_name(attr_p->type), 
                      attr_p->attr.string_val);
        }
        return (SDP_SUCCESS);
    }
}







static void sdp_attr_fmtp_no_value(sdp_t *sdp, char *param_name)
{
  if (sdp->debug_flag[SDP_DEBUG_WARNINGS]) {
    SDP_WARN("%s Warning: No %s value specified for fmtp attribute",
      sdp->debug_str, param_name);
  }
  sdp->conf_p->num_invalid_param++;
}








static void sdp_attr_fmtp_invalid_value(sdp_t *sdp, char *param_name, char* param_value)
{
  if (sdp->debug_flag[SDP_DEBUG_WARNINGS]) {
    SDP_WARN("%s Warning: Invalid %s: %s specified for fmtp attribute",
      sdp->debug_str, param_name, param_value);
  }
  sdp->conf_p->num_invalid_param++;
}
















sdp_result_e sdp_parse_attr_fmtp (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                  const char *ptr)
{
    u16           i;
    u32           mapword;
    u32           bmap;
    u8            low_val;
    u8            high_val;
    const char    *ptr2;
    const char    *fmtp_ptr;
    sdp_result_e  result1 = SDP_SUCCESS;
    sdp_result_e  result2 = SDP_SUCCESS;
    tinybool      done = FALSE;
    tinybool      codec_info_found = FALSE;
    sdp_fmtp_t   *fmtp_p;
    char          tmp[SDP_MAX_STRING_LEN];
    char          *src_ptr;
    char          *temp_ptr = NULL;
    tinybool flag=FALSE;
    char         *tok=NULL;
    char         *temp=NULL;
    u16          custom_x=0;
    u16          custom_y=0;
    u16          custom_mpi=0;
    u16          par_height=0;
    u16          par_width=0;
    u16          cpcf=0;
    u16          iter=0;
    
    ulong        l_val = 0;
    char*        strtok_state;
    unsigned long strtoul_result;
    char*        strtoul_end;

    
    attr_p->attr.fmtp.payload_num = (u16)sdp_getnextnumtok(ptr, &ptr, 
                                                      " \t", &result1);
    if (result1 != SDP_SUCCESS) {
        sdp_attr_fmtp_no_value(sdp_p, "payload type");
        return SDP_INVALID_PARAMETER;
    }
    fmtp_p = &(attr_p->attr.fmtp);
    fmtp_p->fmtp_format = SDP_FMTP_UNKNOWN_TYPE;
    fmtp_p->parameter_add = TRUE;
    fmtp_p->flag = 0;

    





    fmtp_p->packetization_mode = 0;
    fmtp_p->level_asymmetry_allowed = SDP_DEFAULT_LEVEL_ASYMMETRY_ALLOWED_VALUE;

    
    



    temp_ptr = cpr_strdup(ptr);
    if (temp_ptr == NULL) {
        return (SDP_FAILURE);
    }
    fmtp_ptr = src_ptr = temp_ptr;
    while (flag == FALSE) {
        if (*src_ptr == '\n') {
            flag = TRUE;
            break;
        }
        if (*src_ptr == '/') {
            *src_ptr =';' ;
        }
        src_ptr++;
    }
    
    

    while (!done) {
      fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "= \t", &result1);
      if (result1 == SDP_SUCCESS) {
        if (cpr_strncasecmp(tmp, sdp_fmtp_codec_param[1].name,
	                sdp_fmtp_codec_param[1].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr  = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "annexb");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
            tok = tmp;
	    tok++;
	    if (cpr_strncasecmp(tok,sdp_fmtp_codec_param_val[0].name,
	                    sdp_fmtp_codec_param_val[0].strlen) == 0) {
	        fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
		fmtp_p->annexb_required = TRUE;
	        fmtp_p->annexb = TRUE;
	    } else if (cpr_strncasecmp(tok,sdp_fmtp_codec_param_val[1].name,
	                           sdp_fmtp_codec_param_val[1].strlen) == 0) {
	        fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
		fmtp_p->annexb_required = TRUE;
	        fmtp_p->annexb = FALSE;
	    } else {
                sdp_attr_fmtp_invalid_value(sdp_p, "annexb", tok);
		SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    } 
	    codec_info_found = TRUE;
	
	} else if (cpr_strncasecmp(tmp, sdp_fmtp_codec_param[0].name,
	                       sdp_fmtp_codec_param[0].strlen) == 0) {
			
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "annexa");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;
	    if (cpr_strncasecmp(tok,sdp_fmtp_codec_param_val[0].name,
	                    sdp_fmtp_codec_param_val[0].strlen) == 0) {
	        fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
	        fmtp_p->annexa = TRUE;
		fmtp_p->annexa_required = TRUE;
	    } else if (cpr_strncasecmp(tok,sdp_fmtp_codec_param_val[1].name,
	                           sdp_fmtp_codec_param_val[1].strlen) == 0) {
	        fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
	        fmtp_p->annexa = FALSE;
		fmtp_p->annexa_required = TRUE;
	    } else {
                sdp_attr_fmtp_invalid_value(sdp_p, "annexa", tok);
		SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    } 
	    codec_info_found = TRUE;
	    
	} else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[2].name,
                               sdp_fmtp_codec_param[2].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "bitrate");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
            }	  
            tok = tmp;
            tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > UINT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "bitrate", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }

            fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->bitrate = (u32) strtoul_result;
            codec_info_found = TRUE;
            
         } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[41].name,
                               sdp_fmtp_codec_param[41].strlen) == 0) {
            fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
            if (result1 != SDP_SUCCESS) {
                fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
                if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "mode");
                    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
            }
            tok = tmp;
            tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end || strtoul_result > UINT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "mode", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }

            fmtp_p->fmtp_format = SDP_FMTP_MODE;
            fmtp_p->mode = (u32) strtoul_result;
            codec_info_found = TRUE;

	} else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[3].name,
                               sdp_fmtp_codec_param[3].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
           fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
            if (result1 != SDP_SUCCESS) {
                sdp_attr_fmtp_no_value(sdp_p, "qcif");
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }
        }
        tok = tmp;
        tok++;

        errno = 0;
        strtoul_result = strtoul(tok, &strtoul_end, 10);

        if (errno || tok == strtoul_end ||
            strtoul_result < SDP_MIN_CIF_VALUE || strtoul_result > SDP_MAX_CIF_VALUE) {
            sdp_attr_fmtp_invalid_value(sdp_p, "qcif", tok);
            SDP_FREE(temp_ptr);
            return SDP_INVALID_PARAMETER;
	    }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
	    fmtp_p->qcif = (u16) strtoul_result;
	    codec_info_found = TRUE;
	} else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[4].name,
                               sdp_fmtp_codec_param[4].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
            fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
            if (result1 != SDP_SUCCESS) {
                sdp_attr_fmtp_no_value(sdp_p, "cif");
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }
        }
        tok = tmp;
        tok++;

        errno = 0;
        strtoul_result = strtoul(tok, &strtoul_end, 10);

        if (errno || tok == strtoul_end ||
            strtoul_result < SDP_MIN_CIF_VALUE || strtoul_result > SDP_MAX_CIF_VALUE) {
            sdp_attr_fmtp_invalid_value(sdp_p, "cif", tok);
            SDP_FREE(temp_ptr);
            return SDP_INVALID_PARAMETER;
	}

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
	    fmtp_p->cif = (u16) strtoul_result;
	    codec_info_found = TRUE;
	} else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[5].name,
                               sdp_fmtp_codec_param[5].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
            fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "maxbr");
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }
        }
        tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

	    if (errno || tok == strtoul_end ||
                strtoul_result == 0 || strtoul_result > USHRT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "maxbr", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
	    fmtp_p->maxbr = (u16) strtoul_result;
	    codec_info_found = TRUE;
	} else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[6].name,
                               sdp_fmtp_codec_param[6].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "sqcif");
                    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
	    }
	    tok = tmp;
	    tok++;

        errno = 0;
        strtoul_result = strtoul(tok, &strtoul_end, 10);

        if (errno || tok == strtoul_end ||
            strtoul_result < SDP_MIN_CIF_VALUE || strtoul_result > SDP_MAX_CIF_VALUE) {
            sdp_attr_fmtp_invalid_value(sdp_p, "sqcif", tok);
            SDP_FREE(temp_ptr);
            return SDP_INVALID_PARAMETER;
        }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
	    fmtp_p->sqcif = (u16) strtoul_result;
	    codec_info_found = TRUE;
	} else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[7].name,
                               sdp_fmtp_codec_param[7].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "cif4");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

        errno = 0;
        strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end ||
                strtoul_result < SDP_MIN_CIF_VALUE || strtoul_result > SDP_MAX_CIF_VALUE) {
                sdp_attr_fmtp_invalid_value(sdp_p, "cif4", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
	    fmtp_p->cif4 = (u16) strtoul_result;
	    codec_info_found = TRUE;
	} else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[8].name,
                               sdp_fmtp_codec_param[8].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "cif16");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

        errno = 0;
        strtoul_result = strtoul(tok, &strtoul_end, 10);

        if (errno || tok == strtoul_end ||
            strtoul_result < SDP_MIN_CIF_VALUE || strtoul_result > SDP_MAX_CIF_VALUE) {
            sdp_attr_fmtp_invalid_value(sdp_p, "cif16", tok);
            SDP_FREE(temp_ptr);
            return SDP_INVALID_PARAMETER;
	    }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
	    fmtp_p->cif16 = (u16) strtoul_result;
	    codec_info_found = TRUE;
        } else  if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[9].name,
                               sdp_fmtp_codec_param[9].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "custom");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++; temp=PL_strtok_r(tok, ",", &strtok_state);
	    iter++;
        if (temp) {
            iter=1;
            while (temp != NULL) {
                errno = 0;
                strtoul_result = strtoul(temp, &strtoul_end, 10);

                if (errno || temp == strtoul_end || strtoul_result > USHRT_MAX){
                    custom_x = custom_y = custom_mpi = 0;
                    break;
                }

                if (iter == 1)
                    custom_x = (u16) strtoul_result;
                if (iter == 2)
                    custom_y = (u16) strtoul_result;
                if (iter == 3)
                    custom_mpi = (u16) strtoul_result;

                temp=PL_strtok_r(NULL, ",", &strtok_state);
                iter++;
            }
        }

        
	    if (!custom_x || !custom_y || !custom_mpi) {
                sdp_attr_fmtp_invalid_value(sdp_p, "x/y/MPI", temp);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
	    fmtp_p->custom_x = custom_x;
            fmtp_p->custom_y = custom_y;
            fmtp_p->custom_mpi = custom_mpi;
	    codec_info_found = TRUE;
        } else  if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[10].name,
                               sdp_fmtp_codec_param[10].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "par");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++; temp=PL_strtok_r(tok, ":", &strtok_state);
        if (temp) {
            iter=1;
            
            while (temp != NULL) {
                errno = 0;
                strtoul_result = strtoul(temp, &strtoul_end, 10);

                if (errno || temp == strtoul_end || strtoul_result > USHRT_MAX) {
                    par_width = par_height = 0;
                    break;
                }

                if (iter == 1)
                    par_width = (u16) strtoul_result;
                else
                    par_height = (u16) strtoul_result;

                temp=PL_strtok_r(NULL, ",", &strtok_state);
                iter++;
            }
        }
	    if (!par_width || !par_height) {
                sdp_attr_fmtp_invalid_value(sdp_p, "par_width or par_height", temp);
		SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
	    fmtp_p->par_width = par_width;
            fmtp_p->par_height = par_height;
	    codec_info_found = TRUE;
        } else  if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[11].name,
                               sdp_fmtp_codec_param[11].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "cpcf");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++; temp=PL_strtok_r(tok, ".", &strtok_state);
        if ( temp != NULL  ) {
            errno = 0;
            strtoul_result = strtoul(temp, &strtoul_end, 10);

            if (errno || temp == strtoul_end || strtoul_result > USHRT_MAX) {
                cpcf = 0;
            } else {
                cpcf = (u16) strtoul_result;
            }
        }

	    if (!cpcf) {
                sdp_attr_fmtp_invalid_value(sdp_p, "cpcf", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->cpcf = cpcf;
	    codec_info_found = TRUE;
        } else  if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[12].name,
                               sdp_fmtp_codec_param[12].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "bpp");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

        errno = 0;
        strtoul_result = strtoul(tok, &strtoul_end, 10);

        if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > USHRT_MAX) {
            sdp_attr_fmtp_invalid_value(sdp_p, "bpp", tok); 
            SDP_FREE(temp_ptr);
            return SDP_INVALID_PARAMETER;
	    }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
        fmtp_p->bpp = (u16) strtoul_result;
	    codec_info_found = TRUE;
        } else  if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[13].name,
                               sdp_fmtp_codec_param[13].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "hrd");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > USHRT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "hrd", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->hrd = (u16) strtoul_result;
	    codec_info_found = TRUE;
	} else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[14].name,
                               sdp_fmtp_codec_param[14].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "profile");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end ||
                strtoul_result > SDP_MAX_PROFILE_VALUE) {
                sdp_attr_fmtp_invalid_value(sdp_p, "profile", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->profile = (short) strtoul_result;
	    codec_info_found = TRUE;
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[15].name,
                               sdp_fmtp_codec_param[15].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "level");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

        errno = 0;
        strtoul_result = strtoul(tok, &strtoul_end, 10);

	if (errno || tok == strtoul_end ||
            strtoul_result > SDP_MAX_LEVEL_VALUE) {
            sdp_attr_fmtp_invalid_value(sdp_p, "level", tok);
            SDP_FREE(temp_ptr);
            return SDP_INVALID_PARAMETER;
	}

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->level = (short) strtoul_result;
	    codec_info_found = TRUE; 
        } if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[16].name,
                               sdp_fmtp_codec_param[16].strlen) == 0) {
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->is_interlace = TRUE;
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[17].name,
                               sdp_fmtp_codec_param[17].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "profile_level_id");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    }
	    tok = tmp;
	    tok++;
  	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            sstrncpy(fmtp_p->profile_level_id , tok, sizeof(fmtp_p->profile_level_id));
	    codec_info_found = TRUE;
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[18].name,
                               sdp_fmtp_codec_param[18].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "parameter_sets");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    }
	    tok = tmp;
	    tok++;
  	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            sstrncpy(fmtp_p->parameter_sets , tok, sizeof(fmtp_p->parameter_sets));
	    codec_info_found = TRUE;
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[19].name,
                               sdp_fmtp_codec_param[19].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "packetization_mode");
                    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    }
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

    	    if (errno || tok == strtoul_end || strtoul_result > 2) {
                sdp_attr_fmtp_invalid_value(sdp_p, "packetization_mode", tok);
                sdp_p->conf_p->num_invalid_param++;
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
    	    } 

            fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->packetization_mode = (int16) strtoul_result;
            codec_info_found = TRUE;
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[20].name,
                               sdp_fmtp_codec_param[20].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "interleaving_depth");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

	    if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > USHRT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "interleaving_depth", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->interleaving_depth = (u16) strtoul_result;
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[21].name,
                               sdp_fmtp_codec_param[21].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "deint_buf");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;
            if (sdp_checkrange(sdp_p, tok, &l_val) == TRUE) {
		fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
		fmtp_p->deint_buf_req = (u32) l_val;
                fmtp_p->flag |= SDP_DEINT_BUF_REQ_FLAG;
		codec_info_found = TRUE; 
            } else {
                sdp_attr_fmtp_invalid_value(sdp_p, "deint_buf_req", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[22].name,
                               sdp_fmtp_codec_param[22].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "max_don_diff");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

	    if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > UINT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "max_don_diff", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->max_don_diff = (u32) strtoul_result;
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[23].name,
                               sdp_fmtp_codec_param[23].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "init_buf_time");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;
            if (sdp_checkrange(sdp_p, tok, &l_val) == TRUE) {
		fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
		fmtp_p->init_buf_time = (u32) l_val;
                fmtp_p->flag |= SDP_INIT_BUF_TIME_FLAG;
		codec_info_found = TRUE; 
            } else {
                sdp_attr_fmtp_invalid_value(sdp_p, "init_buf_time", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[24].name,
                               sdp_fmtp_codec_param[24].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "max_mbps");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

	    if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > UINT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "max_mbps", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }

	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
        fmtp_p->max_mbps = (u32) strtoul_result;
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[25].name,
                               sdp_fmtp_codec_param[25].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "max_fs");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > UINT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "max_fs", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->max_fs = (u32) strtoul_result;
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[26].name,
                               sdp_fmtp_codec_param[26].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "max_cbp");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

	    if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > UINT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "max_cpb", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->max_cpb = (u32) strtoul_result;
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[27].name,
                               sdp_fmtp_codec_param[27].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "max_dpb");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

	    if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > UINT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "max_dpb", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->max_dpb = (u32) strtoul_result;
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[28].name,
                               sdp_fmtp_codec_param[28].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "max_br");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

	    if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > UINT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "max_br", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
	    }
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->max_br = (u32) strtoul_result;
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[29].name,
                               sdp_fmtp_codec_param[29].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "redundant_pic_cap");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

        errno = 0;
        strtoul_result = strtoul(tok, &strtoul_end, 10);

        if (!errno && tok != strtoul_end && strtoul_result == 1) {
            fmtp_p->redundant_pic_cap = TRUE;
        } else {
            fmtp_p->redundant_pic_cap = FALSE;
        }

	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[30].name,
                               sdp_fmtp_codec_param[30].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "deint_buf_cap");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;
            if (sdp_checkrange(sdp_p, tok, &l_val) == TRUE) {
		fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
		fmtp_p->deint_buf_cap = (u32) l_val;
                fmtp_p->flag |= SDP_DEINT_BUF_CAP_FLAG;
		codec_info_found = TRUE; 
            } else {
                sdp_attr_fmtp_invalid_value(sdp_p, "deint_buf_cap", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }
        }  else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[31].name,
                               sdp_fmtp_codec_param[31].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "max_rcmd_nalu_size");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;
            if (sdp_checkrange(sdp_p, tok, &l_val) == TRUE) {
		fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
		fmtp_p->max_rcmd_nalu_size = (u32) l_val;
                fmtp_p->flag |= SDP_MAX_RCMD_NALU_SIZE_FLAG;
		codec_info_found = TRUE; 
            } else {
                sdp_attr_fmtp_invalid_value(sdp_p, "max_rcmd_nalu_size", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[32].name,
                               sdp_fmtp_codec_param[32].strlen) == 0) {
	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
	    if (result1 != SDP_SUCCESS) {
	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "parameter_add");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
		}
	    } 
	    tok = tmp;
	    tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end || strtoul_result > 1) {
                sdp_attr_fmtp_invalid_value(sdp_p, "parameter_add", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }

		fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;

		if (strtoul_result == 1) {
		    fmtp_p->parameter_add = TRUE;
		} else {
		    fmtp_p->parameter_add = FALSE;
		}

	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[33].name,
                               sdp_fmtp_codec_param[33].strlen) == 0) {
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->annex_d = TRUE;                
	    codec_info_found = TRUE;
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[34].name,
                               sdp_fmtp_codec_param[34].strlen) == 0) {
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->annex_f = TRUE;                
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[35].name,
                               sdp_fmtp_codec_param[35].strlen) == 0) {
            fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->annex_i = TRUE;                
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[36].name,
                               sdp_fmtp_codec_param[36].strlen) == 0) {
            fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->annex_j = TRUE;                
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[37].name,
                               sdp_fmtp_codec_param[36].strlen) == 0) {
	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->annex_t = TRUE;                
	    codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[38].name,
                             sdp_fmtp_codec_param[38].strlen) == 0) {
                fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
                if (result1 != SDP_SUCCESS) {
                    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
                    if (result1 != SDP_SUCCESS) {
                        sdp_attr_fmtp_no_value(sdp_p, "annex_k");
			SDP_FREE(temp_ptr);
                        return SDP_INVALID_PARAMETER;
                    }
                } 
                tok = tmp;
                tok++;

                errno = 0;
                strtoul_result = strtoul(tok, &strtoul_end, 10);

                if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > USHRT_MAX) {
                    sdp_attr_fmtp_invalid_value(sdp_p, "annex_k", tok);
                    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
                fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
                fmtp_p->annex_k_val = (u16) strtoul_result;
                codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[39].name,
                               sdp_fmtp_codec_param[39].strlen) == 0) {
            fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
            if (result1 != SDP_SUCCESS) {
                fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
                if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "annex_n");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
            } 
            tok = tmp;
            tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > USHRT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "annex_n", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }
            fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->annex_n_val = (u16) strtoul_result;
            codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[40].name,
                               sdp_fmtp_codec_param[40].strlen) == 0) {
            fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
            if (result1 != SDP_SUCCESS) {
                fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
                if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "annex_p");
		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
            } 
            fmtp_p->annex_p_val_picture_resize = 0;
            fmtp_p->annex_p_val_warp = 0;
            tok = tmp;
            tok++; temp=PL_strtok_r(tok, ",", &strtok_state);
            if (temp) {
                iter=1;
                while (temp != NULL) {
                    errno = 0;
                    strtoul_result = strtoul(temp, &strtoul_end, 10);

                    if (errno || temp == strtoul_end || strtoul_result > USHRT_MAX) {
                        break;
                    }

                    if (iter == 1)
                        fmtp_p->annex_p_val_picture_resize = (u16) strtoul_result;
                    else if (iter == 2)
                        fmtp_p->annex_p_val_warp = (u16) strtoul_result;

                    temp=PL_strtok_r(NULL, ",", &strtok_state);
                    iter++;
                }
            }

            fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            codec_info_found = TRUE; 
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[42].name,
                               sdp_fmtp_codec_param[42].strlen) == 0) {
            fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
            if (result1 != SDP_SUCCESS) {
                fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
                if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "level_asymmetry_allowed");
                    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
            } 
            tok = tmp;
            tok++;

            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end || strtoul_result > SDP_MAX_LEVEL_ASYMMETRY_ALLOWED_VALUE) {
                sdp_attr_fmtp_invalid_value(sdp_p, "level_asymmetry_allowed", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }
            fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
            fmtp_p->level_asymmetry_allowed = (int) strtoul_result;
            codec_info_found = TRUE;
        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[43].name,
                                   sdp_fmtp_codec_param[43].strlen) == 0) {
    	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
    	    if (result1 != SDP_SUCCESS) {
                    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
    	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "maxaveragebitrate");
    		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
            }
            tok = tmp;
    	    tok++;
            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end || strtoul_result == 0 || strtoul_result > UINT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "maxaveragebitrate", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
    	    }
    	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
    	    fmtp_p->maxaveragebitrate = (u32) strtoul_result;
    	    codec_info_found = TRUE;

        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[44].name,
                                   sdp_fmtp_codec_param[44].strlen) == 0) {
    	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
    	    if (result1 != SDP_SUCCESS) {
                    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
    	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "usedtx");
    		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
            }
            tok = tmp;
    	    tok++;
            errno = 0;
            strtoul_result = strtoul(tok, &strtoul_end, 10);

    	    if (errno || tok == strtoul_end || strtoul_result > 1) {
                sdp_attr_fmtp_invalid_value(sdp_p, "usedtx", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
    	    }
    	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
    	    fmtp_p->usedtx = (u16) strtoul_result;
    	    codec_info_found = TRUE;

        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[45].name,
                                   sdp_fmtp_codec_param[45].strlen) == 0) {
    	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
    	    if (result1 != SDP_SUCCESS) {
                    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
    	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "stereo");
    		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
            }
            tok = tmp;
    	    tok++;
            errno = 0;

            strtoul_result = strtoul(tok, &strtoul_end, 10);

    	    if (errno || tok == strtoul_end || strtoul_result > 1) {
                sdp_attr_fmtp_invalid_value(sdp_p, "stereo", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
    	    }
    	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
    	    fmtp_p->stereo = (u16) strtoul_result;
    	    codec_info_found = TRUE;

        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[46].name,
                                   sdp_fmtp_codec_param[46].strlen) == 0) {
    	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
    	    if (result1 != SDP_SUCCESS) {
                    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
    	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "useinbandfec");
    		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
            }
            tok = tmp;
    	    tok++;
            errno = 0;

            strtoul_result = strtoul(tok, &strtoul_end, 10);

    	    if (errno || tok == strtoul_end || strtoul_result > 1) {
                sdp_attr_fmtp_invalid_value(sdp_p, "useinbandfec", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
    	    }
    	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
    	    fmtp_p->useinbandfec = (u16) strtoul_result;
    	    codec_info_found = TRUE;

        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[47].name,
                                       sdp_fmtp_codec_param[47].strlen) == 0) {
        	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
        	    if (result1 != SDP_SUCCESS) {
        	        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
        	        if (result1 != SDP_SUCCESS) {
                            sdp_attr_fmtp_no_value(sdp_p, "maxcodedaudiobandwidth");
                            sdp_p->conf_p->num_invalid_param++;
        		    SDP_FREE(temp_ptr);
                            return SDP_INVALID_PARAMETER;
        		}
        	    }
        	    tok = tmp;
        	    tok++;
          	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
                    sstrncpy(fmtp_p->maxcodedaudiobandwidth , tok, sizeof(fmtp_p->maxcodedaudiobandwidth));
        	    codec_info_found = TRUE;

        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[48].name,
                                   sdp_fmtp_codec_param[48].strlen) == 0) {
    	    fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
    	    if (result1 != SDP_SUCCESS) {
                fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
    	        if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "cbr");
    		    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
            }
            tok = tmp;
    	    tok++;
            errno = 0;

            strtoul_result = strtoul(tok, &strtoul_end, 10);

    	    if (errno || tok == strtoul_end || strtoul_result > 1) {
                sdp_attr_fmtp_invalid_value(sdp_p, "cbr", tok);
    		SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
    	    }
    	    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;
    	    fmtp_p->cbr = (u16) strtoul_result;
    	    codec_info_found = TRUE;

        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[49].name,
                        sdp_fmtp_codec_param[49].strlen) == 0) {
            fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
            if (result1 != SDP_SUCCESS) {
                fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
                if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "streams");
                    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                }
            }
            tok = tmp;
            tok++;
            errno = 0;

            strtoul_result = strtoul(tok, &strtoul_end, 10);

            if (errno || tok == strtoul_end || strtoul_result > INT_MAX) {
                sdp_attr_fmtp_invalid_value(sdp_p, "streams", tok);
                SDP_FREE(temp_ptr);
                return SDP_INVALID_PARAMETER;
            }

            fmtp_p->fmtp_format = SDP_FMTP_DATACHANNEL;
            fmtp_p->streams = (int) strtoul_result;
            codec_info_found = TRUE;

        } else if (cpr_strncasecmp(tmp,sdp_fmtp_codec_param[50].name,
                sdp_fmtp_codec_param[50].strlen) == 0) {
            fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), "; \t", &result1);
            if (result1 != SDP_SUCCESS) {
                fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), " \t", &result1);
                if (result1 != SDP_SUCCESS) {
                    sdp_attr_fmtp_no_value(sdp_p, "protocol");
                    SDP_FREE(temp_ptr);
                    return SDP_INVALID_PARAMETER;
                 }
             }
             tok = tmp;
             tok++;
             fmtp_p->fmtp_format = SDP_FMTP_DATACHANNEL;
             sstrncpy(fmtp_p->protocol , tok, sizeof(fmtp_p->protocol));
			 codec_info_found = TRUE;

        } else if (fmtp_ptr != NULL && *fmtp_ptr == '\n') {
            temp=PL_strtok_r(tmp, ";", &strtok_state);
            if (temp) {
                if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
                    SDP_PRINT("%s Annexes are possibly there for this fmtp %s  tmp: %s line\n", 
                              sdp_p->debug_str, fmtp_ptr, tmp);
                }
                while (temp != NULL) {
                    if (strchr(temp, 'D') !=NULL) {
                        attr_p->attr.fmtp.annex_d = TRUE;
                    } 
                    if (strchr(temp, 'F') !=NULL) {
                        attr_p->attr.fmtp.annex_f = TRUE;
                    } 
                    if (strchr(temp, 'I') !=NULL) {
                        attr_p->attr.fmtp.annex_i = TRUE;
                    } 
                    if (strchr(temp, 'J') !=NULL) {
                        attr_p->attr.fmtp.annex_j = TRUE;
                    } 
                    if (strchr(temp, 'T') !=NULL) {
                        attr_p->attr.fmtp.annex_t = TRUE;
                    } 
                    temp=PL_strtok_r(NULL, ";", &strtok_state);
                }
            }          
            done = TRUE;
        }
        fmtp_ptr++;
      } else {
          done = TRUE;
      }
    } 

    if (codec_info_found) {
        
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Parsed a=%s, payload type %u, bitrate %lu, mode %u QCIF = %u, CIF = %u, MAXBR= %u, SQCIF=%u, CIF4= %u, CIF16=%u, CUSTOM=%u,%u,%u , PAR=%u:%u,CPCF=%u, BPP=%u, HRD=%u \n", 
                      sdp_p->debug_str,
                      sdp_get_attr_name(attr_p->type),
                      attr_p->attr.fmtp.payload_num,
                      attr_p->attr.fmtp.bitrate,
                      attr_p->attr.fmtp.mode,
                      attr_p->attr.fmtp.qcif,
                      attr_p->attr.fmtp.cif,
                      attr_p->attr.fmtp.maxbr,
                      attr_p->attr.fmtp.sqcif,
                      attr_p->attr.fmtp.cif4,
                      attr_p->attr.fmtp.cif16,
                      attr_p->attr.fmtp.custom_x,attr_p->attr.fmtp.custom_y, 
                      attr_p->attr.fmtp.custom_mpi,
                      attr_p->attr.fmtp.par_width,
                      attr_p->attr.fmtp.par_height,
                      attr_p->attr.fmtp.cpcf,
                      attr_p->attr.fmtp.bpp,
                      attr_p->attr.fmtp.hrd
		      );
        }

        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Parsed a=%s, payload type %u,PROFILE=%u,LEVEL=%u, INTERLACE - %s", 
                      sdp_p->debug_str,
                      sdp_get_attr_name(attr_p->type),
                      attr_p->attr.fmtp.payload_num,
                      attr_p->attr.fmtp.profile,
                      attr_p->attr.fmtp.level,
                      attr_p->attr.fmtp.is_interlace ? "YES":"NO");
        }	              

	if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Parsed H.264 attributes: profile-level-id=%s, parameter-sets=%s, packetization-mode=%d level-asymmetry-allowed=%d interleaving-depth=%d deint-buf-req=%lu max-don-diff=%lu, init_buf-time=%lu\n",
                      sdp_p->debug_str,
                      attr_p->attr.fmtp.profile_level_id,
                      attr_p->attr.fmtp.parameter_sets,
                      attr_p->attr.fmtp.packetization_mode,
                      attr_p->attr.fmtp.level_asymmetry_allowed,
                      attr_p->attr.fmtp.interleaving_depth,
                      attr_p->attr.fmtp.deint_buf_req,
                      attr_p->attr.fmtp.max_don_diff,
                      attr_p->attr.fmtp.init_buf_time
		      );
        }

	if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("\n%s Parsed H.264 opt attributes: max-mbps=%lu, max-fs=%lu, max-cpb=%lu max-dpb=%lu max-br=%lu redundant-pic-cap=%d, deint-buf-cap=%lu, max-rcmd-nalu-size=%lu , parameter-add=%d\n",
                      sdp_p->debug_str,
                      attr_p->attr.fmtp.max_mbps,
                      attr_p->attr.fmtp.max_fs,
                      attr_p->attr.fmtp.max_cpb,
                      attr_p->attr.fmtp.max_dpb,
                      attr_p->attr.fmtp.max_br,
                      attr_p->attr.fmtp.redundant_pic_cap,
                      attr_p->attr.fmtp.deint_buf_cap,
                      attr_p->attr.fmtp.max_rcmd_nalu_size,
                      attr_p->attr.fmtp.parameter_add);

        }
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Parsed annexes are : D=%d F=%d I=%d J=%d T=%d, K=%d N=%d P=%d,%d\n",
                      sdp_p->debug_str,
                      attr_p->attr.fmtp.annex_d,
                      attr_p->attr.fmtp.annex_f,  attr_p->attr.fmtp.annex_i, 
                      attr_p->attr.fmtp.annex_j,  attr_p->attr.fmtp.annex_t,  
                      attr_p->attr.fmtp.annex_k_val,  
		      attr_p->attr.fmtp.annex_n_val,  
                      attr_p->attr.fmtp.annex_p_val_picture_resize,
                      attr_p->attr.fmtp.annex_p_val_warp);

        }
	SDP_FREE(temp_ptr);
        return (SDP_SUCCESS);
    } else {
        done = FALSE;
	fmtp_ptr = src_ptr;
        tmp[0] = '\0';
    }
    
    for (i=0; !done; i++) {
        fmtp_p->fmtp_format = SDP_FMTP_NTE;
        
        fmtp_ptr = sdp_getnextstrtok(fmtp_ptr, tmp, sizeof(tmp), ", \t", &result1);
        if (result1 != SDP_SUCCESS) {
            done = TRUE;
            continue;
        }
        
        ptr2 = tmp;
        low_val = (u8)sdp_getnextnumtok(ptr2, (const char **)&ptr2, 
                                    "- \t", &result1);
        if (*ptr2 == '-') {
            high_val = (u8)sdp_getnextnumtok(ptr2, (const char **)&ptr2, 
                                         "- \t", &result2);
        } else {
            high_val = low_val;
        }

        if ((result1 != SDP_SUCCESS) || (result2 != SDP_SUCCESS)) {
            if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
                SDP_WARN("%s Warning: Invalid named events specified for "
                         "fmtp attribute.", sdp_p->debug_str);
            }
            sdp_p->conf_p->num_invalid_param++;
	    SDP_FREE(temp_ptr);
            return (SDP_INVALID_PARAMETER);
        }

        for (i = low_val; i <= high_val; i++) {
            mapword = i/SDP_NE_BITS_PER_WORD;
            bmap = SDP_NE_BIT_0 << (i%32);
            fmtp_p->bmap[mapword] |= bmap;
        }
        if (high_val > fmtp_p->maxval) {
            fmtp_p->maxval = high_val;
        }
    }

    if (fmtp_p->maxval == 0) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No named events specified for "
                     "fmtp attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
	SDP_FREE(temp_ptr);
        return (SDP_INVALID_PARAMETER);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, payload type %u, ", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type),
                  attr_p->attr.fmtp.payload_num);
    }
    SDP_FREE(temp_ptr);
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_fmtp (sdp_t *sdp_p, sdp_attr_t *attr_p, char **ptr,
                                  u16 len)
{
    u16         event_id;
    u32         mask;
    u32         mapword;
    u8          min = 0;
    u8          max = 0;
    tinybool    range_start = FALSE;
    tinybool    range_end = FALSE;
    tinybool    semicolon = FALSE;
    char       *endbuf_p;
    sdp_fmtp_t *fmtp_p;
    
    endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%u ", sdp_attr[attr_p->type].name,
                     attr_p->attr.fmtp.payload_num);
    len = endbuf_p - *ptr;

    fmtp_p = &(attr_p->attr.fmtp);
    switch (fmtp_p->fmtp_format) {
      case SDP_FMTP_MODE:
          *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "mode=%u",attr_p->attr.fmtp.mode);
          break;
    
      case SDP_FMTP_CODEC_INFO:
         if (fmtp_p->bitrate > 0) {
	    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "bitrate=%u",attr_p->attr.fmtp.bitrate);
	    semicolon = TRUE;
	 }
	 if (fmtp_p->annexa_required) {
	    if (fmtp_p->annexa) {
	        if (semicolon) {
	            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";annexa=yes");
	            semicolon = TRUE;
	        } else {
	            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "annexa=yes");
	            semicolon = TRUE;
	        }
	    } else {
	        if (semicolon) {
	            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";annexa=no");;
	            semicolon = TRUE;
	        } else {
	            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "annexa=no");
	            semicolon = TRUE;
	        }
	    }
	
	 }
	
	 if (fmtp_p->annexb_required) {
	    if (fmtp_p->annexb) {
	        if (semicolon) {
	            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";annexb=yes");
	            semicolon = TRUE;
	        } else {
	           *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "annexb=yes");
	           semicolon = TRUE;
	        }
	   } else {
	       if (semicolon) {
	            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";annexb=no");
	            semicolon = TRUE;
	       } else {
	           *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "annexb=no");
	           semicolon = TRUE;
	      }
	   }
	 }
	
         if (fmtp_p->qcif > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";QCIF=%u",attr_p->attr.fmtp.qcif);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "QCIF=%u",attr_p->attr.fmtp.qcif);
                 semicolon = TRUE;
	    }
	 }
         
         if (fmtp_p->cif> 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";CIF=%u",attr_p->attr.fmtp.cif);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "CIF=%u",attr_p->attr.fmtp.cif);
                 semicolon = TRUE;
	    }
	 }

         if (fmtp_p->maxbr > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";MAXBR=%u",attr_p->attr.fmtp.maxbr);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "MAXBR=%u",attr_p->attr.fmtp.maxbr);
                 semicolon = TRUE;
	    }
	 }
         
         if (fmtp_p->sqcif > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";SQCIF=%u",attr_p->attr.fmtp.sqcif);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "SQCIF=%u",attr_p->attr.fmtp.sqcif);
                 semicolon = TRUE;
	    }
	 }

         if (fmtp_p->cif4 > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";CIF4=%u",attr_p->attr.fmtp.cif4);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "CIF4=%u",attr_p->attr.fmtp.cif4);
                 semicolon = TRUE;
	    }
	 }

         if (fmtp_p->cif16 > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";CIF16=%u",attr_p->attr.fmtp.cif16);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "CIF16=%u",attr_p->attr.fmtp.cif16);
                 semicolon = TRUE;
	    }
	 }

         if ((fmtp_p->custom_x > 0) && (fmtp_p->custom_y > 0) && 
	     (fmtp_p->custom_mpi > 0)) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";CUSTOM=%u,%u,%u", attr_p->attr.fmtp.custom_x, attr_p->attr.fmtp.custom_y, attr_p->attr.fmtp.custom_mpi);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "CUSTOM=%u,%u,%u", attr_p->attr.fmtp.custom_x, attr_p->attr.fmtp.custom_y, attr_p->attr.fmtp.custom_mpi);
                 semicolon = TRUE;
	    }
	 }

         if ((fmtp_p->par_height > 0) && (fmtp_p->par_width > 0)) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";PAR=%u:%u", attr_p->attr.fmtp.par_width, attr_p->attr.fmtp.par_width);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "PAR=%u:%u", attr_p->attr.fmtp.par_width, attr_p->attr.fmtp.par_width);
                semicolon = TRUE;
	    }
	 }

         if (fmtp_p->cpcf > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";CPCF=%u", attr_p->attr.fmtp.cpcf);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "CPCF=%u", attr_p->attr.fmtp.cpcf);
                semicolon = TRUE;
	    }
	 }

         if (fmtp_p->bpp > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";BPP=%u", attr_p->attr.fmtp.bpp);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "BPP=%u", attr_p->attr.fmtp.bpp);
                semicolon = TRUE;
	    }
	 }

         if (fmtp_p->hrd > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";HRD=%u", attr_p->attr.fmtp.hrd);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "HRD=%u", attr_p->attr.fmtp.hrd);
                semicolon = TRUE;
	    }
	 }

         if (fmtp_p->profile >= 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";PROFILE=%u", 
                                 attr_p->attr.fmtp.profile);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "PROFILE=%u", 
                                 attr_p->attr.fmtp.profile);
                semicolon = TRUE;
	    }
	 }

         if (fmtp_p->level >= 0) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";LEVEL=%u", 
                                  attr_p->attr.fmtp.level);
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "LEVEL=%u", 
                                  attr_p->attr.fmtp.level);
                 semicolon = TRUE;
             }
	 }

         if (fmtp_p->is_interlace) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";INTERLACE");
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "INTERLACE");
	     }
             semicolon = TRUE;
         }

         if (fmtp_p->annex_d) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";D");
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "D");
                 semicolon = TRUE;
             }
         } 

         if (fmtp_p->annex_f) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";F");
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "F");
                 semicolon = TRUE;
             }
         } 
         if (fmtp_p->annex_i) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";I");
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "I");
                 semicolon = TRUE;
             }
         } 
         if (fmtp_p->annex_j) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";J");
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "J");
                 semicolon = TRUE;
             }
         } 
         if (fmtp_p->annex_t) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";T");
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "T");
                 semicolon = TRUE;
             }
         } 
         if (fmtp_p->annex_k_val >0) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";K=%u", 
                                  attr_p->attr.fmtp.annex_k_val);
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "K=%u", 
                                  attr_p->attr.fmtp.annex_k_val);
                 semicolon = TRUE;
             }
         } 
         if (fmtp_p->annex_n_val >0) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";N=%u", 
                                  attr_p->attr.fmtp.annex_n_val);
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "N=%u", 
                                  attr_p->attr.fmtp.annex_n_val);
                 semicolon = TRUE;
             }
         } 
         if ((fmtp_p->annex_p_val_picture_resize > 0) && (fmtp_p->annex_p_val_warp > 0)) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";P=%d,%d", 
                                  attr_p->attr.fmtp.annex_p_val_picture_resize, 
                                  attr_p->attr.fmtp.annex_p_val_warp); 
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "P=%d,%d", 
                                  attr_p->attr.fmtp.annex_p_val_picture_resize,
                                  attr_p->attr.fmtp.annex_p_val_warp); 
                 semicolon = TRUE;
             }
         } 

         if (fmtp_p->profile_level_id[0] != '\0') {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";profile-level-id=%s",
                                  attr_p->attr.fmtp.profile_level_id);
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "profile-level-id=%s",
                                  attr_p->attr.fmtp.profile_level_id);
                 semicolon = TRUE;
             }
         }
         
         if (fmtp_p->parameter_sets[0] != '\0') {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";sprop-parameter-sets=%s",
                                  attr_p->attr.fmtp.parameter_sets);
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "sprop-parameter-sets=%s",
                                  attr_p->attr.fmtp.parameter_sets);
                 semicolon = TRUE;
             }
	 }
         
         if (fmtp_p->packetization_mode < SDP_MAX_PACKETIZATION_MODE_VALUE ) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";packetization-mode=%u",
                                 attr_p->attr.fmtp.packetization_mode);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "packetization-mode=%u",
                                 attr_p->attr.fmtp.packetization_mode);
                 semicolon = TRUE;
            }
	    }
        if (fmtp_p->level_asymmetry_allowed <= SDP_MAX_LEVEL_ASYMMETRY_ALLOWED_VALUE ) {
            if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";level-asymmetry-allowed=%u",
                                 attr_p->attr.fmtp.level_asymmetry_allowed);
                 semicolon = TRUE;
            } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "level-asymmetry-allowed=%u",
                                 attr_p->attr.fmtp.level_asymmetry_allowed);
                 semicolon = TRUE;
	    }
	 }
         if (fmtp_p->interleaving_depth > 0) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";sprop-interleaving-depth=%u",
                                 attr_p->attr.fmtp.interleaving_depth);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "sprop-interleaving-depth=%u",
                                 attr_p->attr.fmtp.interleaving_depth);
                 semicolon = TRUE;
	    }
	 }
         if (fmtp_p->flag & SDP_DEINT_BUF_REQ_FLAG) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";sprop-deint-buf-req=%u",
                                  attr_p->attr.fmtp.deint_buf_req);
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "sprop-deint-buf-req=%u",
                                  attr_p->attr.fmtp.deint_buf_req);
                 semicolon = TRUE;
             }
	 }
         if (fmtp_p->max_don_diff > 0) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";sprop-max-don-diff=%u",
                                  attr_p->attr.fmtp.max_don_diff);
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "sprop-max-don-diff=%u",
                                  attr_p->attr.fmtp.max_don_diff);
                 semicolon = TRUE;
             }
	 }

         if (fmtp_p->flag & SDP_INIT_BUF_TIME_FLAG) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";sprop-init-buf-time=%u",
                                 attr_p->attr.fmtp.init_buf_time);
                semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "sprop-init-buf-time=%u",
                                 attr_p->attr.fmtp.init_buf_time);
                semicolon = TRUE;
	    }
	 }
	 
         if (fmtp_p->max_mbps > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";max-mbps=%u",
                                 attr_p->attr.fmtp.max_mbps);
                semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "max-mbps=%u",
                                 attr_p->attr.fmtp.max_mbps);
                semicolon = TRUE;
	    }
	 }
	 
         if (fmtp_p->max_fs > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";max-fs=%u",
                                 attr_p->attr.fmtp.max_fs);
                semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "max-fs=%u",
                                 attr_p->attr.fmtp.max_fs);
                semicolon = TRUE;
	    }
	 }
	 
         if (fmtp_p->max_cpb > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";max-cpb=%u",
                                 attr_p->attr.fmtp.max_cpb);
                semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "max-cpb=%u",
                                 attr_p->attr.fmtp.max_cpb);
                semicolon = TRUE;
	    }
	 }
	 
         if (fmtp_p->max_dpb > 0) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";max-dpb=%u",
                                  attr_p->attr.fmtp.max_dpb);
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "max-dpb=%u",
                                  attr_p->attr.fmtp.max_dpb);
                semicolon = TRUE;
	    }
	 }
	 
         if (fmtp_p->max_br > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";max-br=%u",
                                 attr_p->attr.fmtp.max_br);
                semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "max-br=%u",
                                 attr_p->attr.fmtp.max_br);
                semicolon = TRUE;
	    }
	 }
	 
         if (fmtp_p->redundant_pic_cap > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";redundant-pic-cap=%u",
                                 attr_p->attr.fmtp.redundant_pic_cap);
                semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "redundant-pic-cap=%u",
                                 attr_p->attr.fmtp.redundant_pic_cap);
                semicolon = TRUE;
	    }
	 }
	 
         if (fmtp_p->flag & SDP_DEINT_BUF_CAP_FLAG) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";deint-buf-cap=%u",
                                 attr_p->attr.fmtp.deint_buf_cap);
                semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "deint-buf-cap=%u",
                                 attr_p->attr.fmtp.deint_buf_cap);
                semicolon = TRUE;
	    }
	 }
	 
         if (fmtp_p->flag & SDP_MAX_RCMD_NALU_SIZE_FLAG) {
             if (semicolon) {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";max-rcmd-nalu-size=%u",
                                  attr_p->attr.fmtp.max_rcmd_nalu_size);
                 semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "max-rcmd-nalu-size=%u",
                                  attr_p->attr.fmtp.max_rcmd_nalu_size);
                semicolon = TRUE;
             }
	 }
	 
         if (fmtp_p->parameter_add == FALSE) {
             if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";parameter-add=%u",
                                 attr_p->attr.fmtp.parameter_add);
                semicolon = TRUE;
             } else {
                 *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "parameter-add=%u",
                                 attr_p->attr.fmtp.parameter_add);
                 semicolon = TRUE;
             }
	 }

     if (fmtp_p->maxaveragebitrate > 0) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";maxaveragebitrate=%u",attr_p->attr.fmtp.maxaveragebitrate);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "maxaveragebitrate=%u",attr_p->attr.fmtp.maxaveragebitrate);
                 semicolon = TRUE;
	    }
	 }

     if (fmtp_p->usedtx <= 1) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";usedtx=%u",attr_p->attr.fmtp.usedtx);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "usedtx=%u",attr_p->attr.fmtp.usedtx);
                 semicolon = TRUE;
	    }
	 }

     if (fmtp_p->stereo <= 1) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";stereo=%u",attr_p->attr.fmtp.stereo);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "stereo=%u",attr_p->attr.fmtp.stereo);
                 semicolon = TRUE;
	    }
	 }

     if (fmtp_p->useinbandfec <= 1) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";useinbandfec=%u",attr_p->attr.fmtp.useinbandfec);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "useinbandfec=%u",attr_p->attr.fmtp.useinbandfec);
                 semicolon = TRUE;
	    }
	 }

     if (fmtp_p->maxcodedaudiobandwidth[0] != '\0') {
         if (semicolon) {
             *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";maxcodedaudiobandwidth=%s",
                              attr_p->attr.fmtp.maxcodedaudiobandwidth);
             semicolon = TRUE;
         } else {
             *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "maxcodedaudiobandwidth=%s",
                              attr_p->attr.fmtp.maxcodedaudiobandwidth);
             semicolon = TRUE;
         }
     }

     if (fmtp_p->cbr <= 1) {
	    if (semicolon) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";cbr=%u",attr_p->attr.fmtp.cbr);
                 semicolon = TRUE;
	    } else {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "cbr=%u",attr_p->attr.fmtp.cbr);
                 semicolon = TRUE;
	    }
	 }

         break;

      case SDP_FMTP_DATACHANNEL:

          if (fmtp_p->protocol[0] != '\0') {
              if (semicolon) {
                  *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";protocol=%s",
                                   attr_p->attr.fmtp.protocol);
              } else {
                  *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "protocol=%s",
                                   attr_p->attr.fmtp.protocol);
                  semicolon = TRUE;
              }
          }

          if (fmtp_p->streams > 0) {
    	      if (semicolon) {
                  *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ";streams=%u",attr_p->attr.fmtp.streams);
    	      } else {
                  *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "streams=%u",attr_p->attr.fmtp.streams);
                  semicolon = TRUE;
    	      }
          }

         break;
	 
     case SDP_FMTP_NTE:
      default:
        break;
     }
     
     for(event_id = 0, mapword = 0, mask = SDP_NE_BIT_0;
         event_id <= fmtp_p->maxval;
         event_id++, mapword = event_id/SDP_NE_BITS_PER_WORD ) {

         if (event_id % SDP_NE_BITS_PER_WORD) {
             mask <<= 1;
         } else {
         
         mask = SDP_NE_BIT_0;
             if (!range_start && !range_end && !fmtp_p->bmap[mapword]) {
	    

                event_id += SDP_NE_BITS_PER_WORD - 1;
                continue;
            }
         }

        if (fmtp_p->bmap[mapword] & mask) {
            if (!range_start) {
                range_start = TRUE;
                min = max = (u8)event_id;
            } else {
                max = (u8)event_id;
            }
        range_end = (max == fmtp_p->maxval);
        } else {
        

            range_end = range_start;
        }

        
        if (range_end) {
            range_start = range_end = FALSE;

            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "%u", min);

            if (min != max) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "-%u", max);
            }

            if (max != fmtp_p->maxval) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), ",");
            }
        }
    }

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_direction (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                       const char *ptr)
{
    
    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type));
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_direction (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                       char **ptr, u16 len)
{
    char *endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s\r\n", 
                     sdp_get_attr_name(attr_p->type));

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_qos (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                 const char *ptr)
{
    int i;
    sdp_result_e result;
    char tmp[SDP_MAX_STRING_LEN];

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No qos strength tag specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.qos.strength = SDP_QOS_STRENGTH_UNKNOWN;
    for (i=0; i < SDP_MAX_QOS_STRENGTH; i++) {
        if (cpr_strncasecmp(tmp, sdp_qos_strength[i].name,
                        sdp_qos_strength[i].strlen) == 0) {
            attr_p->attr.qos.strength = (sdp_qos_strength_e)i;
        }
    }
    if (attr_p->attr.qos.strength == SDP_QOS_STRENGTH_UNKNOWN) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: QOS strength tag unrecognized (%s)", 
                     sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No qos direction specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.qos.direction = SDP_QOS_DIR_UNKNOWN;
    for (i=0; i < SDP_MAX_QOS_DIR; i++) {
        if (cpr_strncasecmp(tmp, sdp_qos_direction[i].name,
                        sdp_qos_direction[i].strlen) == 0) {
            attr_p->attr.qos.direction = (sdp_qos_dir_e)i;
        }
    }
    if (attr_p->attr.qos.direction == SDP_QOS_DIR_UNKNOWN) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: QOS direction unrecognized (%s)", 
                     sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    attr_p->attr.qos.confirm = FALSE;
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result == SDP_SUCCESS) {
        if (cpr_strncasecmp(tmp, "confirm", sizeof("confirm")) == 0) {
            attr_p->attr.qos.confirm = TRUE;
        }
        if (attr_p->attr.qos.confirm == FALSE) {
            if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
                SDP_WARN("%s Warning: QOS confirm parameter invalid (%s)",
                         sdp_p->debug_str, tmp);
            }
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, strength %s, direction %s, confirm %s", 
                  sdp_p->debug_str, sdp_get_attr_name(attr_p->type),
                  sdp_get_qos_strength_name(attr_p->attr.qos.strength),
                  sdp_get_qos_direction_name(attr_p->attr.qos.direction),
                  (attr_p->attr.qos.confirm ? "set" : "not set"));
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_qos (sdp_t *sdp_p, sdp_attr_t *attr_p, char **ptr,
                                 u16 len)
{
    char       *endbuf_p;

    
    endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s %s", 
                     sdp_attr[attr_p->type].name, 
                     sdp_get_qos_strength_name(attr_p->attr.qos.strength),
                     sdp_get_qos_direction_name(attr_p->attr.qos.direction));

    if (attr_p->attr.qos.confirm == TRUE) {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), " confirm\r\n");
    } else {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_curr (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                 const char *ptr)
{
    int i;
    sdp_result_e result;
    char tmp[SDP_MAX_STRING_LEN];
   
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No curr attr type specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.curr.type = SDP_CURR_UNKNOWN_TYPE;
    for (i=0; i < SDP_MAX_CURR_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_curr_type[i].name,
                        sdp_curr_type[i].strlen) == 0) {
            attr_p->attr.curr.type = (sdp_curr_type_e)i;
        }
    }
    
    if (attr_p->attr.curr.type != SDP_CURR_QOS_TYPE) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Unknown curr type.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);   
    }
    
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
     if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No curr attr type specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.curr.status_type = SDP_QOS_STATUS_TYPE_UNKNOWN;
    for (i=0; i < SDP_MAX_QOS_STATUS_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_qos_status_type[i].name,
                        sdp_qos_status_type[i].strlen) == 0) {
            attr_p->attr.curr.status_type = (sdp_qos_status_types_e)i;
        }
    }
    

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No qos direction specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.curr.direction = SDP_QOS_DIR_UNKNOWN;
    for (i=0; i < SDP_MAX_QOS_DIR; i++) {
        if (cpr_strncasecmp(tmp, sdp_qos_direction[i].name,
                        sdp_qos_direction[i].strlen) == 0) {
            attr_p->attr.curr.direction = (sdp_qos_dir_e)i;
        }
    }
    if (attr_p->attr.curr.direction == SDP_QOS_DIR_UNKNOWN) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: QOS direction unrecognized (%s)", 
                     sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, type %s status type %s, direction %s", 
                  sdp_p->debug_str, sdp_get_attr_name(attr_p->type),
                  sdp_get_curr_type_name(attr_p->attr.curr.type),
                  sdp_get_qos_status_type_name(attr_p->attr.curr.status_type),
                  sdp_get_qos_direction_name(attr_p->attr.curr.direction));
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_curr (sdp_t *sdp_p, sdp_attr_t *attr_p, char **ptr,
                                 u16 len)
{
    char       *endbuf_p;

    
    endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s %s %s", 
                     sdp_attr[attr_p->type].name, 
                     sdp_get_curr_type_name(attr_p->attr.curr.type),
                     sdp_get_qos_status_type_name(attr_p->attr.curr.status_type),
                     sdp_get_qos_direction_name(attr_p->attr.curr.direction));

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_des (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                 const char *ptr)
{
    int i;
    sdp_result_e result;
    char tmp[SDP_MAX_STRING_LEN];
   
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No des attr type specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.des.type = SDP_DES_UNKNOWN_TYPE;
    for (i=0; i < SDP_MAX_CURR_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_des_type[i].name,
                        sdp_des_type[i].strlen) == 0) {
            attr_p->attr.des.type = (sdp_des_type_e)i;
        }
    }
    
    if (attr_p->attr.des.type != SDP_DES_QOS_TYPE) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Unknown conf type.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);   
    }
    
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No qos strength tag specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.des.strength = SDP_QOS_STRENGTH_UNKNOWN;
    for (i=0; i < SDP_MAX_QOS_STRENGTH; i++) {
        if (cpr_strncasecmp(tmp, sdp_qos_strength[i].name,
                        sdp_qos_strength[i].strlen) == 0) {
            attr_p->attr.des.strength = (sdp_qos_strength_e)i;
        }
    }
    if (attr_p->attr.des.strength == SDP_QOS_STRENGTH_UNKNOWN) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: QOS strength tag unrecognized (%s)", 
                     sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
     if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No des attr type specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.des.status_type = SDP_QOS_STATUS_TYPE_UNKNOWN;
    for (i=0; i < SDP_MAX_QOS_STATUS_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_qos_status_type[i].name,
                        sdp_qos_status_type[i].strlen) == 0) {
            attr_p->attr.des.status_type = (sdp_qos_status_types_e)i;
        }
    }
    

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No qos direction specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.des.direction = SDP_QOS_DIR_UNKNOWN;
    for (i=0; i < SDP_MAX_QOS_DIR; i++) {
        if (cpr_strncasecmp(tmp, sdp_qos_direction[i].name,
                        sdp_qos_direction[i].strlen) == 0) {
            attr_p->attr.des.direction = (sdp_qos_dir_e)i;
        }
    }
    if (attr_p->attr.des.direction == SDP_QOS_DIR_UNKNOWN) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: QOS direction unrecognized (%s)", 
                     sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, type %s strength %s status type %s, direction %s", 
                  sdp_p->debug_str, sdp_get_attr_name(attr_p->type),
                  sdp_get_des_type_name(attr_p->attr.des.type),
                  sdp_get_qos_strength_name(attr_p->attr.qos.strength),
                  sdp_get_qos_status_type_name(attr_p->attr.des.status_type),
                  sdp_get_qos_direction_name(attr_p->attr.des.direction));
    }

    return (SDP_SUCCESS);
}


sdp_result_e sdp_build_attr_des (sdp_t *sdp_p, sdp_attr_t *attr_p, char **ptr,
                                 u16 len)
{
    char       *endbuf_p;

    
    endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s %s %s %s", 
                     sdp_attr[attr_p->type].name, 
                     sdp_get_curr_type_name((sdp_curr_type_e)attr_p->attr.des.type),
                     sdp_get_qos_strength_name(attr_p->attr.des.strength),
                     sdp_get_qos_status_type_name(attr_p->attr.des.status_type),
                     sdp_get_qos_direction_name(attr_p->attr.des.direction));

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_conf (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                 const char *ptr)
{
    int i;
    sdp_result_e result;
    char tmp[SDP_MAX_STRING_LEN];
   
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No conf attr type specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.conf.type = SDP_CONF_UNKNOWN_TYPE;
    for (i=0; i < SDP_MAX_CURR_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_conf_type[i].name,
                        sdp_conf_type[i].strlen) == 0) {
            attr_p->attr.conf.type = (sdp_conf_type_e)i;
        }
    }
    
    if (attr_p->attr.conf.type != SDP_CONF_QOS_TYPE) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Unknown conf type.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);   
    }
    
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
     if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No conf attr type specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.conf.status_type = SDP_QOS_STATUS_TYPE_UNKNOWN;
    for (i=0; i < SDP_MAX_QOS_STATUS_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_qos_status_type[i].name,
                        sdp_qos_status_type[i].strlen) == 0) {
            attr_p->attr.conf.status_type = (sdp_qos_status_types_e)i;
        }
    }
    

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No qos direction specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.conf.direction = SDP_QOS_DIR_UNKNOWN;
    for (i=0; i < SDP_MAX_QOS_DIR; i++) {
        if (cpr_strncasecmp(tmp, sdp_qos_direction[i].name,
                        sdp_qos_direction[i].strlen) == 0) {
            attr_p->attr.conf.direction = (sdp_qos_dir_e)i;
        }
    }
    if (attr_p->attr.conf.direction == SDP_QOS_DIR_UNKNOWN) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: QOS direction unrecognized (%s)", 
                     sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, type %s status type %s, direction %s", 
                  sdp_p->debug_str, sdp_get_attr_name(attr_p->type),
                  sdp_get_conf_type_name(attr_p->attr.conf.type),
                  sdp_get_qos_status_type_name(attr_p->attr.conf.status_type),
                  sdp_get_qos_direction_name(attr_p->attr.conf.direction));
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_conf (sdp_t *sdp_p, sdp_attr_t *attr_p, char **ptr,
                                 u16 len)
{
    char       *endbuf_p;

    
    endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s %s %s", 
                     sdp_attr[attr_p->type].name, 
                     sdp_get_conf_type_name(attr_p->attr.conf.type),
                     sdp_get_qos_status_type_name(attr_p->attr.conf.status_type),
                     sdp_get_qos_direction_name(attr_p->attr.conf.direction));

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return (SDP_SUCCESS);
}






sdp_result_e sdp_parse_attr_transport_map (sdp_t *sdp_p, sdp_attr_t *attr_p, 
	const char *ptr)
{
    sdp_result_e  result;

    attr_p->attr.transport_map.payload_num = 0;
    attr_p->attr.transport_map.encname[0]  = '\0';
    attr_p->attr.transport_map.clockrate   = 0;
    attr_p->attr.transport_map.num_chan    = 1;

    
    attr_p->attr.transport_map.payload_num = 
	(u16)sdp_getnextnumtok(ptr, &ptr, " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Invalid payload type specified for "
                     "%s attribute.", sdp_p->debug_str,
		     sdp_get_attr_name(attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, attr_p->attr.transport_map.encname, 
                            sizeof(attr_p->attr.transport_map.encname), "/ \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No encoding name specified in %s "
                     "attribute.", sdp_p->debug_str,
		     sdp_get_attr_name(attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    attr_p->attr.transport_map.clockrate = 
	sdp_getnextnumtok(ptr, &ptr, "/ \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No clockrate specified for "
                     "%s attribute, set to default of 8000.",
                     sdp_p->debug_str, sdp_get_attr_name(attr_p->type));
        }
        attr_p->attr.transport_map.clockrate = 8000;
    }
    
    
    if (*ptr == '/') {
        
        attr_p->attr.transport_map.num_chan = 
	    (u16)sdp_getnextnumtok(ptr, &ptr, "/ \t", &result);
        if (result != SDP_SUCCESS) {
            if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
                SDP_WARN("%s Warning: Invalid number of channels parameter"
                         " for rtpmap attribute.", sdp_p->debug_str);
            }
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, payload type %u, encoding name %s, "
                  "clockrate %lu", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type),
                  attr_p->attr.transport_map.payload_num,
                  attr_p->attr.transport_map.encname,
                  attr_p->attr.transport_map.clockrate);
        if (attr_p->attr.transport_map.num_chan != 1) {
            SDP_PRINT("/%u", attr_p->attr.transport_map.num_chan);
        }
    }

    return (SDP_SUCCESS);
}






sdp_result_e sdp_build_attr_transport_map (sdp_t *sdp_p, sdp_attr_t *attr_p, 
	char **ptr, u16 len)
{
    char         *endbuf_p;

    
    endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%u %s/%u", 
                     sdp_attr[attr_p->type].name, 
                     attr_p->attr.transport_map.payload_num,
                     attr_p->attr.transport_map.encname,
                     attr_p->attr.transport_map.clockrate);

    if (attr_p->attr.transport_map.num_chan != 1) {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "/%u\r\n", 
		attr_p->attr.transport_map.num_chan);
    } else {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_subnet (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                    const char *ptr)
{
    int i;
    char         *slash_ptr;
    sdp_result_e  result;
    tinybool      type_found = FALSE;
    char          tmp[SDP_MAX_STRING_LEN];

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No network type specified in subnet "
                     "attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.subnet.nettype = SDP_NT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_NETWORK_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_nettype[i].name, 
                        sdp_nettype[i].strlen) == 0) {
            type_found = TRUE;
        }
        if (type_found == TRUE) {
            if (sdp_p->conf_p->nettype_supported[i] == TRUE) {
                attr_p->attr.subnet.nettype = (sdp_nettype_e)i;
            }
            type_found = FALSE;
        }
    }
    if (attr_p->attr.subnet.nettype == SDP_NT_UNSUPPORTED) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Subnet network type "
                     "unsupported (%s).", sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No address type specified in subnet"
                     " attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.subnet.addrtype = SDP_AT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_ADDR_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_addrtype[i].name,
                        sdp_addrtype[i].strlen) == 0) {
            type_found = TRUE;
        }
        if (type_found == TRUE) {
            if (sdp_p->conf_p->addrtype_supported[i] == TRUE) {
                attr_p->attr.subnet.addrtype = (sdp_addrtype_e)i;
            }
            type_found = FALSE;
        }
    }
    if (attr_p->attr.subnet.addrtype == SDP_AT_UNSUPPORTED) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Subnet address type unsupported "
                     "(%s).", sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, attr_p->attr.subnet.addr, 
                            sizeof(attr_p->attr.subnet.addr), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No subnet address specified in "
                     "subnet attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    slash_ptr = sdp_findchar(attr_p->attr.subnet.addr, "/");
    if (*slash_ptr == '/') {
        *slash_ptr++ = '\0';
        
        attr_p->attr.subnet.prefix = sdp_getnextnumtok(slash_ptr, 
                                                  (const char **)&slash_ptr, 
                                                  " \t", &result);
        if (result != SDP_SUCCESS) {
            if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
                SDP_WARN("%s Warning: Invalid subnet prefix specified in "
                         "subnet attribute.", sdp_p->debug_str);
            }
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
    } else {
        attr_p->attr.subnet.prefix = SDP_INVALID_VALUE;
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, network %s, addr type %s, address %s ",
                  sdp_p->debug_str, sdp_get_attr_name(attr_p->type),
                  sdp_get_network_name(attr_p->attr.subnet.nettype),
                  sdp_get_address_name(attr_p->attr.subnet.addrtype),
                  attr_p->attr.subnet.addr);
        if (attr_p->attr.subnet.prefix != SDP_INVALID_VALUE) {
            SDP_PRINT("/%u", (ushort)attr_p->attr.subnet.prefix);
        }
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_subnet (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                    char **ptr, u16 len)
{
    char         *endbuf_p;

    
    endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s %s %s", sdp_attr[attr_p->type].name,
                     sdp_get_network_name(attr_p->attr.subnet.nettype),
                     sdp_get_address_name(attr_p->attr.subnet.addrtype),
                     attr_p->attr.subnet.addr);

    if (attr_p->attr.subnet.prefix != SDP_INVALID_VALUE) {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "/%u", (ushort)attr_p->attr.subnet.prefix);
    }
    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_t38_ratemgmt (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                          const char *ptr)
{
    int i;
    sdp_result_e result;
    char tmp[SDP_MAX_STRING_LEN];

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No t38 rate management specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.t38ratemgmt = SDP_T38_UNKNOWN_RATE;
    for (i=0; i < SDP_T38_MAX_RATES; i++) {
        if (cpr_strncasecmp(tmp, sdp_t38_rate[i].name,
                        sdp_t38_rate[i].strlen) == 0) {
            attr_p->attr.t38ratemgmt = (sdp_t38_ratemgmt_e)i;
        }
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, rate %s", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type),
                  sdp_get_t38_ratemgmt_name(attr_p->attr.t38ratemgmt));
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_t38_ratemgmt (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                          char **ptr, u16 len)
{
    char *endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s\r\n", 
                     sdp_attr[attr_p->type].name, 
                     sdp_get_t38_ratemgmt_name(attr_p->attr.t38ratemgmt));

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_t38_udpec (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                       const char *ptr)
{
    int i;
    sdp_result_e result;
    char tmp[SDP_MAX_STRING_LEN];

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No t38 udpEC specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.t38udpec = SDP_T38_UDPEC_UNKNOWN;
    for (i=0; i < SDP_T38_MAX_UDPEC; i++) {
        if (cpr_strncasecmp(tmp, sdp_t38_udpec[i].name,
                        sdp_t38_udpec[i].strlen) == 0) {
            attr_p->attr.t38udpec = (sdp_t38_udpec_e)i;
        }
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, udpec %s", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type),
                  sdp_get_t38_udpec_name(attr_p->attr.t38udpec));
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_t38_udpec (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                       char **ptr, u16 len)
{
    char *endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s\r\n", 
                     sdp_attr[attr_p->type].name, 
                     sdp_get_t38_udpec_name(attr_p->attr.t38udpec));

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_pc_codec (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                      const char *ptr)
{
    u16 i;
    sdp_result_e result;

    for (i=0; i < SDP_MAX_PAYLOAD_TYPES; i++) {
        attr_p->attr.pccodec.payload_type[i] = (ushort)sdp_getnextnumtok(ptr, &ptr,
                                                               " \t", &result);
        if (result != SDP_SUCCESS) {
            break;
        }
        attr_p->attr.pccodec.num_payloads++;
    }

    if (attr_p->attr.pccodec.num_payloads == 0) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No payloads specified for %s attr.",
                     sdp_p->debug_str, sdp_attr[attr_p->type].name);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, num payloads %u, payloads: ",
                  sdp_p->debug_str, sdp_get_attr_name(attr_p->type),
                  attr_p->attr.pccodec.num_payloads);
        for (i=0; i < attr_p->attr.pccodec.num_payloads; i++) {
            SDP_PRINT("%u ", attr_p->attr.pccodec.payload_type[i]);
        }
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_pc_codec (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                      char **ptr, u16 len)
{
    u16           i;
    char         *endbuf_p;

    
    endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s: ", sdp_attr[attr_p->type].name);

    for (i=0; i < attr_p->attr.pccodec.num_payloads; i++) {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "%u ", 
                         attr_p->attr.pccodec.payload_type[i]);
    }

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return (SDP_SUCCESS);
}


sdp_result_e sdp_parse_attr_cap (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                 const char *ptr)
{
    u16           i;
    sdp_result_e  result;
    sdp_mca_t    *cap_p;
    char          tmp[SDP_MAX_STRING_LEN];

    


    attr_p->attr.cap_p = NULL;
    


    sdp_p->cap_valid = FALSE;

    


    cap_p = sdp_alloc_mca();
    if (cap_p == NULL) {
        sdp_p->conf_p->num_no_resource++;
        return (SDP_NO_RESOURCE);
    }

    

    (void)sdp_getnextnumtok(ptr, &ptr, "/ \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Capability not specified for %s, "
                     "unable to parse.", sdp_p->debug_str,
		     sdp_get_attr_name(attr_p->type));
                     
        }
        SDP_FREE(cap_p);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s No media type specified for %s attribute, "
                     "unable to parse.", sdp_p->debug_str,
		     sdp_get_attr_name(attr_p->type));
        }
        SDP_FREE(cap_p);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    cap_p->media = SDP_MEDIA_UNSUPPORTED;
    for (i=0; i < SDP_MAX_MEDIA_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_media[i].name, sdp_media[i].strlen) == 0) {
            cap_p->media = (sdp_media_e)i;
            break;
        }
    }
    if (cap_p->media == SDP_MEDIA_UNSUPPORTED) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Media type unsupported (%s).", 
                     sdp_p->debug_str, tmp);
        }
        SDP_FREE(cap_p);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s No transport protocol type specified, "
                     "unable to parse.", sdp_p->debug_str);
        }
        SDP_FREE(cap_p);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    cap_p->transport = SDP_TRANSPORT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_TRANSPORT_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_transport[i].name,
                        sdp_transport[i].strlen) == 0) {
            cap_p->transport = (sdp_transport_e)i;
            break;
        }
    }
    if (cap_p->transport == SDP_TRANSPORT_UNSUPPORTED) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Transport protocol type unsupported "
                     "(%s).", sdp_p->debug_str, tmp);
        }
        SDP_FREE(cap_p);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    


    if ((cap_p->transport == SDP_TRANSPORT_AAL2_ITU) ||
        (cap_p->transport == SDP_TRANSPORT_AAL2_ATMF) ||
        (cap_p->transport == SDP_TRANSPORT_AAL2_CUSTOM)) {
        

        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: AAL2 profiles unsupported with "
                     "%s attributes.", sdp_p->debug_str,
		     sdp_get_attr_name(attr_p->type));
        }
        SDP_FREE(cap_p);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        
        sdp_parse_payload_types(sdp_p, cap_p, ptr);
        if (cap_p->num_payloads == 0) {
            SDP_FREE(cap_p);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
    }

    attr_p->attr.cap_p = cap_p;
    



    sdp_p->cap_valid = TRUE;
    sdp_p->last_cap_inst++;

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed %s media type %s, Transport %s, "
                  "Num payloads %u", sdp_p->debug_str, 
		  sdp_get_attr_name(attr_p->type), 
                  sdp_get_media_name(cap_p->media),
                  sdp_get_transport_name(cap_p->transport), 
                  cap_p->num_payloads);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_cap (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                 char **ptr, u16 len)
{
    u16                   i, j;
    char                 *endbuf_p;
    sdp_mca_t            *cap_p;
    sdp_result_e          result;
    sdp_media_profiles_t *profile_p;

    
    endbuf_p = *ptr + len;

    
    cap_p = attr_p->attr.cap_p;

    if (cap_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Invalid %s attribute, unable to build.",
		    sdp_p->debug_str,
		    sdp_get_attr_name(attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        
        return (SDP_SUCCESS);
    }

    
    if ((cap_p->media >= SDP_MAX_MEDIA_TYPES) ||
        (cap_p->transport >= SDP_MAX_TRANSPORT_TYPES)) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Media or transport type invalid for %s "
                     "attribute, unable to build.", sdp_p->debug_str,
		     sdp_get_attr_name(attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        
        return (SDP_SUCCESS);
    }

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s: %u ", sdp_attr[attr_p->type].name,
                     sdp_p->cur_cap_num);

    
    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "%s ", sdp_get_media_name(cap_p->media));

    
    if ((cap_p->transport == SDP_TRANSPORT_AAL2_ITU) ||
        (cap_p->transport == SDP_TRANSPORT_AAL2_ATMF) ||
        (cap_p->transport == SDP_TRANSPORT_AAL2_CUSTOM)) {
        profile_p = cap_p->media_profiles_p;
        for (i=0; i < profile_p->num_profiles; i++) {
            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "%s",
                             sdp_get_transport_name(profile_p->profile[i]));

            for (j=0; j < profile_p->num_payloads[i]; j++) {
                *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), " %u", 
                                 profile_p->payload_type[i][j]);
            }
            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), " "); 
        }
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\n");
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Built m= media line", sdp_p->debug_str);
        }
        return (SDP_SUCCESS);
    }

    
    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "%s", 
                     sdp_get_transport_name(cap_p->transport));

    
    for (i=0; i < cap_p->num_payloads; i++) {
        if (cap_p->payload_indicator[i] == SDP_PAYLOAD_ENUM) {
            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), " %s",
                             sdp_get_payload_name((sdp_payload_e)cap_p->payload_type[i]));
        } else {
            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), " %u", cap_p->payload_type[i]);
        }
    }
    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    
    sdp_p->cur_cap_num += cap_p->num_payloads;
    sdp_p->last_cap_type = attr_p->type;

    
    result = sdp_build_attr_cpar(sdp_p, cap_p->media_attrs_p, ptr, len);

    return (result);
}


sdp_result_e sdp_parse_attr_cpar (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                  const char *ptr)
{
    u16           i;
    sdp_result_e  result;
    sdp_mca_t    *cap_p;
    sdp_attr_t   *cap_attr_p = NULL;
    sdp_attr_t   *prev_attr_p;
    char          tmp[SDP_MAX_STRING_LEN];

    

    if (sdp_p->cap_valid == TRUE) {
	sdp_attr_e cap_type;

	if (attr_p->type == SDP_ATTR_CPAR) {
	    cap_type = SDP_ATTR_CDSC;
	} else {
	    
	    cap_type = SDP_ATTR_X_CAP;
	}

        if (sdp_p->mca_count == 0) {
            cap_attr_p = sdp_find_attr(sdp_p, SDP_SESSION_LEVEL, 0,
                                       cap_type, sdp_p->last_cap_inst);
        } else {
            cap_attr_p = sdp_find_attr(sdp_p, sdp_p->mca_count, 0,
                                       cap_type, sdp_p->last_cap_inst);
        }
    }
    if ((cap_attr_p == NULL) || (cap_attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: %s attribute specified with no "
                     "prior %s attribute", sdp_p->debug_str,
		     sdp_get_attr_name(attr_p->type), 
		     (attr_p->type == SDP_ATTR_CPAR)?
			(sdp_get_attr_name(SDP_ATTR_CDSC)) :
			(sdp_get_attr_name(SDP_ATTR_X_CAP)) );
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    



    if (((cap_attr_p->type == SDP_ATTR_CDSC) && 
	 (attr_p->type == SDP_ATTR_X_CPAR)) || 
	( (cap_attr_p->type == SDP_ATTR_X_CAP) && 
	  (attr_p->type == SDP_ATTR_CPAR)) ) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
	    SDP_WARN("%s Warning: %s attribute inconsistent with "
		    "prior %s attribute", sdp_p->debug_str,
		    sdp_get_attr_name(attr_p->type), 
		    sdp_get_attr_name(cap_attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    cap_p = cap_attr_p->attr.cap_p;

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), "= \t", &result);
	     
    if ((result != SDP_SUCCESS) || (tmp[0] != 'a') || (tmp[1] != '\0')) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Invalid token type (%s) in %s "
                     "attribute, unable to parse", sdp_p->debug_str, tmp,
		     sdp_get_attr_name(attr_p->type)); 
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    



    if (*ptr == '=') {
        ptr++;
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), ": \t", &result);
    



    if (ptr[0] == ':') {
        
        ptr++;
    }
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s No attribute type specified for %s "
                      "attribute, unable to parse.", sdp_p->debug_str,
		      sdp_get_attr_name(attr_p->type)); 
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    

    attr_p->type = SDP_ATTR_INVALID;
    attr_p->next_p = NULL;
    for (i=0; i < SDP_MAX_ATTR_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_attr[i].name, sdp_attr[i].strlen) == 0) {
            attr_p->type = (sdp_attr_e)i;
        }
    }
    if (attr_p->type == SDP_ATTR_INVALID) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Unrecognized attribute (%s) for %s"
		    " attribute, unable to parse.", sdp_p->debug_str, tmp,
		    sdp_get_attr_name(attr_p->type)); 
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    if ((attr_p->type == SDP_ATTR_X_SQN) ||
        (attr_p->type == SDP_ATTR_X_CAP) ||
        (attr_p->type == SDP_ATTR_X_CPAR) ||
	(attr_p->type == SDP_ATTR_SQN) ||
	(attr_p->type == SDP_ATTR_CDSC) ||
	(attr_p->type == SDP_ATTR_CPAR)) {
	if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Invalid attribute (%s) for %s"
		    " attribute, unable to parse.", sdp_p->debug_str, tmp,
		    sdp_get_attr_name(attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    result = sdp_attr[attr_p->type].parse_func(sdp_p, attr_p, ptr);
    if (result != SDP_SUCCESS) {
        return (result);
    }

    
    if (cap_p->media_attrs_p == NULL) {
        cap_p->media_attrs_p = attr_p;
    } else {
        for (prev_attr_p = cap_p->media_attrs_p;
             prev_attr_p->next_p != NULL;
             prev_attr_p = prev_attr_p->next_p) {
            ; 
        }
        prev_attr_p->next_p = attr_p;
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_cpar (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                  char **ptr, u16 len)
{
    char         *endbuf_p;
    sdp_result_e  result;
    const char	 *cpar_name;

    
    if (sdp_p->last_cap_type == SDP_ATTR_CDSC) {
	cpar_name = sdp_get_attr_name(SDP_ATTR_CPAR);
    } else {
	



	cpar_name = sdp_get_attr_name(SDP_ATTR_X_CPAR);
    }

    
    endbuf_p = *ptr + len;

    while (attr_p != NULL) {
        if (attr_p->type >= SDP_MAX_ATTR_TYPES) {
            SDP_WARN("%s Invalid attribute type to build (%u)", 
                     sdp_p->debug_str, attr_p->type);
        } else {
            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s: ", cpar_name);

            result = sdp_attr[attr_p->type].build_func(sdp_p, attr_p, 
                                                       ptr, (u16)(endbuf_p - *ptr));
            
            
            
            if (endbuf_p - *ptr <= 0)
                return (SDP_POTENTIAL_SDP_OVERFLOW);

            if (result == SDP_SUCCESS) {
                if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
                    SDP_PRINT("%s Built %s a=%s attribute line",
                              sdp_p->debug_str, cpar_name,
                              sdp_get_attr_name(attr_p->type));
                }
            }
        }
        attr_p = attr_p->next_p;
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_rtr (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                           const char *ptr)
{
    sdp_result_e  result;
    char tmp[SDP_MAX_STRING_LEN];

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsing a=%s, %s", sdp_p->debug_str,
                     sdp_get_attr_name(attr_p->type),
                     tmp);
    }
    
    attr_p->attr.rtr.confirm = FALSE;

    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS){ 
        return (SDP_SUCCESS);
    } else {
       
       if (cpr_strncasecmp(tmp, "confirm", sizeof("confirm")) == 0) {
           attr_p->attr.rtr.confirm = TRUE;
       }
       if (attr_p->attr.rtr.confirm == FALSE) {
           if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
               SDP_WARN("%s Warning: RTR confirm parameter invalid (%s)",
                        sdp_p->debug_str, tmp);
           }
           sdp_p->conf_p->num_invalid_param++;
           return (SDP_INVALID_PARAMETER);
       }
       if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
           SDP_PRINT("%s Parsed a=%s, %s", sdp_p->debug_str,
                     sdp_get_attr_name(attr_p->type), 
                     tmp);
       }
       return (SDP_SUCCESS);
    }
}

sdp_result_e sdp_build_attr_rtr (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                           char **ptr, u16 len)
{
    char *endbuf_p = *ptr + len;

    if (attr_p->attr.rtr.confirm){
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s\r\n", sdp_attr[attr_p->type].name,
                         "confirm");
    } else {  
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s\r\n", sdp_attr[attr_p->type].name);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_comediadir (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                        const char *ptr)
{
    int i;
    sdp_result_e  result;
    tinybool      type_found = FALSE;
    char          tmp[SDP_MAX_STRING_LEN];

    attr_p->attr.comediadir.role = SDP_MEDIADIR_ROLE_PASSIVE;
    attr_p->attr.comediadir.conn_info_present = FALSE;
    attr_p->attr.comediadir.conn_info.nettype = SDP_NT_INVALID;
    attr_p->attr.comediadir.src_port = 0;

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), ": \t", &result);

    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No role parameter specified for "
                     "comediadir attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.comediadir.role = SDP_MEDIADIR_ROLE_UNSUPPORTED;
    for (i=0; i < SDP_MAX_MEDIADIR_ROLES; i++) {
        if (cpr_strncasecmp(tmp, sdp_mediadir_role[i].name, 
                        sdp_mediadir_role[i].strlen) == 0) {
            type_found = TRUE;
            attr_p->attr.comediadir.role = (sdp_mediadir_role_e)i;
            break;
        }
    }
    if (attr_p->attr.comediadir.role == SDP_MEDIADIR_ROLE_UNSUPPORTED) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Invalid role type specified for "
                     "comediadir attribute (%s).", sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    if (attr_p->attr.comediadir.role == SDP_MEDIADIR_ROLE_PASSIVE) {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Parsed a=%s, passive",
                      sdp_p->debug_str, sdp_get_attr_name(attr_p->type));
        }
        return (SDP_SUCCESS);
    }

    
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No network type specified in comediadir "
                     "attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_SUCCESS); 
    }
    attr_p->attr.comediadir.conn_info.nettype = SDP_NT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_NETWORK_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_nettype[i].name, 
                        sdp_nettype[i].strlen) == 0) {
            type_found = TRUE;
        }
        if (type_found == TRUE) {
            if (sdp_p->conf_p->nettype_supported[i] == TRUE) {
                attr_p->attr.comediadir.conn_info.nettype = (sdp_nettype_e)i;
            }
            type_found = FALSE;
        }
    }
    if (attr_p->attr.comediadir.conn_info.nettype == SDP_NT_UNSUPPORTED) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: ConnInfo in Comediadir: network type "
                     "unsupported (%s).", sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No address type specified in comediadir"
                     " attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
    }
    attr_p->attr.comediadir.conn_info.addrtype = SDP_AT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_ADDR_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_addrtype[i].name,
                        sdp_addrtype[i].strlen) == 0) {
            type_found = TRUE;
        }
        if (type_found == TRUE) {
            if (sdp_p->conf_p->addrtype_supported[i] == TRUE) {
                attr_p->attr.comediadir.conn_info.addrtype = (sdp_addrtype_e)i;
            }
            type_found = FALSE;
        }
    }
    if (attr_p->attr.comediadir.conn_info.addrtype == SDP_AT_UNSUPPORTED) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Conninfo address type unsupported "
                     "(%s).", sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
    }

    
    ptr = sdp_getnextstrtok(ptr, attr_p->attr.comediadir.conn_info.conn_addr, 
                            sizeof(attr_p->attr.comediadir.conn_info.conn_addr), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No conninfo address specified in "
                     "comediadir attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
    }

    
    attr_p->attr.comediadir.src_port  = sdp_getnextnumtok(ptr, &ptr, " \t", 
                                                          &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No src port specified in "
                     "comediadir attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, network %s, addr type %s, address %s "
                  "srcport %u ",
                  sdp_p->debug_str, sdp_get_attr_name(attr_p->type),
                  sdp_get_network_name(attr_p->attr.comediadir.conn_info.nettype),
                  sdp_get_address_name(attr_p->attr.comediadir.conn_info.addrtype),
                  attr_p->attr.comediadir.conn_info.conn_addr,
                  (unsigned int)attr_p->attr.comediadir.src_port);
    }

    if (sdp_p->conf_p->num_invalid_param > 0) {
        return (SDP_INVALID_PARAMETER);
    }
    return (SDP_SUCCESS);
}

sdp_result_e 
sdp_build_attr_comediadir (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                    char **ptr, u16 len)
{
    char         *endbuf_p;

    
    endbuf_p = *ptr + len;
    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s", 
                     sdp_attr[attr_p->type].name,
                     sdp_get_mediadir_role_name(attr_p->attr.
                                                comediadir.role));

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_silencesupp (sdp_t *sdp_p, sdp_attr_t *attr_p,
                                         const char *ptr)
{
    int i;
    sdp_result_e result;
    char tmp[SDP_MAX_STRING_LEN];

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);

    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s No silenceSupp enable value specified, parse failed.",
                      sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (cpr_strncasecmp(tmp, "on", sizeof("on")) == 0) {
        attr_p->attr.silencesupp.enabled = TRUE;
    } else if (cpr_strncasecmp(tmp, "off", sizeof("off")) == 0) {
        attr_p->attr.silencesupp.enabled = FALSE;
    } else if (cpr_strncasecmp(tmp, "-", sizeof("-")) == 0) {
        attr_p->attr.silencesupp.enabled = FALSE;
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: silenceSuppEnable parameter invalid (%s)",
                     sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    

    attr_p->attr.silencesupp.timer =
        (u16)sdp_getnextnumtok_or_null(ptr, &ptr, " \t",
                                       &attr_p->attr.silencesupp.timer_null,
                                       &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Invalid timer value specified for "
                     "silenceSupp attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No silenceSupp pref specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.silencesupp.pref = SDP_SILENCESUPP_PREF_UNKNOWN;
    for (i=0; i < SDP_MAX_SILENCESUPP_PREF; i++) {
        if (cpr_strncasecmp(tmp, sdp_silencesupp_pref[i].name,
                        sdp_silencesupp_pref[i].strlen) == 0) {
            attr_p->attr.silencesupp.pref = (sdp_silencesupp_pref_e)i;
        }
    }
    if (attr_p->attr.silencesupp.pref == SDP_SILENCESUPP_PREF_UNKNOWN) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: silenceSupp pref unrecognized (%s)", 
                     sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No silenceSupp sidUse specified.",
                     sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.silencesupp.siduse = SDP_SILENCESUPP_SIDUSE_UNKNOWN;
    for (i=0; i < SDP_MAX_SILENCESUPP_SIDUSE; i++) {
        if (cpr_strncasecmp(tmp, sdp_silencesupp_siduse[i].name,
                        sdp_silencesupp_siduse[i].strlen) == 0) {
            attr_p->attr.silencesupp.siduse = (sdp_silencesupp_siduse_e)i;
        }
    }
    if (attr_p->attr.silencesupp.siduse == SDP_SILENCESUPP_SIDUSE_UNKNOWN) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: silenceSupp sidUse unrecognized (%s)", 
                     sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    attr_p->attr.silencesupp.fxnslevel =
        (u8)sdp_getnextnumtok_or_null(ptr, &ptr, " \t",
                                      &attr_p->attr.silencesupp.fxnslevel_null,
                                      &result);

    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Invalid fxnslevel value specified for "
                     "silenceSupp attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, enabled %s",
                  sdp_p->debug_str, sdp_get_attr_name(attr_p->type),
                  (attr_p->attr.silencesupp.enabled ? "on" : "off"));
        if (attr_p->attr.silencesupp.timer_null) {
            SDP_PRINT(" timer=-");
        } else {
            SDP_PRINT(" timer=%u,", attr_p->attr.silencesupp.timer);
        }
        SDP_PRINT(" pref=%s, siduse=%s,",
                  sdp_get_silencesupp_pref_name(attr_p->attr.silencesupp.pref),
                  sdp_get_silencesupp_siduse_name(
                                             attr_p->attr.silencesupp.siduse));
        if (attr_p->attr.silencesupp.fxnslevel_null) {
            SDP_PRINT(" fxnslevel=-");
        } else {
            SDP_PRINT(" fxnslevel=%u,", attr_p->attr.silencesupp.fxnslevel);
        }
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_silencesupp (sdp_t *sdp_p, sdp_attr_t *attr_p,
                                         char **ptr, u16 len)
{
    char       *endbuf_p;

    
    endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s ",
                     sdp_attr[attr_p->type].name,
                     (attr_p->attr.silencesupp.enabled ? "on" : "off"));

    if (attr_p->attr.silencesupp.timer_null) {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "-");
    } else {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "%u", attr_p->attr.silencesupp.timer);
    }

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), " %s %s ",
                     sdp_get_silencesupp_pref_name(
                                             attr_p->attr.silencesupp.pref),
                     sdp_get_silencesupp_siduse_name(
                                             attr_p->attr.silencesupp.siduse));

    if (attr_p->attr.silencesupp.fxnslevel_null) {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "-");
    } else {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "%u", attr_p->attr.silencesupp.fxnslevel);
    }

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return (SDP_SUCCESS);
}








tinybool sdp_parse_context_crypto_suite(char * str,  sdp_attr_t *attr_p, sdp_t *sdp_p) {
      






       int i;

           
       for(i=0; i<SDP_SRTP_MAX_NUM_CRYPTO_SUITES; i++) {
	 if (!strcasecmp(sdp_srtp_crypto_suite_array[i].crypto_suite_str, str)) {
	   attr_p->attr.srtp_context.suite = sdp_srtp_crypto_suite_array[i].crypto_suite_val;
	   attr_p->attr.srtp_context.master_key_size_bytes = 
	       sdp_srtp_crypto_suite_array[i].key_size_bytes;
	   attr_p->attr.srtp_context.master_salt_size_bytes = 
	       sdp_srtp_crypto_suite_array[i].salt_size_bytes;
	   return TRUE;  
	 }
       }
       
       if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s No Matching crypto suite for SRTP Context(%s)-'X-crypto:v1' expected",
                      sdp_p->debug_str, str);
       }

       return FALSE;
}


sdp_result_e sdp_build_attr_srtpcontext (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                    char **ptr, u16 len)
{
#define MAX_BASE64_ENCODE_SIZE_BYTES 60
    int          output_len = MAX_BASE64_ENCODE_SIZE_BYTES;
    int		 key_size = attr_p->attr.srtp_context.master_key_size_bytes;
    int		 salt_size = attr_p->attr.srtp_context.master_salt_size_bytes;
    unsigned char  base64_encoded_data[MAX_BASE64_ENCODE_SIZE_BYTES];
    unsigned char  base64_encoded_input[MAX_BASE64_ENCODE_SIZE_BYTES];
    base64_result_t status;
    char     *endbuf_p = *ptr + len;

    output_len = MAX_BASE64_ENCODE_SIZE_BYTES;

    
    bcopy(attr_p->attr.srtp_context.master_key, base64_encoded_input, 
	    key_size );   
    bcopy(attr_p->attr.srtp_context.master_salt,
	    base64_encoded_input + key_size, salt_size );

    if ((status = base64_encode(base64_encoded_input, key_size + salt_size,
		      base64_encoded_data, &output_len)) != BASE64_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s Error: Failure to Base64 Encoded data (%s) ", 
                     sdp_p->debug_str, BASE64_RESULT_TO_STRING(status));
        }
	return (SDP_INVALID_PARAMETER);
    }

    *(base64_encoded_data + output_len) = '\0';

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s inline:%s||\r\n", 
                     sdp_attr[attr_p->type].name, 
                     sdp_srtp_context_crypto_suite[attr_p->attr.srtp_context.suite].name,
		     base64_encoded_data);

    return (SDP_SUCCESS);
}









sdp_result_e sdp_parse_attr_mptime (
    sdp_t *sdp_p,
    sdp_attr_t *attr_p,
    const char *ptr)
{
    u16 i;                      
    sdp_result_e result;        
    tinybool null_ind;          

    




    for (i=0; i<SDP_MAX_PAYLOAD_TYPES; i++) {
        attr_p->attr.mptime.intervals[i] =
            (ushort)sdp_getnextnumtok_or_null(ptr,&ptr," \t",&null_ind,&result);
        if (result != SDP_SUCCESS) {
            break;
        }
        attr_p->attr.mptime.num_intervals++;
    }

    



    if (attr_p->attr.mptime.num_intervals == 0) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No intervals specified for %s attr.",
                     sdp_p->debug_str, sdp_attr[attr_p->type].name);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    



    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, num intervals %u, intervals: ",
                  sdp_p->debug_str, sdp_get_attr_name(attr_p->type),
                  attr_p->attr.mptime.num_intervals);
        for (i=0; i < attr_p->attr.mptime.num_intervals; i++) {
            SDP_PRINT("%u ", attr_p->attr.mptime.intervals[i]);
        }
    }

    return SDP_SUCCESS;
}







sdp_result_e sdp_build_attr_mptime (
    sdp_t *sdp_p,
    sdp_attr_t *attr_p,
    char **ptr,
    u16 len)
{
    u16 i;
    char *endbuf_p;

    



    endbuf_p = *ptr + len;

    


    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:", sdp_attr[attr_p->type].name);

    



    for (i=0; i<attr_p->attr.mptime.num_intervals; i++) {
        if (attr_p->attr.mptime.intervals[i]==0) {
            *ptr += snprintf(*ptr,MAX((endbuf_p - *ptr), 0),"-");
        } else {
            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "%s%u", (i==0)?"":" ", attr_p->attr.mptime.intervals[i]);
        }
    }

    


    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return SDP_SUCCESS;
}



sdp_result_e sdp_parse_attr_x_sidin (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                     const char *ptr)
{
    sdp_result_e  result;
    attr_p->attr.stream_data.x_sidin[0]  = '\0';

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsing a=%s", sdp_p->debug_str,
                     sdp_get_attr_name(attr_p->type));
    }

    
    ptr = sdp_getnextstrtok(ptr, attr_p->attr.stream_data.x_sidin,
                            sizeof(attr_p->attr.stream_data.x_sidin), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No Stream Id incoming specified for "
                     "X-sidin attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, %s", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type), 
                  attr_p->attr.stream_data.x_sidin);
    }
   return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_x_sidin (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                      char **ptr, u16 len)
{
    char *endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s\r\n", 
                     sdp_attr[attr_p->type].name, 
                     attr_p->attr.stream_data.x_sidin);
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_x_sidout (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                      const char *ptr)
{
    sdp_result_e  result;
    attr_p->attr.stream_data.x_sidout[0]  = '\0';

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsing a=%s", sdp_p->debug_str,
                     sdp_get_attr_name(attr_p->type));
    }

    
    ptr = sdp_getnextstrtok(ptr, attr_p->attr.stream_data.x_sidout,
                            sizeof(attr_p->attr.stream_data.x_sidout), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No Stream Id outgoing specified for "
                     "X-sidout attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, %s", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type), 
                  attr_p->attr.stream_data.x_sidout);
    }
   return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_x_sidout (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                      char **ptr, u16 len)
{
    char *endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s\r\n", 
                     sdp_attr[attr_p->type].name, 
                     attr_p->attr.stream_data.x_sidout);
    return (SDP_SUCCESS);
}


sdp_result_e sdp_parse_attr_x_confid (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                      const char *ptr)
{
    sdp_result_e  result;
    attr_p->attr.stream_data.x_confid[0]  = '\0';

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsing a=%s", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type));
    }
    
    
    ptr = sdp_getnextstrtok(ptr, attr_p->attr.stream_data.x_confid,
                            sizeof(attr_p->attr.stream_data.x_confid), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No Conf Id incoming specified for "
                     "X-confid attribute.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    
    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s, %s", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type), 
                  attr_p->attr.stream_data.x_confid);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_x_confid (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                      char **ptr, u16 len)
{
    char *endbuf_p = *ptr + len;

    if (attr_p->attr.stream_data.x_confid[0] != '\0') {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s\r\n", 
                         sdp_attr[attr_p->type].name, 
                         attr_p->attr.stream_data.x_confid);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s X-confid value is not set. Cannot build a=X-confid line\n", 
                      sdp_p->debug_str);
        }
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_attr_group (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                   const char *ptr)
{
    sdp_result_e  result;
    char  tmp[10];
    int i=0;

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsing a=%s", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type));
    }
    
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No group attribute value specified for "
                     "a=group line", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    
    attr_p->attr.stream_data.group_attr = SDP_GROUP_ATTR_UNSUPPORTED;
    for (i=0; i < SDP_MAX_GROUP_ATTR_VAL; i++) {
        if (cpr_strncasecmp(tmp, sdp_group_attr_val[i].name,
                        sdp_group_attr_val[i].strlen) == 0) {
            attr_p->attr.stream_data.group_attr = (sdp_group_attr_e)i;
            break;
        }
    }

    if (attr_p->attr.stream_data.group_attr == SDP_GROUP_ATTR_UNSUPPORTED) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Group attribute type unsupported (%s).", 
                     sdp_p->debug_str, tmp);
        }
    }

    
    



    attr_p->attr.stream_data.num_group_id =0;

    for (i=0; i<SDP_MAX_GROUP_STREAM_ID; i++) {
        attr_p->attr.stream_data.group_id_arr[i] =
            (u16)sdp_getnextnumtok(ptr,&ptr," \t", &result);
        if (result != SDP_SUCCESS) {
            break;
        }
        attr_p->attr.stream_data.num_group_id++;
    }
    
    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed a=%s:%s\n", sdp_p->debug_str,
                  sdp_get_attr_name(attr_p->type), 
                  sdp_get_group_attr_name (attr_p->attr.stream_data.group_attr));
        for (i=0; i < attr_p->attr.stream_data.num_group_id; i++) {
            SDP_PRINT("%s Parsed group line id : %d\n", sdp_p->debug_str,
                      attr_p->attr.stream_data.group_id_arr[i]);
        }
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_group (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                                   char **ptr, u16 len)
{
    int i=0;
    char *endbuf_p = *ptr + len;
    
    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s", 
                     sdp_attr[attr_p->type].name, 
                     sdp_get_group_attr_name (attr_p->attr.stream_data.group_attr));

    for (i=0; i < attr_p->attr.stream_data.num_group_id; i++) {
        if (attr_p->attr.stream_data.group_id_arr[i] > 0) {
            *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), " %u", 
			     attr_p->attr.stream_data.group_id_arr[i]);
        }
    }

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return (SDP_SUCCESS);

}





sdp_result_e sdp_parse_attr_source_filter (sdp_t *sdp_p, sdp_attr_t *attr_p,
                                           const char *ptr)
{
    int i;
    sdp_result_e result;
    char tmp[SDP_MAX_STRING_LEN];

    attr_p->attr.source_filter.mode = SDP_FILTER_MODE_NOT_PRESENT;
    attr_p->attr.source_filter.nettype = SDP_NT_UNSUPPORTED;
    attr_p->attr.source_filter.addrtype = SDP_AT_UNSUPPORTED;
    attr_p->attr.source_filter.dest_addr[0] = '\0';
    attr_p->attr.source_filter.num_src_addr = 0;

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No src filter attribute value specified for "
                     "a=source-filter line", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    for (i = 0; i < SDP_MAX_FILTER_MODE; i++) {
        if (cpr_strncasecmp(tmp, sdp_src_filter_mode_val[i].name,
                        sdp_src_filter_mode_val[i].strlen) == 0) {
            attr_p->attr.source_filter.mode = (sdp_src_filter_mode_e)i;
            break;
        }
    }
    if (attr_p->attr.source_filter.mode == SDP_FILTER_MODE_NOT_PRESENT) {
        
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Invalid src filter mode for a=source-filter "
                     "line", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    for (i = 0; i < SDP_MAX_NETWORK_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_nettype[i].name,
                        sdp_nettype[i].strlen) == 0) {
            if (sdp_p->conf_p->nettype_supported[i] == TRUE) {
                attr_p->attr.source_filter.nettype = (sdp_nettype_e)i;
            }
        }
    }
    if (attr_p->attr.source_filter.nettype == SDP_NT_UNSUPPORTED) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Network type unsupported "
                     "(%s) for a=source-filter", sdp_p->debug_str, tmp);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    for (i = 0; i < SDP_MAX_ADDR_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_addrtype[i].name,
                        sdp_addrtype[i].strlen) == 0) {
            if (sdp_p->conf_p->addrtype_supported[i] == TRUE) {
                attr_p->attr.source_filter.addrtype = (sdp_addrtype_e)i;
            }
        }
    }
    if (attr_p->attr.source_filter.addrtype == SDP_AT_UNSUPPORTED) {
        if (strncmp(tmp, "*", 1) == 0) {
            attr_p->attr.source_filter.addrtype = SDP_AT_FQDN;
        } else {
            if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
                SDP_WARN("%s Warning: Address type unsupported "
                         "(%s) for a=source-filter", sdp_p->debug_str, tmp);
            }
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
    }

    
    ptr = sdp_getnextstrtok(ptr, attr_p->attr.source_filter.dest_addr, 
                            sizeof(attr_p->attr.source_filter.dest_addr), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s No filter destination address specified for "
                      "a=source-filter", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    for (i = 0; i < SDP_MAX_SRC_ADDR_LIST; i++) {
        ptr = sdp_getnextstrtok(ptr, attr_p->attr.source_filter.src_list[i], 
                                sizeof(attr_p->attr.source_filter.src_list[i]), " \t", &result);
        if (result != SDP_SUCCESS) {
            break;
        }
        attr_p->attr.source_filter.num_src_addr++;
    }
    if (attr_p->attr.source_filter.num_src_addr == 0) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No source list provided "
                     "for a=source-filter", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_source_filter (sdp_t *sdp_p, sdp_attr_t *attr_p,
                                      char **ptr, u16 len)
{
    int i = 0;
    char *endbuf_p = *ptr + len;

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s %s %s %s", 
         sdp_get_attr_name(attr_p->type),
         sdp_get_src_filter_mode_name(attr_p->attr.source_filter.mode),
         sdp_get_network_name(attr_p->attr.source_filter.nettype),
         sdp_get_address_name(attr_p->attr.source_filter.addrtype),
         attr_p->attr.source_filter.dest_addr);

    for (i = 0; i < attr_p->attr.source_filter.num_src_addr; i++) {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0)," %s",
                         attr_p->attr.source_filter.src_list[i]);
    }

    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "\r\n");

    return (SDP_SUCCESS);
}




sdp_result_e sdp_parse_attr_rtcp_unicast (sdp_t *sdp_p, sdp_attr_t *attr_p,
                                          const char *ptr)
{
    sdp_result_e result;
    u32 i;
    char tmp[SDP_MAX_STRING_LEN];

    attr_p->attr.u32_val = SDP_RTCP_UNICAST_MODE_NOT_PRESENT;

    memset(tmp, 0, sizeof(tmp));
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No rtcp unicast mode specified for "
                     "a=rtcp-unicast line", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    for (i = 0; i < SDP_RTCP_MAX_UNICAST_MODE;  i++) {
        if (cpr_strncasecmp(tmp, sdp_rtcp_unicast_mode_val[i].name,
                        sdp_rtcp_unicast_mode_val[i].strlen) == 0) {
            attr_p->attr.u32_val = i;
            break;
        }
    }
    if (attr_p->attr.u32_val == SDP_RTCP_UNICAST_MODE_NOT_PRESENT) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: Invalid rtcp unicast mode for "
                     "a=rtcp-unicast line", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_rtcp_unicast (sdp_t *sdp_p, sdp_attr_t *attr_p,
                                          char **ptr, u16 len)
{
    char *endbuf_p = *ptr + len;

    if (attr_p->attr.u32_val >= SDP_RTCP_MAX_UNICAST_MODE) {
        return (SDP_INVALID_PARAMETER);
    }
    *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%s\r\n", 
                     sdp_get_attr_name(attr_p->type),
                     sdp_get_rtcp_unicast_mode_name(
                     (sdp_rtcp_unicast_mode_e)attr_p->attr.u32_val));

    return (SDP_SUCCESS);
}
















 
tinybool 
store_sdescriptions_mki_or_lifetime (char *buf, sdp_attr_t *attr_p) 
{
   
    tinybool  result;
    u16       mkiLen;
    char      mkiValue[SDP_SRTP_MAX_MKI_SIZE_BYTES];
    
    
    if (strstr(buf, ":")) {
        result = verify_sdescriptions_mki(buf, mkiValue, &mkiLen);
	if (result) {
	    attr_p->attr.srtp_context.mki_size_bytes = mkiLen;
	    sstrncpy((char*)attr_p->attr.srtp_context.mki, mkiValue,
	             SDP_SRTP_MAX_MKI_SIZE_BYTES);
	}
	
    } else {
        result =  verify_sdescriptions_lifetime(buf);
	if (result) {
	    sstrncpy((char*)attr_p->attr.srtp_context.master_key_lifetime, buf,
	             SDP_SRTP_MAX_LIFETIME_BYTES);
	}
    }
    
    return result;

}























 
 
tinybool 
sdp_parse_sdescriptions_key_param (const char *str, sdp_attr_t *attr_p, 
                                   sdp_t *sdp_p) 
{
    char            buf[SDP_MAX_STRING_LEN],
                    base64decodeData[SDP_MAX_STRING_LEN];
    const char      *ptr;
    sdp_result_e    result = SDP_SUCCESS;
    tinybool        keyFound = FALSE;
    int             len,
                    keySize,
    		    saltSize;
    base64_result_t status;
  
    ptr = str;
    if (cpr_strncasecmp(ptr, "inline:", 7) != 0) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s Could not find keyword inline", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return FALSE;
    }
    
    
    ptr = ptr + 7;
    ptr = sdp_getnextstrtok(ptr, buf, sizeof(buf), "|", &result);
    while (result == SDP_SUCCESS) {
        
        if (keyFound == FALSE) {
	    keyFound = TRUE;
	    len = SDP_MAX_STRING_LEN;
	    


	    status = base64_decode((unsigned char *)buf, strlen(buf), 
	                           (unsigned char *)base64decodeData, &len);
				   
	    if (status != BASE64_SUCCESS) {
	        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
		    SDP_ERROR("%s key-salt error decoding buffer: %s",
			      sdp_p->debug_str, BASE64_RESULT_TO_STRING(status));
	        }
	        return FALSE;
	   
	    }
	   
	    keySize = attr_p->attr.srtp_context.master_key_size_bytes;
	    saltSize = attr_p->attr.srtp_context.master_salt_size_bytes;
	   
	    if (len != keySize + saltSize) {
		      
	        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
		    SDP_ERROR("%s key-salt size doesn't match: (%d, %d, %d)",
		              sdp_p->debug_str, len, keySize, saltSize);
	        }

	        return(FALSE);
		      
	    }
	   
	    bcopy(base64decodeData, attr_p->attr.srtp_context.master_key, keySize);
		 
	    bcopy(base64decodeData + keySize,
	          attr_p->attr.srtp_context.master_salt, saltSize);
	   
	    
	    SDP_SRTP_CONTEXT_SET_MASTER_KEY
	             (attr_p->attr.srtp_context.selection_flags);
	    SDP_SRTP_CONTEXT_SET_MASTER_SALT
	             (attr_p->attr.srtp_context.selection_flags);
		     
       } else if (store_sdescriptions_mki_or_lifetime(buf, attr_p) == FALSE) {
           return FALSE;
       }
       
       
       ptr = sdp_getnextstrtok(ptr, buf, sizeof(buf), "|", &result);
    }
   
    
    if (keyFound == FALSE) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s Could not find sdescriptions key", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return FALSE;
    }
   
    return TRUE;
       
}











 
sdp_result_e
sdp_build_attr_sdescriptions (sdp_t *sdp_p, sdp_attr_t *attr_p, 
                              char **ptr, u16 len)
{
    
    unsigned char  base64_encoded_data[MAX_BASE64_STRING_LEN];
    unsigned char  base64_encoded_input[MAX_BASE64_STRING_LEN];
    int            keySize,
                   saltSize,
		   outputLen;
    base64_result_t status;
    char *endbuf_p = *ptr + len;
    
    keySize = attr_p->attr.srtp_context.master_key_size_bytes;
    saltSize = attr_p->attr.srtp_context.master_salt_size_bytes;
    
    
    bcopy(attr_p->attr.srtp_context.master_key, 
          base64_encoded_input, keySize);
	  
    bcopy(attr_p->attr.srtp_context.master_salt, 
          base64_encoded_input + keySize, saltSize);
	
    outputLen = MAX_BASE64_STRING_LEN;  
    status = base64_encode(base64_encoded_input, keySize + saltSize,
		           base64_encoded_data, &outputLen);
			   
    if (status != BASE64_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s Error: Failure to Base64 Encoded data (%s) ", 
                       sdp_p->debug_str, BASE64_RESULT_TO_STRING(status));
        }
	return (SDP_INVALID_PARAMETER);
    
    }
    
    base64_encoded_data[outputLen] = 0;
    
    


     
    
    if (attr_p->attr.srtp_context.master_key_lifetime[0] != 0 && 
        attr_p->attr.srtp_context.mki[0] != 0) {
	
	*ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%d %s inline:%s|%s|%s:%d\r\n",
	                 sdp_attr[attr_p->type].name, 
			 attr_p->attr.srtp_context.tag,
                         sdp_srtp_context_crypto_suite[attr_p->attr.srtp_context.suite].name,
			 base64_encoded_data,
			 attr_p->attr.srtp_context.master_key_lifetime,
			 attr_p->attr.srtp_context.mki,
			 attr_p->attr.srtp_context.mki_size_bytes);
			 
	return SDP_SUCCESS;
	
    }
    
    


     
    if (attr_p->attr.srtp_context.master_key_lifetime[0] != 0) {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%d %s inline:%s|%s\r\n",
	                 sdp_attr[attr_p->type].name, 
			 attr_p->attr.srtp_context.tag,
                         sdp_srtp_context_crypto_suite[attr_p->attr.srtp_context.suite].name,
			 base64_encoded_data,
			 attr_p->attr.srtp_context.master_key_lifetime);
    
    } else if (attr_p->attr.srtp_context.mki[0] != 0) {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%d %s inline:%s|%s:%d\r\n",
	                 sdp_attr[attr_p->type].name, 
			 attr_p->attr.srtp_context.tag,
                         sdp_srtp_context_crypto_suite[attr_p->attr.srtp_context.suite].name,
			 base64_encoded_data,
			 attr_p->attr.srtp_context.mki,
			 attr_p->attr.srtp_context.mki_size_bytes);
    
    } else {
        *ptr += snprintf(*ptr, MAX((endbuf_p - *ptr), 0), "a=%s:%d %s inline:%s\r\n",
	                 sdp_attr[attr_p->type].name, 
			 attr_p->attr.srtp_context.tag,
                         sdp_srtp_context_crypto_suite[attr_p->attr.srtp_context.suite].name,
			 base64_encoded_data);
    
    }
       
    return SDP_SUCCESS;

}




















sdp_result_e 
sdp_parse_attr_srtp (sdp_t *sdp_p, sdp_attr_t *attr_p,
                     const char *ptr, sdp_attr_e vtype)
{

    char         tmp[SDP_MAX_STRING_LEN];
    sdp_result_e result = SDP_FAILURE;
    int          k = 0;
       
    
    attr_p->attr.srtp_context.master_key_lifetime[0] = 0;
    attr_p->attr.srtp_context.mki[0] = 0;
    
     
    SDP_SRTP_CONTEXT_SET_ENCRYPT_AUTHENTICATE
             (attr_p->attr.srtp_context.selection_flags);
	     
    
    if (vtype == SDP_ATTR_SDESCRIPTIONS) {
        attr_p->attr.srtp_context.tag = 
                sdp_getnextnumtok(ptr, &ptr, " \t", &result);

        if (result != SDP_SUCCESS) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                SDP_ERROR("%s Could not find sdescriptions tag",
		          sdp_p->debug_str);
            }
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
       
        }
    }
    
    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s Could not find sdescriptions crypto suite", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
       
    if (!sdp_parse_context_crypto_suite(tmp, attr_p, sdp_p)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s Unsupported crypto suite", sdp_p->debug_str);
        }
	    return (SDP_INVALID_PARAMETER);
    }
   
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s Could not find sdescriptions key params", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
   
    if (!sdp_parse_sdescriptions_key_param(tmp, attr_p, sdp_p)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s Failed to parse key-params", sdp_p->debug_str);
        }
        return (SDP_INVALID_PARAMETER);
    } 
    
    




    



    while (*ptr && *ptr != '\n' && *ptr != '\r' && k < SDP_MAX_STRING_LEN) {
         tmp[k++] = *ptr++;
    }
    
    if ((k) && (k < SDP_MAX_STRING_LEN)) {
        tmp[k] = 0;
        attr_p->attr.srtp_context.session_parameters = cpr_strdup(tmp);
    }
       
    return SDP_SUCCESS;  
       
}





 
sdp_result_e 
sdp_parse_attr_sdescriptions (sdp_t *sdp_p, sdp_attr_t *attr_p,
                              const char *ptr)
{

   return sdp_parse_attr_srtp(sdp_p, attr_p, ptr, 
                              SDP_ATTR_SDESCRIPTIONS);

}





 
sdp_result_e sdp_parse_attr_srtpcontext (sdp_t *sdp_p, sdp_attr_t *attr_p,
                                         const char *ptr)
{
   
    return sdp_parse_attr_srtp(sdp_p, attr_p, ptr, 
                               SDP_ATTR_SRTP_CONTEXT);    
}


sdp_result_e sdp_build_attr_from_str (sdp_t *sdp_p, const char *str,
                                      char **ptr, u16 len)
{
    *ptr += snprintf(*ptr, len, "a=%s\r\n", str);

    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_attr_ice_attr (sdp_t *sdp_p, sdp_attr_t *attr_p,
                                          char **ptr, u16 len) {
    return sdp_build_attr_from_str(sdp_p, attr_p->attr.ice_attr, ptr, len);
}


sdp_result_e sdp_parse_attr_ice_attr (sdp_t *sdp_p, sdp_attr_t *attr_p, const char *ptr) {
    sdp_result_e  result;
    char tmp[SDP_MAX_STRING_LEN];

    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), "\r\n", &result);
    if (result != SDP_SUCCESS){

      if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
          SDP_WARN("%s Warning: problem parsing ice attribute ", sdp_p->debug_str);
      }
      sdp_p->conf_p->num_invalid_param++;
      return (SDP_INVALID_PARAMETER);
    }
    
    
    snprintf(attr_p->attr.ice_attr, sizeof(attr_p->attr.ice_attr),
      "%s:%s", sdp_get_attr_name(attr_p->type), tmp);
    
    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
      SDP_PRINT("%s Parsed a=%s, %s", sdp_p->debug_str, sdp_get_attr_name(attr_p->type), tmp);
    }
    return (SDP_SUCCESS);
}


sdp_result_e sdp_parse_attr_fingerprint_attr (sdp_t *sdp_p, sdp_attr_t *attr_p,
                                           const char *ptr)
{
    sdp_result_e  result;

    ptr = sdp_getnextstrtok(ptr, attr_p->attr.string_val, sizeof(attr_p->attr.string_val), "\r\n", &result);

    if (result != SDP_SUCCESS) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            SDP_WARN("%s Warning: No string token found for %s attribute",
                     sdp_p->debug_str, sdp_get_attr_name(attr_p->type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Parsed a=%s, %s", sdp_p->debug_str,
                      sdp_get_attr_name(attr_p->type),
                      attr_p->attr.string_val);
        }
        return (SDP_SUCCESS);
    }
}

sdp_result_e sdp_build_attr_rtcp_mux_attr (sdp_t *sdp_p, sdp_attr_t *attr_p,
                                          char **ptr, u16 len) {
    return sdp_build_attr_from_str(sdp_p, "rtcp-mux", ptr, len);
}

sdp_result_e sdp_parse_attr_rtcp_mux_attr (sdp_t *sdp_p, sdp_attr_t *attr_p, const char *ptr) {
    attr_p->attr.boolean_val = TRUE;

    return (SDP_SUCCESS);
}
