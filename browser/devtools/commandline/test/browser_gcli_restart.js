




const TEST_URI = "data:text/html;charset=utf-8,gcli-command-restart";

function test() {
  DeveloperToolbarTest.test(TEST_URI, function(browser, tab) {
    testRestart();
    finish();
  });
}

function testRestart() {
  DeveloperToolbarTest.checkInputStatus({
    typed:  "restart",
    markup: "VVVVVVV",
    status: "VALID",
    emptyParameters: [ " [nocache]" ],
  });

  DeveloperToolbarTest.checkInputStatus({
    typed:  "restart ",
    markup: "VVVVVVVV",
    status: "VALID",
    directTabText: "false"
  });

  DeveloperToolbarTest.checkInputStatus({
    typed:  "restart t",
    markup: "VVVVVVVVI",
    status: "ERROR",
    directTabText: "rue"
  });

  DeveloperToolbarTest.checkInputStatus({
    typed:  "restart --nocache",
    markup: "VVVVVVVVVVVVVVVVV",
    status: "VALID"
  });

  DeveloperToolbarTest.checkInputStatus({
    typed:  "restart --noca",
    markup: "VVVVVVVVEEEEEE",
    status: "ERROR",
  });
}
