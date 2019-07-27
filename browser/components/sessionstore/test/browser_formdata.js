


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

      
      gBrowser.removeTab(tab);
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





add_task(function test_old_format() {
  const URL = "data:text/html;charset=utf-8,<input%20id=input>";
  const VALUE = "value-" + Math.random();

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  let state = {entries: [{url: URL, formdata: {id: {input: VALUE}}}]};
  ss.setTabState(tab, JSON.stringify(state));
  yield promiseTabRestored(tab);
  is((yield getInputValue(browser, "input")), VALUE, "form data restored");

  
  gBrowser.removeTab(tab);
});





add_task(function test_old_format_inner_html() {
  const URL = "data:text/html;charset=utf-8,<h1>mozilla</h1>" +
              "<script>document.designMode='on'</script>";
  const VALUE = "<h1>value-" + Math.random() + "</h1>";

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  let state = {entries: [{url: URL, innerHTML: VALUE}]};
  ss.setTabState(tab, JSON.stringify(state));
  yield promiseTabRestored(tab);

  
  let html = yield getInnerHTML(browser);
  is(html, VALUE, "editable document has been restored correctly");

  
  gBrowser.removeTab(tab);
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

    ss.setTabState(tab, JSON.stringify(state));
    return promiseTabRestored(tab).then(() => getInputValue(browser, "input"));
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

  
  gBrowser.removeTab(tab);
  let [{state: {formdata}}] = JSON.parse(ss.getClosedTabData(window));
  is(JSON.stringify(formdata), JSON.stringify(FORM_DATA),
    "formdata for iframe stored correctly");

  
  tab = ss.undoCloseTab(window, 0);
  browser = tab.linkedBrowser;
  yield promiseTabRestored(tab);

  
  TabState.flush(browser);
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

  
  gBrowser.removeTab(tab);
  tab = ss.undoCloseTab(window, 0);
  browser = tab.linkedBrowser;
  yield promiseTabRestored(tab);

  
  let html = yield getInnerHTML(browser);
  let expected = "<h1>Mmozilla</h1><script>document.designMode='on'</script>";
  is(html, expected, "editable document has been restored correctly");

  
  gBrowser.removeTab(tab);
  tab = ss.undoCloseTab(window, 0);
  browser = tab.linkedBrowser;
  yield promiseTabRestored(tab);

  
  html = yield getInnerHTML(browser);
  expected = "<h1>Mmozilla</h1><script>document.designMode='on'</script>";
  is(html, expected, "editable document has been restored correctly");

  
  gBrowser.removeTab(tab);
});






add_task(function test_ccNumbers() {
  const validCCNumbers = [
    
    "930771457288760", "474915027480942",
    "924894781317325", "714816113937185",
    "790466087343106", "474320195408363",
    "219211148122351", "633038472250799",
    "354236732906484", "095347810189325",
    
    "3091269135815020", "5471839082338112",
    "0580828863575793", "5015290610002932",
    "9465714503078607", "4302068493801686",
    "2721398408985465", "6160334316984331",
    "8643619970075142", "0218246069710785"
  ];

  const invalidCCNumbers = [
    
    "526931005800649", "724952425140686",
    "379761391174135", "030551436468583",
    "947377014076746", "254848023655752",
    "226871580283345", "708025346034339",
    "917585839076788", "918632588027666",
    
    "9946177098017064", "4081194386488872",
    "3095975979578034", "3662215692222536",
    "6723210018630429", "4411962856225025",
    "8276996369036686", "4449796938248871",
    "3350852696538147", "5011802870046957"
  ];

  const URL = "http://mochi.test:8888/browser/browser/components/" +
              "sessionstore/test/browser_formdata_sample.html";

  
  
  function* createAndRemoveTab(formValue) {
    
    let tab = gBrowser.addTab(URL);
    let browser = tab.linkedBrowser;
    yield promiseBrowserLoaded(browser);

    
    yield setInputValue(browser, {id: "txt", value: formValue});

    
    gBrowser.removeTab(tab);
  }

  
  for (let number of validCCNumbers) {
    yield createAndRemoveTab(number);
    let [{state}] = JSON.parse(ss.getClosedTabData(window));
    ok(!("formdata" in state), "valid CC numbers are not collected");
  }

  
  for (let number of invalidCCNumbers) {
    yield createAndRemoveTab(number);
    let [{state: {formdata}}] = JSON.parse(ss.getClosedTabData(window));
    is(formdata.id.txt, number,
       "numbers that are not valid CC numbers are still collected");
  }
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
