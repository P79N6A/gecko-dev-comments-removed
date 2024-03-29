


"use strict";

const URL = getRootDirectory(gTestPath) + "browser_pageStyle_sample.html";
const URL_NESTED = getRootDirectory(gTestPath) + "browser_pageStyle_sample_nested.html";




add_task(function page_style() {
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);
  let sheets = yield getStyleSheets(browser);

  
  for (let [title, disabled] of sheets) {
    yield enableStyleSheetsForSet(browser, title);

    let tab2 = gBrowser.duplicateTab(tab);
    yield promiseTabRestored(tab2);

    let sheets = yield getStyleSheets(tab2.linkedBrowser);
    let enabled = sheets.filter(([title, disabled]) => !disabled);

    if (title.startsWith("fail_")) {
      ok(!enabled.length, "didn't restore " + title);
    } else {
      is(enabled.length, 1, "restored one style sheet");
      is(enabled[0][0], title, "restored correct sheet");
    }

    gBrowser.removeTab(tab2);
  }

  
  yield setAuthorStyleDisabled(browser, true);

  let tab2 = gBrowser.duplicateTab(tab);
  yield promiseTabRestored(tab2);

  let authorStyleDisabled = yield getAuthorStyleDisabled(tab2.linkedBrowser);
  ok(authorStyleDisabled, "disabled all stylesheets");

  
  gBrowser.removeTab(tab);
  gBrowser.removeTab(tab2);
});





add_task(function nested_page_style() {
  let tab = gBrowser.addTab(URL_NESTED);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  yield enableSubDocumentStyleSheetsForSet(browser, "alternate");
  yield promiseRemoveTab(tab);

  let [{state: {pageStyle}}] = JSON.parse(ss.getClosedTabData(window));
  let expected = JSON.stringify({children: [{pageStyle: "alternate"}]});
  is(JSON.stringify(pageStyle), expected, "correct pageStyle persisted");
});

function getStyleSheets(browser) {
  return sendMessage(browser, "ss-test:getStyleSheets");
}

function enableStyleSheetsForSet(browser, name) {
  return sendMessage(browser, "ss-test:enableStyleSheetsForSet", name);
}

function enableSubDocumentStyleSheetsForSet(browser, name) {
  return sendMessage(browser, "ss-test:enableSubDocumentStyleSheetsForSet", {
    id: "iframe", set: name
  });
}

function getAuthorStyleDisabled(browser) {
  return sendMessage(browser, "ss-test:getAuthorStyleDisabled");
}

function setAuthorStyleDisabled(browser, val) {
  return sendMessage(browser, "ss-test:setAuthorStyleDisabled", val)
}
