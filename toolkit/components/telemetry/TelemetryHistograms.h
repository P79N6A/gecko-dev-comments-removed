











































HISTOGRAM(CYCLE_COLLECTOR, 1, 10000, 50, EXPONENTIAL, "Time(ms) spent on cycle collection")
HISTOGRAM(TELEMETRY_PING, 1, 3000, 10, EXPONENTIAL, "Time(ms) taken to submit telemetry info")
HISTOGRAM(TELEMETRY_SUCCESS, 0, 1, 2, BOOLEAN,  "Success(No, Yes) rate of telemetry submissions")
HISTOGRAM(MEMORY_JS_GC_HEAP, 1024, 512 * 1024, 10, EXPONENTIAL, "Memory(MB) used by the JavaScript GC")
HISTOGRAM(MEMORY_RESIDENT, 32 * 1024, 1024 * 1024, 10, EXPONENTIAL, "Resident memory(MB) reported by OS")
HISTOGRAM(MEMORY_LAYOUT_ALL, 1024, 64 * 1024, 10, EXPONENTIAL, "Memory(MB) reported used by layout")
#if defined(XP_WIN)
HISTOGRAM(EARLY_GLUESTARTUP_READ_OPS, 1, 100, 12, LINEAR, "ProcessIoCounters.ReadOperationCount before glue startup")
HISTOGRAM(EARLY_GLUESTARTUP_READ_TRANSFER, 1, 50 * 1024, 12, EXPONENTIAL, "ProcessIoCounters.ReadTransferCount before glue startup (KB)")
HISTOGRAM(GLUESTARTUP_READ_OPS, 1, 100, 12, LINEAR, "ProcessIoCounters.ReadOperationCount after glue startup")
HISTOGRAM(GLUESTARTUP_READ_TRANSFER, 1, 50 * 1024, 12, EXPONENTIAL, "ProcessIoCounters.ReadTransferCount after glue startup (KB)")
#elif defined(XP_UNIX)
HISTOGRAM(EARLY_GLUESTARTUP_HARD_FAULTS, 1, 100, 12, LINEAR, "Hard faults count before glue startup")
HISTOGRAM(GLUESTARTUP_HARD_FAULTS, 1, 500, 12, EXPONENTIAL, "Hard faults count after glue startup")
#endif
