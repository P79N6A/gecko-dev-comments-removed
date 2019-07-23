



































function test() {
  
  
  waitForExplicitFinish();
  
  
  gPrefService.setIntPref("browser.sessionstore.privacy_level", 0);
  
  let testURL = "chrome://mochikit/content/browser/" +
    "browser/components/sessionstore/test/browser/browser_456342_sample.xhtml";
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);
    gBrowser.removeTab(tab);
    
    let ss = Cc["@mozilla.org/browser/sessionstore;1"]
               .getService(Ci.nsISessionStore);
    let undoItems = eval("(" + ss.getClosedTabData(window) + ")");
    let savedFormData = undoItems[0].state.entries[0].formdata;
    
    let countGood = 0, countBad = 0;
    for each (let value in savedFormData) {
      if (value == "save me")
        countGood++;
      else
        countBad++;
    }
    
    is(countGood, 4, "Saved text for non-standard input fields");
    is(countBad,  0, "Didn't save text for ignored field types");
    
    
    gPrefService.clearUserPref("browser.sessionstore.privacy_level");
    finish();
  }, true);
}
