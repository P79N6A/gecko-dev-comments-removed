


this.EXPORTED_SYMBOLS = ["CrashTestUtils"];

this.CrashTestUtils = {
  
  crash: null,
  lockDir: null,
  dumpHasStream: null,
  dumpHasInstructionPointerMemory: null,

  
  
  CRASH_INVALID_POINTER_DEREF: 0,
  CRASH_PURE_VIRTUAL_CALL:     1,
  CRASH_RUNTIMEABORT:          2,
  CRASH_OOM:                   3,
  CRASH_MOZ_CRASH:             4,
  CRASH_ABORT:                 5,

  
  
  MD_THREAD_LIST_STREAM:       3,
  MD_MEMORY_INFO_LIST_STREAM:  16
};


Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/ctypes.jsm");
let dir = Services.dirsvc.get("CurWorkD", Components.interfaces.nsILocalFile);
let file = dir.clone();
file.append(ctypes.libraryName("testcrasher"));
let lib = ctypes.open(file.path);

CrashTestUtils.crash = (crashType) => {
  
  
  
  if (crashType == CrashTestUtils.CRASH_ABORT) {
    if (Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS == "WINNT") {
      throw new Error("CRASH_ABORT cannot be used on Windows because it " +
                      "launches a dialog that requires user interaction.");
    }
  }

  return CrashTestUtils._crash(crashType);
};

CrashTestUtils._crash = lib.declare("Crash",
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


try {
  CrashTestUtils.TryOverrideExceptionHandler = lib.declare("TryOverrideExceptionHandler",
                                                           ctypes.default_abi,
                                                           ctypes.void_t);
}
catch(ex) {}

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
