


function run_test () {
  Components.utils.import("resource://testing-common/StructuredLog.jsm");

  let testBuffer = [];

  let appendBuffer = function (msg) {
    testBuffer.push(msg);
  }

  let assertLastMsg = function (refData) {
    
    
    let lastMsg = JSON.parse(testBuffer.pop());
    for (let field in refData) {
      deepEqual(lastMsg[field], refData[field]);
    }
    
    equal(lastMsg.source, "test_log");
    
    equal(lastMsg.source_file, "test_structuredlog.js");
  }

  let addFileName = function (data) {
    data.source_file = "test_structuredlog.js";
  }

  let logger = new StructuredLogger("test_log", appendBuffer, [addFileName]);

  
  logger.info("Test message");
  assertLastMsg({
    action: "log",
    message: "Test message",
    level: "INFO",
  });

  logger.info("Test message",
              extra={foo: "bar"});
  assertLastMsg({
    action: "log",
    message: "Test message",
    level: "INFO",
    extra: {foo: "bar"},
  });

  
  logger.testStart("aTest");
  ok(logger._runningTests.has("aTest"));
  assertLastMsg({
    test: "aTest",
    action: "test_start",
  });

  logger.testEnd("aTest", "OK");
  ok(!logger._runningTests.has("aTest"));
  assertLastMsg({
    test: "aTest",
    action: "test_end",
    status: "OK"
  });

  
  logger.testStart("aTest");
  logger.testEnd("aTest", "FAIL", "PASS");
  assertLastMsg({
    action: "test_end",
    test: "aTest",
    status: "FAIL",
    expected: "PASS"
  });

  
  logger.testStart("aTest");
  logger.testEnd("aTest", "FAIL", "PASS", null, "Many\nlines\nof\nstack\n");
  assertLastMsg({
    action: "test_end",
    test: "aTest",
    status: "FAIL",
    expected: "PASS",
    stack: "Many\nlines\nof\nstack\n"
  });

  
  logger.testStart("aTest");
  logger.testEnd("aTest", "SKIP", "PASS");
  ok(!JSON.parse(testBuffer[testBuffer.length - 1]).hasOwnProperty("expected"));
  assertLastMsg({
    action: "test_end",
    test: "aTest",
    status: "SKIP"
  });

  
  logger.testEnd("aTest", "PASS");
  assertLastMsg({
    action: "log",
    level: "ERROR"
  });

  
  logger.testEnd("errantTest", "PASS");
  assertLastMsg({
    action: "log",
    level: "ERROR"
  });

  logger.testStatus("aTest", "foo", "PASS", "PASS", "Passed test");
  ok(!JSON.parse(testBuffer[testBuffer.length - 1]).hasOwnProperty("expected"));
  assertLastMsg({
    action: "test_status",
    test: "aTest",
    subtest: "foo",
    status: "PASS",
    message: "Passed test"
  });

  logger.testStatus("aTest", "bar", "FAIL");
  assertLastMsg({
    action: "test_status",
    test: "aTest",
    subtest: "bar",
    status: "FAIL",
    expected: "PASS"
  });

  logger.testStatus("aTest", "bar", "FAIL", "PASS", null,
                    "Many\nlines\nof\nstack\n");
  assertLastMsg({
    action: "test_status",
    test: "aTest",
    subtest: "bar",
    status: "FAIL",
    expected: "PASS",
    stack: "Many\nlines\nof\nstack\n"
  });

  
  logger.testStatus("aTest", "baz", "SKIP");
  ok(!JSON.parse(testBuffer[testBuffer.length - 1]).hasOwnProperty("expected"));
  assertLastMsg({
    action: "test_status",
    test: "aTest",
    subtest: "baz",
    status: "SKIP"
  });

  
  logger.suiteStart(["aTest"]);
  assertLastMsg({
    action: "suite_start",
    tests: ["aTest"],
  });

  logger.suiteEnd();
  assertLastMsg({
    action: "suite_end",
  });
}
