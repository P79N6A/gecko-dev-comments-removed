



let fm = Cc["@mozilla.org/focus-manager;1"].getService(Ci.nsIFocusManager);

function test() {
  waitForExplicitFinish();
  
  
  let consoleWin = window.open("chrome://global/content/console.xul", "_blank",
                               "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar");
  testWithOpenWindow(consoleWin);
}

function testWithOpenWindow(consoleWin) {
  
  let newTab = gBrowser.addTab("http://example.com");
  gBrowser.selectedTab = newTab;

  let numTabs = gBrowser.tabs.length;

  waitForFocus(function() {
    
    is(fm.activeWindow, consoleWin,
       "the console window is focused");

    gBrowser.tabContainer.addEventListener("TabOpen", function(aEvent) {
      gBrowser.tabContainer.removeEventListener("TabOpen", arguments.callee, true);
      let browser = aEvent.originalTarget.linkedBrowser;
      browser.addEventListener("pageshow", function(event) {
        if (event.target.location.href != "about:addons")
          return;
        browser.removeEventListener("pageshow", arguments.callee, true);

        is(fm.activeWindow, window,
           "the browser window was focused");
        is(browser.currentURI.spec, "about:addons",
           "about:addons was loaded in the window");
        is(gBrowser.tabs.length, numTabs + 1,
           "a new tab was added");

        
        executeSoon(function() {
          consoleWin.close();
          gBrowser.removeTab(gBrowser.selectedTab);
          gBrowser.removeTab(newTab);
          finish();
        });
      }, true);
    }, true);

    
    consoleWin.BrowserOpenAddonsMgr();
  }, consoleWin);
}



