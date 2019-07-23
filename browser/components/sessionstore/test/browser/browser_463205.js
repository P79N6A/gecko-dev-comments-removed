



































function test() {
  
  
  waitForExplicitFinish();
  
  let testURL = "chrome://mochikit/content/browser/" +
    "browser/components/sessionstore/test/browser/browser_463205_sample.html";
  
  var frameCount = 0;
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    
    if (frameCount++ < 3)
      return;
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
    
    function typeText(aTextField, aValue) {
      aTextField.value = aValue;
      
      let event = aTextField.ownerDocument.createEvent("UIEvents");
      event.initUIEvent("input", true, true, aTextField.ownerDocument.defaultView, 0);
      aTextField.dispatchEvent(event);
    }
    
    let uniqueValue = "Unique: " + Math.random();
    let win = tab.linkedBrowser.contentWindow;
    typeText(win.frames[0].document.getElementById("original"), uniqueValue);
    typeText(win.frames[1].document.getElementById("original"), uniqueValue);
    
    frameCount = 0;
    let tab2 = gBrowser.duplicateTab(tab);
    tab2.linkedBrowser.addEventListener("load", function(aEvent) {
      
      if (frameCount++ < 4)
        return;
      tab2.linkedBrowser.removeEventListener("load", arguments.callee, true);

      let win = tab2.linkedBrowser.contentWindow;
      isnot(win.frames[0].document.getElementById("original").value, uniqueValue,
            "subframes must match URL to get text restored");
      is(win.frames[0].document.getElementById("original").value, "preserve me",
         "subframes must match URL to get text restored");
      is(win.frames[1].document.getElementById("original").value, uniqueValue,
         "text still gets restored for all other subframes");
      
      
      gBrowser.removeTab(tab2);
      gBrowser.removeTab(tab);
      
      finish();
    }, true);
  }, true);
}
