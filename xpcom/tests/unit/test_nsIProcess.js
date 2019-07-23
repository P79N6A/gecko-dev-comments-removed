





































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
  
  do_check_false(process.isRunning);

  try {
    process.kill();
    do_throw("Attempting to kill a not-running process should throw");
  }
  catch (e) { }

  process.run(false, [], 0);

  do_check_true(process.isRunning);

  process.kill();

  do_check_false(process.isRunning);

  try {
    process.kill();
    do_throw("Attempting to kill a not-running process should throw");
  }
  catch (e) { }
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
  
  
  process.run(true, [], 0);

  do_check_eq(process.exitValue, 42);
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
  
  process.run(true, args, args.length);
  
  
  do_check_neq(process.exitValue, 255);
}

function run_test() {
  test_kill();
  test_quick();
  test_arguments();
}
