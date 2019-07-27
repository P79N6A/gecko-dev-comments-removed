








function run_test() {
  Components.utils.import("resource:///modules/LogCapture.jsm");

  function verifyLog(log) {
    
    notEqual(log, null);
    
    ok(log.length >= 0);
  }

  let propertiesLog = LogCapture.readProperties();
  notEqual(propertiesLog, null, "Properties should not be null");
  notEqual(propertiesLog, undefined, "Properties should not be undefined");
  equal(propertiesLog["ro.kernel.qemu"], "1", "QEMU property should be 1");

  let mainLog = LogCapture.readLogFile("/dev/log/main");
  verifyLog(mainLog);

  let meminfoLog = LogCapture.readLogFile("/proc/meminfo");
  verifyLog(meminfoLog);
}
