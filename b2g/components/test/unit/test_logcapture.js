








function run_test() {
  Components.utils.import('resource:///modules/LogCapture.jsm');

  function verifyLog(log) {
    
    notEqual(log, null);
    
    ok(log.length >= 0);
  }

  let mainLog = LogCapture.readLogFile('/dev/log/main');
  verifyLog(mainLog);

  let meminfoLog = LogCapture.readLogFile('/proc/meminfo');
  verifyLog(meminfoLog);
}
