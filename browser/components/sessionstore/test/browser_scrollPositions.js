


"use strict";

const URL = ROOT + "browser_scrollPositions_sample.html";
const URL_FRAMESET = ROOT + "browser_scrollPositions_sample_frameset.html";


const SCROLL_X = Math.round(100 * (1 + Math.random()));
const SCROLL_Y = Math.round(200 * (1 + Math.random()));
const SCROLL_STR = SCROLL_X + "," + SCROLL_Y;

const SCROLL2_X = Math.round(300 * (1 + Math.random()));
const SCROLL2_Y = Math.round(400 * (1 + Math.random()));
const SCROLL2_STR = SCROLL2_X + "," + SCROLL2_Y;





add_task(function test_scroll() {
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield sendMessage(browser, "ss-test:setScrollPosition", {x: SCROLL_X, y: SCROLL_Y});
  checkScroll(tab, {scroll: SCROLL_STR}, "scroll is fine");

  
  let tab2 = ss.duplicateTab(window, tab);
  let browser2 = tab2.linkedBrowser;
  yield promiseTabRestored(tab2);

  let scroll = yield sendMessage(browser2, "ss-test:getScrollPosition");
  is(JSON.stringify(scroll), JSON.stringify({x: SCROLL_X, y: SCROLL_Y}),
    "scroll position has been duplicated correctly");

  
  browser2.reload();
  yield promiseBrowserLoaded(browser2);
  checkScroll(tab2, {scroll: SCROLL_STR}, "reloading retains scroll positions");

  
  browser2.reloadWithFlags(Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE);
  yield promiseBrowserLoaded(browser2);
  checkScroll(tab2, null, "force-reload resets scroll positions");

  
  
  
  yield sendMessage(browser, "ss-test:setScrollPosition", {x: 0, y: 0});
  checkScroll(tab, null, "no scroll stored");

  
  gBrowser.removeTab(tab);
  gBrowser.removeTab(tab2);
});





add_task(function test_scroll_nested() {
  let tab = gBrowser.addTab(URL_FRAMESET);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield sendMessage(browser, "ss-test:setScrollPosition", {x: SCROLL_X, y: SCROLL_Y, frame: 0});
  checkScroll(tab, {children: [{scroll: SCROLL_STR}]}, "scroll is fine");

  
  yield sendMessage(browser, "ss-test:setScrollPosition", {x: SCROLL2_X, y: SCROLL2_Y, frame: 1});
  checkScroll(tab, {children: [{scroll: SCROLL_STR}, {scroll: SCROLL2_STR}]}, "scroll is fine");

  
  let tab2 = ss.duplicateTab(window, tab);
  let browser2 = tab2.linkedBrowser;
  yield promiseTabRestored(tab2);

  let scroll = yield sendMessage(browser2, "ss-test:getScrollPosition", {frame: 0});
  is(JSON.stringify(scroll), JSON.stringify({x: SCROLL_X, y: SCROLL_Y}),
    "scroll position #1 has been duplicated correctly");

  scroll = yield sendMessage(browser2, "ss-test:getScrollPosition", {frame: 1});
  is(JSON.stringify(scroll), JSON.stringify({x: SCROLL2_X, y: SCROLL2_Y}),
    "scroll position #2 has been duplicated correctly");

  
  
  yield sendMessage(browser, "ss-test:setScrollPosition", {x: 0, y: 0, frame: 0});
  checkScroll(tab, {children: [null, {scroll: SCROLL2_STR}]}, "scroll is fine");

  
  yield sendMessage(browser, "ss-test:setScrollPosition", {x: 0, y: 0, frame: 1});
  checkScroll(tab, null, "no scroll stored");

  
  gBrowser.removeTab(tab);
  gBrowser.removeTab(tab2);
});





add_task(function test_scroll_old_format() {
  const TAB_STATE = { entries: [{url: URL, scroll: SCROLL_STR}] };

  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  ss.setTabState(tab, JSON.stringify(TAB_STATE));
  yield promiseTabRestored(tab);

  
  let scroll = yield sendMessage(browser, "ss-test:getScrollPosition");
  is(JSON.stringify(scroll), JSON.stringify({x: SCROLL_X, y: SCROLL_Y}),
    "scroll position has been restored correctly");

  
  gBrowser.removeTab(tab);
});

function checkScroll(tab, expected, msg) {
  let browser = tab.linkedBrowser;
  TabState.flush(browser);

  let scroll = JSON.parse(ss.getTabState(tab)).scroll || null;
  is(JSON.stringify(scroll), JSON.stringify(expected), msg);
}
