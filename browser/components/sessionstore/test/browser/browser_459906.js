



































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
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    let iframes = tab.linkedBrowser.contentWindow.frames;
    iframes[1].document.body.innerHTML = uniqueValue;

    frameCount = 0;
    let tab2 = gBrowser.duplicateTab(tab);
    tab2.linkedBrowser.addEventListener("load", function(aEvent) {
      
      if (frameCount++ < 2)
        return;
      tab2.linkedBrowser.removeEventListener("load", arguments.callee, true);

      executeSoon(function() {
        let iframes = tab2.linkedBrowser.contentWindow.frames;
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
