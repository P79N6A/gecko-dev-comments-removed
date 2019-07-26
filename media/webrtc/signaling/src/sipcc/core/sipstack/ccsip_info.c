






































#include "cpr_types.h"
#include "cpr_stdio.h"

#include "ccsip_core.h"
#include "ccsip_callinfo.h"
#include "ccsip_messaging.h"
#include "uiapi.h"
#include "lsm.h"
#include "fsm.h"
#include "vcm.h"
#include "phone_debug.h"
#include "singly_link_list.h"
#include "xml_util.h"
#include "ccapi.h"

extern int httpish_strncasecmp(const char *s1, const char *s2, size_t len);

typedef unsigned int info_index_t;
typedef unsigned int type_index_t;

typedef struct {
    info_package_handler_t handler;
    info_index_t info_index;    
                                
    type_index_t type_index;    
                                
} handler_record_t;


static sll_handle_t s_handler_registry = NULL;

#define INDEX_NOT_FOUND     ((unsigned int)-1)





char *g_registered_info[MAX_INFO_HANDLER];





static char *s_registered_type[MAX_INFO_HANDLER];

#ifdef _CONF_ROSTER_
#define INFO_PACKAGE_CISCO_CONFERENCE         "x-cisco-conference"
#define INFO_PACKAGE_CONFERENCE         "conference"
#define CONTENT_TYPE_CONFERENCE_INFO    "application/conference-info+xml"
#endif

static sll_match_e is_matching_type(void *find_by_p, void *data_p);
static info_index_t find_info_index(const char *info_package);
static info_index_t find_next_available_info_index(void);
static type_index_t find_type_index(const char *type_package);
static type_index_t find_next_available_type_index(void);
static handler_record_t *find_handler_record(info_index_t info_index,
                                             type_index_t type_index);
static boolean is_info_package_registered(info_index_t info_index);
static boolean is_content_type_registered(type_index_t type_index);
static void update_recv_info_list(const char *header_field_value,
                                  string_t *info_packages);
static void media_control_info_package_handler(line_t line, callid_t call_id,
                                               const char *info_package,
                                               const char *content_type,
                                               const char *message_body);
#ifdef _CONF_ROSTER_
static void conf_info_package_handler(line_t line, callid_t call_id,
                                           const char *info_package,
                                           const char *content_type,
                                           const char *message_body);
#endif
















static info_index_t
find_info_index(const char *info_package)
{
    info_index_t info_index;

    for (info_index = 0; info_index < MAX_INFO_HANDLER; info_index++) {
        if (g_registered_info[info_index] &&
            httpish_strncasecmp(info_package,
                                g_registered_info[info_index],
                                strlen(g_registered_info[info_index])) == 0) {
            return info_index;
        }
    }

    return INDEX_NOT_FOUND;
}














static info_index_t
find_next_available_info_index(void)
{
    info_index_t info_index;

    for (info_index = 0; info_index < MAX_INFO_HANDLER; info_index++) {
        if (g_registered_info[info_index] == NULL) {
            return info_index;
        }
    }

    return INDEX_NOT_FOUND;
}














static type_index_t
find_type_index(const char *content_type)
{
    type_index_t type_index;

    for (type_index = 0; type_index < MAX_INFO_HANDLER; type_index++) {
        if (s_registered_type[type_index] &&
            httpish_strncasecmp(content_type,
                                s_registered_type[type_index],
                                strlen(s_registered_type[type_index])) == 0) {
            return type_index;
        }
    }

    return INDEX_NOT_FOUND;
}














static type_index_t
find_next_available_type_index(void)
{
    type_index_t type_index;

    for (type_index = 0; type_index < MAX_INFO_HANDLER; type_index++) {
        if (s_registered_type[type_index] == NULL) {
            return type_index;
        }
    }

    return INDEX_NOT_FOUND;
}















static sll_match_e
is_matching_type(void *find_by_p, void *data_p)
{
    handler_record_t *tuple = (handler_record_t *)find_by_p;
    handler_record_t *node = (handler_record_t *)data_p;

    if ((node->info_index == tuple->info_index) &&
        (node->type_index == tuple->type_index)) {
        return SLL_MATCH_FOUND;
    }
    return SLL_MATCH_NOT_FOUND;
}
















static handler_record_t *
find_handler_record(info_index_t info_index, type_index_t type_index)
{
    handler_record_t tuple;

    tuple.info_index = info_index;
    tuple.type_index = type_index;

    return (handler_record_t *)sll_find(s_handler_registry, &tuple);
}














static boolean
is_info_package_registered(info_index_t info_index)
{
    handler_record_t *record;

    for (record = (handler_record_t *)sll_next(s_handler_registry, NULL);
         record != NULL;
         record = (handler_record_t *)sll_next(s_handler_registry, record)) {
        if (record->info_index == info_index) {
            return TRUE;
        }
    }

    return FALSE;
}














static boolean
is_content_type_registered(type_index_t type_index)
{
    handler_record_t *record;

    for (record = (handler_record_t *)sll_next(s_handler_registry, NULL);
         record != NULL;
         record = (handler_record_t *)sll_next(s_handler_registry, record)) {
        if (record->type_index == type_index) {
            return TRUE;
        }
    }

    return FALSE;
}
















int
ccsip_register_info_package_handler(const char *info_package,
                                    const char *content_type,
                                    info_package_handler_t handler)
{
    static const char *fname = "ccsip_register_info_package_handler";
    info_index_t info_index;
    type_index_t type_index;
    char *tmp_info = NULL;
    char *tmp_type = NULL;
    handler_record_t *record;

    if (s_handler_registry == NULL) {
        CCSIP_DEBUG_TASK("%s: Info Package handler was not initialized\n", fname);
        return SIP_ERROR;
    }

    if ((info_package == NULL) || (content_type == NULL) || (handler == NULL)) {
        CCSIP_DEBUG_ERROR("%s: invalid parameter\n", fname);
        return SIP_ERROR;
    }

    
    info_index = find_info_index(info_package);

    if (info_index == INDEX_NOT_FOUND) {
        
        info_index = find_next_available_info_index();

        if (info_index == INDEX_NOT_FOUND) {
            CCSIP_DEBUG_ERROR("%s: maximum reached\n", fname);
            return SIP_ERROR;
        }

        tmp_info = cpr_strdup(info_package);
        if (tmp_info == NULL) {
            CCSIP_DEBUG_ERROR("%s: failed to duplicate info_package string\n", fname);
            return SIP_ERROR;
        }
    }

    
    type_index = find_type_index(content_type);

    if (type_index == INDEX_NOT_FOUND) {
        
        type_index = find_next_available_type_index();

        if (type_index == INDEX_NOT_FOUND) {
            CCSIP_DEBUG_ERROR("%s: maximum reached\n", fname);
            if (tmp_info != NULL) {
                cpr_free(tmp_info);
            }
            return SIP_ERROR;
        }

        tmp_type = cpr_strdup(content_type);
        if (tmp_type == NULL) {
            CCSIP_DEBUG_ERROR("%s: failed to duplicate info_package string\n", fname);
            if (tmp_info != NULL) {
                cpr_free(tmp_info);
            }
            return SIP_ERROR;
        }
    }

    
    if (find_handler_record(info_index, type_index) != NULL) {
        CCSIP_DEBUG_ERROR("%s: Info Package handler already registered\n", fname);
        return SIP_ERROR;
    }

    










    record = (handler_record_t *)cpr_malloc(sizeof(handler_record_t));
    if (record == NULL) {
        if (tmp_type != NULL) {
            cpr_free(tmp_type);
        }
        if (tmp_info != NULL) {
            cpr_free(tmp_info);
        }
        CCSIP_DEBUG_ERROR("%s: failed to allocate info handler record\n", fname);
        return SIP_ERROR;
    }

    record->handler = handler;
    record->info_index = info_index;
    record->type_index = type_index;

    if (sll_append(s_handler_registry, record) != SLL_RET_SUCCESS) {
        cpr_free(record);
        if (tmp_type != NULL) {
            cpr_free(tmp_type);
        }
        if (tmp_info != NULL) {
            cpr_free(tmp_info);
        }
        CCSIP_DEBUG_ERROR("%s: failed to insert to the registry\n", fname);
        return SIP_ERROR;
    }

    if (tmp_info != NULL) {
        g_registered_info[info_index] = tmp_info;
    }
    if (tmp_type != NULL) {
        s_registered_type[type_index] = tmp_type;
    }

    return SIP_OK;
}
















int
ccsip_deregister_info_package_handler(const char *info_package,
                                      const char *content_type,
                                      info_package_handler_t handler)
{
    static const char *fname = "ccsip_deregister_info_package_handler";
    info_index_t info_index;
    type_index_t type_index;
    handler_record_t *record;

    if (s_handler_registry == NULL) {
        CCSIP_DEBUG_TASK("%s: Info Package handler was not initialized\n", fname);
        return SIP_ERROR;
    }

    
    info_index = find_info_index(info_package);
    if (info_index == INDEX_NOT_FOUND) {
        CCSIP_DEBUG_ERROR("%s: handler was not registered (%s)\n",
                          fname, info_package);
        return SIP_ERROR;
    }

    
    type_index = find_type_index(content_type);
    if (type_index == INDEX_NOT_FOUND) {
        CCSIP_DEBUG_ERROR("%s: handler was not registered (%s)\n",
                          fname, content_type);
        return SIP_ERROR;
    }

    
    record = find_handler_record(info_index, type_index);
    if ((record == NULL) || (record->handler != handler)) {
        CCSIP_DEBUG_ERROR("%s: handler was not registered (%p)\n",
                          fname, handler);
        return SIP_ERROR;
    }

    (void)sll_remove(s_handler_registry, record);

    cpr_free(record);

    if (!is_info_package_registered(info_index)) {
        

        cpr_free(g_registered_info[info_index]);
        g_registered_info[info_index] = NULL;
    }

    if (!is_content_type_registered(type_index)) {
        

        cpr_free(s_registered_type[type_index]);
        s_registered_type[type_index] = NULL;
    }

    return SIP_OK;
}


















static void
update_recv_info_list(const char *header_field_value, string_t *info_packages)
{
    static const char *fname = "update_recv_info_list";
    info_index_t info_index;

    if ((header_field_value == NULL) || (info_packages == NULL) ||
        (*info_packages == NULL)) {
        CCSIP_DEBUG_ERROR("%s: invalid parameter\n", fname);
        return;
    }

    info_index = find_info_index(header_field_value);
    if (info_index != INDEX_NOT_FOUND) {
        
        if (**info_packages == '\0') {
            *info_packages = strlib_update(*info_packages,
                                           g_registered_info[info_index]);
        } else {
            *info_packages = strlib_append(*info_packages, ", ");
            *info_packages = strlib_append(*info_packages,
                                           g_registered_info[info_index]);
        }
    }
}
















void
ccsip_parse_send_info_header(sipMessage_t *pSipMessage, string_t *recv_info_list)
{
    char *send_info[MAX_INFO_HANDLER];
    int count;
    int i;
    char *header_field_values;
    char *header_field_value;
    char *separator;

    
    count = sippmh_get_num_particular_headers(pSipMessage,
                                              SIP_HEADER_SEND_INFO,
                                              NULL,
                                              send_info,
                                              MAX_INFO_HANDLER);

    if (count == 0) {
        return;
    }

    for (i = 0; (i < count) && (i < MAX_INFO_HANDLER); i++) {
        header_field_values = cpr_strdup(send_info[i]);
        if (header_field_values == NULL) {
            return;
        }
        header_field_value = header_field_values;

        while ((separator = strchr(header_field_value, COMMA)) != NULL) {
            *separator++ = '\0';
            update_recv_info_list(header_field_value, recv_info_list);
            header_field_value = separator;
            SKIP_WHITE_SPACE(header_field_value);
        }
        update_recv_info_list(header_field_value, recv_info_list);

        cpr_free(header_field_values);
    }
}

















int
ccsip_handle_info_package(ccsipCCB_t *ccb, sipMessage_t *pSipMessage)
{
    static const char *fname = "ccsip_handle_info_package";
    const char  *info_package;
    const char  *content_type;
    info_index_t info_index;
    type_index_t type_index;
    handler_record_t *record;
    uint16_t     status_code;
    const char  *reason_phrase;
    int          return_code = SIP_ERROR;

    


    
    content_type = sippmh_get_cached_header_val(pSipMessage,
                                                CONTENT_TYPE);
    if (content_type &&
        httpish_strncasecmp(content_type,
                            SIP_CONTENT_TYPE_MEDIA_CONTROL,
                            strlen(SIP_CONTENT_TYPE_MEDIA_CONTROL)) == 0) {

        media_control_info_package_handler(ccb->dn_line, ccb->gsm_id,
                                           "",  
                                           SIP_CONTENT_TYPE_MEDIA_CONTROL,
                                           pSipMessage->mesg_body[0].msgBody);

        if (sipSPISendErrorResponse(pSipMessage, 200, SIP_SUCCESS_SETUP_PHRASE,
                                    0, NULL, NULL) != TRUE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                              fname, SIP_SUCCESS_SETUP_PHRASE);
            return SIP_ERROR;
        }

        return SIP_OK;
    }

    


    
    info_package = sippmh_get_header_val(pSipMessage,
                                         SIP_HEADER_INFO_PACKAGE,
                                         NULL);

    if (info_package == NULL) {
        
        CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Missing Info-Package header\n",
                            DEB_F_PREFIX_ARGS(SIP_INFO_PACKAGE, fname));

        if (pSipMessage->num_body_parts == 0) {
            
            CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Missing message body\n",
                                DEB_F_PREFIX_ARGS(SIP_INFO_PACKAGE, fname));
            
            status_code = 200;
            reason_phrase = SIP_SUCCESS_SETUP_PHRASE;
            return_code = SIP_OK;
        } else {
            
            if (pSipMessage->num_body_parts > 1) {
                CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Multipart Info Package"
                                    DEB_F_PREFIX_ARGS(SIP_INFO_PACKAGE, fname));
            }

            type_index = find_type_index(pSipMessage->mesg_body[0].msgContentType);
            if (type_index == INDEX_NOT_FOUND) {
                
                CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Unsupported Content Type\n",
                                    DEB_F_PREFIX_ARGS(SIP_INFO_PACKAGE, fname));
                
                status_code = SIP_CLI_ERR_MEDIA;
                reason_phrase = SIP_CLI_ERR_MEDIA_PHRASE;
            } else {
                
                
                status_code = 200;
                reason_phrase = SIP_SUCCESS_SETUP_PHRASE;
                return_code = SIP_OK;
            }
        }
    } else {
        
        if (pSipMessage->num_body_parts == 0) {
            
            CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Missing message body\n",
                                DEB_F_PREFIX_ARGS(SIP_INFO_PACKAGE, fname));

            
            
            status_code = SIP_CLI_ERR_BAD_EVENT;
            reason_phrase = SIP_CLI_ERR_BAD_EVENT_PHRASE;

        } else {
            
            if (pSipMessage->num_body_parts > 1) {
                CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Multipart Info Package "
                                    "(only the first part is processed)\n",
                                    DEB_F_PREFIX_ARGS(SIP_INFO_PACKAGE, fname));
            }

            info_index = find_info_index(info_package);
            if (info_index == INDEX_NOT_FOUND) {
                
                CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Unsupported Info Package\n",
                                    DEB_F_PREFIX_ARGS(SIP_INFO_PACKAGE, fname));

                
                status_code = SIP_CLI_ERR_BAD_EVENT;
                reason_phrase = SIP_CLI_ERR_BAD_EVENT_PHRASE;

            } else {
                
                type_index = find_type_index(pSipMessage->mesg_body[0].msgContentType);
                record = find_handler_record(info_index, type_index);
                if (record == NULL) {
                    
                    CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Unsupported Content Type\n",
                                        DEB_F_PREFIX_ARGS(SIP_INFO_PACKAGE, fname));
                    
                    status_code = SIP_CLI_ERR_MEDIA;
                    reason_phrase = SIP_CLI_ERR_MEDIA_PHRASE;
                } else {
                    
                    (*record->handler)(ccb->dn_line, ccb->gsm_id,
                                       g_registered_info[record->info_index],
                                       s_registered_type[record->type_index],
                                       pSipMessage->mesg_body[0].msgBody);
                    
                    status_code = 200;
                    reason_phrase = SIP_SUCCESS_SETUP_PHRASE;
                    return_code = SIP_OK;
                }
            }
        }
    }

    if (sipSPISendErrorResponse(pSipMessage, status_code, reason_phrase,
                                0, NULL, NULL) != TRUE) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                          fname, reason_phrase);
        return SIP_ERROR;
    }

    return return_code;
}

static void
media_control_info_package_handler(line_t line, callid_t call_id,
                                   const char *info_package,
                                   const char *content_type,
                                   const char *message_body)
{
    static const char *fname = "media_control_info_package_handler";
    ccsip_event_data_t * evt_data_ptr = NULL;

    if (xmlDecodeEventData(CC_SUBSCRIPTIONS_MEDIA_INFO,
                        message_body,
                        strlen(message_body),
                        &evt_data_ptr) == SIP_ERROR) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Could not allocate Libxml Context", fname);
    }

    if (evt_data_ptr != NULL) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"media picture_fast_update value=%d", 
            fname, evt_data_ptr->u.media_control_data.picture_fast_update);
        cc_feature(CC_SRC_SIP, call_id, line, CC_FEATURE_FAST_PIC_UPD, NULL);
        free_event_data(evt_data_ptr);
    }
}

#ifdef _CONF_ROSTER_
static void
conf_info_package_handler(line_t line, callid_t call_id,
                               const char *info_package,
                               const char *content_type,
                               const char *message_body)
{
    static const char *fname = "conf_info_package_handler";

    CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"info_package: %s content_type: %s\n",
                        DEB_F_PREFIX_ARGS(SIP_INFO_PACKAGE, fname),
                        info_package, content_type);

    ui_info_received(line, lsm_get_ui_id(call_id), info_package, content_type,
                     message_body);
}
#endif














int
ccsip_info_package_handler_init(void)
{
    static const char *fname = "ccsip_info_package_handler_init";
    info_index_t info_index;
    type_index_t type_index;

    if (s_handler_registry != NULL) {
        
        CCSIP_DEBUG_TASK("%s: Info Package handler already initialized\n", fname);
        return SIP_OK;
    }

    
    s_handler_registry = sll_create(is_matching_type);
    if (s_handler_registry == NULL) {
        CCSIP_DEBUG_ERROR("%s: failed to create the registry\n", fname);
        return SIP_ERROR;
    }

    for (info_index = 0; info_index < MAX_INFO_HANDLER; info_index++) {
        g_registered_info[info_index] = NULL;
    }

    for (type_index = 0; type_index < MAX_INFO_HANDLER; type_index++) {
        s_registered_type[type_index] = NULL;
    }

    
#ifdef _CONF_ROSTER_
    
    ccsip_register_info_package_handler(INFO_PACKAGE_CONFERENCE,
                                        CONTENT_TYPE_CONFERENCE_INFO,
                                        conf_info_package_handler);
    ccsip_register_info_package_handler(INFO_PACKAGE_CISCO_CONFERENCE,
                                        CONTENT_TYPE_CONFERENCE_INFO,
                                        conf_info_package_handler);
#endif
    return SIP_OK;
}













void
ccsip_info_package_handler_shutdown(void)
{
    static const char *fname = "ccsip_info_package_handler_shutdown";
    info_index_t info_index;
    type_index_t type_index;
    handler_record_t *record;

    if (s_handler_registry == NULL) {
        
        CCSIP_DEBUG_TASK("%s: Info Package handler was not initialized\n", fname);
        return;
    }

    for (type_index = 0; type_index < MAX_INFO_HANDLER; type_index++) {
        if (s_registered_type[type_index] != NULL) {
            cpr_free(s_registered_type[type_index]);
            s_registered_type[type_index] = NULL;
        }
    }

    for (info_index = 0; info_index < MAX_INFO_HANDLER; info_index++) {
        if (g_registered_info[info_index] != NULL) {
            cpr_free(g_registered_info[info_index]);
            g_registered_info[info_index] = NULL;
        }
    }

    
    for (record = (handler_record_t *)sll_next(s_handler_registry, NULL);
         record != NULL;
         record = (handler_record_t *)sll_next(s_handler_registry, record)) {
        cpr_free(record);
    }

    
    sll_destroy(s_handler_registry);
    s_handler_registry = NULL;
}
