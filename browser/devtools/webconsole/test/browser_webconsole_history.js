






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";


const HISTORY_BACK = -1;
const HISTORY_FORWARD = 1;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testHistory);
  }, true);
}

function testHistory(hud) {
  let jsterm = hud.jsterm;
  let input = jsterm.inputNode;

  let executeList = ["document", "window", "window.location"];

  for each (var item in executeList) {
    input.value = item;
    jsterm.execute();
  }

  for (var i = executeList.length - 1; i != -1; i--) {
    jsterm.historyPeruse(HISTORY_BACK);
    is (input.value, executeList[i], "check history previous idx:" + i);
  }

  jsterm.historyPeruse(HISTORY_BACK);
  is (input.value, executeList[0], "test that item is still index 0");

  jsterm.historyPeruse(HISTORY_BACK);
  is (input.value, executeList[0], "test that item is still still index 0");

  for (var i = 1; i < executeList.length; i++) {
    jsterm.historyPeruse(HISTORY_FORWARD);
    is (input.value, executeList[i], "check history next idx:" + i);
  }

  jsterm.historyPeruse(HISTORY_FORWARD);
  is (input.value, "", "check input is empty again");

  
  
  jsterm.historyPeruse(HISTORY_FORWARD);
  jsterm.historyPeruse(HISTORY_FORWARD);
  jsterm.historyPeruse(HISTORY_FORWARD);

  is (input.value, "", "check input is still empty");

  let idxLast = executeList.length - 1;
  jsterm.historyPeruse(HISTORY_BACK);
  is (input.value, executeList[idxLast], "check history next idx:" + idxLast);

  executeSoon(finishTest);
}

