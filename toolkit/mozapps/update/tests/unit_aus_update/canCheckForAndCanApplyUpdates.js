




function run_test() {
  setupTestCommon();

  
  logTestInfo("testing write access to the application directory");
  let testFile = getCurrentProcessDir();
  testFile.append("update_write_access_test");
  testFile.create(AUS_Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
  do_check_true(testFile.exists());
  testFile.remove(false);
  do_check_false(testFile.exists());

  standardInit();

  if (IS_WIN) {
    
    logTestInfo("attempting to create mutex");
    let handle = createMutex(getPerInstallationMutexName());

    logTestInfo("testing that the mutex was successfully created");
    do_check_neq(handle, null);

    
    
    logTestInfo("testing nsIApplicationUpdateService:canCheckForUpdates is " +
                "false when there is a mutex");
    do_check_false(gAUS.canCheckForUpdates);

    
    
    logTestInfo("testing nsIApplicationUpdateService:canApplyUpdates is " +
                "false when there is a mutex");
    do_check_false(gAUS.canApplyUpdates);

    logTestInfo("destroying mutex");
    closeHandle(handle)
  }

  
  logTestInfo("testing nsIApplicationUpdateService:canCheckForUpdates is true");
  do_check_true(gAUS.canCheckForUpdates);
  
  logTestInfo("testing nsIApplicationUpdateService:canApplyUpdates is true");
  do_check_true(gAUS.canApplyUpdates);

  if (IS_WIN) {
    
    
    logTestInfo("attempting to create mutex");
    let handle = createMutex(getPerInstallationMutexName());

    logTestInfo("testing that the mutex was not successfully created");
    do_check_eq(handle, null);
  }

  doTestFinish();
}

if (IS_WIN) {
  




  function getPerInstallationMutexName() {
    let hasher = AUS_Cc["@mozilla.org/security/hash;1"].
                 createInstance(AUS_Ci.nsICryptoHash);
    hasher.init(hasher.SHA1);

    let exeFile = Services.dirsvc.get(XRE_EXECUTABLE_FILE, AUS_Ci.nsILocalFile);

    let converter = AUS_Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(AUS_Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let data = converter.convertToByteArray(exeFile.path.toLowerCase());

    hasher.update(data, data.length);
    return "Global\\MozillaUpdateMutex-" + hasher.finish(true);
  }

  





  function closeHandle(aHandle) {
    let lib = ctypes.open("kernel32.dll");
    let CloseHandle = lib.declare("CloseHandle",
                                  ctypes.winapi_abi,
                                  ctypes.int32_t, 
                                  ctypes.void_t.ptr); 
    CloseHandle(aHandle);
    lib.close();
  }

  






  function createMutex(aName) {
    const INITIAL_OWN = 1;
    const ERROR_ALREADY_EXISTS = 0xB7;
    let lib = ctypes.open("kernel32.dll");
    let CreateMutexW = lib.declare("CreateMutexW",
                                   ctypes.winapi_abi,
                                   ctypes.void_t.ptr, 
                                   ctypes.void_t.ptr, 
                                   ctypes.int32_t, 
                                   ctypes.char16_t.ptr); 

    let handle = CreateMutexW(null, INITIAL_OWN, aName);
    lib.close();
    let alreadyExists = ctypes.winLastError == ERROR_ALREADY_EXISTS;
    if (handle && !handle.isNull() && alreadyExists) {
      closeHandle(handle);
      handle = null;
    }

    if (handle && handle.isNull()) {
      handle = null;
    }

    return handle;
  }
}
