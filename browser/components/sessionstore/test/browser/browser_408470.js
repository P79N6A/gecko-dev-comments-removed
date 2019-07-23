



































function test() {
  
  
  waitForExplicitFinish();
  
  let pendingCount = 1;
  let testUrl = "chrome://mochikit/content/browser/" +
    "browser/components/sessionstore/test/browser/browser_408470_sample.html";
  let tab = gBrowser.addTab(testUrl);
  
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
    
    Array.forEach(tab.linkedBrowser.contentDocument.styleSheets, function(aSS, aIx) {
      pendingCount++;
      let ssTitle = aSS.title;
      stylesheetSwitchAll(tab.linkedBrowser.contentWindow, ssTitle);
      
      let newTab = gBrowser.duplicateTab(tab);
      newTab.linkedBrowser.addEventListener("load", function(aEvent) {
        newTab.linkedBrowser.removeEventListener("load", arguments.callee, true);
        let states = Array.map(newTab.linkedBrowser.contentDocument.styleSheets,
                               function(aSS) !aSS.disabled);
        let correct = states.indexOf(true) == aIx && states.indexOf(true, aIx + 1) == -1;
        
        if (/^fail_/.test(ssTitle))
          ok(!correct, "didn't restore stylesheet " + ssTitle);
        else
          ok(correct, "restored stylesheet " + ssTitle);
        
        gBrowser.removeTab(newTab);
        if (--pendingCount == 0)
          finish();
      }, true);
    });
    
    
    tab.linkedBrowser.markupDocumentViewer.authorStyleDisabled = true;
    let newTab = gBrowser.duplicateTab(tab);
    newTab.linkedBrowser.addEventListener("load", function(aEvent) {
      newTab.linkedBrowser.removeEventListener("load", arguments.callee, true);
      is(newTab.linkedBrowser.markupDocumentViewer.authorStyleDisabled, true,
         "disabled all stylesheets");
      
      gBrowser.removeTab(newTab);
      if (--pendingCount == 0)
        finish();
    }, true);
    
    gBrowser.removeTab(tab);
  }, true);
}
