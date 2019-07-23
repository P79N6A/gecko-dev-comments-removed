





































var bindir = Components.classes["@mozilla.org/file/directory_service;1"]
.getService(Components.interfaces.nsIProperties)
.get("CurProcD", Components.interfaces.nsIFile);


var isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);

var filePrefix = "";
var fileSuffix = "";

if (isWindows) {
  filePrefix = bindir.path + "\\";
  fileSuffix = ".exe";
} else {
  filePrefix = bindir.path + "/";
}




function test_kill()
{
  var testapp = filePrefix + "TestBlockingProcess" +fileSuffix;
  print(testapp);
 
  var file = Components.classes["@mozilla.org/file/local;1"]
                       .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath(testapp);
 
  var process = Components.classes["@mozilla.org/process/util;1"]
                          .createInstance(Components.interfaces.nsIProcess);
  process.init(file);
  
  var pid = process.run(false, [], 0);

  if (!pid) {
    return false;
  }

  var rv = process.isRunning;

  if (!rv) {    
    return false;
  }

  process.kill();

  rv = process.isRunning;

  if (rv){
    return false;
  }
  return true;

}



function test_quick()
{
  var testapp = filePrefix + "TestQuickReturn" + fileSuffix;
  
  var file = Components.classes["@mozilla.org/file/local;1"]
  .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath(testapp);
  
  var process = Components.classes["@mozilla.org/process/util;1"]
  .createInstance(Components.interfaces.nsIProcess);
  process.init(file);
  
  
  var pid = process.run(true, [], 0);
  
  if (!pid) {
    return false;
  }
  
  if (process.exitValue != 42) {
    return false;
  }
  return true;
}



function test_arguments()
{
  var testapp = filePrefix + "TestArguments" + fileSuffix;
  
  var file = Components.classes["@mozilla.org/file/local;1"]
  .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath(testapp);
  
  var process = Components.classes["@mozilla.org/process/util;1"]
  .createInstance(Components.interfaces.nsIProcess);
  process.init(file);
  
  var args= ["mozilla"];
  
  var pid = process.run(true, args, args.length);
  
  if (!pid) {
    return false;
  }
    
  if (process.exitValue) {
    return false;
  }
  return true;
}

function run_test() {
  do_check_true(test_kill());
  do_check_true(test_quick());
  do_check_true(test_arguments());
}
