



function test() {
  

  waitForExplicitFinish();

  let testURL = "http://mochi.test:8888/browser/" +
    "browser/components/sessionstore/test/browser_461743_sample.html";

  let frameCount = 0;
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    
    if (frameCount++ < 2)
      return;
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
    let tab2 = gBrowser.duplicateTab(tab);
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
  }, true);
}
