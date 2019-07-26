







Types = (
    
    'bool',
    'char',
    'short',
    'int',
    'long',
    'float',
    'double',

    
    'int8_t',
    'uint8_t',
    'int16_t',
    'uint16_t',
    'int32_t',
    'uint32_t',
    'int64_t',
    'uint64_t',
    'intptr_t',
    'uintptr_t',

    
    'size_t',
    'ssize_t',

    
    'nsresult',
    'nsString',
    'nsCString',
    'mozilla::ipc::Shmem',
    'mozilla::ipc::FileDescriptor'
)


HeaderIncludes = (
    'mozilla/Attributes.h',
    'prtime.h',
    'IPCMessageStart.h',
    'ipc/IPCMessageUtils.h',
    'nsAutoPtr.h',
    'nsStringGlue.h',
    'nsTArray.h',
    'mozilla/ipc/ProtocolUtils.h',
)

CppIncludes = (
    'nsIFile.h',
    'GeckoProfiler.h',
)
