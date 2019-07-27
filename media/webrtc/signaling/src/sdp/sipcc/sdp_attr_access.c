



#include "sdp_os_defs.h"
#include "sdp.h"
#include "sdp_private.h"

#include "CSFLog.h"

static const char* logTag = "sdp_attr_access";


























































































































































sdp_result_e sdp_add_new_attr (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                               sdp_attr_e attr_type, uint16_t *inst_num)
{
    uint16_t          i;
    sdp_mca_t   *mca_p;
    sdp_mca_t   *cap_p;
    sdp_attr_t  *attr_p;
    sdp_attr_t  *new_attr_p;
    sdp_attr_t  *prev_attr_p=NULL;
    sdp_fmtp_t  *fmtp_p;
    sdp_comediadir_t  *comediadir_p;

    *inst_num = 0;

    if ((cap_num != 0) &&
        ((attr_type == SDP_ATTR_X_CAP) || (attr_type == SDP_ATTR_X_CPAR) ||
         (attr_type == SDP_ATTR_X_SQN) || (attr_type == SDP_ATTR_CDSC) ||
         (attr_type == SDP_ATTR_CPAR) || (attr_type == SDP_ATTR_SQN))) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            CSFLogDebug(logTag, "%s Warning: Invalid attribute type for X-cpar/cdsc "
                     "parameter.", sdp_p->debug_str);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    if (level == SDP_SESSION_LEVEL) {
        switch (attr_type) {
            case SDP_ATTR_RTCP:
            case SDP_ATTR_LABEL:
                return (SDP_INVALID_MEDIA_LEVEL);

            default:
                break;
        }
    }

    new_attr_p = (sdp_attr_t *)SDP_MALLOC(sizeof(sdp_attr_t));
    if (new_attr_p == NULL) {
        sdp_p->conf_p->num_no_resource++;
        return (SDP_NO_RESOURCE);
    }

    new_attr_p->type = attr_type;
    new_attr_p->next_p = NULL;

    
    if ((new_attr_p->type == SDP_ATTR_X_CAP) ||
        (new_attr_p->type == SDP_ATTR_CDSC)) {
        new_attr_p->attr.cap_p = (sdp_mca_t *)SDP_MALLOC(sizeof(sdp_mca_t));
        if (new_attr_p->attr.cap_p == NULL) {
            sdp_free_attr(new_attr_p);
            sdp_p->conf_p->num_no_resource++;
            return (SDP_NO_RESOURCE);
        }
    } else if (new_attr_p->type == SDP_ATTR_FMTP) {
        fmtp_p = &(new_attr_p->attr.fmtp);
        fmtp_p->fmtp_format = SDP_FMTP_UNKNOWN_TYPE;
        
        fmtp_p->packetization_mode = SDP_INVALID_PACKETIZATION_MODE_VALUE;
        fmtp_p->level_asymmetry_allowed = SDP_INVALID_LEVEL_ASYMMETRY_ALLOWED_VALUE;
        fmtp_p->annexb_required = FALSE;
        fmtp_p->annexa_required = FALSE;
        fmtp_p->maxval = 0;
        fmtp_p->bitrate = 0;
        fmtp_p->cif = 0;
        fmtp_p->qcif = 0;
        fmtp_p->profile = SDP_INVALID_VALUE;
        fmtp_p->level = SDP_INVALID_VALUE;
        fmtp_p->parameter_add = SDP_FMTP_UNUSED;
        fmtp_p->usedtx = SDP_FMTP_UNUSED;
        fmtp_p->stereo = SDP_FMTP_UNUSED;
        fmtp_p->useinbandfec = SDP_FMTP_UNUSED;
        fmtp_p->cbr = SDP_FMTP_UNUSED;
        for (i=0; i < SDP_NE_NUM_BMAP_WORDS; i++) {
            fmtp_p->bmap[i] = 0;
        }
    } else if ((new_attr_p->type == SDP_ATTR_RTPMAP) ||
               (new_attr_p->type == SDP_ATTR_SPRTMAP)) {
        new_attr_p->attr.transport_map.num_chan = 1;
    } else if (new_attr_p->type == SDP_ATTR_DIRECTION) {
        comediadir_p = &(new_attr_p->attr.comediadir);
        comediadir_p->role = SDP_MEDIADIR_ROLE_PASSIVE;
        comediadir_p->conn_info_present = FALSE;
    } else if (new_attr_p->type == SDP_ATTR_MPTIME) {
        sdp_mptime_t *mptime = &(new_attr_p->attr.mptime);
        mptime->num_intervals = 0;
    }

    if (cap_num == 0) {
        
        if (level == SDP_SESSION_LEVEL) {
            if (sdp_p->sess_attrs_p == NULL) {
                sdp_p->sess_attrs_p = new_attr_p;
            } else {
                for (attr_p = sdp_p->sess_attrs_p;
                     attr_p != NULL;
                     prev_attr_p = attr_p, attr_p = attr_p->next_p) {
                    
                    if (attr_p->type == attr_type) {
                        (*inst_num)++;
                    }
                }
                prev_attr_p->next_p = new_attr_p;
            }
        } else {
            mca_p = sdp_find_media_level(sdp_p, level);
            if (mca_p == NULL) {
                sdp_free_attr(new_attr_p);
                sdp_p->conf_p->num_invalid_param++;
                return (SDP_INVALID_PARAMETER);
            }
            if (mca_p->media_attrs_p == NULL) {
                mca_p->media_attrs_p = new_attr_p;
            } else {
                for (attr_p = mca_p->media_attrs_p;
                     attr_p != NULL;
                     prev_attr_p = attr_p, attr_p = attr_p->next_p) {
                    
                    if (attr_p->type == attr_type) {
                        (*inst_num)++;
                    }
                }
                prev_attr_p->next_p = new_attr_p;
            }
        }
    } else {
        
        attr_p = sdp_find_capability(sdp_p, level, cap_num);
        if (attr_p == NULL) {
            sdp_free_attr(new_attr_p);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
        cap_p = attr_p->attr.cap_p;
        if (cap_p->media_attrs_p == NULL) {
            cap_p->media_attrs_p = new_attr_p;
        } else {
            for (attr_p = cap_p->media_attrs_p;
                 attr_p != NULL;
                 prev_attr_p = attr_p, attr_p = attr_p->next_p) {
                
                if (attr_p->type == attr_type) {
                    (*inst_num)++;
                }
            }
            prev_attr_p->next_p = new_attr_p;
        }
    }

    
    (*inst_num)++;
    return (SDP_SUCCESS);
}














sdp_result_e sdp_attr_num_instances (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                     sdp_attr_e attr_type, uint16_t *num_attr_inst)
{
    sdp_attr_t  *attr_p;
    sdp_result_e rc;
    static char  fname[] = "attr_num_instances";

    *num_attr_inst = 0;

    rc = sdp_find_attr_list(sdp_p, level, cap_num, &attr_p, fname);
    if (rc == SDP_SUCCESS) {
        

        for (; attr_p != NULL; attr_p = attr_p->next_p) {
            if (attr_p->type == attr_type) {
                (*num_attr_inst)++;
            }
        }

    }

    return (rc);
}










void sdp_free_attr (sdp_attr_t *attr_p)
{
    sdp_mca_t   *cap_p;
    sdp_attr_t  *cpar_p;
    sdp_attr_t  *next_cpar_p;
    int          i;

    

    if ((attr_p->type == SDP_ATTR_X_CAP) ||
        (attr_p->type == SDP_ATTR_CDSC)) {
        cap_p = attr_p->attr.cap_p;
        if (cap_p != NULL) {
            for (cpar_p = cap_p->media_attrs_p; cpar_p != NULL;) {
                next_cpar_p = cpar_p->next_p;
                sdp_free_attr(cpar_p);
                cpar_p = next_cpar_p;
            }
            SDP_FREE(cap_p);
        }
    } else if ((attr_p->type == SDP_ATTR_SDESCRIPTIONS) ||
              (attr_p->type == SDP_ATTR_SRTP_CONTEXT)) {
              SDP_FREE(attr_p->attr.srtp_context.session_parameters);
    }


    if (attr_p->type == SDP_ATTR_GROUP) {
        for (i = 0; i < attr_p->attr.stream_data.num_group_id; i++) {
            SDP_FREE(attr_p->attr.stream_data.group_ids[i]);
        }
    } else if (attr_p->type == SDP_ATTR_MSID_SEMANTIC) {
        for (i = 0; i < SDP_MAX_MEDIA_STREAMS; ++i) {
            SDP_FREE(attr_p->attr.msid_semantic.msids[i]);
        }
    }

    
    SDP_FREE(attr_p);

}



















sdp_result_e sdp_find_attr_list (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                 sdp_attr_t **attr_p, char *fname)
{
    sdp_mca_t   *mca_p;
    sdp_mca_t   *cap_p;
    sdp_attr_t  *cap_attr_p;

    
    *attr_p = NULL;

    if (cap_num == 0) {
        
        if (level == SDP_SESSION_LEVEL) {
            *attr_p = sdp_p->sess_attrs_p;
        } else {
            mca_p = sdp_find_media_level(sdp_p, level);
            if (mca_p == NULL) {
                sdp_p->conf_p->num_invalid_param++;
                return (SDP_INVALID_PARAMETER);
            }
            *attr_p = mca_p->media_attrs_p;
        }
    } else {
        
        cap_attr_p = sdp_find_capability(sdp_p, level, cap_num);
        if (cap_attr_p == NULL) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s %s, invalid capability %u at "
                          "level %u specified.", sdp_p->debug_str, fname,
                          (unsigned)cap_num, (unsigned)level);
            }
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_CAPABILITY);
        }
        cap_p = cap_attr_p->attr.cap_p;
        *attr_p = cap_p->media_attrs_p;
    }

    return (SDP_SUCCESS);
}


int sdp_find_fmtp_inst (sdp_t *sdp_p, uint16_t level, uint16_t payload_num)
{
    uint16_t          attr_count=0;
    sdp_mca_t   *mca_p;
    sdp_attr_t  *attr_p;

    
    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
      return (-1);
    }
    for (attr_p = mca_p->media_attrs_p; attr_p != NULL;
         attr_p = attr_p->next_p) {
      if (attr_p->type == SDP_ATTR_FMTP) {
        attr_count++;
        if (attr_p->attr.fmtp.payload_num == payload_num) {
          return (attr_count);
        }
      }
    }

    return (-1);

}















sdp_attr_t *sdp_find_attr (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                           sdp_attr_e attr_type, uint16_t inst_num)
{
    uint16_t          attr_count=0;
    sdp_mca_t   *mca_p;
    sdp_mca_t   *cap_p;
    sdp_attr_t  *attr_p;

    if (inst_num < 1) {
        return (NULL);
    }

    if (cap_num == 0) {
        if (level == SDP_SESSION_LEVEL) {
            for (attr_p = sdp_p->sess_attrs_p; attr_p != NULL;
                 attr_p = attr_p->next_p) {
                if (attr_p->type == attr_type) {
                    attr_count++;
                    if (attr_count == inst_num) {
                        return (attr_p);
                    }
                }
            }
        } else {  
            mca_p = sdp_find_media_level(sdp_p, level);
            if (mca_p == NULL) {
                return (NULL);
            }
            for (attr_p = mca_p->media_attrs_p; attr_p != NULL;
                 attr_p = attr_p->next_p) {
                if (attr_p->type == attr_type) {
                    attr_count++;
                    if (attr_count == inst_num) {
                        return (attr_p);
                    }
                }
            }
        }  
    } else {
        
        attr_p = sdp_find_capability(sdp_p, level, cap_num);
        if (attr_p == NULL) {
            return (NULL);
        }
        cap_p = attr_p->attr.cap_p;
        
        for (attr_p = cap_p->media_attrs_p; attr_p != NULL;
             attr_p = attr_p->next_p) {
            if (attr_p->type == attr_type) {
                attr_count++;
                if (attr_count == inst_num) {
                    return (attr_p);
                }
            }
        }
    }

    return (NULL);
}










sdp_attr_t *sdp_find_capability (sdp_t *sdp_p, uint16_t level, uint8_t cap_num)
{
    uint8_t           cur_cap_num=0;
    sdp_mca_t   *mca_p;
    sdp_mca_t   *cap_p;
    sdp_attr_t  *attr_p;

    if (level == SDP_SESSION_LEVEL) {
        for (attr_p = sdp_p->sess_attrs_p; attr_p != NULL;
             attr_p = attr_p->next_p) {
            if ((attr_p->type == SDP_ATTR_X_CAP) ||
                (attr_p->type == SDP_ATTR_CDSC)) {
                cap_p = attr_p->attr.cap_p;
                cur_cap_num += cap_p->num_payloads;
                if (cap_num <= cur_cap_num) {
                    
                    return (attr_p);
                }
            }
        }
    } else {  
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (NULL);
        }
        for (attr_p = mca_p->media_attrs_p; attr_p != NULL;
             attr_p = attr_p->next_p) {
            if ((attr_p->type == SDP_ATTR_X_CAP) ||
                (attr_p->type == SDP_ATTR_CDSC)) {
                cap_p = attr_p->attr.cap_p;
                cur_cap_num += cap_p->num_payloads;
                if (cap_num <= cur_cap_num) {
                    
                    return (attr_p);
                }
            }
        }
    }

    
    if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
        CSFLogError(logTag, "%s Unable to find specified capability (level %u, "
                  "cap_num %u).", sdp_p->debug_str, (unsigned)level, (unsigned)cap_num);
    }
    sdp_p->conf_p->num_invalid_param++;
    return (NULL);
}













tinybool sdp_attr_valid (sdp_t *sdp_p, sdp_attr_e attr_type, uint16_t level,
                         uint8_t cap_num, uint16_t inst_num)
{
    if (sdp_find_attr(sdp_p, level, cap_num, attr_type, inst_num) == NULL) {
        return (FALSE);
    } else {
        return (TRUE);
    }
}













uint32_t sdp_attr_line_number (sdp_t *sdp_p, sdp_attr_e attr_type, uint16_t level,
                          uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, attr_type, inst_num);
    if (attr_p == NULL) {
        return 0;
    } else {
        return attr_p->line_number;
    }
}

static boolean sdp_attr_is_simple_string(sdp_attr_e attr_type) {
    if ((attr_type != SDP_ATTR_BEARER) &&
        (attr_type != SDP_ATTR_CALLED) &&
        (attr_type != SDP_ATTR_CONN_TYPE) &&
        (attr_type != SDP_ATTR_DIALED) &&
        (attr_type != SDP_ATTR_DIALING) &&
        (attr_type != SDP_ATTR_FRAMING) &&
        (attr_type != SDP_ATTR_MID) &&
        (attr_type != SDP_ATTR_X_SIDIN) &&
        (attr_type != SDP_ATTR_X_SIDOUT)&&
        (attr_type != SDP_ATTR_X_CONFID) &&
        (attr_type != SDP_ATTR_LABEL) &&
        (attr_type != SDP_ATTR_IDENTITY) &&
        (attr_type != SDP_ATTR_ICE_OPTIONS)) {
      return FALSE;
    }
    return TRUE;
}


















const char *sdp_attr_get_simple_string (sdp_t *sdp_p, sdp_attr_e attr_type,
                                        uint16_t level, uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    if (!sdp_attr_is_simple_string(attr_type)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Attribute type is not a simple string (%s)",
                      sdp_p->debug_str, sdp_get_attr_name(attr_type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    }

    attr_p = sdp_find_attr(sdp_p, level, cap_num, attr_type, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Attribute %s, level %u instance %u not found.",
                      sdp_p->debug_str, sdp_get_attr_name(attr_type),
                      (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    } else {
        return (attr_p->attr.string_val);
    }
}

static boolean sdp_attr_is_simple_u32(sdp_attr_e attr_type) {
    if ((attr_type != SDP_ATTR_EECID) &&
        (attr_type != SDP_ATTR_PTIME) &&
        (attr_type != SDP_ATTR_MAXPTIME) &&
        (attr_type != SDP_ATTR_T38_VERSION) &&
        (attr_type != SDP_ATTR_T38_MAXBITRATE) &&
        (attr_type != SDP_ATTR_T38_MAXBUFFER) &&
        (attr_type != SDP_ATTR_T38_MAXDGRAM) &&
        (attr_type != SDP_ATTR_X_SQN) &&
        (attr_type != SDP_ATTR_TC1_PAYLOAD_BYTES) &&
        (attr_type != SDP_ATTR_TC1_WINDOW_SIZE) &&
        (attr_type != SDP_ATTR_TC2_PAYLOAD_BYTES) &&
        (attr_type != SDP_ATTR_TC2_WINDOW_SIZE) &&
        (attr_type != SDP_ATTR_FRAMERATE)) {
        return FALSE;
    }

    return TRUE;
}





















uint32_t sdp_attr_get_simple_u32 (sdp_t *sdp_p, sdp_attr_e attr_type, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    if (!sdp_attr_is_simple_u32(attr_type)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Attribute type is not a simple uint32_t (%s)",
                      sdp_p->debug_str, sdp_get_attr_name(attr_type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    }

    attr_p = sdp_find_attr(sdp_p, level, cap_num, attr_type, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Attribute %s, level %u instance %u not found.",
                      sdp_p->debug_str, sdp_get_attr_name(attr_type),
                      (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.u32_val);
    }
}




















tinybool sdp_attr_get_simple_boolean (sdp_t *sdp_p, sdp_attr_e attr_type,
                                      uint16_t level, uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    if ((attr_type != SDP_ATTR_T38_FILLBITREMOVAL) &&
        (attr_type != SDP_ATTR_T38_TRANSCODINGMMR) &&
        (attr_type != SDP_ATTR_T38_TRANSCODINGJBIG) &&
        (attr_type != SDP_ATTR_TMRGWXID)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Attribute type is not a simple boolean (%s)",
                      sdp_p->debug_str, sdp_get_attr_name(attr_type));
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    }

    attr_p = sdp_find_attr(sdp_p, level, cap_num, attr_type, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Attribute %s, level %u instance %u not found.",
                      sdp_p->debug_str, sdp_get_attr_name(attr_type),
                      (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.boolean_val);
    }
}














const char*
sdp_attr_get_maxprate (sdp_t *sdp_p, uint16_t level, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_MAXPRATE, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Attribute %s, level %u instance %u not found.",
                      sdp_p->debug_str, sdp_get_attr_name(SDP_ATTR_MAXPRATE),
                      (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    } else {
        return (attr_p->attr.string_val);
    }
}












sdp_t38_ratemgmt_e sdp_attr_get_t38ratemgmt (sdp_t *sdp_p, uint16_t level,
                                             uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_T38_RATEMGMT, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s t38ratemgmt attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_T38_UNKNOWN_RATE);
    } else {
        return (attr_p->attr.t38ratemgmt);
    }
}












sdp_t38_udpec_e sdp_attr_get_t38udpec (sdp_t *sdp_p, uint16_t level,
                                       uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_T38_UDPEC, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s t38udpec attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_T38_UDPEC_UNKNOWN);
    } else {
        return (attr_p->attr.t38udpec);
    }
}













sdp_direction_e sdp_get_media_direction (sdp_t *sdp_p, uint16_t level,
                                         uint8_t cap_num)
{
    sdp_mca_t   *mca_p;
    sdp_attr_t  *attr_p;
    sdp_direction_e direction = SDP_DIRECTION_SENDRECV;

    if (cap_num == 0) {
        
        if (level == SDP_SESSION_LEVEL) {
            attr_p = sdp_p->sess_attrs_p;
        } else {  
            mca_p = sdp_find_media_level(sdp_p, level);
            if (mca_p == NULL) {
                return (direction);
            }
            attr_p = mca_p->media_attrs_p;
        }

        
        for (; attr_p != NULL; attr_p = attr_p->next_p) {
            if (attr_p->type == SDP_ATTR_INACTIVE) {
                direction = SDP_DIRECTION_INACTIVE;
            } else if (attr_p->type == SDP_ATTR_SENDONLY) {
                direction = SDP_DIRECTION_SENDONLY;
            } else if (attr_p->type == SDP_ATTR_RECVONLY) {
                direction = SDP_DIRECTION_RECVONLY;
            } else if (attr_p->type == SDP_ATTR_SENDRECV) {
                direction = SDP_DIRECTION_SENDRECV;
            }
        }
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            CSFLogDebug(logTag, "%s Warning: Invalid cap_num for media direction.",
                     sdp_p->debug_str);
        }
    }

    return (direction);
}







tinybool sdp_validate_qos_attr (sdp_attr_e qos_attr)
{
    if ((qos_attr == SDP_ATTR_QOS) ||
        (qos_attr == SDP_ATTR_SECURE) ||
        (qos_attr == SDP_ATTR_X_PC_QOS) ||
        (qos_attr == SDP_ATTR_X_QOS) ||
        (qos_attr == SDP_ATTR_CURR) ||
        (qos_attr == SDP_ATTR_DES) ||
        (qos_attr == SDP_ATTR_CONF)){
        return (TRUE);
    } else {
        return (FALSE);
    }
}















sdp_qos_strength_e sdp_attr_get_qos_strength (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, sdp_attr_e qos_attr, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    if (sdp_validate_qos_attr(qos_attr) == FALSE) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            CSFLogDebug(logTag, "%s Warning: Invalid QOS attribute specified for"
                     "get qos strength.", sdp_p->debug_str);
        }
        return (SDP_QOS_STRENGTH_UNKNOWN);
    }
    attr_p = sdp_find_attr(sdp_p, level, cap_num, qos_attr, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s %s attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str,
                      sdp_get_attr_name(qos_attr), (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_QOS_STRENGTH_UNKNOWN);
    } else {
        switch (qos_attr) {
            case SDP_ATTR_QOS:
                return (attr_p->attr.qos.strength);
            case SDP_ATTR_DES:
                return (attr_p->attr.des.strength);
            default:
                return SDP_QOS_STRENGTH_UNKNOWN;

        }
    }
}















sdp_qos_dir_e sdp_attr_get_qos_direction (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, sdp_attr_e qos_attr, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    if (sdp_validate_qos_attr(qos_attr) == FALSE) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            CSFLogDebug(logTag, "%s Warning: Invalid QOS attribute specified "
                     "for get qos direction.", sdp_p->debug_str);
        }
        return (SDP_QOS_DIR_UNKNOWN);
    }
    attr_p = sdp_find_attr(sdp_p, level, cap_num, qos_attr, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s %s attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str,
                      sdp_get_attr_name(qos_attr), (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_QOS_DIR_UNKNOWN);
    } else {
         switch (qos_attr) {
            case SDP_ATTR_QOS:
                 return (attr_p->attr.qos.direction);
            case SDP_ATTR_CURR:
                 return (attr_p->attr.curr.direction);
            case SDP_ATTR_DES:
                 return (attr_p->attr.des.direction);
            case SDP_ATTR_CONF:
                 return (attr_p->attr.conf.direction);
            default:
                return SDP_QOS_DIR_UNKNOWN;

        }
    }
}















sdp_qos_status_types_e sdp_attr_get_qos_status_type (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, sdp_attr_e qos_attr, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    if (sdp_validate_qos_attr(qos_attr) == FALSE) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            CSFLogDebug(logTag, "%s Warning: Invalid QOS attribute specified "
                     "for get qos status_type.", sdp_p->debug_str);
        }
        return (SDP_QOS_STATUS_TYPE_UNKNOWN);
    }
    attr_p = sdp_find_attr(sdp_p, level, cap_num, qos_attr, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s %s attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str,
                      sdp_get_attr_name(qos_attr), (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_QOS_STATUS_TYPE_UNKNOWN);
    } else {
        switch (qos_attr) {
            case SDP_ATTR_CURR:
                return (attr_p->attr.curr.status_type);
            case SDP_ATTR_DES:
                return (attr_p->attr.des.status_type);
            case SDP_ATTR_CONF:
                return (attr_p->attr.conf.status_type);
            default:
                return SDP_QOS_STATUS_TYPE_UNKNOWN;

        }
    }
}














tinybool sdp_attr_get_qos_confirm (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, sdp_attr_e qos_attr, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    if (sdp_validate_qos_attr(qos_attr) == FALSE) {
        if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
            CSFLogDebug(logTag, "%s Warning: Invalid QOS attribute specified "
                     "for get qos confirm.", sdp_p->debug_str);
        }
        return (FALSE);
    }
    attr_p = sdp_find_attr(sdp_p, level, cap_num, qos_attr, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s %s attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str,
                      sdp_get_attr_name(qos_attr), (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.qos.confirm);
    }
}















sdp_curr_type_e sdp_attr_get_curr_type (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, sdp_attr_e qos_attr, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, qos_attr, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s %s attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str,
                      sdp_get_attr_name(qos_attr), (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_CURR_UNKNOWN_TYPE);
    } else {
        return (attr_p->attr.curr.type);
    }
}















sdp_des_type_e sdp_attr_get_des_type (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, sdp_attr_e qos_attr, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, qos_attr, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s %s attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str,
                      sdp_get_attr_name(qos_attr), (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_DES_UNKNOWN_TYPE);
    } else {
        return (attr_p->attr.des.type);
    }
}















sdp_conf_type_e sdp_attr_get_conf_type (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, sdp_attr_e qos_attr, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, qos_attr, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s %s attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str,
                      sdp_get_attr_name(qos_attr), (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_CONF_UNKNOWN_TYPE);
    } else {
        return (attr_p->attr.conf.type);
    }
}












sdp_nettype_e sdp_attr_get_subnet_nettype (sdp_t *sdp_p, uint16_t level,
                                           uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SUBNET, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Subnet attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_NT_INVALID);
    } else {
        return (attr_p->attr.subnet.nettype);
    }
}












sdp_addrtype_e sdp_attr_get_subnet_addrtype (sdp_t *sdp_p, uint16_t level,
                                             uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SUBNET, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Subnet attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_AT_INVALID);
    } else {
        return (attr_p->attr.subnet.addrtype);
    }
}














const char *sdp_attr_get_subnet_addr (sdp_t *sdp_p, uint16_t level,
                                      uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SUBNET, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Subnet attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    } else {
        return (attr_p->attr.subnet.addr);
    }
}














int32_t sdp_attr_get_subnet_prefix (sdp_t *sdp_p, uint16_t level,
                                  uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SUBNET, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Subnet attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.subnet.prefix);
    }
}














tinybool sdp_attr_rtpmap_payload_valid (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                        uint16_t *inst_num, uint16_t payload_type)
{
    uint16_t          i;
    sdp_attr_t  *attr_p;
    uint16_t          num_instances;

    *inst_num = 0;

    if (sdp_attr_num_instances(sdp_p, level, cap_num,
                          SDP_ATTR_RTPMAP, &num_instances) != SDP_SUCCESS) {
        return (FALSE);
    }

    for (i=1; i <= num_instances; i++) {
        attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_RTPMAP, i);
        if ((attr_p != NULL) &&
            (attr_p->attr.transport_map.payload_num == payload_type)) {
            *inst_num = i;
            return (TRUE);
        }
    }

    return (FALSE);
}












uint16_t sdp_attr_get_rtpmap_payload_type (sdp_t *sdp_p, uint16_t level,
                                      uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_RTPMAP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtpmap attribute, level %u instance %u "
                                "not found.",
                                sdp_p->debug_str,
                                (unsigned)level,
                                (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.transport_map.payload_num);
    }
}














const char *sdp_attr_get_rtpmap_encname (sdp_t *sdp_p, uint16_t level,
                                         uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_RTPMAP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtpmap attribute, level %u instance %u "
                                "not found.",
                                sdp_p->debug_str,
                                (unsigned)level,
                                (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    } else {
        return (attr_p->attr.transport_map.encname);
    }
}












uint32_t sdp_attr_get_rtpmap_clockrate (sdp_t *sdp_p, uint16_t level,
                                   uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_RTPMAP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtpmap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.transport_map.clockrate);
    }
}












uint16_t sdp_attr_get_rtpmap_num_chan (sdp_t *sdp_p, uint16_t level,
                                  uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_RTPMAP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtpmap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.transport_map.num_chan);
    }
}
















sdp_result_e sdp_attr_get_ice_attribute (sdp_t *sdp_p, uint16_t level,
                                  uint8_t cap_num, sdp_attr_e sdp_attr, uint16_t inst_num,
                                  char **out)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, sdp_attr, inst_num);
    if (attr_p != NULL) {
        *out = attr_p->attr.ice_attr;
        return (SDP_SUCCESS);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s ice attribute, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
}














tinybool sdp_attr_is_present (sdp_t *sdp_p, sdp_attr_e attr_type, uint16_t level,
                              uint8_t cap_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, attr_type, 1);
    if (attr_p != NULL) {
        return (TRUE);
    }
    if (sdp_p->debug_flag[SDP_DEBUG_WARNINGS]) {
        CSFLogDebug(logTag, "%s Attribute %s, level %u not found.",
                    sdp_p->debug_str, sdp_get_attr_name(attr_type), level);
    }

    return (FALSE);
}

















sdp_result_e sdp_attr_get_rtcp_mux_attribute (sdp_t *sdp_p, uint16_t level,
                                  uint8_t cap_num, sdp_attr_e sdp_attr, uint16_t inst_num,
                                  tinybool *rtcp_mux)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, sdp_attr, inst_num);
    if (attr_p != NULL) {
        *rtcp_mux = attr_p->attr.boolean_val;
        return (SDP_SUCCESS);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s rtcp-mux attribute, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
}















sdp_result_e sdp_attr_get_setup_attribute (sdp_t *sdp_p, uint16_t level,
    uint8_t cap_num, uint16_t inst_num, sdp_setup_type_e *setup_type)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_SETUP, inst_num);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag,
                "%s setup attribute, level %u instance %u not found.",
                sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return SDP_INVALID_PARAMETER;
    }

    *setup_type = attr_p->attr.setup;
    return SDP_SUCCESS;
}















sdp_result_e sdp_attr_get_connection_attribute (sdp_t *sdp_p, uint16_t level,
    uint8_t cap_num, uint16_t inst_num, sdp_connection_type_e *connection_type)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_CONNECTION,
        inst_num);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag,
                "%s setup attribute, level %u instance %u not found.",
                sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return SDP_INVALID_PARAMETER;
    }

    *connection_type = attr_p->attr.connection;
    return SDP_SUCCESS;
}















sdp_result_e sdp_attr_get_dtls_fingerprint_attribute (sdp_t *sdp_p, uint16_t level,
                                  uint8_t cap_num, sdp_attr_e sdp_attr, uint16_t inst_num,
                                  char **out)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, sdp_attr, inst_num);
    if (attr_p != NULL) {
        *out = attr_p->attr.string_val;
        return (SDP_SUCCESS);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s dtls fingerprint attribute, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
}














tinybool sdp_attr_sprtmap_payload_valid (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                        uint16_t *inst_num, uint16_t payload_type)
{
    uint16_t          i;
    sdp_attr_t  *attr_p;
    uint16_t          num_instances;

    *inst_num = 0;

    if (sdp_attr_num_instances(sdp_p, level, cap_num,
                          SDP_ATTR_SPRTMAP, &num_instances) != SDP_SUCCESS) {
        return (FALSE);
    }

    for (i=1; i <= num_instances; i++) {
        attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_SPRTMAP, i);
        if ((attr_p != NULL) &&
            (attr_p->attr.transport_map.payload_num == payload_type)) {
            *inst_num = i;
            return (TRUE);
        }
    }

    return (FALSE);
}












uint16_t sdp_attr_get_sprtmap_payload_type (sdp_t *sdp_p, uint16_t level,
                                      uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_SPRTMAP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s sprtmap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.transport_map.payload_num);
    }
}














const char *sdp_attr_get_sprtmap_encname (sdp_t *sdp_p, uint16_t level,
                                         uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_SPRTMAP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s sprtmap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    } else {
        return (attr_p->attr.transport_map.encname);
    }
}












uint32_t sdp_attr_get_sprtmap_clockrate (sdp_t *sdp_p, uint16_t level,
                                   uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_SPRTMAP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s sprtmap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.transport_map.clockrate);
    }
}












uint16_t sdp_attr_get_sprtmap_num_chan (sdp_t *sdp_p, uint16_t level,
                                  uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_SPRTMAP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s sprtmap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.transport_map.num_chan);
    }
}






















tinybool sdp_attr_fmtp_payload_valid (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                      uint16_t *inst_num, uint16_t payload_type)
{
    uint16_t          i;
    sdp_attr_t  *attr_p;
    uint16_t          num_instances;

    if (sdp_attr_num_instances(sdp_p, level, cap_num,
                               SDP_ATTR_FMTP, &num_instances) != SDP_SUCCESS) {
        return (FALSE);
    }

    for (i=1; i <= num_instances; i++) {
        attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, i);
        if ((attr_p != NULL) &&
            (attr_p->attr.fmtp.payload_num == payload_type)) {
            *inst_num = i;
            return (TRUE);
        }
    }

    return (FALSE);
}












uint16_t sdp_attr_get_fmtp_payload_type (sdp_t *sdp_p, uint16_t level,
                                    uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.fmtp.payload_num);
    }
}



















sdp_ne_res_e sdp_attr_fmtp_is_range_set (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                         uint16_t inst_num, uint8_t low_val, uint8_t high_val)
{
    uint16_t          i;
    uint32_t          mapword;
    uint32_t          bmap;
    uint32_t          num_vals = 0;
    uint32_t          num_vals_set = 0;
    sdp_attr_t  *attr_p;
    sdp_fmtp_t  *fmtp_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_NO_MATCH);
    }

    fmtp_p = &(attr_p->attr.fmtp);
    for (i = low_val; i <= high_val; i++) {
        num_vals++;
        mapword = i/SDP_NE_BITS_PER_WORD;
        bmap = SDP_NE_BIT_0 << (i%32);
        if (fmtp_p->bmap[ mapword ] & bmap) {
            num_vals_set++;
        }
    }

    if (num_vals == num_vals_set) {
        return (SDP_FULL_MATCH);
    } else if (num_vals_set == 0) {
        return (SDP_NO_MATCH);
    } else {
        return (SDP_PARTIAL_MATCH);
    }
}



















tinybool
sdp_attr_fmtp_valid(sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                    uint16_t inst_num, uint16_t appl_maxval, uint32_t* evt_array)
{
    uint16_t          i;
    uint32_t          mapword;
    sdp_attr_t  *attr_p;
    sdp_fmtp_t  *fmtp_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return FALSE;
    }

    fmtp_p = &(attr_p->attr.fmtp);

    
    if (fmtp_p->maxval > appl_maxval)
      return FALSE;

    


    mapword = appl_maxval/SDP_NE_BITS_PER_WORD;
    for (i=0; i<mapword; i++) {
      if (fmtp_p->bmap[i] & ~(evt_array[i])) {
        
        return FALSE;
      }
    }
    return TRUE;
}













sdp_result_e sdp_attr_set_fmtp_payload_type (sdp_t *sdp_p, uint16_t level,
                                             uint8_t cap_num, uint16_t inst_num,
                                             uint16_t payload_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        attr_p->attr.fmtp.payload_num = payload_num;
        return (SDP_SUCCESS);
    }
}











sdp_result_e sdp_attr_get_fmtp_range (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                      uint16_t inst_num, uint32_t *bmap)
{
    sdp_attr_t  *attr_p;
    sdp_fmtp_t  *fmtp_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    fmtp_p = &(attr_p->attr.fmtp);
    memcpy(bmap, fmtp_p->bmap, SDP_NE_NUM_BMAP_WORDS * sizeof(uint32_t) );

    return (SDP_SUCCESS);
}













sdp_result_e sdp_attr_clear_fmtp_range (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                        uint16_t inst_num, uint8_t low_val, uint8_t high_val)
{
    uint16_t          i;
    uint32_t          mapword;
    uint32_t          bmap;
    sdp_attr_t  *attr_p;
    sdp_fmtp_t  *fmtp_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    fmtp_p = &(attr_p->attr.fmtp);
    for (i = low_val; i <= high_val; i++) {
        mapword = i/SDP_NE_BITS_PER_WORD;
        bmap = SDP_NE_BIT_0 << (i%32);
        fmtp_p->bmap[ mapword ] &= ~bmap;
    }
    if (high_val > fmtp_p->maxval) {
        fmtp_p->maxval = high_val;
    }
    return (SDP_SUCCESS);
}

















sdp_ne_res_e sdp_attr_compare_fmtp_ranges (sdp_t *src_sdp_p,sdp_t *dst_sdp_p,
                                           uint16_t src_level, uint16_t dst_level,
                                           uint8_t src_cap_num, uint8_t dst_cap_num,
                                           uint16_t src_inst_num, uint16_t dst_inst_num)
{
    uint16_t          i,j;
    uint32_t          bmap;
    uint32_t          num_vals_match = 0;
    sdp_attr_t  *src_attr_p;
    sdp_attr_t  *dst_attr_p;
    sdp_fmtp_t  *src_fmtp_p;
    sdp_fmtp_t  *dst_fmtp_p;

    src_attr_p = sdp_find_attr(src_sdp_p, src_level, src_cap_num,
                               SDP_ATTR_FMTP, src_inst_num);
    dst_attr_p = sdp_find_attr(dst_sdp_p, dst_level, dst_cap_num,
                               SDP_ATTR_FMTP, dst_inst_num);
    if ((src_attr_p == NULL) || (dst_attr_p == NULL)) {
        if (src_sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s source or destination fmtp attribute for "
                      "compare not found.", src_sdp_p->debug_str);
        }
        src_sdp_p->conf_p->num_invalid_param++;
        return (SDP_NO_MATCH);
    }

    src_fmtp_p = &(src_attr_p->attr.fmtp);
    dst_fmtp_p = &(dst_attr_p->attr.fmtp);
    for (i = 0; i < SDP_NE_NUM_BMAP_WORDS; i++) {
        for (j = 0; j < SDP_NE_BITS_PER_WORD; j++) {
            bmap = SDP_NE_BIT_0 << j;
            if ((src_fmtp_p->bmap[i] & bmap) && (dst_fmtp_p->bmap[i] & bmap)) {
                num_vals_match++;
            } else if ((!(src_fmtp_p->bmap[i] & bmap)) &&
                       (!(dst_fmtp_p->bmap[i] & bmap))) {
                num_vals_match++;
            }
        }
    }

    if (num_vals_match == (SDP_NE_NUM_BMAP_WORDS * SDP_NE_BITS_PER_WORD)) {
        return (SDP_FULL_MATCH);
    } else if (num_vals_match == 0) {
        return (SDP_NO_MATCH);
    } else {
        return (SDP_PARTIAL_MATCH);
    }
}













sdp_result_e sdp_attr_copy_fmtp_ranges (sdp_t *src_sdp_p, sdp_t *dst_sdp_p,
                                        uint16_t src_level, uint16_t dst_level,
                                        uint8_t src_cap_num, uint8_t dst_cap_num,
                                        uint16_t src_inst_num, uint16_t dst_inst_num)
{
    uint16_t          i;
    sdp_attr_t  *src_attr_p;
    sdp_attr_t  *dst_attr_p;
    sdp_fmtp_t  *src_fmtp_p;
    sdp_fmtp_t  *dst_fmtp_p;

    if (!src_sdp_p || !dst_sdp_p) {
        return (SDP_INVALID_SDP_PTR);
    }

    src_attr_p = sdp_find_attr(src_sdp_p, src_level, src_cap_num,
                               SDP_ATTR_FMTP, src_inst_num);
    dst_attr_p = sdp_find_attr(dst_sdp_p, dst_level, dst_cap_num,
                               SDP_ATTR_FMTP, dst_inst_num);
    if ((src_attr_p == NULL) || (dst_attr_p == NULL)) {
        if (src_sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s source or destination fmtp attribute for "
                      "copy not found.", src_sdp_p->debug_str);
        }
        src_sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    src_fmtp_p = &(src_attr_p->attr.fmtp);
    dst_fmtp_p = &(dst_attr_p->attr.fmtp);
    dst_fmtp_p->maxval = src_fmtp_p->maxval;
    for (i = 0; i < SDP_NE_NUM_BMAP_WORDS; i++) {
        dst_fmtp_p->bmap[i] = src_fmtp_p->bmap[i];
    }
    return (SDP_SUCCESS);
}











uint32_t sdp_attr_get_fmtp_mode_for_payload_type (sdp_t *sdp_p, uint16_t level,
                                             uint8_t cap_num, uint32_t payload_type)
{
    uint16_t          num_a_lines = 0;
    int          i;
    sdp_attr_t  *attr_p;

    


    (void) sdp_attr_num_instances(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                                  &num_a_lines);
    for (i = 0; i < num_a_lines; i++) {
        attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, (uint16_t) (i + 1));
        if ((attr_p != NULL) &&
            (attr_p->attr.fmtp.payload_num == (uint16_t)payload_type)) {
            if (attr_p->attr.fmtp.fmtp_format == SDP_FMTP_MODE) {
                return attr_p->attr.fmtp.mode;
            }
        }
    }
   return 0;
}

sdp_result_e sdp_attr_set_fmtp_max_fs (sdp_t *sdp_p, uint16_t level,
                                       uint8_t cap_num, uint16_t inst_num,
                                       uint32_t max_fs)
{
    sdp_attr_t  *attr_p;
    sdp_fmtp_t  *fmtp_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    fmtp_p = &(attr_p->attr.fmtp);
    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;

    if (max_fs > 0) {
        fmtp_p->max_fs  = max_fs;
        return (SDP_SUCCESS);
    } else {
        return (SDP_FAILURE);
    }
}

sdp_result_e sdp_attr_set_fmtp_max_fr (sdp_t *sdp_p, uint16_t level,
                                       uint8_t cap_num, uint16_t inst_num,
                                       uint32_t max_fr)
{
    sdp_attr_t  *attr_p;
    sdp_fmtp_t  *fmtp_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    fmtp_p = &(attr_p->attr.fmtp);
    fmtp_p->fmtp_format = SDP_FMTP_CODEC_INFO;

    if (max_fr > 0) {
        fmtp_p->max_fr  = max_fr;
        return (SDP_SUCCESS);
    } else {
        return (SDP_FAILURE);
    }
}











sdp_result_e sdp_attr_get_fmtp_max_average_bitrate (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, uint32_t* val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP, 1);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.fmtp.maxaveragebitrate;
        return (SDP_SUCCESS);
    }
}












sdp_result_e sdp_attr_get_fmtp_usedtx (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, tinybool* val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = (tinybool)attr_p->attr.fmtp.usedtx;
        return (SDP_SUCCESS);
    }
}











sdp_result_e sdp_attr_get_fmtp_stereo (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, tinybool* val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = (tinybool)attr_p->attr.fmtp.stereo;
        return (SDP_SUCCESS);
    }
}











sdp_result_e sdp_attr_get_fmtp_useinbandfec (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, tinybool* val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = (tinybool)attr_p->attr.fmtp.useinbandfec;
        return (SDP_SUCCESS);
    }
}










char* sdp_attr_get_fmtp_maxcodedaudiobandwidth (sdp_t *sdp_p, uint16_t level,
                                          uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.fmtp.maxcodedaudiobandwidth);
    }
}











sdp_result_e sdp_attr_get_fmtp_cbr (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, tinybool* val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = (tinybool)attr_p->attr.fmtp.cbr;
        return (SDP_SUCCESS);
    }
}

uint16_t sdp_attr_get_sctpmap_port(sdp_t *sdp_p, uint16_t level,
                              uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_SCTPMAP, inst_num);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s sctpmap port, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return 0;
    } else {
        return attr_p->attr.sctpmap.port;
    }
}

sdp_result_e sdp_attr_get_sctpmap_streams (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, uint32_t* val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_SCTPMAP, inst_num);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s sctpmap streams, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.sctpmap.streams;
        return (SDP_SUCCESS);
    }
}

sdp_result_e sdp_attr_get_sctpmap_protocol (sdp_t *sdp_p, uint16_t level,
                                            uint8_t cap_num, uint16_t inst_num,
                                            char* protocol)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_SCTPMAP,
                           inst_num);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s sctpmap, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        sstrncpy(protocol, attr_p->attr.sctpmap.protocol, SDP_MAX_STRING_LEN+1);
    }
    return (SDP_SUCCESS);
}













tinybool sdp_attr_fmtp_is_annexb_set (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                      uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.fmtp.annexb);
    }
}













tinybool sdp_attr_fmtp_is_annexa_set (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                      uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.fmtp.annexa);
    }
}











int32_t sdp_attr_get_fmtp_bitrate_type (sdp_t *sdp_p, uint16_t level,
                                      uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.bitrate);
    }
}











int32_t sdp_attr_get_fmtp_qcif (sdp_t *sdp_p, uint16_t level,
                            uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.qcif);
    }
}










int32_t sdp_attr_get_fmtp_cif (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.cif);
    }
}












int32_t sdp_attr_get_fmtp_sqcif (sdp_t *sdp_p, uint16_t level,
                               uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.sqcif);
    }
}











int32_t sdp_attr_get_fmtp_cif4 (sdp_t *sdp_p, uint16_t level,
                              uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.cif4);
    }
}












int32_t sdp_attr_get_fmtp_cif16 (sdp_t *sdp_p, uint16_t level,
                               uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.cif16);
    }
}












int32_t sdp_attr_get_fmtp_maxbr (sdp_t *sdp_p, uint16_t level,
                               uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.maxbr);
    }
}












int32_t sdp_attr_get_fmtp_custom_x (sdp_t *sdp_p, uint16_t level,
                                  uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.custom_x);
    }
}











int32_t sdp_attr_get_fmtp_custom_y (sdp_t *sdp_p, uint16_t level,
                                  uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.custom_y);
    }
}












int32_t sdp_attr_get_fmtp_custom_mpi (sdp_t *sdp_p, uint16_t level,
                                    uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.custom_mpi);
    }
}










int32_t sdp_attr_get_fmtp_par_width (sdp_t *sdp_p, uint16_t level,
                                   uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.par_width);
    }
}










int32_t sdp_attr_get_fmtp_par_height (sdp_t *sdp_p, uint16_t level,
                                    uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.par_height);
    }
}










int32_t sdp_attr_get_fmtp_cpcf (sdp_t *sdp_p, uint16_t level,
                              uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.cpcf);
    }
}










int32_t sdp_attr_get_fmtp_bpp (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.bpp);
    }
}










int32_t sdp_attr_get_fmtp_hrd (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.hrd);
    }
}










int32_t sdp_attr_get_fmtp_profile (sdp_t *sdp_p, uint16_t level,
                                 uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.profile);
    }
}










int32_t sdp_attr_get_fmtp_level (sdp_t *sdp_p, uint16_t level,
                               uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.level);
    }
}










tinybool sdp_attr_get_fmtp_interlace (sdp_t *sdp_p, uint16_t level,
                                      uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return FALSE;
    } else {
        return (attr_p->attr.fmtp.is_interlace);
    }
}











sdp_result_e sdp_attr_get_fmtp_pack_mode (sdp_t *sdp_p, uint16_t level,
                                 uint8_t cap_num, uint16_t inst_num, uint16_t *val)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        if (SDP_INVALID_PACKETIZATION_MODE_VALUE == attr_p->attr.fmtp.packetization_mode) {
            
            *val = SDP_DEFAULT_PACKETIZATION_MODE_VALUE;
        } else {
            *val = attr_p->attr.fmtp.packetization_mode;
        }
        return (SDP_SUCCESS);
    }
}











sdp_result_e sdp_attr_get_fmtp_level_asymmetry_allowed (sdp_t *sdp_p, uint16_t level,
                                 uint8_t cap_num, uint16_t inst_num, uint16_t *val)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.fmtp.level_asymmetry_allowed;
        return (SDP_SUCCESS);
    }
}










const char* sdp_attr_get_fmtp_profile_id (sdp_t *sdp_p, uint16_t level,
                                          uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.fmtp.profile_level_id);
    }
}










const char* sdp_attr_get_fmtp_param_sets (sdp_t *sdp_p, uint16_t level,
                                          uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.fmtp.parameter_sets);
    }
}











sdp_result_e sdp_attr_get_fmtp_interleaving_depth (sdp_t *sdp_p, uint16_t level,
                                            uint8_t cap_num, uint16_t inst_num, uint16_t* val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.fmtp.interleaving_depth;
        return (SDP_SUCCESS);
    }
}











sdp_result_e sdp_attr_get_fmtp_deint_buf_req (sdp_t *sdp_p, uint16_t level,
                                             uint8_t cap_num, uint16_t inst_num,
                                             uint32_t *val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        if (attr_p->attr.fmtp.flag & SDP_DEINT_BUF_REQ_FLAG) {
            *val = attr_p->attr.fmtp.deint_buf_req;
            return (SDP_SUCCESS);
        } else {
            return (SDP_FAILURE);
        }
    }
}










sdp_result_e sdp_attr_get_fmtp_max_don_diff (sdp_t *sdp_p, uint16_t level,
                                      uint8_t cap_num, uint16_t inst_num,
                                      uint32_t *val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.fmtp.max_don_diff;
        return (SDP_SUCCESS);
    }
}










sdp_result_e sdp_attr_get_fmtp_init_buf_time (sdp_t *sdp_p, uint16_t level,
                                             uint8_t cap_num, uint16_t inst_num,
                                             uint32_t *val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        if (attr_p->attr.fmtp.flag & SDP_INIT_BUF_TIME_FLAG) {
            *val = attr_p->attr.fmtp.init_buf_time;
            return (SDP_SUCCESS);
        } else {
            return (SDP_FAILURE);
        }
    }
}











sdp_result_e sdp_attr_get_fmtp_max_mbps (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, uint16_t inst_num,
                                uint32_t *val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.fmtp.max_mbps;
        return (SDP_SUCCESS);
    }
}











sdp_result_e sdp_attr_get_fmtp_max_fs (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, uint32_t *val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.fmtp.max_fs;
        return (SDP_SUCCESS);
    }
}











sdp_result_e sdp_attr_get_fmtp_max_fr (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, uint32_t *val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.fmtp.max_fr;
        return (SDP_SUCCESS);
    }
}











sdp_result_e sdp_attr_get_fmtp_max_cpb (sdp_t *sdp_p, uint16_t level,
                                 uint8_t cap_num, uint16_t inst_num, uint32_t *val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.fmtp.max_cpb;
        return (SDP_SUCCESS);
    }
}











sdp_result_e sdp_attr_get_fmtp_max_dpb (sdp_t *sdp_p, uint16_t level,
                               uint8_t cap_num, uint16_t inst_num, uint32_t *val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.fmtp.max_dpb;
        return (SDP_SUCCESS);
    }
}












sdp_result_e sdp_attr_get_fmtp_max_br (sdp_t *sdp_p, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, uint32_t* val)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        *val = attr_p->attr.fmtp.max_br;
        return (SDP_SUCCESS);
    }
}










tinybool sdp_attr_fmtp_is_redundant_pic_cap (sdp_t *sdp_p, uint16_t level,
                                             uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.fmtp.redundant_pic_cap);
    }
}











sdp_result_e sdp_attr_get_fmtp_deint_buf_cap (sdp_t *sdp_p, uint16_t level,
                                             uint8_t cap_num, uint16_t inst_num,
                                             uint32_t *val)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        if (attr_p->attr.fmtp.flag & SDP_DEINT_BUF_CAP_FLAG) {
            *val = attr_p->attr.fmtp.deint_buf_cap;
            return (SDP_SUCCESS);
        } else {
            return (SDP_FAILURE);
        }
    }
}










sdp_result_e sdp_attr_get_fmtp_max_rcmd_nalu_size (sdp_t *sdp_p, uint16_t level,
                                                  uint8_t cap_num, uint16_t inst_num,
                                                  uint32_t *val)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        if (attr_p->attr.fmtp.flag & SDP_MAX_RCMD_NALU_SIZE_FLAG) {
            *val = attr_p->attr.fmtp.max_rcmd_nalu_size;
            return (SDP_SUCCESS);
        } else {
            return (SDP_FAILURE);
        }
    }
}










tinybool sdp_attr_fmtp_is_parameter_add (sdp_t *sdp_p, uint16_t level,
                                         uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        

        return (attr_p->attr.fmtp.parameter_add != 0);
    }
}














tinybool sdp_attr_get_fmtp_annex_d (sdp_t *sdp_p, uint16_t level,
                                    uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.fmtp.annex_d);
    }
}

tinybool sdp_attr_get_fmtp_annex_f (sdp_t *sdp_p, uint16_t level,
                                    uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.fmtp.annex_f);
    }
}

tinybool sdp_attr_get_fmtp_annex_i (sdp_t *sdp_p, uint16_t level,
                                    uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.fmtp.annex_i);
    }
}

tinybool sdp_attr_get_fmtp_annex_j (sdp_t *sdp_p, uint16_t level,
                                    uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.fmtp.annex_j);
    }
}

tinybool sdp_attr_get_fmtp_annex_t (sdp_t *sdp_p, uint16_t level,
                                    uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.fmtp.annex_t);
    }
}

int32_t sdp_attr_get_fmtp_annex_k_val (sdp_t *sdp_p, uint16_t level,
                                     uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.annex_k_val);
    }
}

int32_t sdp_attr_get_fmtp_annex_n_val (sdp_t *sdp_p, uint16_t level,
                                     uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.annex_n_val);
    }
}

int32_t sdp_attr_get_fmtp_annex_p_picture_resize (sdp_t *sdp_p, uint16_t level,
                                                uint8_t cap_num, uint16_t inst_num)
{


    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.annex_p_val_picture_resize);
    }
}

int32_t sdp_attr_get_fmtp_annex_p_warp (sdp_t *sdp_p, uint16_t level,
                                      uint8_t cap_num, uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    } else {
        return (attr_p->attr.fmtp.annex_p_val_warp);
    }
}














sdp_fmtp_format_type_e  sdp_attr_fmtp_get_fmtp_format (sdp_t *sdp_p,
                                                       uint16_t level, uint8_t cap_num,
                                                       uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_FMTP,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s fmtp attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_FMTP_UNKNOWN_TYPE);
    } else {
        return (attr_p->attr.fmtp.fmtp_format);
    }
}












uint16_t sdp_attr_get_pccodec_num_payload_types (sdp_t *sdp_p, uint16_t level,
                                            uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_X_PC_CODEC,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-pc-codec attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        return (attr_p->attr.pccodec.num_payloads);
    }
}














uint16_t sdp_attr_get_pccodec_payload_type (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                       uint16_t inst_num, uint16_t payload_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_X_PC_CODEC,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-pc-codec attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        if ((payload_num < 1) ||
            (payload_num > attr_p->attr.pccodec.num_payloads)) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s X-pc-codec attribute, level %u instance %u, "
                          "invalid payload number %u requested.",
                          sdp_p->debug_str, (unsigned)level, (unsigned)inst_num, (unsigned)payload_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return (0);
        } else {
            return (attr_p->attr.pccodec.payload_type[payload_num-1]);
        }
    }
}















sdp_result_e sdp_attr_add_pccodec_payload_type (sdp_t *sdp_p, uint16_t level,
                                                uint8_t cap_num, uint16_t inst_num,
                                                uint16_t payload_type)
{
    uint16_t          payload_num;
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_X_PC_CODEC,
                           inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-pc-codec attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    } else {
        payload_num = attr_p->attr.pccodec.num_payloads++;
        attr_p->attr.pccodec.payload_type[payload_num] = payload_type;
        return (SDP_SUCCESS);
    }
}











uint16_t sdp_attr_get_xcap_first_cap_num (sdp_t *sdp_p, uint16_t level, uint16_t inst_num)
{
    uint16_t          cap_num=1;
    uint16_t          attr_count=0;
    sdp_attr_t  *attr_p;
    sdp_mca_t   *mca_p;

    if (level == SDP_SESSION_LEVEL) {
        for (attr_p = sdp_p->sess_attrs_p; attr_p != NULL;
             attr_p = attr_p->next_p) {
            if (attr_p->type == SDP_ATTR_X_CAP) {
                attr_count++;
                if (attr_count == inst_num) {
                    return (cap_num);
                } else {
                    cap_num += attr_p->attr.cap_p->num_payloads;
                }
            }
        }
    } else {  
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (0);
        }
        for (attr_p = mca_p->media_attrs_p; attr_p != NULL;
             attr_p = attr_p->next_p) {
            if (attr_p->type == SDP_ATTR_X_CAP) {
                attr_count++;
                if (attr_count == inst_num) {
                    return (cap_num);
                } else {
                    cap_num += attr_p->attr.cap_p->num_payloads;
                }
            }
        }
    }  

    if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
        CSFLogError(logTag, "%s X-cap attribute, level %u instance %u "
                  "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
    }
    sdp_p->conf_p->num_invalid_param++;
    return (0);
}











sdp_media_e sdp_attr_get_xcap_media_type (sdp_t *sdp_p, uint16_t level,
                                          uint16_t inst_num)
{
    sdp_attr_t  *attr_p;
    sdp_mca_t   *cap_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_X_CAP, inst_num);
    if ((attr_p == NULL) || (attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-cap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_MEDIA_INVALID);
    } else {
        cap_p = attr_p->attr.cap_p;
        return (cap_p->media);
    }
}











sdp_transport_e sdp_attr_get_xcap_transport_type (sdp_t *sdp_p, uint16_t level,
                                                  uint16_t inst_num)
{
    sdp_attr_t  *attr_p;
    sdp_mca_t   *cap_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_X_CAP,
                           inst_num);
    if ((attr_p == NULL) || (attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-cap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_TRANSPORT_INVALID);
    } else {
        cap_p = attr_p->attr.cap_p;
        return (cap_p->transport);
    }
}














uint16_t sdp_attr_get_xcap_num_payload_types (sdp_t *sdp_p, uint16_t level,
                                         uint16_t inst_num)
{
    sdp_attr_t  *attr_p;
    sdp_mca_t   *cap_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_X_CAP, inst_num);
    if ((attr_p == NULL) || (attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-cap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        cap_p = attr_p->attr.cap_p;
        return (cap_p->num_payloads);
    }
}














uint16_t sdp_attr_get_xcap_payload_type (sdp_t *sdp_p, uint16_t level,
                                    uint16_t inst_num, uint16_t payload_num,
                                    sdp_payload_ind_e *indicator)
{
    sdp_attr_t  *attr_p;
    sdp_mca_t   *cap_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_X_CAP, inst_num);
    if ((attr_p == NULL) || (attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-cap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        cap_p = attr_p->attr.cap_p;
        if ((payload_num < 1) ||
            (payload_num > cap_p->num_payloads)) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s X-cap attribute, level %u instance %u, "
                          "payload num %u invalid.", sdp_p->debug_str,
                          (unsigned)level, (unsigned)inst_num, (unsigned)payload_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return (0);
        } else {
            *indicator = cap_p->payload_indicator[payload_num-1];
            return (cap_p->payload_type[payload_num-1]);
        }
    }
}













sdp_result_e sdp_attr_add_xcap_payload_type(sdp_t *sdp_p, uint16_t level,
                                            uint16_t inst_num, uint16_t payload_type,
                                            sdp_payload_ind_e indicator)
{
    sdp_attr_t  *attr_p;
    sdp_mca_t   *cap_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_X_CAP, inst_num);
    if ((attr_p == NULL) || (attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-cap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    cap_p = attr_p->attr.cap_p;
    cap_p->payload_indicator[cap_p->num_payloads] = indicator;
    cap_p->payload_type[cap_p->num_payloads++] = payload_type;
    return (SDP_SUCCESS);
}











uint16_t sdp_attr_get_cdsc_first_cap_num(sdp_t *sdp_p, uint16_t level, uint16_t inst_num)
{
    uint16_t          cap_num=1;
    uint16_t          attr_count=0;
    sdp_attr_t  *attr_p;
    sdp_mca_t   *mca_p;

    if (level == SDP_SESSION_LEVEL) {
        for (attr_p = sdp_p->sess_attrs_p; attr_p != NULL;
             attr_p = attr_p->next_p) {
            if (attr_p->type == SDP_ATTR_CDSC) {
                attr_count++;
                if (attr_count == inst_num) {
                    return (cap_num);
                } else {
                    cap_num += attr_p->attr.cap_p->num_payloads;
                }
            }
        }
    } else {  
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (0);
        }
        for (attr_p = mca_p->media_attrs_p; attr_p != NULL;
             attr_p = attr_p->next_p) {
            if (attr_p->type == SDP_ATTR_CDSC) {
                attr_count++;
                if (attr_count == inst_num) {
                    return (cap_num);
                } else {
                    cap_num += attr_p->attr.cap_p->num_payloads;
                }
            }
        }
    }  

    if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
        CSFLogError(logTag, "%s CDSC attribute, level %u instance %u "
                  "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
    }
    sdp_p->conf_p->num_invalid_param++;
    return (0);
}











sdp_media_e sdp_attr_get_cdsc_media_type(sdp_t *sdp_p, uint16_t level,
                                         uint16_t inst_num)
{
    sdp_attr_t  *attr_p;
    sdp_mca_t   *cdsc_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_CDSC, inst_num);
    if ((attr_p == NULL) || (attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s CDSC attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_MEDIA_INVALID);
    } else {
        cdsc_p = attr_p->attr.cap_p;
        return (cdsc_p->media);
    }
}











sdp_transport_e sdp_attr_get_cdsc_transport_type(sdp_t *sdp_p, uint16_t level,
                                                 uint16_t inst_num)
{
    sdp_attr_t  *attr_p;
    sdp_mca_t   *cdsc_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_CDSC,
                           inst_num);
    if ((attr_p == NULL) || (attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s CDSC attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_TRANSPORT_INVALID);
    } else {
        cdsc_p = attr_p->attr.cap_p;
        return (cdsc_p->transport);
    }
}














uint16_t sdp_attr_get_cdsc_num_payload_types (sdp_t *sdp_p, uint16_t level,
                                         uint16_t inst_num)
{
    sdp_attr_t  *attr_p;
    sdp_mca_t   *cdsc_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_CDSC, inst_num);
    if ((attr_p == NULL) || (attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s CDSC attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        cdsc_p = attr_p->attr.cap_p;
        return (cdsc_p->num_payloads);
    }
}














uint16_t sdp_attr_get_cdsc_payload_type (sdp_t *sdp_p, uint16_t level,
                                    uint16_t inst_num, uint16_t payload_num,
                                    sdp_payload_ind_e *indicator)
{
    sdp_attr_t  *attr_p;
    sdp_mca_t   *cdsc_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_CDSC, inst_num);
    if ((attr_p == NULL) || (attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s CDSC attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        cdsc_p = attr_p->attr.cap_p;
        if ((payload_num < 1) ||
            (payload_num > cdsc_p->num_payloads)) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s CDSC attribute, level %u instance %u, "
                          "payload num %u invalid.", sdp_p->debug_str,
                          (unsigned)level, (unsigned)inst_num, (unsigned)payload_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return (0);
        } else {
            *indicator = cdsc_p->payload_indicator[payload_num-1];
            return (cdsc_p->payload_type[payload_num-1]);
        }
    }
}












sdp_result_e sdp_attr_add_cdsc_payload_type (sdp_t *sdp_p, uint16_t level,
                                             uint16_t inst_num, uint16_t payload_type,
                                             sdp_payload_ind_e indicator)
{
    sdp_attr_t  *attr_p;
    sdp_mca_t   *cdsc_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_CDSC, inst_num);
    if ((attr_p == NULL) || (attr_p->attr.cap_p == NULL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s CDSC attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    cdsc_p = attr_p->attr.cap_p;
    cdsc_p->payload_indicator[cdsc_p->num_payloads] = indicator;
    cdsc_p->payload_type[cdsc_p->num_payloads++] = payload_type;
    return (SDP_SUCCESS);
}











tinybool sdp_media_dynamic_payload_valid (sdp_t *sdp_p, uint16_t payload_type,
                                          uint16_t m_line)
{
   uint16_t p_type,m_ptype;
   ushort num_payload_types;
   sdp_payload_ind_e ind;
   tinybool payload_matches = FALSE;
   tinybool result = TRUE;

   if ((payload_type < SDP_MIN_DYNAMIC_PAYLOAD) ||
       (payload_type > SDP_MAX_DYNAMIC_PAYLOAD)) {
       return FALSE;
   }

   num_payload_types =
       sdp_get_media_num_payload_types(sdp_p, m_line);

   for(p_type=1; p_type <=num_payload_types;p_type++){

       m_ptype = (uint16_t)sdp_get_media_payload_type(sdp_p,
                                            m_line, p_type, &ind);
       if (payload_type == m_ptype) {
           payload_matches = TRUE;
           break;
       }

   }

   if (!payload_matches) {
       return FALSE;
   }

   return (result);

}












tinybool sdp_attr_get_rtr_confirm (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_RTR, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s %s attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str,
                      sdp_get_attr_name(SDP_ATTR_RTR), (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.rtr.confirm);
    }
}



sdp_mediadir_role_e sdp_attr_get_comediadir_role (sdp_t *sdp_p, uint16_t level,
                                             uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_DIRECTION, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Comediadir role attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_MEDIADIR_ROLE_UNKNOWN);
    } else {
        return (attr_p->attr.comediadir.role);
    }
}












tinybool sdp_attr_get_silencesupp_enabled (sdp_t *sdp_p, uint16_t level,
                                           uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SILENCESUPP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s silenceSuppEnable attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (FALSE);
    } else {
        return (attr_p->attr.silencesupp.enabled);
    }
}














uint16_t sdp_attr_get_silencesupp_timer (sdp_t *sdp_p, uint16_t level,
                                    uint8_t cap_num, uint16_t inst_num,
                                    tinybool *null_ind)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SILENCESUPP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s silenceTimer attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        *null_ind = attr_p->attr.silencesupp.timer_null;
        return (attr_p->attr.silencesupp.timer);
    }
}














sdp_silencesupp_pref_e sdp_attr_get_silencesupp_pref (sdp_t *sdp_p,
                                                      uint16_t level, uint8_t cap_num,
                                                      uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SILENCESUPP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s silence suppPref attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_SILENCESUPP_PREF_UNKNOWN);
    } else {
        return (attr_p->attr.silencesupp.pref);
    }
}













sdp_silencesupp_siduse_e sdp_attr_get_silencesupp_siduse (sdp_t *sdp_p,
                                                          uint16_t level,
                                                          uint8_t cap_num,
                                                          uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SILENCESUPP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s silence sidUse attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_SILENCESUPP_SIDUSE_UNKNOWN);
    } else {
        return (attr_p->attr.silencesupp.siduse);
    }
}














uint8_t sdp_attr_get_silencesupp_fxnslevel (sdp_t *sdp_p, uint16_t level,
                                       uint8_t cap_num, uint16_t inst_num,
                                       tinybool *null_ind)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SILENCESUPP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s silence fxnslevel attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        *null_ind = attr_p->attr.silencesupp.fxnslevel_null;
        return (attr_p->attr.silencesupp.fxnslevel);
    }
}












uint16_t sdp_attr_get_mptime_num_intervals (
    sdp_t *sdp_p,
    uint16_t level,
    uint8_t cap_num,
    uint16_t inst_num) {

    sdp_attr_t *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_MPTIME, inst_num);
    if (attr_p != NULL) {
        return attr_p->attr.mptime.num_intervals;
    }

    if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
        CSFLogError(logTag, "%s mptime attribute, level %u instance %u not found.",
                  sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
    }
    sdp_p->conf_p->num_invalid_param++;
    return 0;
}














uint16_t sdp_attr_get_mptime_interval (
    sdp_t *sdp_p,
    uint16_t level,
    uint8_t cap_num,
    uint16_t inst_num,
    uint16_t interval_num) {

    sdp_attr_t *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_MPTIME, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s mptime attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return 0;
    }

    if ((interval_num<1) || (interval_num>attr_p->attr.mptime.num_intervals)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s mptime attribute, level %u instance %u, "
                      "invalid interval number %u requested.",
                      sdp_p->debug_str, (unsigned)level, (unsigned)inst_num, (unsigned)interval_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return 0;
    }

    return attr_p->attr.mptime.intervals[interval_num-1];
}
















sdp_result_e sdp_attr_add_mptime_interval (
    sdp_t *sdp_p,
    uint16_t level,
    uint8_t cap_num,
    uint16_t inst_num,
    uint16_t mp_interval) {

    uint16_t interval_num;
    sdp_attr_t *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num, SDP_ATTR_MPTIME, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s mptime attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return SDP_INVALID_PARAMETER;
    }

    interval_num = attr_p->attr.mptime.num_intervals;
    if (interval_num>=SDP_MAX_PAYLOAD_TYPES) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s mptime attribute, level %u instance %u "
                      "exceeds maximum length.",
                      sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return SDP_INVALID_PARAMETER;
    }

    attr_p->attr.mptime.intervals[interval_num] = mp_interval;
    ++attr_p->attr.mptime.num_intervals;
    return SDP_SUCCESS;
}











sdp_group_attr_e sdp_get_group_attr (sdp_t *sdp_p, uint16_t level,
                                     uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t          *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_GROUP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Group (a= group line) attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_GROUP_ATTR_UNSUPPORTED);
    } else {
       if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
           SDP_PRINT("%s Stream data group attr field is :%s ",
                     sdp_p->debug_str,
                     sdp_get_group_attr_name(attr_p->attr.stream_data.group_attr) );
        }
        return (attr_p->attr.stream_data.group_attr);
    }
}







uint16_t sdp_get_group_num_id (sdp_t *sdp_p, uint16_t level,
                          uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t          *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_GROUP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s a=group level attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Stream data group attr - num of ids is :%u ",
                      sdp_p->debug_str,
                      (unsigned)attr_p->attr.stream_data.num_group_id);
        }
    }
    return (attr_p->attr.stream_data.num_group_id);
}










const char* sdp_get_group_id (sdp_t *sdp_p, uint16_t level,
                        uint8_t cap_num, uint16_t inst_num, uint16_t id_num)
{
    sdp_attr_t          *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_GROUP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s a=group level attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Stream data group attr - num of ids is :%u ",
                      sdp_p->debug_str,
                      (unsigned)attr_p->attr.stream_data.num_group_id);
        }
        if ((id_num < 1) || (id_num > attr_p->attr.stream_data.num_group_id)) {
            return (NULL);
        }
    }
    return (attr_p->attr.stream_data.group_ids[id_num-1]);
}











const char* sdp_attr_get_x_sidin (sdp_t *sdp_p, uint16_t level,
                                     uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t          *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_X_SIDIN, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-sidin attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Stream X-sidin attr field is :%s ",
                      sdp_p->debug_str,
                      attr_p->attr.stream_data.x_sidin);
        }
        return (attr_p->attr.stream_data.x_sidin);
    }
}











const char* sdp_attr_get_x_sidout (sdp_t *sdp_p, uint16_t level,
                                     uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t          *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_X_SIDOUT, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-sidout attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Stream X-sidout attr field is :%s ",
                      sdp_p->debug_str,
                      attr_p->attr.stream_data.x_sidout);
        }
        return (attr_p->attr.stream_data.x_sidout);
    }
}











const char* sdp_attr_get_x_confid (sdp_t *sdp_p, uint16_t level,
                                     uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t          *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_X_CONFID, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s X-confid attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    } else {
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Stream X-confid attr field is :%s ",
                      sdp_p->debug_str,
                      attr_p->attr.stream_data.x_confid);
        }
        return (attr_p->attr.stream_data.x_confid);
    }
}








sdp_src_filter_mode_e
sdp_get_source_filter_mode (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                            uint16_t inst_num)
{
    sdp_attr_t *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SOURCE_FILTER, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Source filter attribute, level %u, "
                      "instance %u not found", sdp_p->debug_str,
                      (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_FILTER_MODE_NOT_PRESENT);
    }
    return (attr_p->attr.source_filter.mode);
}








sdp_result_e
sdp_get_filter_destination_attributes (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                                       uint16_t inst_num, sdp_nettype_e *nettype,
                                       sdp_addrtype_e *addrtype,
                                       char *dest_addr)
{
    sdp_attr_t *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SOURCE_FILTER, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Source filter attribute, level %u instance %u "
                      "not found", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    if (nettype) {
        *nettype = attr_p->attr.source_filter.nettype;
    }
    if (addrtype) {
        *addrtype = attr_p->attr.source_filter.addrtype;
    }
    sstrncpy(dest_addr, attr_p->attr.source_filter.dest_addr,
             SDP_MAX_STRING_LEN+1);

    return (SDP_SUCCESS);
}









int32_t
sdp_get_filter_source_address_count (sdp_t *sdp_p, uint16_t level,
                                     uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SOURCE_FILTER, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Source filter attribute, level %u instance %u "
                      "not found", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    }
    return (attr_p->attr.source_filter.num_src_addr);
}












sdp_result_e
sdp_get_filter_source_address (sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                               uint16_t inst_num, uint16_t src_addr_id,
                               char *src_addr)
{
    sdp_attr_t *attr_p;

    src_addr[0] = '\0';

    if (src_addr_id >= SDP_MAX_SRC_ADDR_LIST) {
        return (SDP_INVALID_PARAMETER);
    }
    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SOURCE_FILTER, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Source filter attribute, level %u instance %u "
                      "not found", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    if (src_addr_id >= attr_p->attr.source_filter.num_src_addr) {
        return (SDP_INVALID_PARAMETER);
    }
    sstrncpy(src_addr, attr_p->attr.source_filter.src_list[src_addr_id],
             SDP_MAX_STRING_LEN+1);

    return (SDP_SUCCESS);
}

sdp_rtcp_unicast_mode_e
sdp_get_rtcp_unicast_mode(sdp_t *sdp_p, uint16_t level, uint8_t cap_num,
                          uint16_t inst_num)
{
    sdp_attr_t *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_RTCP_UNICAST, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s RTCP Unicast attribute, level %u, "
                      "instance %u not found", sdp_p->debug_str,
                      (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_RTCP_UNICAST_MODE_NOT_PRESENT);
    }
    return ((sdp_rtcp_unicast_mode_e)attr_p->attr.u32_val);
}













int32_t
sdp_attr_get_sdescriptions_tag (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SDESCRIPTIONS, inst_num);

    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s srtp attribute tag, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return SDP_INVALID_VALUE;
    } else {
        return attr_p->attr.srtp_context.tag;
    }
}




















sdp_srtp_crypto_suite_t
sdp_attr_get_sdescriptions_crypto_suite (sdp_t *sdp_p, uint16_t level,
                                         uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;


    
    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SRTP_CONTEXT, inst_num);

    if (attr_p == NULL) {
        
        attr_p = sdp_find_attr(sdp_p, level, cap_num,
                               SDP_ATTR_SDESCRIPTIONS, inst_num);
        if (attr_p == NULL) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s srtp attribute suite, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return SDP_SRTP_UNKNOWN_CRYPTO_SUITE;
        }
    }

    return attr_p->attr.srtp_context.suite;

}



















const char*
sdp_attr_get_sdescriptions_key (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    
    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SRTP_CONTEXT, inst_num);

    if (attr_p == NULL) {
        
        attr_p = sdp_find_attr(sdp_p, level, cap_num,
                               SDP_ATTR_SDESCRIPTIONS, inst_num);

        if (attr_p == NULL) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s srtp attribute key, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return NULL;
        }
    }

    return (char*)attr_p->attr.srtp_context.master_key;
}




















const char*
sdp_attr_get_sdescriptions_salt (sdp_t *sdp_p, uint16_t level,
                                 uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    
    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SRTP_CONTEXT, inst_num);

    if (attr_p == NULL) {
        
        attr_p = sdp_find_attr(sdp_p, level, cap_num,
                               SDP_ATTR_SDESCRIPTIONS, inst_num);

        if (attr_p == NULL) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s srtp attribute salt, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return NULL;
        }
    }

    return (char*) attr_p->attr.srtp_context.master_salt;

}





















const char*
sdp_attr_get_sdescriptions_lifetime (sdp_t *sdp_p, uint16_t level,
                                     uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    
    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SRTP_CONTEXT, inst_num);

    if (attr_p == NULL) {
        
        attr_p = sdp_find_attr(sdp_p, level, cap_num,
                               SDP_ATTR_SDESCRIPTIONS, inst_num);

        if (attr_p == NULL) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s srtp attribute lifetime, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return NULL;
        }
    }

    return (char*)attr_p->attr.srtp_context.master_key_lifetime;

}
























sdp_result_e
sdp_attr_get_sdescriptions_mki (sdp_t *sdp_p, uint16_t level,
                                uint8_t cap_num, uint16_t inst_num,
                                const char **mki_value,
                                uint16_t *mki_length)
{
    sdp_attr_t  *attr_p;

    *mki_value = NULL;
    *mki_length = 0;

    
    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SRTP_CONTEXT, inst_num);

    if (attr_p == NULL) {
        
        attr_p = sdp_find_attr(sdp_p, level, cap_num,
                               SDP_ATTR_SDESCRIPTIONS, inst_num);
        if (attr_p == NULL) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s srtp attribute MKI, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return SDP_INVALID_PARAMETER;
        }
    }

    *mki_value = (char*)attr_p->attr.srtp_context.mki;
    *mki_length = attr_p->attr.srtp_context.mki_size_bytes;
    return SDP_SUCCESS;

}






















const char*
sdp_attr_get_sdescriptions_session_params (sdp_t *sdp_p, uint16_t level,
                                           uint8_t cap_num, uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    
    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SRTP_CONTEXT, inst_num);

    if (attr_p == NULL) {
        
        attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SDESCRIPTIONS, inst_num);
        if (attr_p == NULL) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s srtp attribute session params, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return NULL;
        }
    }

    return attr_p->attr.srtp_context.session_parameters;
}




















unsigned char
sdp_attr_get_sdescriptions_key_size (sdp_t *sdp_p,
                                     uint16_t level,
                                     uint8_t cap_num,
                                     uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    
    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SRTP_CONTEXT, inst_num);

    if (attr_p == NULL) {
        
        attr_p = sdp_find_attr(sdp_p, level, cap_num,
                               SDP_ATTR_SDESCRIPTIONS, inst_num);
        if (attr_p == NULL) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s srtp attribute MKI, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return SDP_SDESCRIPTIONS_KEY_SIZE_UNKNOWN;
        }
    }

    return attr_p->attr.srtp_context.master_key_size_bytes;

}




















unsigned char
sdp_attr_get_sdescriptions_salt_size (sdp_t *sdp_p,
                                      uint16_t level,
                                      uint8_t cap_num,
                                      uint16_t inst_num)
{

    sdp_attr_t  *attr_p;

    
    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SRTP_CONTEXT, inst_num);

    if (attr_p == NULL) {
        
        attr_p = sdp_find_attr(sdp_p, level, cap_num,
                               SDP_ATTR_SDESCRIPTIONS, inst_num);
        if (attr_p == NULL) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s srtp attribute MKI, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return SDP_SDESCRIPTIONS_KEY_SIZE_UNKNOWN;
        }
    }

    return attr_p->attr.srtp_context.master_salt_size_bytes;

}





















unsigned long
sdp_attr_get_srtp_crypto_selection_flags (sdp_t *sdp_p,
                                          uint16_t level,
                                          uint8_t cap_num,
                                          uint16_t inst_num)
{


    sdp_attr_t  *attr_p;

    
    attr_p = sdp_find_attr(sdp_p, level, cap_num,
                           SDP_ATTR_SRTP_CONTEXT, inst_num);

    if (attr_p == NULL) {
        
        attr_p = sdp_find_attr(sdp_p, level, cap_num,
                               SDP_ATTR_SDESCRIPTIONS, inst_num);
        if (attr_p == NULL) {
            if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s srtp attribute MKI, level %u instance %u "
                          "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
            }
            sdp_p->conf_p->num_invalid_param++;
            return SDP_SRTP_CRYPTO_SELECTION_FLAGS_UNKNOWN;
        }
    }

    return attr_p->attr.srtp_context.selection_flags;

}














sdp_attr_t *
sdp_find_rtcp_fb_attr (sdp_t *sdp_p,
                       uint16_t level,
                       uint16_t payload_type,
                       sdp_rtcp_fb_type_e fb_type,
                       uint16_t inst_num)
{
    uint16_t          attr_count=0;
    sdp_mca_t   *mca_p;
    sdp_attr_t  *attr_p;

    mca_p = sdp_find_media_level(sdp_p, level);
    if (!mca_p) {
        return (NULL);
    }
    for (attr_p = mca_p->media_attrs_p; attr_p; attr_p = attr_p->next_p) {
        if (attr_p->type == SDP_ATTR_RTCP_FB &&
            (attr_p->attr.rtcp_fb.payload_num == payload_type ||
             attr_p->attr.rtcp_fb.payload_num == SDP_ALL_PAYLOADS) &&
            attr_p->attr.rtcp_fb.feedback_type == fb_type) {
            attr_count++;
            if (attr_count == inst_num) {
                return (attr_p);
            }
        }
    }
    return NULL;
}









sdp_rtcp_fb_ack_type_e
sdp_attr_get_rtcp_fb_ack(sdp_t *sdp_p, uint16_t level, uint16_t payload_type, uint16_t inst)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_rtcp_fb_attr(sdp_p, level, payload_type,
                                   SDP_RTCP_FB_ACK, inst);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtcp-fb attribute, level %u, pt %u, "
                      "instance %u not found.", sdp_p->debug_str, (unsigned)level,
                      (unsigned)payload_type, (unsigned)inst);
        }
        sdp_p->conf_p->num_invalid_param++;
        return SDP_RTCP_FB_ACK_NOT_FOUND;
    }
    return (attr_p->attr.rtcp_fb.param.ack);
}









sdp_rtcp_fb_nack_type_e
sdp_attr_get_rtcp_fb_nack(sdp_t *sdp_p, uint16_t level, uint16_t payload_type, uint16_t inst)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_rtcp_fb_attr(sdp_p, level, payload_type,
                                   SDP_RTCP_FB_NACK, inst);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtcp-fb attribute, level %u, pt %u, "
                      "instance %u not found.", sdp_p->debug_str, (unsigned)level,
                      (unsigned)payload_type, (unsigned)inst);
        }
        sdp_p->conf_p->num_invalid_param++;
        return SDP_RTCP_FB_NACK_NOT_FOUND;
    }
    return (attr_p->attr.rtcp_fb.param.nack);
}









uint32_t
sdp_attr_get_rtcp_fb_trr_int(sdp_t *sdp_p, uint16_t level,
                             uint16_t payload_type, uint16_t inst)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_rtcp_fb_attr(sdp_p, level, payload_type,
                                   SDP_RTCP_FB_TRR_INT, inst);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtcp-fb attribute, level %u, pt %u, "
                      "instance %u not found.", sdp_p->debug_str, (unsigned)level,
                      (unsigned)payload_type, (unsigned)inst);
        }
        sdp_p->conf_p->num_invalid_param++;
        return 0xFFFFFFFF;
    }
    return (attr_p->attr.rtcp_fb.param.trr_int);
}









sdp_rtcp_fb_ccm_type_e
sdp_attr_get_rtcp_fb_ccm(sdp_t *sdp_p, uint16_t level, uint16_t payload_type, uint16_t inst)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_rtcp_fb_attr(sdp_p, level, payload_type,
                                   SDP_RTCP_FB_CCM, inst);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtcp-fb attribute, level %u, pt %u, "
                      "instance %u not found.", sdp_p->debug_str, (unsigned)level,
                      (unsigned)payload_type, (unsigned)inst);
        }
        sdp_p->conf_p->num_invalid_param++;
        return SDP_RTCP_FB_CCM_NOT_FOUND;
    }
    return (attr_p->attr.rtcp_fb.param.ccm);
}












sdp_result_e
sdp_attr_set_rtcp_fb_ack(sdp_t *sdp_p, uint16_t level, uint16_t payload_type, uint16_t inst,
                         sdp_rtcp_fb_ack_type_e type)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_RTCP_FB, inst);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtcp_fb ack attribute, level %u "
                      "instance %u not found.", sdp_p->debug_str, (unsigned)level,
                      (unsigned)inst);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    attr_p->attr.rtcp_fb.payload_num = payload_type;
    attr_p->attr.rtcp_fb.feedback_type = SDP_RTCP_FB_ACK;
    attr_p->attr.rtcp_fb.param.ack = type;
    attr_p->attr.rtcp_fb.extra[0] = '\0';
    return (SDP_SUCCESS);
}













sdp_result_e
sdp_attr_set_rtcp_fb_nack(sdp_t *sdp_p, uint16_t level, uint16_t payload_type, uint16_t inst,
                          sdp_rtcp_fb_nack_type_e type)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_RTCP_FB, inst);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtcp_fb nack attribute, level %u "
                      "instance %u not found.", sdp_p->debug_str, (unsigned)level,
                      (unsigned)inst);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    attr_p->attr.rtcp_fb.payload_num = payload_type;
    attr_p->attr.rtcp_fb.feedback_type = SDP_RTCP_FB_NACK;
    attr_p->attr.rtcp_fb.param.nack = type;
    attr_p->attr.rtcp_fb.extra[0] = '\0';
    return (SDP_SUCCESS);
}












sdp_result_e
sdp_attr_set_rtcp_fb_trr_int(sdp_t *sdp_p, uint16_t level, uint16_t payload_type,
                             uint16_t inst, uint32_t interval)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_RTCP_FB, inst);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtcp_fb trr-int attribute, level %u "
                      "instance %u not found.", sdp_p->debug_str, (unsigned)level,
                      (unsigned)inst);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    attr_p->attr.rtcp_fb.payload_num = payload_type;
    attr_p->attr.rtcp_fb.feedback_type = SDP_RTCP_FB_TRR_INT;
    attr_p->attr.rtcp_fb.param.trr_int = interval;
    attr_p->attr.rtcp_fb.extra[0] = '\0';
    return (SDP_SUCCESS);
}












sdp_result_e
sdp_attr_set_rtcp_fb_ccm(sdp_t *sdp_p, uint16_t level, uint16_t payload_type, uint16_t inst,
                         sdp_rtcp_fb_ccm_type_e type)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_RTCP_FB, inst);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s rtcp_fb ccm attribute, level %u "
                      "instance %u not found.", sdp_p->debug_str, (unsigned)level,
                      (unsigned)inst);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    attr_p->attr.rtcp_fb.payload_num = payload_type;
    attr_p->attr.rtcp_fb.feedback_type = SDP_RTCP_FB_CCM;
    attr_p->attr.rtcp_fb.param.ccm = type;
    attr_p->attr.rtcp_fb.extra[0] = '\0';
    return (SDP_SUCCESS);
}












const char *sdp_attr_get_extmap_uri(sdp_t *sdp_p, uint16_t level,
                                    uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_EXTMAP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s extmap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (NULL);
    } else {
        return (attr_p->attr.extmap.uri);
    }
}










uint16_t sdp_attr_get_extmap_id(sdp_t *sdp_p, uint16_t level,
                           uint16_t inst_num)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_EXTMAP, inst_num);
    if (attr_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s extmap attribute, level %u instance %u "
                      "not found.", sdp_p->debug_str, (unsigned)level, (unsigned)inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return 0xFFFF;
    } else {
        return (attr_p->attr.extmap.id);
    }
}











sdp_result_e
sdp_attr_set_extmap(sdp_t *sdp_p, uint16_t level, uint16_t id, const char* uri, uint16_t inst)
{
    sdp_attr_t  *attr_p;

    attr_p = sdp_find_attr(sdp_p, level, 0, SDP_ATTR_EXTMAP, inst);
    if (!attr_p) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s extmap attribute, level %u "
                      "instance %u not found.", sdp_p->debug_str, (unsigned)level,
                      (unsigned)inst);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    attr_p->attr.extmap.id = id;
    sstrncpy(attr_p->attr.extmap.uri, uri, SDP_MAX_STRING_LEN+1);
    return (SDP_SUCCESS);
}

const char *sdp_attr_get_msid_identifier(sdp_t *sdp_p, uint16_t level,
                                       uint8_t cap_num, uint16_t inst)
{
    sdp_attr_t  *attr_p = sdp_find_attr(sdp_p, level, cap_num,
                                        SDP_ATTR_MSID, inst);
    if (!attr_p) {
      return NULL;
    }
    return attr_p->attr.msid.identifier;
}

const char *sdp_attr_get_msid_appdata(sdp_t *sdp_p, uint16_t level,
                                      uint8_t cap_num, uint16_t inst)
{
    sdp_attr_t  *attr_p = sdp_find_attr(sdp_p, level, cap_num,
                                        SDP_ATTR_MSID, inst);
    if (!attr_p) {
      return NULL;
    }
    return attr_p->attr.msid.appdata;
}
