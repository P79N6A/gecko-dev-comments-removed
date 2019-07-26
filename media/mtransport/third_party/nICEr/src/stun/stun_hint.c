
































static char *RCSSTRING __UNUSED__="$Id: stun_hint.c,v 1.2 2008/04/28 18:21:30 ekr Exp $";


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






int
nr_is_stun_message(UCHAR *buf, int len)
{
   const UINT4 cookie = htonl(NR_STUN_MAGIC_COOKIE);
   const UINT4 cookie2 = htonl(NR_STUN_MAGIC_COOKIE2);
#if 0
   nr_stun_message msg;
#endif
   UINT2 type;
   nr_stun_encoded_attribute* attr;
   unsigned int attrLen;
   int atrType;

   if (sizeof(nr_stun_message_header) > len)
       return 0;

   if ((buf[0] & (0x80|0x40)) != 0)
       return 0;

   memcpy(&type, buf, 2);
   type = ntohs(type);

   switch (type) {
   case NR_STUN_MSG_BINDING_REQUEST:
   case NR_STUN_MSG_BINDING_INDICATION:
   case NR_STUN_MSG_BINDING_RESPONSE:
   case NR_STUN_MSG_BINDING_ERROR_RESPONSE:

#ifdef USE_TURN
    case NR_STUN_MSG_ALLOCATE_REQUEST:
    case NR_STUN_MSG_ALLOCATE_RESPONSE:
    case NR_STUN_MSG_ALLOCATE_ERROR_RESPONSE:
    case NR_STUN_MSG_SEND_INDICATION:
    case NR_STUN_MSG_DATA_INDICATION:
    case NR_STUN_MSG_SET_ACTIVE_DEST_REQUEST:
    case NR_STUN_MSG_SET_ACTIVE_DEST_RESPONSE:
    case NR_STUN_MSG_SET_ACTIVE_DEST_ERROR_RESPONSE:
#ifdef NR_STUN_MSG_CONNECT_REQUEST
    case NR_STUN_MSG_CONNECT_REQUEST:
#endif
#ifdef NR_STUN_MSG_CONNECT_RESPONSE
    case NR_STUN_MSG_CONNECT_RESPONSE:
#endif
#ifdef NR_STUN_MSG_CONNECT_ERROR_RESPONSE
    case NR_STUN_MSG_CONNECT_ERROR_RESPONSE:
#endif
#ifdef NR_STUN_MSG_CONNECT_STATUS_INDICATION
    case NR_STUN_MSG_CONNECT_STATUS_INDICATION:
#endif
#endif


        break;
   default:
        return 0;
        break;
   }

   if (!memcmp(&cookie2, &buf[4], sizeof(UINT4))) {
       

       return 1;
   }

   if (memcmp(&cookie, &buf[4], sizeof(UINT4)))
       return 0;

   


   attr = (nr_stun_encoded_attribute*)(buf + (len - 8));
   attrLen = ntohs(attr->length);
   atrType = ntohs(attr->type);

   if (atrType != NR_STUN_ATTR_FINGERPRINT || attrLen != 4)
       return 1;

   


#if 0





   if (nr_stun_parse_attr_UINT4(buf + (len - 4), attrLen, &msg.fingerprint))
       return 2;


   if (nr_stun_compute_fingerprint(buf, len - 8, &computedFingerprint))
       return 2;

   if (msg.fingerprint.number != computedFingerprint)
       return 2;

   
#endif

   return 3;
}

int
nr_is_stun_request_message(UCHAR *buf, int len)
{
   UINT2 type;

   if (sizeof(nr_stun_message_header) > len)
       return 0;

   if (!nr_is_stun_message(buf, len))
       return 0;

   memcpy(&type, buf, 2);
   type = ntohs(type);

   return NR_STUN_GET_TYPE_CLASS(type) == NR_CLASS_REQUEST;
}

int
nr_is_stun_indication_message(UCHAR *buf, int len)
{
   UINT2 type;

   if (sizeof(nr_stun_message_header) > len)
       return 0;

   if (!nr_is_stun_message(buf, len))
       return 0;

   memcpy(&type, buf, 2);
   type = ntohs(type);

   return NR_STUN_GET_TYPE_CLASS(type) == NR_CLASS_INDICATION;
}

int
nr_is_stun_response_message(UCHAR *buf, int len)
{
   UINT2 type;

   if (sizeof(nr_stun_message_header) > len)
       return 0;

   if (!nr_is_stun_message(buf, len))
       return 0;

   memcpy(&type, buf, 2);
   type = ntohs(type);

   return NR_STUN_GET_TYPE_CLASS(type) == NR_CLASS_RESPONSE
       || NR_STUN_GET_TYPE_CLASS(type) == NR_CLASS_ERROR_RESPONSE;
}

int
nr_has_stun_cookie(UCHAR *buf, int len)
{
   static UINT4 cookie;

   cookie = htonl(NR_STUN_MAGIC_COOKIE);

   if (sizeof(nr_stun_message_header) > len)
       return 0;

   if (memcmp(&cookie, &buf[4], sizeof(UINT4)))
       return 0;

   return 1;
}

