



function test() {
  waitForExplicitFinish();

  let testURL = "http://example.org/browser/browser/base/content/test/general/dummy_page.html";

  function testOnWindow(aOptions, aCallback) {
    whenNewWindowLoaded(aOptions, function(aWin) {
      
      
      
      executeSoon(function() aCallback(aWin));
    });
  };

  testOnWindow({}, function(aNormalWindow) {
    testOnWindow({private: true}, function(aPrivateWindow) {
      runTest(aNormalWindow, aPrivateWindow, false, function() {
        aNormalWindow.close();
        aPrivateWindow.close();
        testOnWindow({}, function(aNormalWindow) {
          testOnWindow({private: true}, function(aPrivateWindow) {
            runTest(aPrivateWindow, aNormalWindow, false, function() {
              aNormalWindow.close();
              aPrivateWindow.close();
              testOnWindow({private: true}, function(aPrivateWindow) {
                runTest(aPrivateWindow, aPrivateWindow, false, function() {
                  aPrivateWindow.close();
                  testOnWindow({}, function(aNormalWindow) {
                    runTest(aNormalWindow, aNormalWindow, true, function() {
                      aNormalWindow.close();
                      finish();
                    });
                  });
                });
              });
            });
          });
        });
      });
    });
  });

  function runTest(aSourceWindow, aDestWindow, aExpectSuccess, aCallback) {
    
    let baseTab = aSourceWindow.gBrowser.addTab(testURL);
    baseTab.linkedBrowser.addEventListener("load", function() {
      
      if (baseTab.linkedBrowser.currentURI.spec == "about:blank")
        return;
      baseTab.linkedBrowser.removeEventListener("load", arguments.callee, true);

      let testTab = aDestWindow.gBrowser.addTab();

      waitForFocus(function() {
        
        aDestWindow.gBrowser.selectedTab = testTab;

        
        ok(testTab.linkedBrowser.sessionHistory.count < 2,
           "The test tab has 1 or less history entries");
        
        is(testTab.linkedBrowser.currentURI.spec, "about:blank",
           "The test tab is on about:blank");
        
        ok(!testTab.linkedBrowser.contentDocument.body.hasChildNodes(),
           "The test tab has no child nodes");
        ok(!testTab.hasAttribute("busy"),
           "The test tab doesn't have the busy attribute");

        
        aDestWindow.gURLBar.value = "moz-action:switchtab," + testURL;
        
        aDestWindow.gURLBar.focus();

        
        
        
        
        
        
        
        
        
        
        

        function onTabClose(aEvent) {
          aDestWindow.gBrowser.tabContainer.removeEventListener("TabClose", onTabClose, false);
          aDestWindow.gBrowser.removeEventListener("load", onLoad, false);
          clearTimeout(timeout);
          
          ok(aExpectSuccess, "Tab closed as expected");
          aCallback();
        }
        function onLoad(aEvent) {
          aDestWindow.gBrowser.tabContainer.removeEventListener("TabClose", onTabClose, false);
          aDestWindow.gBrowser.removeEventListener("load", onLoad, false);
          clearTimeout(timeout);
          
          ok(aExpectSuccess, "Tab loaded as expected");
          aCallback();
        }

        aDestWindow.gBrowser.tabContainer.addEventListener("TabClose", onTabClose, false);
        aDestWindow.gBrowser.addEventListener("load", onLoad, false);
        let timeout = setTimeout(function() {
          aDestWindow.gBrowser.tabContainer.removeEventListener("TabClose", onTabClose, false);
          aDestWindow.gBrowser.removeEventListener("load", onLoad, false);
          aCallback();
        }, 500);

        
        EventUtils.synthesizeKey("VK_RETURN", {});
      }, aDestWindow);
    }, true);
  }
}

