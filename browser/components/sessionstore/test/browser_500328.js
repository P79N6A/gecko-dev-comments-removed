



function checkState(tab) {
  
  
  
  
  
  

  let popStateCount = 0;

  tab.linkedBrowser.addEventListener('popstate', function(aEvent) {
    let contentWindow = tab.linkedBrowser.contentWindow;
    if (popStateCount == 0) {
      popStateCount++;

      is(tab.linkedBrowser.contentWindow.testState, 'foo',
         'testState after going back');

      ok(aEvent.state, "Event should have a state property.");
      is(JSON.stringify(tab.linkedBrowser.contentWindow.history.state), JSON.stringify({obj1:1}),
         "first popstate object.");

      
      let doc = contentWindow.document;
      ok(!doc.getElementById("new-elem"),
         "doc shouldn't contain new-elem before we add it.");
      let elem = doc.createElement("div");
      elem.id = "new-elem";
      doc.body.appendChild(elem);

      tab.linkedBrowser.goForward();
    }
    else if (popStateCount == 1) {
      popStateCount++;
      
      
      
      
      
      
      runInContent(tab.linkedBrowser, function(win, event) {
        return Cu.waiveXrays(event.state).obj3.toString();
      }, aEvent).then(function(stateStr) {
        is(stateStr, '/^a$/', "second popstate object.");

        
        
        
        let doc = contentWindow.document;
        let newElem = doc.getElementById("new-elem");
        ok(newElem, "doc should contain new-elem.");
        newElem.parentNode.removeChild(newElem);
        ok(!doc.getElementById("new-elem"), "new-elem should be removed.");

        tab.linkedBrowser.removeEventListener("popstate", arguments.callee, true);
        gBrowser.removeTab(tab);
        finish();
      });
    }
  });

  
  
  tab.linkedBrowser.contentWindow.testState = 'foo';

  
  tab.linkedBrowser.goBack();
}

function test() {
  
  

  waitForExplicitFinish();

  
  
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;

  whenBrowserLoaded(browser, function() {
    browser.loadURI("http://example.com", null, null);

    whenBrowserLoaded(browser, function() {
      
      
      
      
      
      function contentTest(win) {
        let history = win.history;
        history.pushState({obj1:1}, "title-obj1");
        history.pushState({obj2:2}, "title-obj2", "?page2");
        history.replaceState({obj3:/^a$/}, "title-obj3");
      }
      runInContent(browser, contentTest, null).then(function() {
        SyncHandlers.get(tab.linkedBrowser).flush();
        let state = ss.getTabState(tab);
        gBrowser.removeTab(tab);

        
        
        let tab2 = gBrowser.addTab("about:blank");
        ss.setTabState(tab2, state, true);

        
        whenTabRestored(tab2, function() {
          checkState(tab2);
        });
      });
    });
  });
}
