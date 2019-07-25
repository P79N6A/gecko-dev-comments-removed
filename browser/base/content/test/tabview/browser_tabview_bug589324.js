




































function test() {
  const DUMMY_PAGE_URL = "http://mochi.test:8888/browser/browser/base/content/test/tabview/dummy_page.html";
  const DUMMY_PAGE_URL_2 = "http://mochi.test:8888/";

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  waitForExplicitFinish();

  
  let newWin = openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no");
  newWin.addEventListener("load", function(event) {
    this.removeEventListener("load", arguments.callee, false);

    let newState = {
      windows: [{
        tabs: [{
          entries: [{ url: DUMMY_PAGE_URL }],
          hidden: true,
          attributes: {},
          extData: {
            "tabview-tab":
              '{"bounds":{"left":21,"top":29,"width":204,"height":153},' +
              '"userSize":null,"url":"' + DUMMY_PAGE_URL + '","groupID":1,' + 
              '"imageData":null,"title":null}'
          }
        },{
          entries: [{ url: DUMMY_PAGE_URL_2 }],
          hidden: false,
          attributes: {},
          extData: {
            "tabview-tab": 
              '{"bounds":{"left":315,"top":29,"width":111,"height":84},' + 
              '"userSize":null,"url":"' + DUMMY_PAGE_URL_2 + '","groupID":2,' + 
              '"imageData":null,"title":null}'
          },
        }],
        selected:2,
        _closedTabs: [],
        extData: {
          "tabview-groups": '{"nextID":3,"activeGroupId":2}',
          "tabview-group": 
            '{"1":{"bounds":{"left":15,"top":5,"width":280,"height":232},' + 
            '"userSize":null,"locked":{},"title":"","id":1},' + 
            '"2":{"bounds":{"left":309,"top":5,"width":267,"height":226},' + 
            '"userSize":null,"locked":{},"title":"","id":2}}',
          "tabview-ui": '{"pageBounds":{"left":0,"top":0,"width":788,"height":548}}'
        }, sizemode:"normal"
      }]
    };
    ss.setWindowState(newWin, JSON.stringify(newState), true);

    let firstTab = newWin.gBrowser.tabs[0];
    let secondTab = newWin.gBrowser.tabs[1];

    
    let browser = newWin.gBrowser.getBrowserForTab(firstTab);
    let onLoad = function() {
      browser.removeEventListener("load", onLoad, true);

      is(browser.currentURI.spec, DUMMY_PAGE_URL, 
         "The url of first tab url is dummy_page.html");

      
      ok(firstTab.hidden, "The first tab is hidden");
      ok(!secondTab.hidden, "The second tab is not hidden");
      is(secondTab, newWin.gBrowser.selectedTab, "The second tab is selected");

      
      
      let onTabHide = function() {
        newWin.gBrowser.tabContainer.addEventListener("TabHide", onTabHide, true);

        ok(newWin.TabView.getContentWindow(), "");

        ok(!firstTab.hidden, "The first tab is not hidden");
        is(firstTab, newWin.gBrowser.selectedTab, "The first tab is selected");
        ok(secondTab.hidden, "The second tab is hidden");

        
        newWin.close();

        finish();
      };
      newWin.gBrowser.tabContainer.addEventListener("TabHide", onTabHide, true);

      
      newWin.switchToTabHavingURI(DUMMY_PAGE_URL);
    }
    browser.addEventListener("load", onLoad, true);
  }, false);
}
