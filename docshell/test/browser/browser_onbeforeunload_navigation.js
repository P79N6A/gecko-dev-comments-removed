var contentWindow;
var originalLocation;
var currentTest = -1;
var stayingOnPage = true;

var TEST_PAGE = "http://mochi.test:8888/browser/docshell/test/browser/file_bug1046022.html";
var TARGETED_PAGE = "data:text/html," + encodeURIComponent("<body>Shouldn't be seeing this</body>");

var loadExpected = TEST_PAGE;
var testTab;

var loadStarted = false;
var tabStateListener = {
  onStateChange: function(webprogress, request, stateFlags, status) {
    let startDocumentFlags = Ci.nsIWebProgressListener.STATE_START |
                             Ci.nsIWebProgressListener.STATE_IS_DOCUMENT;
    if ((stateFlags & startDocumentFlags) == startDocumentFlags) {
      loadStarted = true;
    }
  },
  onStatusChange: () => {},
  onLocationChange: () => {},
  onSecurityChange: () => {},
  onProgressChange: () => {},
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener])
};

function onTabLoaded(event) {
  info("A document loaded in a tab!");
  let loadedPage = event.target.location.href;
  if (loadedPage == "about:blank" ||
      event.originalTarget != testTab.linkedBrowser.contentDocument) {
    return;
  }

  if (!loadExpected) {
    ok(false, "Expected no page loads, but loaded " + loadedPage + " instead!");
    return;
  }

  is(loadedPage, loadExpected, "Loaded the expected page");
  if (contentWindow) {
    is(contentWindow.document, event.target, "Same doc");
  }
  if (onAfterPageLoad) {
    onAfterPageLoad();
  }
}

function onAfterTargetedPageLoad() {
  ok(!stayingOnPage, "We should only fire if we're expecting to let the onbeforeunload dialog proceed to the new location");
  is(testTab.linkedBrowser.currentURI.spec, TARGETED_PAGE, "Should have loaded the expected new page");

  runNextTest();
}

function onTabModalDialogLoaded(node) {
  let content = testTab.linkedBrowser.contentWindow;
  ok(!loadStarted, "No load should be started.");
  info(content.location.href);
  is(content, contentWindow, "Window should be the same still.");
  is(content.location.href, originalLocation, "Page should not have changed.");
  is(content.mySuperSpecialMark, 42, "Page should not have refreshed.");

  ok(!content.dialogWasInvoked, "Dialog should only be invoked once per test.");
  content.dialogWasInvoked = true;


  
  let observer = new MutationObserver(function(muts) {
    if (!node.parentNode) {
      info("Dialog is gone");
      observer.disconnect();
      observer = null;
      
      if (stayingOnPage) {
        
        
        
        executeSoon(runNextTest);
      }
      
      
    }
  });
  observer.observe(node.parentNode, {childList: true});

  
  if (!stayingOnPage) {
    loadExpected = TARGETED_PAGE;
    onAfterPageLoad = onAfterTargetedPageLoad;
  }

  let button = stayingOnPage ? node.ui.button1 : node.ui.button0;
  
  info("Clicking button: " + button.label);
  EventUtils.synthesizeMouseAtCenter(button, {});
}


Services.obs.addObserver(onTabModalDialogLoaded, "tabmodal-dialog-loaded", false);

var testFns = [
  function(e) {
    e.target.location.href = 'otherpage-href-set.html';
    return "stop";
  },
  function(e) {
    e.target.location.reload();
    return "stop";
  },
  function(e) {
    e.target.location.replace('otherpage-location-replaced.html');
    return "stop";
  },
];

function runNextTest() {
  currentTest++;
  if (currentTest >= testFns.length) {
    if (!stayingOnPage) {
      finish();
      return;
    }
    
    stayingOnPage = false;
    currentTest = 0;
  }


  if (!stayingOnPage) {
    onAfterPageLoad = runCurrentTest;
    loadExpected = TEST_PAGE;
    
    contentWindow.onbeforeunload = null;
    testTab.linkedBrowser.loadURI(TEST_PAGE);
  } else {
    runCurrentTest();
  }
}

function runCurrentTest() {
  
  contentWindow = testTab.linkedBrowser.contentWindow;
  contentWindow.mySuperSpecialMark = 42;
  contentWindow.dialogWasInvoked = false;
  originalLocation = contentWindow.location.href;
  
  info("Running test with onbeforeunload " + testFns[currentTest].toSource());
  contentWindow.onbeforeunload = testFns[currentTest];
  loadStarted = false;
  contentWindow.location.href = TARGETED_PAGE;
}

var onAfterPageLoad = runNextTest;

function test() {
  waitForExplicitFinish();
  gBrowser.addProgressListener(tabStateListener);

  testTab = gBrowser.selectedTab = gBrowser.addTab();
  testTab.linkedBrowser.addEventListener("load", onTabLoaded, true);
  testTab.linkedBrowser.loadURI(TEST_PAGE);
}

registerCleanupFunction(function() {
  
  if (contentWindow) {
    try {
      contentWindow.onbeforeunload = null;
    } catch (ex) {}
  }
  contentWindow = null;
  testTab.linkedBrowser.removeEventListener("load", onTabLoaded, true);
  Services.obs.removeObserver(onTabModalDialogLoaded, "tabmodal-dialog-loaded");
  gBrowser.removeProgressListener(tabStateListener);
  gBrowser.removeTab(testTab);
});

