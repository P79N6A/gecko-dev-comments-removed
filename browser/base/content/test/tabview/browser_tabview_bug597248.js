




































let newWin;
let restoredWin;
let newTabOne;
let newTabTwo;
let restoredNewTabOneLoaded = false;
let restoredNewTabTwoLoaded = false;
let frameInitialized = false;

function test() {
  waitForExplicitFinish();

  
  newWin = openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no");
  newWin.addEventListener("load", function(event) {
    newWin.removeEventListener("load", arguments.callee, false);
    setupOne();
  }, false);
}

function setupOne() {
  let loadedCount = 0;
  let allLoaded = function() {
    if (++loadedCount == 2) {
      newWin.addEventListener("tabviewshown", setupTwo, false);
      newWin.TabView.toggle();
    }
  }
  
  newTabOne = newWin.gBrowser.tabs[0];
  newTabTwo = newWin.gBrowser.addTab();
  load(newTabOne, "http://mochi.test:8888/browser/browser/base/content/test/tabview/search1.html", allLoaded);
  load(newTabTwo, "http://mochi.test:8888/browser/browser/base/content/test/tabview/dummy_page.html", allLoaded);
}

function setupTwo() {
  newWin.removeEventListener("tabviewshown", setupTwo, false);

  let contentWindow = newWin.document.getElementById("tab-view").contentWindow;

  let tabItems = contentWindow.TabItems.getItems();
  is(tabItems.length, 2, "There should be 2 tab items before closing");

  let numTabsToSave = tabItems.length;

  
  tabItems.forEach(function(tabItem) {
    contentWindow.TabItems._update(tabItem.tab);
    tabItem.addSubscriber(tabItem, "savedImageData", function(item) {
      item.removeSubscriber(item, "savedImageData");
      --numTabsToSave;
    });
  });

  
  let xulWindowDestory = function() {
    Services.obs.removeObserver(
       xulWindowDestory, "xul-window-destroyed", false);

    newWin = null;
    
    
    executeSoon(function() {
      restoredWin = undoCloseWindow();
      restoredWin.addEventListener("load", function(event) {
        restoredWin.removeEventListener("load", arguments.callee, false);

        
        is(numTabsToSave, 0, "All tabs were saved when window was closed.");

        
        let onTabViewFrameInitialized = function() {
          restoredWin.removeEventListener(
            "tabviewframeinitialized", onTabViewFrameInitialized, false);

          






          restoredWin.close();
          finish();
        }
        restoredWin.addEventListener(
          "tabviewframeinitialized", onTabViewFrameInitialized, false);

        is(restoredWin.gBrowser.tabs.length, 2, "The total number of tabs is 2");

        








      }, false);
    });
  };

  Services.obs.addObserver(
    xulWindowDestory, "xul-window-destroyed", false);

  newWin.close();
}

let gTabsProgressListener = {
  onStateChange: function(browser, webProgress, request, stateFlags, status) {
    
    if ((stateFlags & Ci.nsIWebProgressListener.STATE_STOP) &&
        (stateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW) &&
         browser.currentURI.spec != "about:blank") {
      if (newTabOne.linkedBrowser == browser)
        restoredNewTabOneLoaded = true;
      else if (newTabTwo.linkedBrowser == browser)
        restoredNewTabTwoLoaded = true;

      
      
      if (restoredNewTabOneLoaded && restoredNewTabTwoLoaded) {
        restoredWin.gBrowser.removeTabsProgressListener(gTabsProgressListener);

        if (frameInitialized) {
          
          
          
          executeSoon(updateAndCheck); 
        }
      }
    }
  }
};

function onTabViewShown() {
  restoredWin.removeEventListener("tabviewshown", onTabViewShown, false);

  let contentWindow = 
    restoredWin.document.getElementById("tab-view").contentWindow;

  let nextStep = function() {
    
    
    if (restoredNewTabOneLoaded && restoredNewTabTwoLoaded) {
      
      
      
      executeSoon(updateAndCheck);
    } else {
      frameInitialized = true;
    }
  }

  let tabItems = contentWindow.TabItems.getItems();
  let count = tabItems.length;
  tabItems.forEach(function(tabItem) {
    
    
    if (tabItem._reconnected) {
      ok(tabItem.isShowingCachedData(), 
         "Tab item is showing cached data and is already connected. " +
         tabItem.tab.linkedBrowser.currentURI.spec);
      if (--count == 0)
        nextStep();
    } else {
      tabItem.addSubscriber(tabItem, "reconnected", function() {
        tabItem.removeSubscriber(tabItem, "reconnected");
        ok(tabItem.isShowingCachedData(), 
           "Tab item is showing cached data and is just connected. "  +
           tabItem.tab.linkedBrowser.currentURI.spec);
        if (--count == 0)
          nextStep();
      });
    }
  });
}

function updateAndCheck() {
  
  let contentWindow = 
    restoredWin.document.getElementById("tab-view").contentWindow;

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

function load(tab, url, callback) {
  tab.linkedBrowser.addEventListener("load", function (event) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
    callback();
  }, true);
  tab.linkedBrowser.loadURI(url);
}

