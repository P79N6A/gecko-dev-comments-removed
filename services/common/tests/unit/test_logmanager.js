





Cu.import("resource://services-common/logmanager.js");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

function run_test() {
  run_next_test();
}



function getAppenders(log) {
  let capps = log.appenders.filter(app => app instanceof Log.ConsoleAppender);
  equal(capps.length, 1, "should only have one console appender");
  let dapps = log.appenders.filter(app => app instanceof Log.DumpAppender);
  equal(dapps.length, 1, "should only have one dump appender");
  let fapps = log.appenders.filter(app => app instanceof Log.StorageStreamAppender);
  return [capps[0], dapps[0], fapps];
}


add_task(function* test_noPrefs() {
  
  let lm = new LogManager("no-such-branch.", ["TestLog"], "test");

  let log = Log.repository.getLogger("TestLog");
  let [capp, dapp, fapps] = getAppenders(log);
  
  equal(capp.level, Log.Level.Error);
  equal(dapp.level, Log.Level.Error);
  
  equal(fapps.length, 1, "only 1 file appender");
  equal(fapps[0].level, Log.Level.Debug);
  lm.finalize();
});


add_task(function* test_PrefChanges() {
  Services.prefs.setCharPref("log-manager.test.log.appender.console", "Trace");
  Services.prefs.setCharPref("log-manager.test.log.appender.dump", "Trace");
  Services.prefs.setCharPref("log-manager.test.log.appender.file.level", "Trace");
  let lm = new LogManager("log-manager.test.", ["TestLog2"], "test");

  let log = Log.repository.getLogger("TestLog2");
  let [capp, dapp, [fapp]] = getAppenders(log);
  equal(capp.level, Log.Level.Trace);
  equal(dapp.level, Log.Level.Trace);
  equal(fapp.level, Log.Level.Trace);
  
  Services.prefs.setCharPref("log-manager.test.log.appender.console", "Debug");
  Services.prefs.setCharPref("log-manager.test.log.appender.dump", "Debug");
  Services.prefs.setCharPref("log-manager.test.log.appender.file.level", "Debug");
  equal(capp.level, Log.Level.Debug);
  equal(dapp.level, Log.Level.Debug);
  equal(fapp.level, Log.Level.Debug);
  
  Services.prefs.setCharPref("log-manager.test.log.appender.console", "xxx");
  Services.prefs.setCharPref("log-manager.test.log.appender.dump", "xxx");
  Services.prefs.setCharPref("log-manager.test.log.appender.file.level", "xxx");
  equal(capp.level, Log.Level.Error);
  equal(dapp.level, Log.Level.Error);
  equal(fapp.level, Log.Level.Debug);
  lm.finalize();
});


add_task(function* test_SharedLogs() {
  
  Services.prefs.setCharPref("log-manager-1.test.log.appender.console", "Trace");
  Services.prefs.setCharPref("log-manager-1.test.log.appender.dump", "Trace");
  Services.prefs.setCharPref("log-manager-1.test.log.appender.file.level", "Trace");
  let lm1 = new LogManager("log-manager-1.test.", ["TestLog3"], "test");

  
  Services.prefs.setCharPref("log-manager-2.test.log.appender.console", "Debug");
  Services.prefs.setCharPref("log-manager-2.test.log.appender.dump", "Debug");
  Services.prefs.setCharPref("log-manager-2.test.log.appender.file.level", "Debug");
  let lm2 = new LogManager("log-manager-2.test.", ["TestLog3"], "test");

  let log = Log.repository.getLogger("TestLog3");
  let [capp, dapp, fapps] = getAppenders(log);

  
  
  equal(capp.level, Log.Level.Trace);
  equal(dapp.level, Log.Level.Trace);

  
  
  Services.prefs.setCharPref("log-manager-1.test.log.appender.console", "Error");
  Services.prefs.setCharPref("log-manager-1.test.log.appender.dump", "Error");
  Services.prefs.setCharPref("log-manager-1.test.log.appender.file.level", "Error");

  equal(capp.level, Log.Level.Debug);
  equal(dapp.level, Log.Level.Debug);

  lm1.finalize();
  lm2.finalize();
});



function checkLogFile(prefix) {
  let logsdir = FileUtils.getDir("ProfD", ["weave", "logs"], true);
  let entries = logsdir.directoryEntries;
  if (!prefix) {
    
    ok(!entries.hasMoreElements());
  } else {
    
    ok(entries.hasMoreElements());
    let logfile = entries.getNext().QueryInterface(Ci.nsILocalFile);
    equal(logfile.leafName.slice(-4), ".txt");
    ok(logfile.leafName.startsWith(prefix + "-test-"), logfile.leafName);
    
    logfile.remove(false);
  }
}


add_task(function* test_logFileErrorDefault() {
  let lm = new LogManager("log-manager.test.", ["TestLog2"], "test");

  let log = Log.repository.getLogger("TestLog2");
  log.error("an error message");
  yield lm.resetFileLog(lm.REASON_ERROR);
  
  checkLogFile("error");
  lm.finalize();
});
