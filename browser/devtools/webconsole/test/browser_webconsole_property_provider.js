







const TEST_URI = "data:text/html;charset=utf8,<p>test the JS property provider";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", testPropertyProvider, true);
}

function testPropertyProvider() {
  browser.removeEventListener("load", testPropertyProvider, true);
  let tools = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
  let JSPropertyProvider = tools.require("devtools/toolkit/webconsole/utils").JSPropertyProvider;

  let tmp = Cu.import("resource://gre/modules/jsdebugger.jsm", {});
  tmp.addDebuggerToGlobal(tmp);
  let dbg = new tmp.Debugger;
  let dbgWindow = dbg.makeGlobalObjectReference(content);

  let completion = JSPropertyProvider(dbgWindow, null, "thisIsNotDefined");
  is (completion.matches.length, 0, "no match for 'thisIsNotDefined");

  
  completion = JSPropertyProvider(dbgWindow, null, "window[1].acb");
  is (completion, null, "no match for 'window[1].acb");

  
  var strComplete =
    'function a() { }document;document.getElementById(window.locatio';
  completion = JSPropertyProvider(dbgWindow, null, strComplete);
  ok(completion.matches.length == 2, "two matches found");
  ok(completion.matchProp == "locatio", "matching part is 'test'");
  var matches = completion.matches;
  matches.sort();
  ok(matches[0] == "location", "the first match is 'location'");
  ok(matches[1] == "locationbar", "the second match is 'locationbar'");

  finishTest();
}

