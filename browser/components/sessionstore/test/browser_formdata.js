


"use strict";

requestLongerTimeout(2);





add_task(function test_formdata() {
  const URL = "http://mochi.test:8888/browser/browser/components/" +
              "sessionstore/test/browser_formdata_sample.html";

  const OUTER_VALUE = "browser_formdata_" + Math.random();
  const INNER_VALUE = "browser_formdata_" + Math.random();

  
  
  function createAndRemoveTab() {
    return Task.spawn(function () {
      
      let tab = gBrowser.addTab(URL);
      let browser = tab.linkedBrowser;
      yield promiseBrowserLoaded(browser);

      
      yield setInputValue(browser, {id: "txt", value: OUTER_VALUE});
      yield setInputValue(browser, {id: "txt", value: INNER_VALUE, frame: 0});

      
      yield promiseRemoveTab(tab);
    });
  }

  yield createAndRemoveTab();
  let [{state: {formdata}}] = JSON.parse(ss.getClosedTabData(window));
  is(formdata.id.txt, OUTER_VALUE, "outer value is correct");
  is(formdata.children[0].id.txt, INNER_VALUE, "inner value is correct");

  
  Services.prefs.setIntPref("browser.sessionstore.privacy_level", 1);

  yield createAndRemoveTab();
  [{state: {formdata}}] = JSON.parse(ss.getClosedTabData(window));
  is(formdata.id.txt, OUTER_VALUE, "outer value is correct");
  ok(!formdata.children, "inner value was *not* stored");

  
  Services.prefs.setIntPref("browser.sessionstore.privacy_level", 2);

  yield createAndRemoveTab();
  [{state: {formdata}}] = JSON.parse(ss.getClosedTabData(window));
  ok(!formdata, "form data has *not* been stored");

  
  Services.prefs.clearUserPref("browser.sessionstore.privacy_level");
});






add_task(function test_url_check() {
  const URL = "data:text/html;charset=utf-8,<input%20id=input>";
  const VALUE = "value-" + Math.random();

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  function restoreStateWithURL(url) {
    let state = {entries: [{url: URL}], formdata: {id: {input: VALUE}}};

    if (url) {
      state.formdata.url = url;
    }

    return promiseTabState(tab, state).then(() => getInputValue(browser, "input"));
  }

  
  is((yield restoreStateWithURL(URL)), VALUE, "form data restored");

  
  is((yield restoreStateWithURL(URL + "?")), "", "form data not restored");
  is((yield restoreStateWithURL()), "", "form data not restored");

  
  gBrowser.removeTab(tab);
});





add_task(function test_nested() {
  const URL = "data:text/html;charset=utf-8," +
              "<iframe src='data:text/html;charset=utf-8," +
              "<input autofocus=true>'/>";

  const FORM_DATA = {
    children: [{
      xpath: {"/xhtml:html/xhtml:body/xhtml:input": "M"},
      url: "data:text/html;charset=utf-8,<input%20autofocus=true>"
    }]
  };

  
  let tab = gBrowser.selectedTab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield sendMessage(browser, "ss-test:sendKeyEvent", {key: "m", frame: 0});

  
  yield promiseRemoveTab(tab);
  let [{state: {formdata}}] = JSON.parse(ss.getClosedTabData(window));
  is(JSON.stringify(formdata), JSON.stringify(FORM_DATA),
    "formdata for iframe stored correctly");

  
  tab = ss.undoCloseTab(window, 0);
  browser = tab.linkedBrowser;
  yield promiseTabRestored(tab);

  
  yield TabStateFlusher.flush(browser);
  ({formdata} = JSON.parse(ss.getTabState(tab)));
  is(JSON.stringify(formdata), JSON.stringify(FORM_DATA),
    "formdata for iframe restored correctly");

  
  gBrowser.removeTab(tab);
});





add_task(function test_design_mode() {
  const URL = "data:text/html;charset=utf-8,<h1>mozilla</h1>" +
              "<script>document.designMode='on'</script>";

  
  let tab = gBrowser.selectedTab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield sendMessage(browser, "ss-test:sendKeyEvent", {key: "m"});

  
  yield promiseRemoveTab(tab);
  tab = ss.undoCloseTab(window, 0);
  browser = tab.linkedBrowser;
  yield promiseTabRestored(tab);

  
  let html = yield getInnerHTML(browser);
  let expected = "<h1>Mmozilla</h1><script>document.designMode='on'</script>";
  is(html, expected, "editable document has been restored correctly");

  
  yield promiseRemoveTab(tab);
  tab = ss.undoCloseTab(window, 0);
  browser = tab.linkedBrowser;
  yield promiseTabRestored(tab);

  
  html = yield getInnerHTML(browser);
  expected = "<h1>Mmozilla</h1><script>document.designMode='on'</script>";
  is(html, expected, "editable document has been restored correctly");

  
  gBrowser.removeTab(tab);
});

function getInputValue(browser, id) {
  return sendMessage(browser, "ss-test:getInputValue", {id: id});
}

function setInputValue(browser, data) {
  return sendMessage(browser, "ss-test:setInputValue", data);
}

function getInnerHTML(browser) {
  return sendMessage(browser, "ss-test:getInnerHTML", {selector: "body"});
}
