function test() {
  
  
  
  try {
    var ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  }
  catch (ex) { }
  ok(ss, "SessionStore service is available");
  let tabbrowser = gBrowser;
  waitForExplicitFinish();
  
  
  
  
  let key = "Unique key: " + Date.now();
  let value = "Unique value: " + Math.random();
  let testURL = "about:config";
  
  
  let tab = tabbrowser.addTab(testURL);
  ss.setTabValue(tab, key, value);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);
    
    let state = ss.getTabState(tab);
    ok(state, "get the tab's state");
    
    
    state = eval("(" + state + ")");
    ok(state instanceof Object && state.entries instanceof Array && state.entries.length > 0,
       "state object seems valid");
    ok(state.entries.length == 1 && state.entries[0].url == testURL,
       "Got the expected state object (test URL)");
    ok(state.extData && state.extData[key] == value,
       "Got the expected state object (test manually set tab value)");
    
    
    tabbrowser.removeTab(tab);
  }, true);
  
  
  
  
  let key2 = "key2";
  let value2 = "Value " + Math.random();
  let value3 = "Another value: " + Date.now();
  let state = { entries: [{ url: testURL }], extData: { key2: value2 } };
  
  
  let tab2 = tabbrowser.addTab();
  
  ss.setTabState(tab2, state.toSource());
  tab2.linkedBrowser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);
    
    ok(ss.getTabValue(tab2, key2) == value2 && this.currentURI.spec == testURL,
       "the tab's state was correctly restored");
    
    
    let textbox = this.contentDocument.getElementById("textbox");
    textbox.wrappedJSObject.value = value3;
    
    
    let duplicateTab = ss.duplicateTab(window, tab2);
    tabbrowser.removeTab(tab2);
    
    duplicateTab.linkedBrowser.addEventListener("load", function(aEvent) {
      this.removeEventListener("load", arguments.callee, true);
      
      ok(ss.getTabValue(duplicateTab, key2) == value2 && this.currentURI.spec == testURL,
         "correctly duplicated the tab's state");
      let textbox = this.contentDocument.getElementById("textbox");
      is(textbox.wrappedJSObject.value, value3, "also duplicated text data");
      
      
      tabbrowser.removeTab(duplicateTab);
      finish();
    }, true);
  }, true);
}
