




































var isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);

function get_test_program(prog)
{
    var progPath = do_get_cwd();
    progPath.append(prog);
    if (isWindows) {
	progPath.leafName = progPath.leafName + ".exe";
    }
    return progPath;
}

function set_environment()
{
  var envSvc = Components.classes["@mozilla.org/process/environment;1"].
    getService(Components.interfaces.nsIEnvironment);
  var dirSvc = Components.classes["@mozilla.org/file/directory_service;1"].
    getService(Components.interfaces.nsIProperties);
  var greDir = dirSvc.get("GreD", Components.interfaces.nsIFile);

  envSvc.set("DYLD_LIBRARY_PATH", greDir.path);
  
  envSvc.set("LD_LIBRARY_PATH", greDir.path);
  
}




function test_kill()
{
  var file = get_test_program("TestBlockingProcess");
 
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
  var file = get_test_program("TestQuickReturn");
  
  var process = Components.classes["@mozilla.org/process/util;1"]
                          .createInstance(Components.interfaces.nsIProcess);
  process.init(file);
  
  
  process.run(true, [], 0);

  do_check_eq(process.exitValue, 42);
}



function test_arguments()
{
  var file = get_test_program("TestArguments");
  
  var process = Components.classes["@mozilla.org/process/util;1"]
                          .createInstance(Components.interfaces.nsIProcess);
  process.init(file);
  
  var args= ["mozilla"];
  
  process.run(true, args, args.length);
  
  
  do_check_neq(process.exitValue, 255);
}

var gProcess;



function test_nonblocking()
{
  var file = get_test_program("TestQuickReturn");

  gProcess = Components.classes["@mozilla.org/process/util;1"]
                       .createInstance(Components.interfaces.nsIProcess);
  gProcess.init(file);

  gProcess.run(false, [], 0);

  do_test_pending();
  do_timeout(100, "check_nonblocking()");
}

function check_nonblocking()
{
  if (gProcess.isRunning) {
    do_timeout(100, "check_nonblocking()");
    return;
  }

  do_check_eq(gProcess.exitValue, 42);
  do_test_finished();
}

function run_test() {
  set_environment();
  test_kill();
  test_quick();
  test_arguments();
  if (isWindows)
    test_nonblocking();
}
