


function test() {
  waitForExplicitFinish();

  let newTab = gBrowser.loadOneTab("http://www.example.com/#1",
                                   { inBackground: false });

  
  
  
  
  afterAllTabsLoaded(function() {
    executeSoon(function() {
      ok(!newTab.linkedBrowser.canGoBack, 
         "Browser should not be able to go back in history");

      newTab.linkedBrowser.loadURI("http://www.example.com/#2");

      afterAllTabsLoaded(function() {
        ok(newTab.linkedBrowser.canGoBack, 
           "Browser can go back in history after loading another URL");

        showTabView(function() {
          let contentWindow = document.getElementById("tab-view").contentWindow;

          EventUtils.synthesizeKey("VK_BACK_SPACE", { type: "keyup" }, contentWindow);

          
          executeSoon(function() {
            ok(newTab.linkedBrowser.canGoBack, 
               "Browser can still go back in history");

            hideTabView(function() {
              gBrowser.removeTab(newTab);
              finish();
            });
          });
        });
      });
    });
  });
}

