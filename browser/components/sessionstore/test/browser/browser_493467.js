



































function test() {
  
  
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
}
