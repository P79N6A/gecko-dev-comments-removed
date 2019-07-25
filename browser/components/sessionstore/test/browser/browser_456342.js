



































function test() {
  
  
  waitForExplicitFinish();
  
  
  gPrefService.setIntPref("browser.sessionstore.privacy_level", 0);
  
  let rootDir = getRootDirectory(gTestPath);
  let testURL = rootDir + "browser_456342_sample.xhtml";
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);

    let expectedValue = "try to save me";
    
    
    let formEls = aEvent.originalTarget.forms[0].elements;
    for (let i = 0; i < formEls.length; i++)
      formEls[i].value = expectedValue;

    gBrowser.removeTab(tab);
    
    let undoItems = JSON.parse(ss.getClosedTabData(window));
    let savedFormData = undoItems[0].state.entries[0].formdata;
    
    let countGood = 0, countBad = 0;
    for each (let value in savedFormData) {
      if (value == expectedValue)
        countGood++;
      else
        countBad++;
    }
    
    is(countGood, 4, "Saved text for non-standard input fields");
    is(countBad,  0, "Didn't save text for ignored field types");
    
    
    if (gPrefService.prefHasUserValue("browser.sessionstore.privacy_level"))
      gPrefService.clearUserPref("browser.sessionstore.privacy_level");
    finish();
  }, true);
}
