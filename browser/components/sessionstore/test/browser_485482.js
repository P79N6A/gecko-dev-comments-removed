



function test() {
  

  waitForExplicitFinish();

  let uniqueValue = Math.random();

  let rootDir = getRootDirectory(gTestPath);
  let testURL = rootDir + "browser_485482_sample.html";
  let tab = gBrowser.addTab(testURL);
  whenBrowserLoaded(tab.linkedBrowser, function() {
    let doc = tab.linkedBrowser.contentDocument;
    doc.querySelector("input[type=text]").value = uniqueValue;
    doc.querySelector("input[type=checkbox]").checked = true;

    let tab2 = gBrowser.duplicateTab(tab);
    whenTabRestored(tab2, function() {
      doc = tab2.linkedBrowser.contentDocument;
      is(doc.querySelector("input[type=text]").value, uniqueValue,
         "generated XPath expression was valid");
      ok(doc.querySelector("input[type=checkbox]").checked,
         "generated XPath expression was valid");

      
      gBrowser.removeTab(tab2);
      gBrowser.removeTab(tab);
      finish();
    });
  });
}
