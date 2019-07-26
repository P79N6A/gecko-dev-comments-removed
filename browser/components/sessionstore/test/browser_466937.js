


"use strict";

const URL = ROOT + "browser_466937_sample.html";




add_task(function test_prevent_file_stealing() {
  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  let file = Services.dirsvc.get("TmpD", Ci.nsIFile);
  file.append("466937_test.file");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("666", 8));
  let testPath = file.path;

  
  yield setInputValue(browser, {id: "reverse_thief", value: "/home/user/secret2"});
  yield setInputValue(browser, {id: "bystander", value: testPath});

  
  let tab2 = gBrowser.duplicateTab(tab);
  let browser2 = tab2.linkedBrowser;
  yield promiseTabRestored(tab2);

  let thief = yield getInputValue(browser2, {id: "thief"});
  is(thief, "", "file path wasn't set to text field value");
  let reverse_thief = yield getInputValue(browser2, {id: "reverse_thief"});
  is(reverse_thief, "", "text field value wasn't set to full file path");
  let bystander = yield getInputValue(browser2, {id: "bystander"});
  is(bystander, testPath, "normal case: file path was correctly preserved");

  
  gBrowser.removeTab(tab);
  gBrowser.removeTab(tab2);
});
