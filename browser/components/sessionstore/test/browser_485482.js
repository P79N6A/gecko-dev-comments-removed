


"use strict";

const URL = ROOT + "browser_485482_sample.html";





add_task(function test_xpath_exp_for_strange_documents() {
  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  let uniqueValue = Math.random();
  yield setInputValue(browser, {selector: "input[type=text]", value: uniqueValue});
  yield setInputChecked(browser, {selector: "input[type=checkbox]", checked: true});

  
  let tab2 = gBrowser.duplicateTab(tab);
  let browser2 = tab2.linkedBrowser;
  yield promiseTabRestored(tab2);

  
  let text = yield getInputValue(browser2, {selector: "input[type=text]"});
  is(text, uniqueValue, "generated XPath expression was valid");
  let checkbox = yield getInputChecked(browser2, {selector: "input[type=checkbox]"});
  ok(checkbox, "generated XPath expression was valid");

  
  gBrowser.removeTab(tab2);
  gBrowser.removeTab(tab);
});
