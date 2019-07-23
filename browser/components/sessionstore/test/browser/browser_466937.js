



































function test() {
  
  
  waitForExplicitFinish();
  
  let testURL = "http://localhost:8888/browser/" +
    "browser/components/sessionstore/test/browser/browser_466937_sample.html";
  let testPath = "/home/user/regular.file";
  
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);
    let doc = tab.linkedBrowser.contentDocument;
    doc.getElementById("reverse_thief").value = "/home/user/secret2";
    doc.getElementById("bystander").value = testPath;
    
    let tab2 = gBrowser.duplicateTab(tab);
    tab2.linkedBrowser.addEventListener("load", function(aEvent) {
      this.removeEventListener("load", arguments.callee, true);
      doc = tab2.linkedBrowser.contentDocument;
      is(doc.getElementById("thief").value, "",
         "file path wasn't set to text field value");
      is(doc.getElementById("reverse_thief").value, "",
         "text field value wasn't set to full file path");
      is(doc.getElementById("bystander").value, testPath,
         "normal case: file path was correctly preserved");
      
      
      gBrowser.removeTab(tab2);
      gBrowser.removeTab(tab);
      
      finish();
    }, true);
  }, true);
}
