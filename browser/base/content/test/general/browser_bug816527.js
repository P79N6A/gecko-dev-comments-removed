



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

  function runTest(aSourceWindow, aDestWindow, aExpectSwitch, aCallback) {
    
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

        let urlbar = aDestWindow.gURLBar;
        let controller = urlbar.controller;

        
        urlbar.focus();
        urlbar.value = testURL;
        controller.startSearch(testURL);

        
        promisePopupShown(aDestWindow.gURLBar.popup).then(() => {
          function searchIsComplete() {
            return controller.searchStatus ==
              Ci.nsIAutoCompleteController.STATUS_COMPLETE_MATCH;
          }

          
          waitForCondition(searchIsComplete, function () {
            if (aExpectSwitch) {
              
              
              let tabContainer = aDestWindow.gBrowser.tabContainer;
              tabContainer.addEventListener("TabClose", function onClose(event) {
                if (event.target == testTab) {
                  tabContainer.removeEventListener("TabClose", onClose);
                  executeSoon(aCallback);
                }
              });
            } else {
              
              testTab.addEventListener("load", function onLoad() {
                testTab.removeEventListener("load", onLoad, true);
                executeSoon(aCallback);
              }, true);
            }

            
            if (controller.matchCount > 1) {
              controller.handleKeyNavigation(KeyEvent.DOM_VK_DOWN);
            }

            
            controller.handleEnter(true);
          });
        });

      }, aDestWindow);
    }, true);
  }
}
