






function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  waitForExplicitFinish();

  let tabBlank = gBrowser.selectedTab;
  let blankBrowser = gBrowser.getBrowserForTab(tabBlank);
  blankBrowser.addEventListener("load", function() {
    blankBrowser.removeEventListener("load", arguments.callee, true);

    
    FullZoom.enlarge();
    isnot(ZoomManager.zoom, 1, "Zoom level for about:blank should be changed");

    
    pb.privateBrowsingEnabled = true;
    let tabAboutPB = gBrowser.selectedTab;
    let browserAboutPB = gBrowser.getBrowserForTab(tabAboutPB);
    browserAboutPB.addEventListener("load", function() {
      browserAboutPB.removeEventListener("load", arguments.callee, true);
      setTimeout(function() {
        
        is(ZoomManager.zoom, 1, "Zoom level for about:privatebrowsing should be reset");
        finishTest();
      }, 0);
    }, true);
  }, true);
  blankBrowser.loadURI("about:blank");
}

function finishTest() {
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  
  pb.privateBrowsingEnabled = false;
  let tabBlank = gBrowser.selectedTab;
  let blankBrowser = gBrowser.getBrowserForTab(tabBlank);
  blankBrowser.addEventListener("load", function() {
    blankBrowser.removeEventListener("load", arguments.callee, true);

    executeSoon(function() {
      
      FullZoom.reset();
      finish();
    });
  }, true);
}
