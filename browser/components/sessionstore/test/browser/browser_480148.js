



































function browserWindowsCount() {
  let count = 0;
  let e = Cc["@mozilla.org/appshell/window-mediator;1"]
            .getService(Ci.nsIWindowMediator)
            .getEnumerator("navigator:browser");
  while (e.hasMoreElements()) {
    if (!e.getNext().closed)
      ++count;
  }
  return count;
}

function test() {
  
  is(browserWindowsCount(), 1, "Only one browser window should be open initially");

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  waitForExplicitFinish();

  
  function buildTestState(num, selected) {
    let state = { windows: [ { "tabs": [], "selected": selected } ] };
    while (num--)
      state.windows[0].tabs.push({entries: [{url: "http://example.com/"}]});
    return state;
  }

  
  function buildExpectedOrder(num, selected, shown) {
    
    selected--;
    let expected = [selected];
    
    for (let i = selected - (shown - expected.length); i >= 0 && i < selected; i++)
      expected.push(i);
    
    for (let i = selected + 1; expected.length < shown && i < num; i++)
      expected.push(i);
    
    for (let i = 0; i < num; i++) {
      if (expected.indexOf(i) == -1) {
        expected.push(i);
      }
    }
    return expected;
  }

  
  let numTests = 4;
  let completedTests = 0;

  
  let tabMinWidth = gPrefService.getIntPref("browser.tabs.tabMinWidth");

  function runTest(testNum, totalTabs, selectedTab, shownTabs, order) {
    let test = {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMEventListener,
                                             Ci.nsISupportsWeakReference]),

      state: buildTestState(totalTabs, selectedTab),
      numTabsToShow: shownTabs,
      expectedOrder: order,
      actualOrder: [],
      windowWidth: null,
      callback: null,
      window: null,

      handleSSTabRestoring: function (aEvent) {
        let tab = aEvent.originalTarget;
        let tabbrowser = this.window.gBrowser;
        let currentIndex = Array.indexOf(tabbrowser.mTabs, tab);
        this.actualOrder.push(currentIndex);

        if (this.actualOrder.length < this.state.windows[0].tabs.length)
          return;

        
        is(this.actualOrder.length, this.state.windows[0].tabs.length,
           "Test #" + testNum + ": Number of restored tabs is as expected");

        is(this.actualOrder.join(" "), this.expectedOrder.join(" "),
           "Test #" + testNum + ": 'visible' tabs restored first");

        
        this.window.close();
        
        if (++completedTests == numTests) {
          this.window.removeEventListener("load", this, false);
          this.window.removeEventListener("SSTabRestoring", this, false);
          is(browserWindowsCount(), 1, "Only one browser window should be open eventually");
          finish();
        }
      },

      handleLoad: function (aEvent) {
        let _this = this;
        executeSoon(function () {
          _this.window.resizeTo(_this.windowWidth, _this.window.outerHeight);
          ss.setWindowState(_this.window, JSON.stringify(_this.state), true);
        });
      },

      
      handleEvent: function (aEvent) {
        switch (aEvent.type) {
          case "load":
            this.handleLoad(aEvent);
            break;
          case "SSTabRestoring":
            this.handleSSTabRestoring(aEvent);
            break;
        }
      },

      
      run: function () {
        this.windowWidth = Math.floor((this.numTabsToShow - 0.5) * tabMinWidth);
        this.window = openDialog(location, "_blank", "chrome,all,dialog=no");
        this.window.addEventListener("SSTabRestoring", this, false);
        this.window.addEventListener("load", this, false);
      }
    };
    test.run();
  }

  
  runTest(1, 13, 1, 6,  [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]);
  runTest(2, 13, 13, 6, [12, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6]);
  runTest(3, 13, 4, 6,  [3, 4, 5, 6, 7, 8, 0, 1, 2, 9, 10, 11, 12]);
  runTest(4, 13, 11, 6, [10, 7, 8, 9, 11, 12, 0, 1, 2, 3, 4, 5, 6]);

  
}
