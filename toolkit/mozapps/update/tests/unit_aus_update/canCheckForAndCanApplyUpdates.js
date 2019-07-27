




function run_test() {
  setupTestCommon();

  
  debugDump("testing write access to the application directory");
  let testFile = getCurrentProcessDir();
  testFile.append("update_write_access_test");
  testFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
  Assert.ok(testFile.exists(), MSG_SHOULD_EXIST);
  testFile.remove(false);
  Assert.ok(!testFile.exists(), MSG_SHOULD_NOT_EXIST);

  standardInit();

  if (IS_WIN) {
    
    debugDump("attempting to create mutex");
    let handle = createMutex(getPerInstallationMutexName());
    Assert.ok(!!handle, "the update mutex should have been created");

    
    
    Assert.ok(!gAUS.canCheckForUpdates, "should not be able to check for " +
              "updates when there is an update mutex");

    
    
    Assert.ok(!gAUS.canApplyUpdates, "should not be able to apply updates " +
              "when there is an update mutex");

    debugDump("destroying mutex");
    closeHandle(handle);
  }

  
  Assert.ok(gAUS.canCheckForUpdates, "should be able to check for updates");
  
  Assert.ok(gAUS.canApplyUpdates, "should be able to apply updates");

  if (IS_WIN) {
    
    
    debugDump("attempting to create mutex");
    let handle = createMutex(getPerInstallationMutexName());

    Assert.ok(!handle, "should not be able to create the update mutex when " +
              "the application has created the update mutex");
  }

  doTestFinish();
}






function getPerInstallationMutexName() {
  if (!IS_WIN) {
    do_throw("Windows only function called by a different platform!");
  }

  let hasher = Cc["@mozilla.org/security/hash;1"].
               createInstance(Ci.nsICryptoHash);
  hasher.init(hasher.SHA1);

  let exeFile = Services.dirsvc.get(XRE_EXECUTABLE_FILE, Ci.nsILocalFile);

  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let data = converter.convertToByteArray(exeFile.path.toLowerCase());

  hasher.update(data, data.length);
  return "Global\\MozillaUpdateMutex-" + hasher.finish(true);
}







function closeHandle(aHandle) {
  if (!IS_WIN) {
    do_throw("Windows only function called by a different platform!");
  }

  let lib = ctypes.open("kernel32.dll");
  let CloseHandle = lib.declare("CloseHandle",
                                ctypes.winapi_abi,
                                ctypes.int32_t, 
                                ctypes.void_t.ptr); 
  CloseHandle(aHandle);
  lib.close();
}








function createMutex(aName) {
  if (!IS_WIN) {
    do_throw("Windows only function called by a different platform!");
  }

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
