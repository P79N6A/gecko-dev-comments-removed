


Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/log4moz.js");

const logsdir = FileUtils.getDir("ProfD", ["weave", "logs"], true);

function run_test() {
  run_next_test();
}

add_test(function test_noOutput() {
  
  Svc.Prefs.set("log.appender.file.logOnSuccess", false);
  Svc.Obs.notify("weave:service:sync:finish");

  
  Svc.Prefs.set("log.appender.file.logOnSuccess", true);

  Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
    Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);

    Svc.Prefs.resetBranch("");
    run_next_test();
  });

  
  Svc.Obs.notify("weave:service:sync:finish");
});

add_test(function test_logOnSuccess_false() {
  Svc.Prefs.set("log.appender.file.logOnSuccess", false);

  let log = Log4Moz.repository.getLogger("Sync.Test.FileLog");
  log.info("this won't show up");

  Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
    Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);
    
    do_check_false(logsdir.directoryEntries.hasMoreElements());

    Svc.Prefs.resetBranch("");
    run_next_test();
  });

  
  Svc.Obs.notify("weave:service:sync:finish");
});

function readFile(file, callback) {
  NetUtil.asyncFetch(file, function (inputStream, statusCode, request) {
    let data = NetUtil.readInputStreamToString(inputStream,
                                               inputStream.available());
    callback(statusCode, data);
  });
}

add_test(function test_logOnSuccess_true() {
  Svc.Prefs.set("log.appender.file.logOnSuccess", true);

  let log = Log4Moz.repository.getLogger("Sync.Test.FileLog");
  const MESSAGE = "this WILL show up";
  log.info(MESSAGE);

  Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
    Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);

    
    let entries = logsdir.directoryEntries;
    do_check_true(entries.hasMoreElements());
    let logfile = entries.getNext().QueryInterface(Ci.nsILocalFile);
    do_check_eq(logfile.leafName.slice(-4), ".log");
    do_check_false(entries.hasMoreElements());

    
    readFile(logfile, function (error, data) {
      do_check_true(Components.isSuccessCode(error));
      do_check_neq(data.indexOf(MESSAGE), -1);

      
      try {
        logfile.remove(false);
      } catch(ex) {
        dump("Couldn't delete file: " + ex + "\n");
        
      }

      Svc.Prefs.resetBranch("");
      run_next_test();
    });
  });

  
  Svc.Obs.notify("weave:service:sync:finish");
});

add_test(function test_logOnError_false() {
  Svc.Prefs.set("log.appender.file.logOnError", false);

  let log = Log4Moz.repository.getLogger("Sync.Test.FileLog");
  log.info("this won't show up");

  Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
    Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);
    
    do_check_false(logsdir.directoryEntries.hasMoreElements());

    Svc.Prefs.resetBranch("");
    run_next_test();
  });

  
  Svc.Obs.notify("weave:service:sync:error");
});

add_test(function test_logOnError_true() {
  Svc.Prefs.set("log.appender.file.logOnError", true);

  let log = Log4Moz.repository.getLogger("Sync.Test.FileLog");
  const MESSAGE = "this WILL show up";
  log.info(MESSAGE);

  Svc.Obs.add("weave:service:reset-file-log", function onResetFileLog() {
    Svc.Obs.remove("weave:service:reset-file-log", onResetFileLog);

    
    let entries = logsdir.directoryEntries;
    do_check_true(entries.hasMoreElements());
    let logfile = entries.getNext().QueryInterface(Ci.nsILocalFile);
    do_check_eq(logfile.leafName.slice(-4), ".log");
    do_check_false(entries.hasMoreElements());

    
    readFile(logfile, function (error, data) {
      do_check_true(Components.isSuccessCode(error));
      do_check_neq(data.indexOf(MESSAGE), -1);

      
      try {
        logfile.remove(false);
      } catch(ex) {
        dump("Couldn't delete file: " + ex + "\n");
        
      }

      Svc.Prefs.resetBranch("");
      run_next_test();
    });
  });

  
  Svc.Obs.notify("weave:service:sync:error");
});
