





































const kTestURI = "http://\u65e5\u672c\u8a93.jp/";
const kExpectedURI = "http://xn--wgv71a309e.jp/";
const kOutputFile = "result.txt";


const kMaxCheckExistAttempts = 30; 
var gCheckExistsAttempts = 0;

function checkFile() {
  
  var tempFile = do_get_cwd();
  tempFile.append(kOutputFile);

  if (!tempFile.exists()) {
    if (gCheckExistsAttempts >= kMaxCheckExistAttempts) {
      do_throw("Expected File " + tempFile.path + " does not exist after " +
                 kMaxCheckExistAttempts + " seconds");
    }
    else {
      ++gCheckExistsAttempts;
      
      do_timeout(1000, checkFile);
      return;
    }
  }

  
  var fstream =
    Components.classes["@mozilla.org/network/file-input-stream;1"]
              .createInstance(Components.interfaces.nsIFileInputStream);
  var sstream =
    Components.classes["@mozilla.org/scriptableinputstream;1"]
              .createInstance(Components.interfaces.nsIScriptableInputStream);
  fstream.init(tempFile, -1, 0, 0);
  sstream.init(fstream);

  
  
  var data = sstream.read(4096);

  sstream.close();
  fstream.close();

  
  tempFile.remove(false);

  
  
  
  
  
  if (data.substring(0, 7) != "-psn_0_")
    do_check_eq(data, kExpectedURI);

  do_test_finished();
}

function run_test() {
  var isOSX = ("nsILocalFileMac" in Components.interfaces);
  if (isOSX) {
    dump("INFO | test_punycodeURIs.js | Skipping test on mac, bug 599475")
    return;
  }

  
  var ioService =
    Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService);

  
  var localHandler =
    Components.classes["@mozilla.org/uriloader/local-handler-app;1"]
              .createInstance(Components.interfaces.nsILocalHandlerApp);
  localHandler.name = "Test Local Handler App";

  
  var processDir = do_get_cwd();
  var exe = processDir.clone();
  exe.append("WriteArgument");

  if (!exe.exists()) {
    
    exe.leafName = "WriteArgument.exe";
    if (!exe.exists())
      do_throw("Could not locate the WriteArgument tests executable\n");
  }

  var outFile = processDir.clone();
  outFile.append(kOutputFile);

  
  var envSvc =
    Components.classes["@mozilla.org/process/environment;1"]
              .getService(Components.interfaces.nsIEnvironment);

  
  
  
  var greDir = HandlerServiceTest._dirSvc.get("GreD", Components.interfaces.nsIFile);

  envSvc.set("DYLD_LIBRARY_PATH", greDir.path);
  
  envSvc.set("LD_LIBRARY_PATH", greDir.path);
  

  
  envSvc.set("WRITE_ARGUMENT_FILE", outFile.path);

  var uri = ioService.newURI(kTestURI, null, null);

  
  
  do_check_eq(uri.asciiSpec, kExpectedURI);

  localHandler.executable = exe;
  localHandler.launchWithURI(uri);

  do_test_pending();
  do_timeout(1000, checkFile);
}
