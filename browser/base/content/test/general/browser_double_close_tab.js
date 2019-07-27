"use strict";
const TEST_PAGE = "http://mochi.test:8888/browser/browser/base/content/test/general/file_double_close_tab.html";
let testTab;

function waitForDialog(callback) {
  function onTabModalDialogLoaded(node) {
    Services.obs.removeObserver(onTabModalDialogLoaded, "tabmodal-dialog-loaded");
    callback(node);
  }

  
  Services.obs.addObserver(onTabModalDialogLoaded, "tabmodal-dialog-loaded", false);
}

function waitForDialogDestroyed(node, callback) {
  
  let observer = new MutationObserver(function(muts) {
    if (!node.parentNode) {
      ok(true, "Dialog is gone");
      done();
    }
  });
  observer.observe(node.parentNode, {childList: true});
  let failureTimeout = setTimeout(function() {
    ok(false, "Dialog should have been destroyed");
    done();
  }, 10000);

  function done() {
    clearTimeout(failureTimeout);
    observer.disconnect();
    observer = null;
    callback();
  }
}

add_task(function*() {
  testTab = gBrowser.selectedTab = gBrowser.addTab();
  yield promiseTabLoadEvent(testTab, TEST_PAGE);
  
  
  
  
  
  let dialogNode = yield new Promise(resolveOuter => {
    waitForDialog(dialogNode => {
      waitForDialogDestroyed(dialogNode, () => {
        let doCompletion = () => setTimeout(resolveOuter, 0);
        info("Now checking if dialog is destroyed");
        ok(!dialogNode.parentNode, "onbeforeunload dialog should be gone.");
        if (dialogNode.parentNode) {
          
          let leaveBtn = dialogNode.ui.button0;
          waitForDialogDestroyed(dialogNode, doCompletion);
          EventUtils.synthesizeMouseAtCenter(leaveBtn, {});
          return;
        }
        doCompletion();
      });
      
      document.getAnonymousElementByAttribute(testTab, "anonid", "close-button").click();
    });
    
    document.getAnonymousElementByAttribute(testTab, "anonid", "close-button").click();
  });
  yield promiseWaitForCondition(() => !testTab.parentNode);
  ok(!testTab.parentNode, "Tab should be closed completely");
});

registerCleanupFunction(function() {
  if (testTab.parentNode) {
    
    try {
      testTab.linkedBrowser.contentWindow.onbeforeunload = null;
    } catch (ex) {}
    gBrowser.removeTab(testTab);
  }
});


