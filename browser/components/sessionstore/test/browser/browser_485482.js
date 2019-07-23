



































function test() {
  
  
  waitForExplicitFinish();
  
  let uniqueValue = Math.random();
  
  let testURL = "chrome://mochikit/content/browser/" +
    "browser/components/sessionstore/test/browser/browser_485482_sample.html";
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);
    let doc = tab.linkedBrowser.contentDocument;
    doc.querySelector("input[type=text]").value = uniqueValue;
    doc.querySelector("input[type=checkbox]").checked = true;
    
    let tab2 = gBrowser.duplicateTab(tab);
    tab2.linkedBrowser.addEventListener("load", function(aEvent) {
      this.removeEventListener("load", arguments.callee, true);
      doc = tab2.linkedBrowser.contentDocument;
      is(doc.querySelector("input[type=text]").value, uniqueValue,
         "generated XPath expression was valid");
      ok(doc.querySelector("input[type=checkbox]").checked,
         "generated XPath expression was valid");
      
      
      gBrowser.removeTab(tab2);
      gBrowser.removeTab(tab);
      finish();
    }, true);
  }, true);
}
