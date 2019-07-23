



































function test() {
  
  
  waitForExplicitFinish();
  
  let testURL = "http://localhost:8888/browser/" +
    "browser/components/sessionstore/test/browser/browser_464620_b.html";
  
  var frameCount = 0;
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    
    if (frameCount++ < 6)
      return;
    this.removeEventListener("load", arguments.callee, true);
    
    executeSoon(function() {
      frameCount = 0;
      let tab2 = gBrowser.duplicateTab(tab);
      tab2.linkedBrowser.addEventListener("464620_b", function(aEvent) {
        is(aEvent.data, "done", "XSS injection was attempted");
        
        
        
        executeSoon(function() {
          setTimeout(function() {
            let win = tab2.linkedBrowser.contentWindow;
            isnot(win.frames[1].document.location, testURL,
                  "cross domain document was loaded");
            ok(!/XXX/.test(win.frames[1].document.body.innerHTML),
               "no content was injected");
            
            
            gBrowser.removeTab(tab2);
            gBrowser.removeTab(tab);
            
            finish();
          }, 0);
        });
      }, true, true);
    });
  }, true);
}
