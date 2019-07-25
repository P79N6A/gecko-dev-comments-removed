











































HISTOGRAM(CYCLE_COLLECTOR, "nsCycleCollector::Collect (ms)", 1, 10000, 50, EXPONENTIAL, "Time spent on cycle collection")
HISTOGRAM(TELEMETRY_PING, "Telemetry.ping (ms)", 1, 3000, 10, EXPONENTIAL, "Telemetry submission lag")
HISTOGRAM(TELEMETRY_SUCCESS, "Telemetry.success (No, Yes)", 0, 1, 2, BOOLEAN,  "Success rate of telemetry submissions")
HISTOGRAM(MEMORY_JS_GC_HEAP, "Memory::explicit/js/gc-heap (MB)", 1024, 512 * 1024, 10, EXPONENTIAL, "Memory used by the JavaScript GC")
HISTOGRAM(MEMORY_RESIDENT, "Memory::resident (MB)", 32 * 1024, 1024 * 1024, 10, EXPONENTIAL, "Resident memory reported by OS")
HISTOGRAM(MEMORY_LAYOUT_ALL, "Memory::explicit/layout/all (MB)", 1024, 64 * 1024, 10, EXPONENTIAL, "Memory reported used by layout")
