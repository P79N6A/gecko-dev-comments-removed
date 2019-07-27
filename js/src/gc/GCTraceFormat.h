





#ifndef gc_GCTraceFormat_h
#define gc_GCTraceFormat_h










enum GCTraceEvent {
    
    TraceEventInit,
    TraceEventThingSize,
    TraceEventNurseryAlloc,
    TraceEventTenuredAlloc,
    TraceEventClassInfo,
    TraceEventTypeInfo,
    TraceEventTypeNewScript,
    TraceEventCreateObject,
    TraceEventMinorGCStart,
    TraceEventPromoteToTenured,
    TraceEventMinorGCEnd,
    TraceEventMajorGCStart,
    TraceEventTenuredFinalize,
    TraceEventMajorGCEnd,

    TraceDataAddress,  
    TraceDataInt,      
    TraceDataString,   

    GCTraceEventCount
};

const unsigned TraceFormatVersion = 1;

const unsigned TracePayloadBits = 48;

const unsigned TraceExtraShift = 48;
const unsigned TraceExtraBits = 8;

const unsigned TraceEventShift = 56;
const unsigned TraceEventBits = 8;

const unsigned AllocKinds = 22;
const unsigned LastObjectAllocKind = 11;

#endif
