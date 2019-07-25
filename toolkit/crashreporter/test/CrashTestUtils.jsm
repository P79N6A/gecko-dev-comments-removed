var EXPORTED_SYMBOLS = ["CrashTestUtils"];

let CrashTestUtils = {
  
  crash: null,
  lockDir: null,
  dumpHasStream: null,
  dumpHasInstructionPointerMemory: null,

  
  
  CRASH_INVALID_POINTER_DEREF: 0,
  CRASH_PURE_VIRTUAL_CALL:     1,
  CRASH_RUNTIMEABORT:          2,
  CRASH_OOM:                   3,

  
  
  MD_THREAD_LIST_STREAM:       3,
  MD_MEMORY_INFO_LIST_STREAM:  16
};


Components.utils.import("resource://gre/modules/ctypes.jsm");
let dir = __LOCATION__.parent;
let file = dir.clone();
file.append(ctypes.libraryName("testcrasher"));
let lib = ctypes.open(file.path);
CrashTestUtils.crash = lib.declare("Crash",
                                   ctypes.default_abi,
                                   ctypes.void_t,
                                   ctypes.int16_t);
CrashTestUtils.saveAppMemory = lib.declare("SaveAppMemory",
                                           ctypes.default_abi,
                                           ctypes.uint64_t);

CrashTestUtils.lockDir = lib.declare("LockDir",
                                     ctypes.default_abi,
                                     ctypes.voidptr_t,   
                                     ctypes.voidptr_t);  


CrashTestUtils.dumpHasStream = lib.declare("DumpHasStream",
                                           ctypes.default_abi,
                                           ctypes.bool,
                                           ctypes.char.ptr,
                                           ctypes.uint32_t);

CrashTestUtils.dumpHasInstructionPointerMemory =
  lib.declare("DumpHasInstructionPointerMemory",
              ctypes.default_abi,
              ctypes.bool,
              ctypes.char.ptr);

CrashTestUtils.dumpCheckMemory = lib.declare("DumpCheckMemory",
                                             ctypes.default_abi,
                                             ctypes.bool,
                                             ctypes.char.ptr);
