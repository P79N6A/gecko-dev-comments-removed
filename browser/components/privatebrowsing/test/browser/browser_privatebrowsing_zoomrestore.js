







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let cps = Cc["@mozilla.org/content-pref/service;1"].
            getService(Ci.nsIContentPrefService);
  waitForExplicitFinish();

  let tabBlank = gBrowser.selectedTab;
  gBrowser.removeAllTabsBut(tabBlank);

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

        
        let isOSX = ("nsILocalFileMac" in Components.interfaces);
        if (isOSX) {
          finishTest();
          return;
        }

        
        testPrintPreview(browserAboutPB, function() {
          browserAboutPB.addEventListener("load", function() {
            browserAboutPB.removeEventListener("load", arguments.callee, true);

            
            testPrintPreview(browserAboutPB, finishTest);
          }, true);
          browserAboutPB.loadURI("about:logo");
        });
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

function testPrintPreview(aBrowser, aCallback) {
  FullZoom.enlarge();
  let level = ZoomManager.getZoomForBrowser(aBrowser);

  let onEnterOrig = PrintPreviewListener.onEnter;
  PrintPreviewListener.onEnter = function () {
    PrintPreviewListener.onEnter = onEnterOrig;
    PrintPreviewListener.onEnter.apply(PrintPreviewListener, arguments);
    PrintUtils.exitPrintPreview();
  };

  let onExitOrig = PrintPreviewListener.onExit;
  PrintPreviewListener.onExit = function () {
    PrintPreviewListener.onExit = onExitOrig;
    PrintPreviewListener.onExit.apply(PrintPreviewListener, arguments);

    is(ZoomManager.getZoomForBrowser(aBrowser), level,
       "Toggling print preview mode should not affect zoom level");

    FullZoom.reset();
    aCallback();
  };

  let printPreview = new Function(document.getElementById("cmd_printPreview")
                                          .getAttribute("oncommand"));
  executeSoon(printPreview);
}
