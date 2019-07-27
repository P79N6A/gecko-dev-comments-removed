


function test() {
  waitForExplicitFinish();

  var triggers = encodeURIComponent(JSON.stringify(TESTROOT + "unsigned.xpi"));
  gBrowser.selectedTab = gBrowser.addTab();

  function loadListener() {
    gBrowser.selectedBrowser.removeEventListener("load", loadListener, true);
    gBrowser.contentWindow.addEventListener("InstallTriggered", page_loaded, false);
  }

  gBrowser.selectedBrowser.addEventListener("load", loadListener, true);

  
  if (!gMultiProcessBrowser)
    expectUncaughtException();

  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
}

function page_loaded() {
  gBrowser.contentWindow.removeEventListener("InstallTriggered", page_loaded, false);
  var doc = gBrowser.contentDocument;
  is(doc.getElementById("return").textContent, "exception", "installTrigger should have failed");

  
  
  
  executeSoon(() => {
    gBrowser.removeCurrentTab();
    finish();
  });
}

