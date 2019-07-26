


function testFailed(msg) {
  ok(false, msg);
}

function testPassed(msg) {
  ok(true, msg);
}

function finishJSTest() {
  SimpleTest.finish();
}

