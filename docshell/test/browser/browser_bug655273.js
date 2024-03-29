









function test() {
  waitForExplicitFinish();

  let tab = gBrowser.addTab('http://example.com');
  let tabBrowser = tab.linkedBrowser;

  tabBrowser.addEventListener('load', function(aEvent) {
    tabBrowser.removeEventListener('load', arguments.callee, true);

    let cw = tabBrowser.contentWindow;
    let oldTitle = cw.document.title;
    ok(oldTitle, 'Content window should initially have a title.');
    cw.history.pushState('', '', 'new_page');

    let shistory = cw.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIWebNavigation)
                     .sessionHistory;

    is(shistory.getEntryAtIndex(shistory.index, false).title,
       oldTitle, 'SHEntry title after pushstate.');

    gBrowser.removeTab(tab);
    finish();
  }, true);
}
