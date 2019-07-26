
























#ifndef _EVMAP_H_
#define _EVMAP_H_









struct event_base;
struct event;



void evmap_io_initmap(struct event_io_map* ctx);
void evmap_signal_initmap(struct event_signal_map* ctx);





void evmap_io_clear(struct event_io_map* ctx);
void evmap_signal_clear(struct event_signal_map* ctx);











int evmap_io_add(struct event_base *base, evutil_socket_t fd, struct event *ev);








int evmap_io_del(struct event_base *base, evutil_socket_t fd, struct event *ev);






void evmap_io_active(struct event_base *base, evutil_socket_t fd, short events);





int evmap_signal_add(struct event_base *base, int signum, struct event *ev);
int evmap_signal_del(struct event_base *base, int signum, struct event *ev);
void evmap_signal_active(struct event_base *base, evutil_socket_t signum, int ncalls);

void *evmap_io_get_fdinfo(struct event_io_map *ctx, evutil_socket_t fd);

void evmap_check_integrity(struct event_base *base);

#endif 
