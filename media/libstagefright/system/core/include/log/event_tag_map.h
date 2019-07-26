















#ifndef _LIBS_CUTILS_EVENTTAGMAP_H
#define _LIBS_CUTILS_EVENTTAGMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_TAG_MAP_FILE  "/system/etc/event-log-tags"

struct EventTagMap;
typedef struct EventTagMap EventTagMap;






EventTagMap* android_openEventTagMap(const char* fileName);




void android_closeEventTagMap(EventTagMap* map);




const char* android_lookupEventTag(const EventTagMap* map, int tag);

#ifdef __cplusplus
}
#endif

#endif
