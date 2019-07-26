
































static char *RCSSTRING __UNUSED__="$Id: stun_proc.c,v 1.2 2008/04/28 18:21:30 ekr Exp $";

#include <errno.h>
#include <csi_platform.h>

#ifdef WIN32
#include <winsock2.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>
#else   
#include <string.h>
#endif  
#include <assert.h>

#include "stun.h"
#include "stun_reg.h"
#include "registry.h"

static int
nr_stun_add_realm_and_nonce(int new_nonce, nr_stun_server_client *clnt, nr_stun_message *res);


int
nr_stun_receive_message(nr_stun_message *req, nr_stun_message *msg)
{
    int _status;
    nr_stun_message_attribute *attr;

#ifdef USE_RFC_3489_BACKWARDS_COMPATIBLE
    


    if (msg->header.magic_cookie == NR_STUN_MAGIC_COOKIE
     || msg->header.magic_cookie == NR_STUN_MAGIC_COOKIE2) {
#endif
    if (!nr_is_stun_message(msg->buffer, msg->length)) {
        r_log(NR_LOG_STUN, LOG_DEBUG, "Not a STUN message");
        ABORT(R_REJECTED);
    }
#ifdef USE_RFC_3489_BACKWARDS_COMPATIBLE
    }
#endif 

    if (req == 0) {
        if (NR_STUN_GET_TYPE_CLASS(msg->header.type) != NR_CLASS_REQUEST) {
            r_log(NR_LOG_STUN,LOG_NOTICE,"Illegal message type: %03x", msg->header.type);
            ABORT(R_REJECTED);
        }
    }
    else {
        if (NR_STUN_GET_TYPE_CLASS(msg->header.type) != NR_CLASS_RESPONSE
         && NR_STUN_GET_TYPE_CLASS(msg->header.type) != NR_CLASS_ERROR_RESPONSE) {
            r_log(NR_LOG_STUN,LOG_NOTICE,"Illegal message class: %03x", msg->header.type);
            ABORT(R_REJECTED);
        }

        if (NR_STUN_GET_TYPE_METHOD(req->header.type) != NR_STUN_GET_TYPE_METHOD(msg->header.type)) {
            r_log(NR_LOG_STUN,LOG_NOTICE,"Inconsistent message method: %03x", msg->header.type);
            ABORT(R_REJECTED);
        }

        if (nr_stun_different_transaction(msg->buffer, msg->length, req)) {
            r_log(NR_LOG_STUN, LOG_DEBUG, "Unrecognized STUN transaction");
            ABORT(R_REJECTED);
        }
    }

    switch (msg->header.magic_cookie) {
    case NR_STUN_MAGIC_COOKIE:
        

        if (nr_stun_message_has_attribute(msg, NR_STUN_ATTR_FINGERPRINT, &attr)
         && !attr->u.fingerprint.valid) {
            r_log(NR_LOG_STUN, LOG_DEBUG, "Invalid fingerprint");
            ABORT(R_REJECTED);
        }

        break;

#ifdef USE_STUND_0_96
    case NR_STUN_MAGIC_COOKIE2:
        
        break;
#endif 

    default:
#ifdef USE_RFC_3489_BACKWARDS_COMPATIBLE
        
#else
#ifdef NDEBUG
        
        r_log(NR_LOG_STUN, LOG_ERR, "Missing Magic Cookie");
        ABORT(R_REJECTED);
#else
        



#endif 
#endif 
        break;
    }

    _status=0;
  abort:
    return _status;
}


int
nr_stun_process_request(nr_stun_message *req, nr_stun_message *res)
{
    int _status;
#ifdef USE_STUN_PEDANTIC
    int r;
    nr_stun_attr_unknown_attributes unknown_attributes = { { 0 } };
    nr_stun_message_attribute *attr;

    if (req->comprehension_required_unknown_attributes > 0) {
        nr_stun_form_error_response(req, res, 420, "Unknown Attributes");

        TAILQ_FOREACH(attr, &req->attributes, entry) {
            if (attr->name == 0) {
                

                
                if (unknown_attributes.num_attributes > NR_STUN_MAX_UNKNOWN_ATTRIBUTES)
                    break;

                unknown_attributes.attribute[unknown_attributes.num_attributes++] = attr->type;
            }
        }

        assert(req->comprehension_required_unknown_attributes + req->comprehension_optional_unknown_attributes == unknown_attributes.num_attributes);

        if ((r=nr_stun_message_add_unknown_attributes_attribute(res, &unknown_attributes)))
            ABORT(R_ALREADY);

        ABORT(R_ALREADY);
    }
#endif 

    _status=0;
#ifdef USE_STUN_PEDANTIC
  abort:
#endif 
    return _status;
}


int
nr_stun_process_indication(nr_stun_message *ind)
{
    int _status;
#ifdef USE_STUN_PEDANTIC

    if (ind->comprehension_required_unknown_attributes > 0)
        ABORT(R_REJECTED);
#endif 

    _status=0;
#ifdef USE_STUN_PEDANTIC
  abort:
#endif 
    return _status;
}


int
nr_stun_process_success_response(nr_stun_message *res)
{
    int _status;

#ifdef USE_STUN_PEDANTIC
    if (res->comprehension_required_unknown_attributes > 0)
        ABORT(R_REJECTED);
#endif 

    if (NR_STUN_GET_TYPE_METHOD(res->header.type) == NR_METHOD_BINDING) {
        if (! nr_stun_message_has_attribute(res, NR_STUN_ATTR_XOR_MAPPED_ADDRESS, 0)) {
            r_log(NR_LOG_STUN, LOG_ERR, "Missing XOR-MAPPED-ADDRESS");
            ABORT(R_REJECTED);
        }
    }

    _status=0;
 abort:
    return _status;
}


int
nr_stun_process_error_response(nr_stun_message *res)
{
    int _status;
    nr_stun_message_attribute *attr;

    if (res->comprehension_required_unknown_attributes > 0) {
        ABORT(R_REJECTED);
    }

    if (! nr_stun_message_has_attribute(res, NR_STUN_ATTR_ERROR_CODE, &attr)) {
        r_log(NR_LOG_STUN, LOG_ERR, "Missing ERROR-CODE");
        ABORT(R_REJECTED);
    }

    switch (attr->u.error_code.number / 100) {
    case 3:
        



        if (attr->u.error_code.number == 300) {
            if (!nr_stun_message_has_attribute(res, NR_STUN_ATTR_ALTERNATE_SERVER, 0)) {
                r_log(NR_LOG_STUN, LOG_ERR, "Missing ALTERNATE-SERVER");
                ABORT(R_REJECTED);
            }

            
            if (!nr_stun_message_has_attribute(res, NR_STUN_ATTR_MESSAGE_INTEGRITY, 0)) {
                r_log(NR_LOG_STUN, LOG_ERR, "Missing MESSAGE-INTEGRITY");
                ABORT(R_REJECTED);
            }

            ABORT(R_RETRY);
        }

        ABORT(R_REJECTED);
        break;

    case 4:
        



        if (attr->u.error_code.number == 420)
            ABORT(R_REJECTED);

        

        ABORT(R_RETRY);
        break;

    case 5:
        


        
        break;

    default:
        ABORT(R_REJECTED);
        break;
    }

    




    _status=0;
 abort:
    return _status;
}


int
nr_stun_receive_request_or_indication_short_term_auth(nr_stun_message *msg,
                                                      nr_stun_message *res)
{
    int _status;
    nr_stun_message_attribute *attr;

    switch (msg->header.magic_cookie) {
    default:
        
        
    case NR_STUN_MAGIC_COOKIE:
        if (!nr_stun_message_has_attribute(msg, NR_STUN_ATTR_MESSAGE_INTEGRITY, &attr)) {
            nr_stun_form_error_response(msg, res, 400, "Missing MESSAGE-INTEGRITY");
            ABORT(R_ALREADY);
        }

        if (!nr_stun_message_has_attribute(msg, NR_STUN_ATTR_USERNAME, 0)) {
            nr_stun_form_error_response(msg, res, 400, "Missing USERNAME");
            ABORT(R_ALREADY);
        }

        if (attr->u.message_integrity.unknown_user) {
            nr_stun_form_error_response(msg, res, 401, "Unrecognized USERNAME");
            ABORT(R_ALREADY);
        }

        if (!attr->u.message_integrity.valid) {
            nr_stun_form_error_response(msg, res, 401, "Bad MESSAGE-INTEGRITY");
            ABORT(R_ALREADY);
        }

        break;

#ifdef USE_STUND_0_96
    case NR_STUN_MAGIC_COOKIE2:
        
        break;
#endif 
    }

    _status=0;
 abort:
    return _status;
}


int
nr_stun_receive_response_short_term_auth(nr_stun_message *res)
{
    int _status;
    nr_stun_message_attribute *attr;

    switch (res->header.magic_cookie) {
    default:
        
        
    case NR_STUN_MAGIC_COOKIE:
        if (!nr_stun_message_has_attribute(res, NR_STUN_ATTR_MESSAGE_INTEGRITY, &attr)) {
            r_log(NR_LOG_STUN, LOG_DEBUG, "Missing MESSAGE-INTEGRITY");
            ABORT(R_REJECTED);
        }

        if (!attr->u.message_integrity.valid) {
            r_log(NR_LOG_STUN, LOG_DEBUG, "Bad MESSAGE-INTEGRITY");
            ABORT(R_REJECTED);
        }

        break;

#ifdef USE_STUND_0_96
    case NR_STUN_MAGIC_COOKIE2:
        
        break;
#endif 
    }

   _status=0;
 abort:
     return _status;
}

static int
nr_stun_add_realm_and_nonce(int new_nonce, nr_stun_server_client *clnt, nr_stun_message *res)
{
    int r,_status;
    char *realm = 0;
    char *nonce;
    UINT2 size;

    if ((r=NR_reg_alloc_string(NR_STUN_REG_PREF_SERVER_REALM, &realm)))
        ABORT(r);

    if ((r=nr_stun_message_add_realm_attribute(res, realm)))
        ABORT(r);

    if (clnt) {
        if (strlen(clnt->nonce) < 1)
            new_nonce = 1;

        if (new_nonce) {
            if (NR_reg_get_uint2(NR_STUN_REG_PREF_SERVER_NONCE_SIZE, &size))
                size = 48;

            if (size > (sizeof(clnt->nonce) - 1))
                size = sizeof(clnt->nonce) - 1;

            nr_random_alphanum(clnt->nonce, size);
            clnt->nonce[size] = '\0';
        }

        nonce = clnt->nonce;
    }
    else {
        



        nonce = "STALE";
    }

    if ((r=nr_stun_message_add_nonce_attribute(res, nonce)))
        ABORT(r);

    _status=0;
 abort:
#ifdef USE_TURN
assert(_status == 0); 
#endif
    RFREE(realm);
    return _status;
}


int
nr_stun_receive_request_long_term_auth(nr_stun_message *req, nr_stun_server_ctx *ctx, nr_stun_message *res)
{
    int r,_status;
    nr_stun_message_attribute *mi;
    nr_stun_message_attribute *n;
    nr_stun_server_client *clnt = 0;

    switch (req->header.magic_cookie) {
    default:
        
        
    case NR_STUN_MAGIC_COOKIE:
        if (!nr_stun_message_has_attribute(req, NR_STUN_ATTR_USERNAME, 0)) {
            nr_stun_form_error_response(req, res, 400, "Missing USERNAME");
            nr_stun_add_realm_and_nonce(0, 0, res);
            ABORT(R_ALREADY);
        }

        if ((r=nr_stun_get_message_client(ctx, req, &clnt))) {
            nr_stun_form_error_response(req, res, 401, "Unrecognized USERNAME");
            nr_stun_add_realm_and_nonce(0, 0, res);
            ABORT(R_ALREADY);
        }

        if (!nr_stun_message_has_attribute(req, NR_STUN_ATTR_MESSAGE_INTEGRITY, &mi)) {
            nr_stun_form_error_response(req, res, 401, "Missing MESSAGE-INTEGRITY");
            nr_stun_add_realm_and_nonce(0, clnt, res);
            ABORT(R_ALREADY);
        }

        assert(!mi->u.message_integrity.unknown_user);

        if (!nr_stun_message_has_attribute(req, NR_STUN_ATTR_REALM, 0)) {
            nr_stun_form_error_response(req, res, 400, "Missing REALM");
            ABORT(R_ALREADY);
        }

        if (!nr_stun_message_has_attribute(req, NR_STUN_ATTR_NONCE, &n)) {
            nr_stun_form_error_response(req, res, 400, "Missing NONCE");
            ABORT(R_ALREADY);
        }

        assert(sizeof(clnt->nonce) == sizeof(n->u.nonce));
        if (strncmp(clnt->nonce, n->u.nonce, sizeof(n->u.nonce))) {
            nr_stun_form_error_response(req, res, 438, "Stale NONCE");
            nr_stun_add_realm_and_nonce(1, clnt, res);
            ABORT(R_ALREADY);
        }

        if (!mi->u.message_integrity.valid) {
            nr_stun_form_error_response(req, res, 401, "Bad MESSAGE-INTEGRITY");
            nr_stun_add_realm_and_nonce(0, clnt, res);
            ABORT(R_ALREADY);
        }

        break;

#ifdef USE_STUND_0_96
    case NR_STUN_MAGIC_COOKIE2:
        
        break;
#endif 
    }

    _status=0;
 abort:

    return _status;
}


int
nr_stun_receive_response_long_term_auth(nr_stun_message *res, nr_stun_client_ctx *ctx)
{
    int _status;
    nr_stun_message_attribute *attr;

    switch (res->header.magic_cookie) {
    default:
        
        
    case NR_STUN_MAGIC_COOKIE:
        if (nr_stun_message_has_attribute(res, NR_STUN_ATTR_REALM, &attr)) {
            RFREE(ctx->realm);
            ctx->realm = r_strdup(attr->u.realm);
            if (!ctx->realm)
                ABORT(R_NO_MEMORY);
        }
        else {
            r_log(NR_LOG_STUN, LOG_DEBUG, "Missing REALM");
            ABORT(R_REJECTED);
        }

        if (nr_stun_message_has_attribute(res, NR_STUN_ATTR_NONCE, &attr)) {
            RFREE(ctx->nonce);
            ctx->nonce = r_strdup(attr->u.nonce);
            if (!ctx->nonce)
                ABORT(R_NO_MEMORY);
        }
        else {
            r_log(NR_LOG_STUN, LOG_DEBUG, "Missing NONCE");
            ABORT(R_REJECTED);
        }

        if (nr_stun_message_has_attribute(res, NR_STUN_ATTR_MESSAGE_INTEGRITY, &attr)) {
            if (!attr->u.message_integrity.valid) {
                r_log(NR_LOG_STUN, LOG_DEBUG, "Bad MESSAGE-INTEGRITY");
                ABORT(R_REJECTED);
            }
        }

        break;

#ifdef USE_STUND_0_96
    case NR_STUN_MAGIC_COOKIE2:
        
        break;
#endif 
    }

    _status=0;
 abort:
    return _status;
}

