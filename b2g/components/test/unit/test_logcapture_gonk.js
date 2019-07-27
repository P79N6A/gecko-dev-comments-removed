








function run_test() {
  Components.utils.import("resource:///modules/LogCapture.jsm");
  run_next_test();
}

function verifyLog(log) {
  
  notEqual(log, null);
  
  ok(log.length >= 0);
}

add_test(function test_readLogFile() {
  let mainLog = LogCapture.readLogFile("/dev/log/main");
  verifyLog(mainLog);

  let meminfoLog = LogCapture.readLogFile("/proc/meminfo");
  verifyLog(meminfoLog);

  run_next_test();
});

add_test(function test_readProperties() {
  let propertiesLog = LogCapture.readProperties();
  notEqual(propertiesLog, null, "Properties should not be null");
  notEqual(propertiesLog, undefined, "Properties should not be undefined");
  equal(propertiesLog["ro.kernel.qemu"], "1", "QEMU property should be 1");

  run_next_test();
});

add_test(function test_readAppIni() {
  let appIni = LogCapture.readLogFile("/system/b2g/application.ini");
  verifyLog(appIni);

  run_next_test();
});

add_test(function test_get_about_memory() {
  let memLog = LogCapture.readAboutMemory();

  ok(memLog, "Should have returned a valid Promise object");

  memLog.then(file => {
    ok(file, "Should have returned a filename");
    run_next_test();
  }, error => {
    ok(false, "Dumping about:memory promise rejected: " + error);
    run_next_test();
  });
});
