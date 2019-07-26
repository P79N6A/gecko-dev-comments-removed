

























#ifndef _EVENT2_DNS_COMPAT_H_
#define _EVENT2_DNS_COMPAT_H_









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

















int evdns_init(void);

struct evdns_base;







struct evdns_base *evdns_get_global_base(void);
















void evdns_shutdown(int fail_requests);















int evdns_nameserver_add(unsigned long int address);
















int evdns_count_nameservers(void);













int evdns_clear_nameservers_and_suspend(void);














int evdns_resume(void);














int evdns_nameserver_ip_add(const char *ip_as_string);















int evdns_resolve_ipv4(const char *name, int flags, evdns_callback_type callback, void *ptr);











int evdns_resolve_ipv6(const char *name, int flags, evdns_callback_type callback, void *ptr);

struct in_addr;
struct in6_addr;















int evdns_resolve_reverse(const struct in_addr *in, int flags, evdns_callback_type callback, void *ptr);















int evdns_resolve_reverse_ipv6(const struct in6_addr *in, int flags, evdns_callback_type callback, void *ptr);

















int evdns_set_option(const char *option, const char *val, int flags);


























int evdns_resolv_conf_parse(int flags, const char *const filename);








void evdns_search_clear(void);










void evdns_search_add(const char *domain);













void evdns_search_ndots_set(const int ndots);









struct evdns_server_port *evdns_add_server_port(evutil_socket_t socket, int flags, evdns_request_callback_fn_type callback, void *user_data);

#ifdef WIN32
int evdns_config_windows_nameservers(void);
#define EVDNS_CONFIG_WINDOWS_NAMESERVERS_IMPLEMENTED
#endif

#ifdef __cplusplus
}
#endif

#endif
