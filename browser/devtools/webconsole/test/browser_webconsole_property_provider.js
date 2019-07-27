







"use strict";

const TEST_URI = "data:text/html;charset=utf8,<p>test the JS property provider";

function test() {
  loadTab(TEST_URI).then(testPropertyProvider);
}

function testPropertyProvider({browser}) {
  browser.removeEventListener("load", testPropertyProvider, true);
  let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
  let {require} = devtools;
  let {JSPropertyProvider} = require("devtools/toolkit/webconsole/utils");

  let tmp = Cu.import("resource://gre/modules/jsdebugger.jsm", {});
  tmp.addDebuggerToGlobal(tmp);
  let dbg = new tmp.Debugger();
  let dbgWindow = dbg.makeGlobalObjectReference(content);

  let completion = JSPropertyProvider(dbgWindow, null, "thisIsNotDefined");
  is(completion.matches.length, 0, "no match for 'thisIsNotDefined");

  
  completion = JSPropertyProvider(dbgWindow, null, "window[1].acb");
  is(completion, null, "no match for 'window[1].acb");

  
  let strComplete =
    "function a() { }document;document.getElementById(window.locatio";
  completion = JSPropertyProvider(dbgWindow, null, strComplete);
  ok(completion.matches.length == 2, "two matches found");
  ok(completion.matchProp == "locatio", "matching part is 'test'");
  let matches = completion.matches;
  matches.sort();
  ok(matches[0] == "location", "the first match is 'location'");
  ok(matches[1] == "locationbar", "the second match is 'locationbar'");

  finishTest();
}
