



































function test() {
  
  
  waitForExplicitFinish();
  
  let testURL = "http://localhost:8888/browser/" +
    "browser/components/sessionstore/test/browser/browser_339445_sample.html";
  
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);
    let doc = tab.linkedBrowser.contentDocument;
    is(doc.getElementById("storageTestItem").textContent, "PENDING",
       "sessionStorage value has been set");
    
    let tab2 = gBrowser.duplicateTab(tab);
    tab2.linkedBrowser.addEventListener("load", function(aEvent) {
      this.removeEventListener("load", arguments.callee, true);
      let doc2 = tab2.linkedBrowser.contentDocument;
      is(doc2.getElementById("storageTestItem").textContent, "SUCCESS",
         "sessionStorage value has been duplicated");
      
      
      gBrowser.removeTab(tab2);
      gBrowser.removeTab(tab);
      
      finish();
    }, true);
  }, true);
}
