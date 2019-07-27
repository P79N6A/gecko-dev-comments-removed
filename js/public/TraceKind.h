





#ifndef js_TraceKind_h
#define js_TraceKind_h

namespace JS {










enum class TraceKind
{
    
    
    
    Object = 0x00,
    String = 0x01,
    Symbol = 0x02,
    Script = 0x03,

    
    Shape = 0x04,

    
    ObjectGroup = 0x05,

    
    Null = 0x06,

    
    BaseShape = 0x0F,
    JitCode = 0x1F,
    LazyScript = 0x2F
};
const static uintptr_t OutOfLineTraceKindMask = 0x07;
static_assert(uintptr_t(JS::TraceKind::BaseShape) & OutOfLineTraceKindMask, "mask bits are set");
static_assert(uintptr_t(JS::TraceKind::JitCode) & OutOfLineTraceKindMask, "mask bits are set");
static_assert(uintptr_t(JS::TraceKind::LazyScript) & OutOfLineTraceKindMask, "mask bits are set");

} 

#endif
