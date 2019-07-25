var testURL = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_01.html";


var gTests = [];
var gCurrentTest = null;
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
  _currentTab: null,

  run: function() {
    Browser.addTab("about:blank", true);
    this._currentTab = Browser.addTab(testURL, true);

    
    messageManager.addMessageListener("pageshow",
    function(aMessage) {
      if (gCurrentTest._currentTab.browser.currentURI.spec != "about:blank") {
        messageManager.removeMessageListener(aMessage.name, arguments.callee);
        gCurrentTest.onPageReady();
      }
    });
  },

  onPageReady: function() {
    
    ss.setTabValue(gCurrentTest._currentTab.chromeTab, "test1", "hello");
    is(ss.getTabValue(gCurrentTest._currentTab.chromeTab, "test1"), "hello", "Set/Get tab value matches");

    
    gCurrentTest.numTabs = Browser.tabs.length;
    gCurrentTest.numClosed = ss.getClosedTabCount(window);

    Browser.closeTab(gCurrentTest._currentTab);

    isnot(Browser.tabs.length, gCurrentTest.numTabs, "Tab was closed");

    
    todo_isnot(ss.getClosedTabCount(window), gCurrentTest.numClosed, "Tab was stored");

    
    
    gCurrentTest._currentTab = Browser.getTabFromChrome(ss.undoCloseTab(window, 0));

    
    messageManager.addMessageListener("pageshow",
    function(aMessage) {
      if (gCurrentTest._currentTab.browser.currentURI.spec != "about:blank") {
        messageManager.removeMessageListener(aMessage.name, arguments.callee);
        gCurrentTest.onPageUndo();
      }
    });
  },

  onPageUndo: function() {
    is(Browser.tabs.length, gCurrentTest.numTabs, "Tab was reopened");
    
    todo_is(ss.getClosedTabCount(window), gCurrentTest.numClosed, "Tab was removed from store");

    is(ss.getTabValue(gCurrentTest._currentTab.chromeTab, "test1"), "hello", "Set/Get tab value matches after un-close");

    ss.deleteTabValue(gCurrentTest._currentTab.chromeTab, "test1");
    is(ss.getTabValue(gCurrentTest._currentTab.chromeTab, "test1"), "", "Set/Get tab value matches after removing value");

    
    Browser.closeTab(gCurrentTest._currentTab);
    runNextTest();
  }
});
