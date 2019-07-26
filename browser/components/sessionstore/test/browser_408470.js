



function test() {
  

  waitForExplicitFinish();

  let pendingCount = 1;
  let rootDir = getRootDirectory(gTestPath);
  let testUrl = rootDir + "browser_408470_sample.html";
  let tab = gBrowser.addTab(testUrl);

  whenBrowserLoaded(tab.linkedBrowser, function() {
    
    Array.forEach(tab.linkedBrowser.contentDocument.styleSheets, function(aSS, aIx) {
      pendingCount++;
      let ssTitle = aSS.title;
      gPageStyleMenu.switchStyleSheet(ssTitle, tab.linkedBrowser.contentWindow);

      let newTab = gBrowser.duplicateTab(tab);
      whenTabRestored(newTab, function() {
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
      });
    });

    
    tab.linkedBrowser.markupDocumentViewer.authorStyleDisabled = true;
    let newTab = gBrowser.duplicateTab(tab);
    whenTabRestored(newTab, function() {
      is(newTab.linkedBrowser.markupDocumentViewer.authorStyleDisabled, true,
         "disabled all stylesheets");

      gBrowser.removeTab(newTab);
      if (--pendingCount == 0)
        finish();
    });

    gBrowser.removeTab(tab);
  });
}
