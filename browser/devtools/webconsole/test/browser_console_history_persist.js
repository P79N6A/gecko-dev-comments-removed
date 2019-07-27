







"use strict";

const TEST_URI = "data:text/html;charset=utf-8,Web Console test for persisting history - bug 943306";
const INPUT_HISTORY_COUNT = 10;

let test = asyncTest(function* () {
  info ("Setting custom input history pref to " + INPUT_HISTORY_COUNT);
  Services.prefs.setIntPref("devtools.webconsole.inputHistoryCount", INPUT_HISTORY_COUNT);

  
  
  yield loadTab(TEST_URI);
  let hud1 = yield openConsole();
  is (JSON.stringify(hud1.jsterm.history), "[]", "No history on first tab initially");
  yield populateInputHistory(hud1);
  is (JSON.stringify(hud1.jsterm.history), '["0","1","2","3","4","5","6","7","8","9"]',
    "First tab has populated history");

  
  
  yield loadTab(TEST_URI);
  let hud2 = yield openConsole();
  is (JSON.stringify(hud2.jsterm.history), '["0","1","2","3","4","5","6","7","8","9"]',
    "Second tab has populated history");
  yield testNaviatingHistoryInUI(hud2);
  is (JSON.stringify(hud2.jsterm.history), '["0","1","2","3","4","5","6","7","8","9",""]',
    "An empty entry has been added in the second tab due to history perusal");

  
  
  yield loadTab(TEST_URI);
  let hud3 = yield openConsole();
  is (JSON.stringify(hud3.jsterm.history), '["0","1","2","3","4","5","6","7","8","9"]',
    "Third tab has populated history");

  
  hud3.jsterm.setInputValue('"hello from third tab"');
  hud3.jsterm.execute();

  is (JSON.stringify(hud1.jsterm.history), '["0","1","2","3","4","5","6","7","8","9"]',
    "First tab history hasn't changed due to command in third tab");
  is (JSON.stringify(hud2.jsterm.history), '["0","1","2","3","4","5","6","7","8","9",""]',
    "Second tab history hasn't changed due to command in third tab");
  is (JSON.stringify(hud3.jsterm.history), '["1","2","3","4","5","6","7","8","9","\\"hello from third tab\\""]',
    "Third tab has updated history (and purged the first result) after running a command");

  
  
  yield loadTab(TEST_URI);
  let hud4 = yield openConsole();
  is (JSON.stringify(hud4.jsterm.history), '["1","2","3","4","5","6","7","8","9","\\"hello from third tab\\""]',
    "Fourth tab has most recent history");

  yield hud4.jsterm.clearHistory();
  is (JSON.stringify(hud4.jsterm.history), '[]',
    "Clearing history for a tab works");

  yield loadTab(TEST_URI);
  let hud5 = yield openConsole();
  is (JSON.stringify(hud5.jsterm.history), '[]',
    "Clearing history carries over to a new tab");

  info ("Clearing custom input history pref");
  Services.prefs.clearUserPref("devtools.webconsole.inputHistoryCount");
});





function* populateInputHistory(hud) {
  let jsterm = hud.jsterm;
  let {inputNode} = jsterm;

  for (let i = 0; i < INPUT_HISTORY_COUNT; i++) {
    
    jsterm.setInputValue(i);
    jsterm.execute();
  }
}





function* testNaviatingHistoryInUI(hud) {
  let jsterm = hud.jsterm;
  let {inputNode} = jsterm;
  inputNode.focus();

  
  
  for (let i = INPUT_HISTORY_COUNT - 1; i >= 0; i--) {
    EventUtils.synthesizeKey("VK_UP", {});
    is(inputNode.value, i, "Pressing up restores last input");
  }
}
