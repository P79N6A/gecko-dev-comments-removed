



































function test() {
  
  
  waitForExplicitFinish();
  
  let fieldValues = {
    username: "User " + Math.random(),
    passwd:   "pwd" + Date.now()
  };
  
  
  gPrefService.setIntPref("browser.sessionstore.privacy_level", 0);
  
  let testURL = "chrome://mochikit/content/browser/" +
    "browser/components/sessionstore/test/browser/browser_454908_sample.html";
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
    let doc = tab.linkedBrowser.contentDocument;
    for (let id in fieldValues)
      doc.getElementById(id).value = fieldValues[id];
    
    gBrowser.removeTab(tab);
    
    tab = undoCloseTab();
    tab.linkedBrowser.addEventListener("load", function(aEvent) {
      tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
      let doc = tab.linkedBrowser.contentDocument;
      for (let id in fieldValues) {
        let node = doc.getElementById(id);
        if (node.type == "password")
          is(node.value, "", "password wasn't saved/restored");
        else
          is(node.value, fieldValues[id], "username was saved/restored");
      }
      
      
      if (gPrefService.prefHasUserValue("browser.sessionstore.privacy_level"))
        gPrefService.clearUserPref("browser.sessionstore.privacy_level");
      
      
      if (gBrowser.tabContainer.childNodes.length == 1)
        gBrowser.addTab();
      gBrowser.removeTab(tab);
      finish();
    }, true);
  }, true);
}
