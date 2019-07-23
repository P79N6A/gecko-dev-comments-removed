



































const TEST_ARGS = ["mozilla", "firefox", "thunderbird", "seamonkey", "foo",
                   "bar", "argument with spaces", "\"argument with quotes\""];

const TEST_UNICODE_ARGS = ["M\u00F8z\u00EEll\u00E5",
                           "\u041C\u043E\u0437\u0438\u043B\u043B\u0430",
                           "\u09AE\u09CB\u099C\u09BF\u09B2\u09BE",
                           "\uD808\uDE2C\uD808\uDF63\uD808\uDDB7"];

var isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);

function get_test_program(prog)
{
  var progPath = do_get_cwd();
  progPath.append(prog);
  if (isWindows)
    progPath.leafName = progPath.leafName + ".exe";
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

function test_args(file, args, argsAreASCII)
{
  var process = Components.classes["@mozilla.org/process/util;1"]
                          .createInstance(Components.interfaces.nsIProcess);
  process.init(file);

  if (argsAreASCII)  
    process.run(true, args, args.length);
  else  
    process.runw(true, args, args.length);
  
  do_check_eq(process.exitValue, 0);
}



function test_arguments()
{
  test_args(get_test_program("TestArguments"), TEST_ARGS, true);
}


function test_unicode_arguments()
{
  test_args(get_test_program("TestUnicodeArguments"), TEST_UNICODE_ARGS, false);
}

function rename_and_test(asciiName, unicodeName, args, argsAreASCII)
{
  var asciiFile = get_test_program(asciiName);
  var asciiLeaf = asciiFile.leafName;
  var unicodeLeaf = asciiLeaf.replace(asciiName, unicodeName);

  asciiFile.moveTo(null, unicodeLeaf);

  var unicodeFile = get_test_program(unicodeName);

  test_args(unicodeFile, args, argsAreASCII);

  unicodeFile.moveTo(null, asciiLeaf);
}


function test_unicode_app()
{
  rename_and_test("TestArguments",
                  
                  "\u0BAF\u0BC1\u0BA9\u0BBF\u0B95\u0BCB\u0B9F\u0BCD",
                  TEST_ARGS, true);

  rename_and_test("TestUnicodeArguments",
                  
                  "\u0E22\u0E39\u0E19\u0E34\u0E42\u0E04\u0E14",
                  TEST_UNICODE_ARGS, false);
}


function test_notify_blocking()
{
  var file = get_test_program("TestQuickReturn");

  var process = Components.classes["@mozilla.org/process/util;1"]
                          .createInstance(Components.interfaces.nsIProcess);
  process.init(file);

  process.runAsync([], 0, {
    observe: function(subject, topic, data) {
      process = subject.QueryInterface(Components.interfaces.nsIProcess);
      do_check_eq(topic, "process-finished");
      do_check_eq(process.exitValue, 42);
      test_notify_nonblocking();
    }
  });
}


function test_notify_nonblocking()
{
  var file = get_test_program("TestArguments");

  var process = Components.classes["@mozilla.org/process/util;1"]
                          .createInstance(Components.interfaces.nsIProcess);
  process.init(file);

  process.runAsync(TEST_ARGS, TEST_ARGS.length, {
    observe: function(subject, topic, data) {
      process = subject.QueryInterface(Components.interfaces.nsIProcess);
      do_check_eq(topic, "process-finished");
      do_check_eq(process.exitValue, 0);
      test_notify_killed();
    }
  });
}


function test_notify_killed()
{
  var file = get_test_program("TestBlockingProcess");

  var process = Components.classes["@mozilla.org/process/util;1"]
                          .createInstance(Components.interfaces.nsIProcess);
  process.init(file);

  process.runAsync([], 0, {
    observe: function(subject, topic, data) {
      process = subject.QueryInterface(Components.interfaces.nsIProcess);
      do_check_eq(topic, "process-finished");
      do_test_finished();
    }
  });

  process.kill();
}


function test_kill_2()
{
  var file = get_test_program("TestQuickReturn");

  for (var i = 0; i < 1000; i++) {
    var process = Components.classes["@mozilla.org/process/util;1"]
                          .createInstance(Components.interfaces.nsIProcess);
    process.init(file);

    process.run(false, [], 0);

    try {
      process.kill();
    }
    catch (e) { }
  }
}

function run_test() {
  set_environment();
  test_kill();
  test_quick();
  test_arguments();
  test_unicode_arguments();
  test_unicode_app();
  do_test_pending();
  test_notify_blocking();
  test_kill_2();
}
