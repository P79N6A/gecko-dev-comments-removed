



































function test() {
  
  
  waitForExplicitFinish();
  
  let testURL = "http://localhost:8888/browser/" +
    "browser/components/sessionstore/test/browser/browser_461743_sample.html";
  
  let tab = gBrowser.addTab(testURL);
  info("New tab added");
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    info("New tab loaded");
    this.removeEventListener("load", arguments.callee, true);
    executeSoon(function() {
      let tab2 = gBrowser.duplicateTab(tab);
      info("Duplicated tab");
      tab2.linkedBrowser.addEventListener("461743", function(aEvent) {
        tab2.linkedBrowser.removeEventListener("461743", arguments.callee, true);
        is(aEvent.data, "done", "XSS injection was attempted");
        
        executeSoon(function() {
          let iframes = tab2.linkedBrowser.contentWindow.frames;
          let innerHTML = iframes[1].document.body.innerHTML;
          isnot(innerHTML, Components.utils.reportError.toString(),
                "chrome access denied!");
          
          
          gBrowser.removeTab(tab2);
          gBrowser.removeTab(tab);
          
          finish();
        });
      }, true, true);
    });
  }, true);
}
