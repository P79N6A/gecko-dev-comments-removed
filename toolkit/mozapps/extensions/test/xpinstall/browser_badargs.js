


function test() {
  waitForExplicitFinish();
  ignoreAllUncaughtExceptions();

  var triggers = encodeURIComponent(JSON.stringify(TESTROOT + "unsigned.xpi"));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    
    executeSoon(page_loaded);
  }, true);
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
}

function page_loaded() {
  var doc = gBrowser.contentDocument;
  is(doc.getElementById("return").textContent, "exception", "installTrigger should have failed");
  gBrowser.removeCurrentTab();
  finish();
}

