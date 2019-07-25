





































function test() {
  waitForExplicitFinish();

  
  is(gBrowser.tabs.length, 1, "we start with one tab");
  
  
  let tab = gBrowser.loadOneTab("about:blank");
  ok(!tab.hidden, "tab starts out not hidden");
  is(gBrowser.tabs.length, 2, "we now have two tabs");

  
  tab.hidden = true; 
  ok(!tab.hidden, "can't set .hidden directly");

  
  gBrowser.hideTab(tab);
  ok(tab.hidden, "tab is hidden");
  
  
  gBrowser.pinTab(tab);
  ok(tab.pinned, "tab was pinned");
  ok(!tab.hidden, "tab was unhidden");
  
  
  gBrowser.hideTab(tab);
  ok(!tab.hidden, "tab did not hide");
    
  
  gBrowser.removeTab(tab);
  is(gBrowser.tabs.length, 1, "we finish with one tab");

  finish();
}
