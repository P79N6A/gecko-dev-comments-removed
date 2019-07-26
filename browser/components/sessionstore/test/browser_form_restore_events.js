


"use strict";

const URL = ROOT + "browser_form_restore_events_sample.html";





add_task(function () {
  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield setInputValue(browser, {id: "modify01", value: Math.random()});
  yield setInputValue(browser, {id: "modify02", value: Date.now()});

  
  yield setInputValue(browser, {id: "modify03", value: Math.random()});
  yield setInputValue(browser, {id: "modify04", value: Date.now()});

  
  let file = Services.dirsvc.get("TmpD", Ci.nsIFile);
  yield setInputValue(browser, {id: "modify05", value: file.path});

  
  yield setSelectedIndex(browser, {id: "modify06", index: 1});
  yield setMultipleSelected(browser, {id: "modify07", indices: [0,1,2]});

  
  yield setInputChecked(browser, {id: "modify08", checked: true});
  yield setInputChecked(browser, {id: "modify09", checked: false});

  
  yield setInputChecked(browser, {id: "modify10", checked: true});
  yield setInputChecked(browser, {id: "modify11", checked: true});

  
  
  let tab2 = gBrowser.duplicateTab(tab);
  let browser2 = tab2.linkedBrowser;
  yield promiseTabRestored(tab2);

  let inputFired = yield getTextContent(browser2, {id: "inputFired"});
  inputFired = inputFired.trim().split().sort().join(" ");

  let changeFired = yield getTextContent(browser2, {id: "changeFired"});
  changeFired = changeFired.trim().split().sort().join(" ");

  is(inputFired, "modify01 modify02 modify03 modify04 modify05",
     "input events were only dispatched for modified input, textarea fields");

  is(changeFired, "modify06 modify07 modify08 modify09 modify11",
     "change events were only dispatched for modified select, checkbox, radio fields");

  
  gBrowser.removeTab(tab2);
  gBrowser.removeTab(tab);
});
