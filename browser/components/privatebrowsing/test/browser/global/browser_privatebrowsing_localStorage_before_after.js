











function test() {
  let prefix = 'http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/global/';
  waitForExplicitFinish();

  
  
  Components.utils.schedulePreciseGC(function() {
    let tab = gBrowser.selectedTab = gBrowser.addTab();
    let browser = gBrowser.selectedBrowser;
    browser.docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing = true;
    browser.addEventListener('load', function() {
      browser.removeEventListener('load', arguments.callee, true);
      is(browser.contentWindow.document.title, '1', "localStorage should contain 1 item");
      browser.docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing = false;

      gBrowser.selectedTab = gBrowser.addTab();
      let browser2 = gBrowser.selectedBrowser;
      gBrowser.removeTab(tab);
      browser2.addEventListener('load', function() {
        browser2.removeEventListener('load', arguments.callee, true);
        is(browser2.contentWindow.document.title, 'null|0', 'localStorage should contain 0 items');
        gBrowser.removeCurrentTab();
        finish();
      }, true);
      browser2.loadURI(prefix + 'browser_privatebrowsing_localStorage_before_after_page2.html');
    }, true);
    browser.loadURI(prefix + 'browser_privatebrowsing_localStorage_before_after_page.html');
  });
}
