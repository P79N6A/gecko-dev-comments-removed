





#ifndef timecard_h__
#define timecard_h__

#include <stdlib.h>
#include "prtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STAMP_TIMECARD(card,event)                                       \
  do {                                                                   \
    if (card) {                                                          \
      stamp_timecard((card), (event), __FILE__, __LINE__, __FUNCTION__); \
    }                                                                    \
  } while (0)

#define TIMECARD_INITIAL_TABLE_SIZE 16






typedef struct {
  PRTime timestamp;
  const char *event;
  const char *file;
  unsigned int line;
  const char *function;
} TimecardEntry;

typedef struct Timecard {
  size_t curr_entry;
  size_t entries_allocated;
  TimecardEntry *entries;
  PRTime start_time;
} Timecard;




Timecard *
create_timecard();





void
destroy_timecard(Timecard *tc);






void
stamp_timecard(Timecard *tc,
               const char *event,
               const char *file,
               unsigned int line,
               const char *function);




void
print_timecard(Timecard *tc);

#ifdef __cplusplus
}
#endif

#endif
