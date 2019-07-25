function test()
{
  waitForExplicitFinish();

  var level1 = false;
  var level2 = false;
  function test1() {
    
    
    
    
    
    
    var chromeroot = getRootDirectory(gTestPath);
    var uri = chromeroot + "browser_tab_dragdrop2_frame1.xul";
    let window_B = openDialog(location, "_blank", "chrome,all,dialog=no,left=200,top=200,width=200,height=200", uri);
    window_B.addEventListener("load", function(aEvent) {
      window_B.removeEventListener("load", arguments.callee, false);
      if (level1) return; level1=true;
      executeSoon(function () {
        window_B.gBrowser.addEventListener("load", function(aEvent) {
          window_B.removeEventListener("load", arguments.callee, true);
          if (level2) return; level2=true;
          is(window_B.gBrowser.getBrowserForTab(window_B.gBrowser.tabs[0]).contentWindow.location, uri, "sanity check");
          
          var windowB_tab2 = window_B.gBrowser.addTab("about:blank", {skipAnimation: true});
          setTimeout(function () {
            
            window_B.gBrowser.addEventListener("pagehide", function(aEvent) {
              window_B.gBrowser.removeEventListener("pagehide", arguments.callee, true);
              executeSoon(function () {
                
                
                window_B.close();

                var doc = window_C.gBrowser.getBrowserForTab(window_C.gBrowser.tabs[0])
                            .docShell.contentViewer.DOMDocument;
                var calls = doc.defaultView.test_panels();
                window_C.close();
                finish();
              });
            }, true);
            window_B.gBrowser.selectedTab = window_B.gBrowser.tabs[0];
            var window_C = window_B.gBrowser.replaceTabWithWindow(window_B.gBrowser.tabs[0]);
            }, 1000);  
        }, true);
      });
    }, false);
  }

  test1();
}
