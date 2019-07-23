



































function test() {
  
  
  waitForExplicitFinish();
  
  let testURL = "http://localhost:8888/browser/" +
    "browser/components/sessionstore/test/browser/browser_459906_sample.html";
  let uniqueValue = "<b>Unique:</b> " + Date.now();
  
  var frameCount = 0;
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    
    if (frameCount++ < 2)
      return;
    this.removeEventListener("load", arguments.callee, true);
    
    let iframes = tab.linkedBrowser.contentWindow.frames;
    iframes[1].document.body.innerHTML = uniqueValue;
    
    frameCount = 0;
    let tab2 = gBrowser.duplicateTab(tab);
    tab2.linkedBrowser.addEventListener("load", function(aEvent) {
      
      if (frameCount++ < 2)
        return;
      this.removeEventListener("load", arguments.callee, true);

      let pass = 0;
      const MAX_PASS = 6;
      executeSoon(function() {
        info("Checking innerHTML, pass: " + (pass + 1));
        let iframes = tab2.linkedBrowser.contentWindow.frames;
        if (iframes[1].document.body.innerHTML != uniqueValue &&
            ++pass <= MAX_PASS) {
          SetTimeout(500, arguments.callee);
          return;
        }
        is(iframes[1].document.body.innerHTML, uniqueValue,
           "rich textarea's content correctly duplicated");
        
        let innerDomain = null;
        try {
          innerDomain = iframes[0].document.domain;
        }
        catch (ex) {  }
        is(innerDomain, "localhost", "XSS exploit prevented!");
        
        
        gBrowser.removeTab(tab2);
        gBrowser.removeTab(tab);
        
        finish();
      });
    }, true);
  }, true);
}
