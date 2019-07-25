function test() {
  waitForExplicitFinish();

  let fm = Components.classes["@mozilla.org/focus-manager;1"]
                     .getService(Components.interfaces.nsIFocusManager);

  let tabs = [ gBrowser.selectedTab, gBrowser.addTab(), gBrowser.addTab() ];

  
  
  let testingList = [
    { uri: "data:text/html,<!DOCTYPE html><html><body><input autofocus id='target'></body></html>",
      tagName: "INPUT"},
    { uri: "data:text/html,<!DOCTYPE html><html><body><input id='target'></body></html>",
      tagName: "BODY"}
  ];

  function runTest() {
    
    tabs[0].linkedBrowser.focus();

    
    tabs[1].linkedBrowser.addEventListener("load", onLoadBackgroundFirstTab, true);
    tabs[1].linkedBrowser.loadURI(testingList[0].uri);
  }

  function onLoadBackgroundFirstTab() {
    tabs[1].linkedBrowser.removeEventListener("load", onLoadBackgroundFirstTab, true);

    
    tabs[2].linkedBrowser.addEventListener("load", onLoadBackgroundSecondTab, true);
    tabs[2].linkedBrowser.loadURI(testingList[1].uri);
  }

  function onLoadBackgroundSecondTab() {
    tabs[2].linkedBrowser.removeEventListener("load", onLoadBackgroundSecondTab, true);

    
    setTimeout(doTest, 1000);
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

    
    gBrowser.addTab();
    for (let i = 0; i < tabs.length; i++) {
      gBrowser.removeTab(tabs[i]);
    }
    finish();
  }

  runTest();
}
