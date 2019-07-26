












function test() {
  let prefix = 'http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/browser_privatebrowsing_concurrent_page.html';
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  let non_private_tab = gBrowser.selectedBrowser;
  non_private_tab.addEventListener('load', function() {
    non_private_tab.removeEventListener('load', arguments.callee, true);
    gBrowser.selectedTab = gBrowser.addTab();
    let private_tab = gBrowser.selectedBrowser;
    private_tab.docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing = true;
    private_tab.addEventListener('load', function() {
      private_tab.removeEventListener('load', arguments.callee, true);

      non_private_tab.addEventListener('load', function() {
        non_private_tab.removeEventListener('load', arguments.callee, true);
        var elts = non_private_tab.contentWindow.document.title.split('|');
        isnot(elts[0], 'value2', "public window shouldn't see private storage");
        is(elts[1], '1', "public window should only see public items");

        private_tab.addEventListener('load', function() {
          private_tab.removeEventListener('load', arguments.callee, true);
          var elts = private_tab.contentWindow.document.title.split('|');
          isnot(elts[0], 'value', "private window shouldn't see public storage");
          is(elts[1], '1', "private window should only see private items");
          private_tab.docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing = false;

          Components.utils.schedulePreciseGC(function() {
            private_tab.addEventListener('load', function() {
              private_tab.removeEventListener('load', arguments.callee, true);
              var elts = private_tab.contentWindow.document.title.split('|');
              isnot(elts[0], 'value2', "public window shouldn't see cleared private storage");
              is(elts[1], '1', "public window should only see public items");

              private_tab.docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing = true;
              private_tab.addEventListener('load', function() {
                private_tab.removeEventListener('load', arguments.callee, true);
                var elts = private_tab.contentWindow.document.title.split('|');
                is(elts[1], '1', "private window should only see new private items");

                non_private_tab.addEventListener('load', function() {
                  gBrowser.removeCurrentTab();
                  gBrowser.removeCurrentTab();
                  finish();
                }, true);
                non_private_tab.loadURI(prefix + '?final=true');

              }, true);
              private_tab.loadURI(prefix + '?action=set&name=test3&value=value3');
            }, true);
            private_tab.loadURI(prefix + '?action=get&name=test2');
          });
        }, true);
        private_tab.loadURI(prefix + '?action=get&name=test');
      }, true);
      non_private_tab.loadURI(prefix + '?action=get&name=test2');
    }, true);
    private_tab.loadURI(prefix + '?action=set&name=test2&value=value2');
  }, true);
  non_private_tab.loadURI(prefix + '?action=set&name=test&value=value&initial=true');
}
