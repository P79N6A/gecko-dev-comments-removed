


function test() {
  waitForExplicitFinish();

  var cr = Components.classes["@mozilla.org/chrome/chrome-registry;1"]
                     .getService(Components.interfaces.nsIChromeRegistry);
  
  var chromeroot = getChromeRoot(gTestPath);              
  try {
    var xpipath = cr.convertChromeURL(makeURI(chromeroot + "unsigned.xpi")).spec;
  } catch (ex) {
    var xpipath = chromeroot + "unsigned.xpi"; 
  }
  
  var triggers = encodeURIComponent(JSON.stringify({
    "Unsigned XPI": xpipath
  }));
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

