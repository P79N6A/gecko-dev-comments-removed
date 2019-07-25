


function test() {
  waitForExplicitFinish();

  var triggers = encodeURIComponent(JSON.stringify(TESTROOT + "unsigned.xpi"));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    
    executeSoon(page_loaded);
  }, true);
  expectUncaughtException();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
}

function page_loaded() {
  var doc = gBrowser.contentDocument;
  is(doc.getElementById("return").textContent, "exception", "installTrigger should have failed");
  gBrowser.removeCurrentTab();
  finish();
}

