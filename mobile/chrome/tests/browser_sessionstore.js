var testURL = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_01.html";


var gTests = [];
var gCurrentTest = null;

function pageLoaded(url) {
  return function() {
    let tab = gCurrentTest._tab;
    return !tab.isLoading() && tab.browser.currentURI.spec == url;
  }
}
  
var ss = null;



function test() {
  
  
  waitForExplicitFinish();
  
  ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);

  
  runNextTest();
}



function runNextTest() {
  
  if (gTests.length > 0) {
    gCurrentTest = gTests.shift();
    info(gCurrentTest.desc);
    gCurrentTest.run();
  }
  else {
    
    try {
      
    }
    finally {
      
      finish();
    }
  }
}



gTests.push({
  desc: "Loading a page and test setting tab values",
  _tab: null,

  run: function() {
    Browser.addTab("about:blank", true);
    this._tab = Browser.addTab(testURL, true);

    
    waitFor(gCurrentTest.onPageReady, pageLoaded(testURL));
  },

  onPageReady: function() {
    
    ss.setTabValue(gCurrentTest._tab.chromeTab, "test1", "hello");
    is(ss.getTabValue(gCurrentTest._tab.chromeTab, "test1"), "hello", "Set/Get tab value matches");

    
    gCurrentTest.numTabs = Browser.tabs.length;
    gCurrentTest.numClosed = ss.getClosedTabCount(window);

    Browser.closeTab(gCurrentTest._tab);

    is(Browser.tabs.length, gCurrentTest.numTabs - 1, "Tab was closed");
    is(ss.getClosedTabCount(window), gCurrentTest.numClosed + 1, "Tab was stored");

    
    
    gCurrentTest._tab = Browser.getTabFromChrome(ss.undoCloseTab(window, 0));

    
    waitFor(gCurrentTest.onPageUndo, pageLoaded(testURL));
  },

  onPageUndo: function() {
    is(Browser.tabs.length, gCurrentTest.numTabs, "Tab was reopened");
    is(ss.getClosedTabCount(window), gCurrentTest.numClosed, "Tab was removed from store");

    is(ss.getTabValue(gCurrentTest._tab.chromeTab, "test1"), "hello", "Set/Get tab value matches after un-close");

    ss.deleteTabValue(gCurrentTest._tab.chromeTab, "test1");
    is(ss.getTabValue(gCurrentTest._tab.chromeTab, "test1"), "", "Set/Get tab value matches after removing value");

    
    Browser.closeTab(gCurrentTest._tab);
    runNextTest();
  }  
});
