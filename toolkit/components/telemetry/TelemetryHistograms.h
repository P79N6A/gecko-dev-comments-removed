













































HISTOGRAM(CYCLE_COLLECTOR, 1, 10000, 50, EXPONENTIAL, "Time spent on one cycle collection (ms)")
HISTOGRAM(TELEMETRY_PING, 1, 3000, 10, EXPONENTIAL, "Time taken to submit telemetry info (ms)")
HISTOGRAM(TELEMETRY_SUCCESS, 0, 1, 2, BOOLEAN,  "Successful telemetry submission")
HISTOGRAM(MEMORY_JS_GC_HEAP, 1024, 512 * 1024, 10, EXPONENTIAL, "Memory used by the garbage-collected JavaScript heap (KB)")
HISTOGRAM(MEMORY_RESIDENT, 32 * 1024, 1024 * 1024, 10, EXPONENTIAL, "Resident memory size (KB)")
HISTOGRAM(MEMORY_LAYOUT_ALL, 1024, 64 * 1024, 10, EXPONENTIAL, "Memory used by layout (KB)")
#if defined(XP_WIN)
HISTOGRAM(EARLY_GLUESTARTUP_READ_OPS, 1, 100, 12, LINEAR, "ProcessIoCounters.ReadOperationCount before glue startup")
HISTOGRAM(EARLY_GLUESTARTUP_READ_TRANSFER, 1, 50 * 1024, 12, EXPONENTIAL, "ProcessIoCounters.ReadTransferCount before glue startup (KB)")
HISTOGRAM(GLUESTARTUP_READ_OPS, 1, 100, 12, LINEAR, "ProcessIoCounters.ReadOperationCount after glue startup")
HISTOGRAM(GLUESTARTUP_READ_TRANSFER, 1, 50 * 1024, 12, EXPONENTIAL, "ProcessIoCounters.ReadTransferCount after glue startup (KB)")
#elif defined(XP_UNIX)
HISTOGRAM(EARLY_GLUESTARTUP_HARD_FAULTS, 1, 100, 12, LINEAR, "Hard faults count before glue startup")
HISTOGRAM(GLUESTARTUP_HARD_FAULTS, 1, 500, 12, EXPONENTIAL, "Hard faults count after glue startup")
HISTOGRAM(HARD_PAGE_FAULTS, 8, 64 * 1024, 13, EXPONENTIAL, "Hard page faults (since last telemetry ping)")
#endif
HISTOGRAM(ZIPARCHIVE_CRC, 0, 1, 2, BOOLEAN, "Zip item CRC check pass")




HISTOGRAM(TOTAL_CONTENT_PAGE_LOAD_TIME, 100, 10000, 100, EXPONENTIAL, "HTTP: Total page load time (ms)")
HISTOGRAM(HTTP_SUBITEM_OPEN_LATENCY_TIME, 1, 30000, 50, EXPONENTIAL, "HTTP subitem: Page start -> subitem open() (ms)")
HISTOGRAM(HTTP_SUBITEM_FIRST_BYTE_LATENCY_TIME, 1, 30000, 50, EXPONENTIAL, "HTTP subitem: Page start -> first byte received for subitem reply (ms)")
HISTOGRAM(HTTP_REQUEST_PER_PAGE, 1, 1000, 50, EXPONENTIAL, "HTTP: Requests per page (count)")
HISTOGRAM(HTTP_REQUEST_PER_PAGE_FROM_CACHE, 1, 101, 102, LINEAR, "HTTP: Requests serviced from cache (%)")

#define _HISTOGRAM_CHANNEL_TIMING(name, label) \
  HISTOGRAM(name, 1, 10000, 50, EXPONENTIAL, "HTTP " label) \

#define _HISTOGRAM_CHANNEL_TIMINGS(prefix, labelprefix) \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_DNS_ISSUE_TIME, labelprefix "open() -> DNS request issued (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_DNS_LOOKUP_TIME, labelprefix "DNS lookup time (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_TCP_CONNECTION, labelprefix "TCP connection setup (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_OPEN_TO_FIRST_SENT, labelprefix "Open -> first byte of request sent (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_FIRST_SENT_TO_LAST_RECEIVED, labelprefix "First byte of request sent -> last byte of response received (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_OPEN_TO_FIRST_RECEIVED, labelprefix "Open -> first byte of reply received (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_OPEN_TO_FIRST_FROM_CACHE, labelprefix "Open -> cache read start (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_CACHE_READ_TIME, labelprefix "Cache read time (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_REVALIDATION, labelprefix "Positive cache validation time (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_COMPLETE_LOAD, labelprefix "Overall load time - all (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_COMPLETE_LOAD_CACHED, labelprefix "Overall load time - cache hits (ms)") \
  _HISTOGRAM_CHANNEL_TIMING(HTTP_##prefix##_COMPLETE_LOAD_NET, labelprefix "Overall load time - network (ms)") \

_HISTOGRAM_CHANNEL_TIMINGS(PAGE, "page: ")
_HISTOGRAM_CHANNEL_TIMINGS(SUB, "subitem: ")

#undef _HISTOGRAM_CHANNEL_TIMING
#undef _HISTOGRAM_CHANNEL_TIMINGS
