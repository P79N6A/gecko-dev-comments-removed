









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testHistory, false);
}

function testHistory() {
  browser.removeEventListener("DOMContentLoaded", testHistory, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];
  let HUD = HUDService.hudWeakReferences[hudId].get();
  let jsterm = HUD.jsterm;
  let input = jsterm.inputNode;

  let executeList = ["document", "window", "window.location"];

  for each (var item in executeList) {
    input.value = item;
    jsterm.execute();
  }

  for (var i = executeList.length - 1; i != -1; i--) {
    jsterm.historyPeruse(true);
    is (input.value, executeList[i], "check history previous idx:" + i);
  }

  jsterm.historyPeruse(true);
  is (input.value, executeList[0], "test that item is still index 0");

  jsterm.historyPeruse(true);
  is (input.value, executeList[0], "test that item is still still index 0");


  for (var i = 1; i < executeList.length; i++) {
    jsterm.historyPeruse(false);
    is (input.value, executeList[i], "check history next idx:" + i);
  }

  jsterm.historyPeruse(false);
  is (input.value, "", "check input is empty again");

  
  
  jsterm.historyPeruse(false);
  jsterm.historyPeruse(false);
  jsterm.historyPeruse(false);

  is (input.value, "", "check input is still empty");

  let idxLast = executeList.length - 1;
  jsterm.historyPeruse(true);
  is (input.value, executeList[idxLast], "check history next idx:" + idxLast);

  jsterm.clearOutput();
  jsterm.history.splice(0);   

  finishTest();
}

