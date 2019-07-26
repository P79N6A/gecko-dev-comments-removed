



function test() {
  

  waitForExplicitFinish();

  let testURL = "http://mochi.test:8888/browser/" +
    "browser/components/sessionstore/test/browser_339445_sample.html";

  let tab = gBrowser.addTab(testURL);
  whenBrowserLoaded(tab.linkedBrowser, function() {
    let doc = tab.linkedBrowser.contentDocument;
    is(doc.getElementById("storageTestItem").textContent, "PENDING",
       "sessionStorage value has been set");

    let tab2 = gBrowser.duplicateTab(tab);
    whenTabRestored(tab2, function() {
      let doc2 = tab2.linkedBrowser.contentDocument;
      is(doc2.getElementById("storageTestItem").textContent, "SUCCESS",
         "sessionStorage value has been duplicated");

      
      gBrowser.removeTab(tab2);
      gBrowser.removeTab(tab);

      finish();
    });
  });
}
