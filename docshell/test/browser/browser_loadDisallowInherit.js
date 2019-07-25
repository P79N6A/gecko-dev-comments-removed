


function test() {
  waitForExplicitFinish();

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  registerCleanupFunction(function () {
    gBrowser.removeTab(tab);
  });

  let browser = gBrowser.getBrowserForTab(tab);

  function loadURL(url, flags, func) {
    browser.addEventListener("load", function loadListener(e) {
      if (browser.currentURI.spec != url)
        return;
      browser.removeEventListener(e.type, loadListener, true);
      func();
    }, true);
    browser.loadURIWithFlags(url, flags, null, null, null);
  }

  
  loadURL("http://example.com/", 0, function () {
    let pagePrincipal = browser.contentPrincipal;

    
    loadURL("data:text/html,<body>inherit", 0, function () {
      let dataPrincipal = browser.contentPrincipal;
      ok(dataPrincipal.equals(pagePrincipal), "data URI should inherit principal");

      
      loadURL("http://example.com/", 0, function () {
        let innerPagePrincipal = browser.contentPrincipal;

        
        let webNav = Components.interfaces.nsIWebNavigation;
        loadURL("data:text/html,<body>noinherit", webNav.LOAD_FLAGS_DISALLOW_INHERIT_OWNER, function () {
          let innerDataPrincipal = browser.contentPrincipal;
          ok(!innerDataPrincipal.equals(innerPagePrincipal), "data URI should not inherit principal");

          finish();
        });
      });
    });
  });
}
