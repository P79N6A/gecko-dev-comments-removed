


let newTabOne;
let newTabTwo;
let newTabThree;
let restoredNewTabTwoLoaded = false;
let restoredNewTabThreeLoaded = false;
let frameInitialized = false;

function test() {
  waitForExplicitFinish();
  newWindowWithTabView(setupOne);
}

function setupOne(win) {
  win.TabView.firstUseExperienced = true;

  win.gBrowser.addTab("http://mochi.test:8888/browser/browser/base/content/test/tabview/search1.html");
  win.gBrowser.addTab("http://mochi.test:8888/browser/browser/base/content/test/tabview/dummy_page.html");

  afterAllTabsLoaded(function () setupTwo(win), win);
}

let restoreWin;

function setupTwo(win) {
  let contentWindow = win.TabView.getContentWindow();

  let tabItems = contentWindow.TabItems.getItems();
  is(tabItems.length, 3, "There should be 3 tab items before closing");

  let numTabsToSave = tabItems.length;

  
  tabItems.forEach(function(tabItem) {
    contentWindow.TabItems.update(tabItem.tab);
    tabItem.addSubscriber(tabItem, "savedCachedImageData", function(item) {
      item.removeSubscriber(item, "savedCachedImageData");
      --numTabsToSave;
    });
  });

  
  let xulWindowDestory = function() {
    Services.obs.removeObserver(
       xulWindowDestory, "xul-window-destroyed", false);

    
    
    executeSoon(function() {
      restoredWin = undoCloseWindow();
      restoredWin.addEventListener("load", function onLoad(event) {
        restoredWin.removeEventListener("load", onLoad, false);

        registerCleanupFunction(function() restoredWin.close());

        
        is(numTabsToSave, 0, "All tabs were saved when window was closed.");
        is(restoredWin.gBrowser.tabs.length, 3, "The total number of tabs is 3");

        
        newTabOne = restoredWin.gBrowser.tabs[0];
        newTabTwo = restoredWin.gBrowser.tabs[1];
        newTabThree = restoredWin.gBrowser.tabs[2];
        restoredWin.gBrowser.addTabsProgressListener(gTabsProgressListener);

        
        let onTabViewFrameInitialized = function() {
          restoredWin.removeEventListener(
            "tabviewframeinitialized", onTabViewFrameInitialized, false);

          let restoredContentWindow = restoredWin.TabView.getContentWindow();
          
          restoredContentWindow.TabItems._pauseUpdateForTest = true;

          let nextStep = function() {
            
            
            if (restoredNewTabTwoLoaded && restoredNewTabThreeLoaded)
              updateAndCheck();
            else
              frameInitialized = true;
          }

          let tabItems = restoredContentWindow.TabItems.getItems();
          let count = tabItems.length;

          tabItems.forEach(function(tabItem) {
            tabItem.addSubscriber(tabItem, "loadedCachedImageData", function() {
              tabItem.removeSubscriber(tabItem, "loadedCachedImageData");
              ok(tabItem.isShowingCachedData(),
                "Tab item is showing cached data and is just connected. " +
                tabItem.tab.linkedBrowser.currentURI.spec);
              if (--count == 0)
                nextStep();
            });
          });
        }

        restoredWin.addEventListener(
          "tabviewframeinitialized", onTabViewFrameInitialized, false);
      }, false);
    });
  };
  Services.obs.addObserver(xulWindowDestory, "xul-window-destroyed", false);

  win.close();
}

let gTabsProgressListener = {
  onStateChange: function(browser, webProgress, request, stateFlags, status) {
    
    if ((stateFlags & Ci.nsIWebProgressListener.STATE_STOP) &&
        (stateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW) &&
         browser.currentURI.spec != "about:blank") {
      if (newTabTwo.linkedBrowser == browser)
        restoredNewTabTwoLoaded = true;
      else if (newTabThree.linkedBrowser == browser)
        restoredNewTabThreeLoaded = true;

      
      
      if (restoredNewTabTwoLoaded && restoredNewTabThreeLoaded) {
        restoredWin.gBrowser.removeTabsProgressListener(gTabsProgressListener);

        if (frameInitialized)
          updateAndCheck();
      }
    }
  }
};

function updateAndCheck() {
  
  let contentWindow = restoredWin.TabView.getContentWindow();

  contentWindow.TabItems._pauseUpdateForTest = false;

  let tabItems = contentWindow.TabItems.getItems();
  tabItems.forEach(function(tabItem) {
    contentWindow.TabItems._update(tabItem.tab);
    ok(!tabItem.isShowingCachedData(),
      "Tab item is not showing cached data anymore. " +
      tabItem.tab.linkedBrowser.currentURI.spec);
  });

  
  restoredWin.close();
  finish();
}
