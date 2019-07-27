


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

  
  function testURL(url, func) {
    loadURL("http://example.com/", 0, function () {
      let pagePrincipal = browser.contentPrincipal;
      ok(pagePrincipal, "got principal for http:// page");

      
      loadURL(url, 0, function () {
        ok(browser.contentPrincipal.equals(pagePrincipal), url + " should inherit principal");

        
        let webNav = Components.interfaces.nsIWebNavigation;
        loadURL(url, webNav.LOAD_FLAGS_DISALLOW_INHERIT_OWNER, function () {
          let newPrincipal = browser.contentPrincipal;
          ok(newPrincipal, "got inner principal");
          ok(!newPrincipal.equals(pagePrincipal),
             url + " should not inherit principal when loaded with DISALLOW_INHERIT_OWNER");
  
          func();
        });
      });
    });
  }

  let urls = [
    "data:text/html,<body>hi",
    "javascript:1;"
  ];

  function nextTest() {
    let url = urls.shift();
    if (url)
      testURL(url, nextTest);
    else
      finish();
  }
  
  nextTest();
}

