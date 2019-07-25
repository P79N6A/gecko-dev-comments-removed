



































function test() {
  
  requestLongerTimeout(2);

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  waitForExplicitFinish();

  
  function buildTestState(num, selected, hidden) {
    let state = { windows: [ { "tabs": [], "selected": selected } ] };
    while (num--) {
      state.windows[0].tabs.push({entries: [{url: "http://example.com/"}]});
      let i = state.windows[0].tabs.length - 1;
      if (hidden.length > 0 && i == hidden[0]) {
        state.windows[0].tabs[i].hidden = true;
        hidden.splice(0, 1);
      }
    }
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

  
  let numTests = 7;
  let completedTests = 0;

  let tabMinWidth = parseInt(getComputedStyle(gBrowser.selectedTab, null).minWidth);

  function runTest(testNum, totalTabs, selectedTab, shownTabs, hiddenTabs, order) {
    let test = {
      state: buildTestState(totalTabs, selectedTab, hiddenTabs),
      numTabsToShow: shownTabs,
      expectedOrder: order,
      actualOrder: [],
      windowWidth: null,
      callback: null,
      window: null,

      handleSSTabRestoring: function (aEvent) {
        let tab = aEvent.originalTarget;
        let tabbrowser = this.window.gBrowser;
        let currentIndex = Array.indexOf(tabbrowser.tabs, tab);
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
          finish();
        }
      },

      handleLoad: function (aEvent) {
        let _this = this;
        executeSoon(function () {
          let extent = _this.window.outerWidth - _this.window.gBrowser.tabContainer.mTabstrip.scrollClientSize;
          let windowWidth = _this.tabbarWidth + extent;
          _this.window.resizeTo(windowWidth, _this.window.outerHeight);
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
        this.tabbarWidth = Math.floor((this.numTabsToShow - 0.5) * tabMinWidth);
        this.window = openDialog(location, "_blank", "chrome,all,dialog=no");
        this.window.addEventListener("SSTabRestoring", this, false);
        this.window.addEventListener("load", this, false);
      }
    };
    test.run();
  }

  
  runTest(1, 13, 1,  6, [],         [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]);
  runTest(2, 13, 13, 6, [],         [12, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6]);
  runTest(3, 13, 4,  6, [],         [3, 4, 5, 6, 7, 8, 0, 1, 2, 9, 10, 11, 12]);
  runTest(4, 13, 11, 6, [],         [10, 7, 8, 9, 11, 12, 0, 1, 2, 3, 4, 5, 6]);
  runTest(5, 13, 13, 6, [0, 4, 9],  [12, 6, 7, 8, 10, 11, 1, 2, 3, 5, 0, 4, 9]);
  runTest(6, 13, 4,  6, [1, 7, 12], [3, 4, 5, 6, 8, 9, 0, 2, 10, 11, 1, 7, 12]);
  runTest(7, 13, 4,  6, [0, 1, 2],  [3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0, 1, 2]);

  
}
