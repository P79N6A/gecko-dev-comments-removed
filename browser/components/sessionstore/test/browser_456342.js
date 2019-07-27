


"use strict";

const URL = ROOT + "browser_456342_sample.xhtml";




add_task(function test_restore_nonstandard_input_values() {
  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  let expectedValue = Math.random();
  yield setFormElementValues(browser, {value: expectedValue});

  
  yield promiseRemoveTab(tab);
  let undoItems = JSON.parse(ss.getClosedTabData(window));
  let savedFormData = undoItems[0].state.formdata;

  let countGood = 0, countBad = 0;
  for (let id of Object.keys(savedFormData.id)) {
    if (savedFormData.id[id] == expectedValue) {
      countGood++;
    } else {
      countBad++;
    }
  }

  for (let exp of Object.keys(savedFormData.xpath)) {
    if (savedFormData.xpath[exp] == expectedValue) {
      countGood++;
    } else {
      countBad++;
    }
  }

  is(countGood, 4, "Saved text for non-standard input fields");
  is(countBad,  0, "Didn't save text for ignored field types");
});

function setFormElementValues(browser, data) {
  return sendMessage(browser, "ss-test:setFormElementValues", data);
}
