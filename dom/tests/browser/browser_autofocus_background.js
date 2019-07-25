function test() {
  waitForExplicitFinish();

  let fm = Components.classes["@mozilla.org/focus-manager;1"]
                     .getService(Components.interfaces.nsIFocusManager);

  let tabs = [ gBrowser.selectedTab, gBrowser.addTab() ];

  
  
  let testingList = [
    { uri: "data:text/html,<!DOCTYPE html><html><body><input autofocus id='target'></body></html>",
      tagName: "INPUT"},
  ];

  function runTest() {
    
    tabs[0].linkedBrowser.focus();

    
    tabs[1].linkedBrowser.addEventListener("load", onLoadBackgroundTab, true);
    tabs[1].linkedBrowser.loadURI(testingList[0].uri);
  }

  function onLoadBackgroundTab() {
    tabs[1].linkedBrowser.removeEventListener("load", onLoadBackgroundTab, true);

    
    
    
    executeSoon(doTest);
  }

  function doTest() {
    for (var i=0; i<testingList.length; ++i) {
      
      var e = tabs[i+1].linkedBrowser.contentDocument.activeElement;

      is(e.tagName, testingList[i].tagName,
         "The background tab's focused element should be " +
         testingList[i].tagName);
      isnot(fm.focusedElement, e,
            "The background tab's focused element should not be the focus " +
            "manager focused element");
    }

    
    for (let i = 1; i < tabs.length; i++) {
      gBrowser.removeTab(tabs[i]);
    }
    finish();
  }

  runTest();
}
