



































function test() {
  function tabAdded(event) {
    let tab = event.target;
    tabs.push(tab);
  }

  let tabs = [];

  let container = gBrowser.tabContainer;
  container.addEventListener("TabOpen", tabAdded, false);

  gBrowser.addTab("about:blank");
  BrowserSearch.loadSearch("mozilla", true);
  BrowserSearch.loadSearch("firefox", true);
  
  is(tabs[0], gBrowser.mTabs[3], "blank tab has been pushed to the end");
  is(tabs[1], gBrowser.mTabs[1], "first search tab opens next to the current tab");
  is(tabs[2], gBrowser.mTabs[2], "second search tab opens next to the first search tab");

  container.removeEventListener("TabOpen", tabAdded, false);
  tabs.forEach(gBrowser.removeTab, gBrowser);
}
