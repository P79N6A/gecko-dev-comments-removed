

























#ifndef _EVENT2_DNS_STRUCT_H_
#define _EVENT2_DNS_STRUCT_H_








#ifdef __cplusplus
extern "C" {
#endif

#include <event2/event-config.h>
#ifdef _EVENT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif


#include <event2/util.h>





struct evdns_server_request {
	int flags;
	int nquestions;
	struct evdns_server_question **questions;
};
struct evdns_server_question {
	int type;
#ifdef __cplusplus
	int dns_question_class;
#else
	


	int class;
#define dns_question_class class
#endif
	char name[1];
};

#ifdef __cplusplus
}
#endif

#endif

