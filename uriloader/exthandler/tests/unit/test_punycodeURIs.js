





































const kTestURI = "http://\u65e5\u672c\u8a93.jp/";
const kExpectedURI = "http://xn--wgv71a309e.jp/";
const kOutputFile = "result.txt";

function checkFile() {
  
  var tempFile = Components.classes["@mozilla.org/file/local;1"].
    createInstance(Components.interfaces.nsILocalFile);
  tempFile = HandlerServiceTest._dirSvc.get("CurProcD", Components.interfaces.nsIFile);
  tempFile.append(kOutputFile);

  if (!tempFile.exists())
    do_throw("Expected File " + tempFile.path + " does not exist");

  
  var fstream = Components.classes["@mozilla.org/network/file-input-stream;1"].
    createInstance(Components.interfaces.nsIFileInputStream);
  var sstream = Components.classes["@mozilla.org/scriptableinputstream;1"].
    createInstance(Components.interfaces.nsIScriptableInputStream);
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
  
  const httpHandler =
    Components.classes["@mozilla.org/network/protocol;1?name=http"]
              .getService(Components.interfaces.nsIHttpProtocolHandler);
  const oscpu = httpHandler.oscpu;
  if (oscpu.match(/Mac OS X 10.4/)) {
    dump("This test is not run on Mac OS 10.4 because it fails for unknown " +
         "reasons. See bug 447999.\n");
    return;
  }

  
  var ioService = Components.classes["@mozilla.org/network/io-service;1"].
    getService(Components.interfaces.nsIIOService);

  
  var localHandler = 
    Components.classes["@mozilla.org/uriloader/local-handler-app;1"].
    createInstance(Components.interfaces.nsILocalHandlerApp);
  localHandler.name = "Test Local Handler App";

  
  var processDir = HandlerServiceTest._dirSvc.get("CurProcD", Components.interfaces.nsIFile);
  var exe = processDir.clone();
  exe.append("WriteArgument");

  if (!exe.exists()) {
    
    exe.leafName = "WriteArgument.exe";
    if (!exe.exists())
      do_throw("Could not locate the WriteArgument tests executable\n");
  }

  var outFile = processDir.clone();
  outFile.append(kOutputFile);

  
  var envSvc = Components.classes["@mozilla.org/process/environment;1"].
    getService(Components.interfaces.nsIEnvironment);

  
  
  
  envSvc.set("DYLD_LIBRARY_PATH", processDir.path);
  
  envSvc.set("LD_LIBRARY_PATH", processDir.path);

  
  envSvc.set("WRITE_ARGUMENT_FILE", outFile.path);

  var uri = ioService.newURI(kTestURI, null, null);

  
  
  do_check_eq(uri.asciiSpec, kExpectedURI);

  localHandler.executable = exe;
  localHandler.launchWithURI(uri);

  do_test_pending();
  do_timeout(1000, "checkFile()");
}
