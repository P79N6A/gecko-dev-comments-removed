



































function checkState(tab) {
  
  
  
  
  
  

  let popStateCount = 0;

  tab.linkedBrowser.addEventListener("popstate", function(aEvent) {
    let contentWindow = tab.linkedBrowser.contentWindow;
    if (popStateCount == 0) {
      popStateCount++;
      ok(aEvent.state, "Event should have a state property.");
      is(JSON.stringify(aEvent.state), JSON.stringify({obj1:1}),
         "first popstate object.");

      
      let doc = contentWindow.document;
      ok(!doc.getElementById("new-elem"),
         "doc shouldn't contain new-elem before we add it.");
      let elem = doc.createElement("div");
      elem.id = "new-elem";
      doc.body.appendChild(elem);

      contentWindow.history.forward();
    }
    else if (popStateCount == 1) {
      popStateCount++;
      is(JSON.stringify(aEvent.state), JSON.stringify({obj3:3}),
         "second popstate object.");

      
      
      
      let doc = contentWindow.document;
      let newElem = doc.getElementById("new-elem");
      ok(newElem, "doc should contain new-elem.");
      newElem.parentNode.removeChild(newElem);
      ok(!doc.getElementById("new-elem"), "new-elem should be removed.");

      
      tab.linkedBrowser.removeEventListener("popstate", arguments.callee, false);
      gBrowser.removeTab(tab);
      finish();
    }
  }, true);

  tab.linkedBrowser.contentWindow.history.back();
}

function test() {
  
  

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  waitForExplicitFinish();

  
  
  
  let tab = gBrowser.addTab("about:blank");
  let tabBrowser = tab.linkedBrowser;

  tabBrowser.addEventListener("load", function(aEvent) {
    tabBrowser.removeEventListener("load", arguments.callee, true);

    tabBrowser.loadURI("http://example.com", null, null);

    tabBrowser.addEventListener("load", function(aEvent) {
      tabBrowser.removeEventListener("load", arguments.callee, true);

      
      
      
      
      
      let contentWindow = tab.linkedBrowser.contentWindow;
      let history = contentWindow.history;
      history.pushState({obj1:1}, "title-obj1");
      history.pushState({obj2:2}, "title-obj2", "page2");
      history.replaceState({obj3:3}, "title-obj3");

      let state = ss.getTabState(tab);

      
      
      
      history.replaceState({should_be_overwritten:true}, "title-overwritten");

      
      
      ss.setTabState(tab, state, true);
      executeSoon(function() { checkState(tab); });

    }, true);
  }, true);
}
