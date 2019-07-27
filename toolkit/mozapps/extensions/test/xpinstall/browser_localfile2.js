


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

