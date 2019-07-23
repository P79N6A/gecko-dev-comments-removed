



































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
  
  let tab = gBrowser.addTab();
  tab.linkedBrowser.stop();
  let tabState = JSON.parse(ss.getTabState(tab));
  is(tabState.disallow || "", "", "Everything is allowed per default");
  
  
  
  
  let permissions = [];
  let docShell = tab.linkedBrowser.docShell;
  for (let attribute in docShell) {
    if (/^allow([A-Z].*)/.test(attribute)) {
      permissions.push(RegExp.$1);
      docShell[attribute] = false;
    }
  }
  
  
  tabState = JSON.parse(ss.getTabState(tab));
  let disallow = tabState.disallow.split(",");
  permissions.forEach(function(aName) {
    ok(disallow.indexOf(aName) > -1, "Saved state of allow" + aName);
  });
  
  
  
  gBrowser.removeTab(tab);
  is(browserWindowsCount(), 1, "Only one browser window should be open eventually");
}
