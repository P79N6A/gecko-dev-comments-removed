



"use strict";

const HOST = 'mochi.test:8888';
const URI  = "http://" + HOST + "/browser/browser/devtools/sourceeditor/test/codemirror.html";

function test() {
  requestLongerTimeout(2);
  waitForExplicitFinish();

  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;

  let browser = gBrowser.getBrowserForTab(tab);
  browser.loadURI(URI);

  function check() {
    var win = browser.contentWindow.wrappedJSObject;
    var doc = win.document;
    var out = doc.getElementById("status");

    if (!out || !out.classList.contains("done"))
      return void setTimeout(check, 100);

    ok(!win.failed, "CodeMirror tests all passed");

    while (gBrowser.tabs.length > 1) gBrowser.removeCurrentTab();
    finish();
  }

  setTimeout(check, 100);
}