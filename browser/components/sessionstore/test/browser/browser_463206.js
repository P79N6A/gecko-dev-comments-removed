



































function test() {
  
  
  waitForExplicitFinish();
  
  let testURL = "http://localhost:8888/browser/" +
    "browser/components/sessionstore/test/browser/browser_463206_sample.html";
  
  var frameCount = 0;
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    
    if (frameCount++ < 5)
      return;
    this.removeEventListener("load", arguments.callee, true);
    
    function typeText(aTextField, aValue) {
      aTextField.value = aValue;
      
      let event = aTextField.ownerDocument.createEvent("UIEvents");
      event.initUIEvent("input", true, true, aTextField.ownerDocument.defaultView, 0);
      aTextField.dispatchEvent(event);
    }
    
    let doc = tab.linkedBrowser.contentDocument;
    typeText(doc.getElementById("out1"), Date.now());
    typeText(doc.getElementsByName("1|#out2")[0], Math.random());
    typeText(doc.defaultView.frames[0].frames[1].document.getElementById("in1"), new Date());
    
    frameCount = 0;
    let tab2 = gBrowser.duplicateTab(tab);
    tab2.linkedBrowser.addEventListener("load", function(aEvent) {
      
      if (frameCount++ < 5)
        return;
      this.removeEventListener("load", arguments.callee, true);

      let doc = tab2.linkedBrowser.contentDocument;
      let win = tab2.linkedBrowser.contentWindow;
      isnot(doc.getElementById("out1").value,
            win.frames[1].document.getElementById("out1").value,
            "text isn't reused for frames");
      isnot(doc.getElementsByName("1|#out2")[0].value, "",
            "text containing | and # is correctly restored");
      is(win.frames[1].document.getElementById("out2").value, "",
            "id prefixes can't be faked");
      isnot(win.frames[0].frames[1].document.getElementById("in1").value, "",
            "id prefixes aren't mixed up");
      is(win.frames[1].frames[0].document.getElementById("in1").value, "",
            "id prefixes aren't mixed up");
      
      
      gBrowser.removeTab(tab2);
      gBrowser.removeTab(tab);
      
      finish();
    }, true);
  }, true);
}
