let {OS} = Components.utils.import("resource://gre/modules/osfile.jsm", {});
let {Services} = Components.utils.import("resource://gre/modules/Services.jsm", {});




add_task(function* duration() {
  Services.prefs.setBoolPref("toolkit.osfile.log", true);
  
  let copyOptions = {
    
    
    outExecutionDuration: null
  };
  let currentDir = yield OS.File.getCurrentDirectory();
  let pathSource = OS.Path.join(currentDir, "test_duration.js");
  let copyFile = pathSource + ".bak";
  function testOptions(options, name) {
    do_print("Checking outExecutionDuration for operation: " + name);
    do_print(name + ": Gathered method duration time: " +
      options.outExecutionDuration + "ms");
    
    do_check_eq(typeof options.outExecutionDuration, "number");
    do_check_true(options.outExecutionDuration >= 0);
  };
  
  yield OS.File.copy(pathSource, copyFile, copyOptions);
  testOptions(copyOptions, "OS.File.copy");
  yield OS.File.remove(copyFile);

  
  let pathDest = OS.Path.join(OS.Constants.Path.tmpDir,
    "osfile async test read writeAtomic.tmp");
  let tmpPath = pathDest + ".tmp";
  let readOptions = {
    outExecutionDuration: null
  };
  let contents = yield OS.File.read(pathSource, undefined, readOptions);
  testOptions(readOptions, "OS.File.read");
  
  let writeAtomicOptions = {
    
    
    outExecutionDuration: null,
    tmpPath: tmpPath
  };
  yield OS.File.writeAtomic(pathDest, contents, writeAtomicOptions);
  testOptions(writeAtomicOptions, "OS.File.writeAtomic");
  yield OS.File.remove(pathDest);

  do_print("Ensuring that we can use outExecutionDuration to accumulate durations");

  let ARBITRARY_BASE_DURATION = 5;
  copyOptions = {
    
    
    outExecutionDuration: ARBITRARY_BASE_DURATION
  };
  let backupDuration = ARBITRARY_BASE_DURATION;
  
  yield OS.File.copy(pathSource, copyFile, copyOptions);

  do_check_true(copyOptions.outExecutionDuration >= backupDuration);

  backupDuration = copyOptions.outExecutionDuration;
  yield OS.File.remove(copyFile, copyOptions);
  do_check_true(copyOptions.outExecutionDuration >= backupDuration);

  
  
  writeAtomicOptions = {
    
    
    outExecutionDuration: copyOptions.outExecutionDuration,
    tmpPath: tmpPath
  };
  backupDuration = writeAtomicOptions.outExecutionDuration;

  yield OS.File.writeAtomic(pathDest, contents, writeAtomicOptions);
  do_check_true(copyOptions.outExecutionDuration >= backupDuration);
  OS.File.remove(pathDest);

  
  let file = yield OS.File.open(pathSource);
  yield file.stat();
  yield file.close();
});

function run_test() {
  run_next_test();
}
