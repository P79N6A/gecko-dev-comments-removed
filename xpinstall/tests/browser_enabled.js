
var scriptLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
                             .getService(Components.interfaces.mozIJSSubScriptLoader);
scriptLoader.loadSubScript("chrome://mochikit/content/browser/xpinstall/tests/harness.js", this);



function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    
    executeSoon(page_loaded);
  }, true);
  gBrowser.loadURI(TESTROOT + "enabled.html");
}

function page_loaded() {
  gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, false);
  var doc = gBrowser.contentDocument;
  is(doc.getElementById("enabled").textContent, "true", "installTrigger should have been enabled");
  gBrowser.removeCurrentTab();
  finish();
}
