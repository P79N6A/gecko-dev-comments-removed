




































function test() {
  
  waitForExplicitFinish();

  let testURL = getRootDirectory(gTestPath) + "browser_597315_index.html";
  let tab = gBrowser.addTab(testURL);
  gBrowser.selectedTab = tab;

  waitForLoadsInBrowser(tab.linkedBrowser, 4, function() {
    let browser_b = tab.linkedBrowser.contentDocument.getElementsByTagName("frame")[1];
    let document_b = browser_b.contentDocument;
    let links = document_b.getElementsByTagName("a");

    
    waitForLoadsInBrowser(tab.linkedBrowser, 1, function() {
      waitForLoadsInBrowser(tab.linkedBrowser, 1, function() {

        gBrowser.removeTab(tab);
        
        let newTab = ss.undoCloseTab(window, 0);

        waitForLoadsInBrowser(newTab.linkedBrowser, 4, function() {
          gBrowser.goBack();
          waitForLoadsInBrowser(newTab.linkedBrowser, 1, function() {

            let expectedURLEnds = ["a.html", "b.html", "c1.html"];
            let frames = newTab.linkedBrowser.contentDocument.getElementsByTagName("frame");
            for (let i = 0; i < frames.length; i++) {
              is(frames[i].contentDocument.location,
                 getRootDirectory(gTestPath) + "browser_597315_" + expectedURLEnds[i],
                 "frame " + i + " has the right url");
            }
            gBrowser.removeTab(newTab);
            executeSoon(finish);
          });
        });
      });
      EventUtils.sendMouseEvent({type:"click"}, links[1], browser_b.contentWindow);
    });
    EventUtils.sendMouseEvent({type:"click"}, links[0], browser_b.contentWindow);
  });
}


function waitForLoadsInBrowser(aBrowser, aLoadCount, aCallback) {
  let loadCount = 0;
  aBrowser.addEventListener("load", function(aEvent) {
    if (++loadCount < aLoadCount)
      return;

    aBrowser.removeEventListener("load", arguments.callee, true);
    aCallback();
  }, true);
}
