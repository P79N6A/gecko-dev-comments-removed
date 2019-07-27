




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");


let gTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

function sleep(wait) {
  return new Promise((resolve, reject) => {
    do_print("sleep start");
    gTimer.initWithCallback({
      notify: function () {
        do_print("sleep end");
        resolve();
      },
    }, wait, gTimer.TYPE_ONE_SHOT);
  });
}

function promiseBrowserLoaded(browser, eventType="load") {
  return new Promise((resolve, reject) => {
    do_print("Wait browser event: " + eventType);

    function handle(event) {
      if (event.target != browser.contentDocument) {
        do_print("Skipping spurious '" + eventType + "' event" + " for " + event.target.location.href);
        return;
      }

      browser.removeEventListener(eventType, handle, true);
      do_print("Browser event received: " + eventType);
      resolve(event);
    }

    browser.addEventListener(eventType, handle, true);
  });
}

function queryElement(contentWindow, data) {
  let frame = contentWindow;
  if (data.hasOwnProperty("frame")) {
    frame = contentWindow.frames[data.frame];
  }

  let doc = frame.document;

  if (data.hasOwnProperty("id")) {
    return doc.getElementById(data.id);
  }

  if (data.hasOwnProperty("selector")) {
    return doc.querySelector(data.selector);
  }

  if (data.hasOwnProperty("xpath")) {
    let xptype = Ci.nsIDOMXPathResult.FIRST_ORDERED_NODE_TYPE;
    return doc.evaluate(data.xpath, doc, null, xptype, null).singleNodeValue;
  }

  throw new Error("couldn't query element");
}

function dispatchUIEvent(input, type) {
  let event = input.ownerDocument.createEvent("UIEvents");
  event.initUIEvent(type, true, true, input.ownerDocument.defaultView, 0);
  input.dispatchEvent(event);
}

function setInputValue(browser, data) {
  let input = queryElement(browser.contentWindow, data);
  input.value = data.value;
  dispatchUIEvent(input, "input");
}

function getInputValue(browser, data) {
  let input = queryElement(browser.contentWindow, data);
  return input.value;
}

let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);

let gChromeWin;
let gBrowserApp;



const CLOSE_TAB_WAIT = 2000;

add_test(function setup_browser() {
  gChromeWin = Services.wm.getMostRecentWindow("navigator:browser");
  gBrowserApp = gChromeWin.BrowserApp;

  Services.tm.mainThread.dispatch(run_next_test, Ci.nsIThread.DISPATCH_NORMAL);
});





add_task(function test_formdata() {
  const URL = "http://example.org/tests/robocop/session_formdata_sample.html";

  const OUTER_VALUE = "browser_formdata_" + Math.random();
  const INNER_VALUE = "browser_formdata_" + Math.random();

  
  
  function createAndRemoveTab() {
    return Task.spawn(function () {
      
      let tab = gBrowserApp.addTab(URL);
      let browser = tab.browser;
      yield promiseBrowserLoaded(browser);

      
      setInputValue(browser, {id: "txt", value: OUTER_VALUE});
      setInputValue(browser, {id: "txt", value: INNER_VALUE, frame: 0});

      
      gBrowserApp.closeTab(tab);
      yield sleep(CLOSE_TAB_WAIT);
    });
  }

  yield createAndRemoveTab();
  let state = ss.getClosedTabs(gChromeWin);
  let [{formdata}] = state;
  is(formdata.id.txt, OUTER_VALUE, "outer value is correct");
  is(formdata.children[0].id.txt, INNER_VALUE, "inner value is correct");

  
  Services.prefs.setIntPref("browser.sessionstore.privacy_level", 1);

  yield createAndRemoveTab();
  state = ss.getClosedTabs(gChromeWin);
  [{formdata}] = state;
  is(formdata.id.txt, OUTER_VALUE, "outer value is correct");
  ok(!formdata.children, "inner value was *not* stored");

  
  Services.prefs.setIntPref("browser.sessionstore.privacy_level", 2);

  yield createAndRemoveTab();
  state = ss.getClosedTabs(gChromeWin);
  [{formdata}] = state;
  ok(!formdata, "form data has *not* been stored");

  
  Services.prefs.clearUserPref("browser.sessionstore.privacy_level");
});




add_task(function test_formdata() {
  const URL = "http://example.org/tests/robocop/session_formdata_sample.html";

  const OUTER_VALUE = "browser_formdata_" + Math.random();
  const INNER_VALUE = "browser_formdata_" + Math.random();

  
  
  function createAndRemoveTab() {
    return Task.spawn(function () {
      
      let tab = gBrowserApp.addTab(URL);
      let browser = tab.browser;
      yield promiseBrowserLoaded(browser);

      
      setInputValue(browser, {id: "txt", value: OUTER_VALUE});
      setInputValue(browser, {id: "txt", value: INNER_VALUE, frame: 0});

      
      gBrowserApp.closeTab(tab);
      yield sleep(CLOSE_TAB_WAIT);
    });
  }

  yield createAndRemoveTab();
  let state = ss.getClosedTabs(gChromeWin);
  let [{formdata}] = state;
  is(formdata.id.txt, OUTER_VALUE, "outer value is correct");
  is(formdata.children[0].id.txt, INNER_VALUE, "inner value is correct");

  
  let closedTabData = ss.getClosedTabs(gChromeWin)[0];
  let browser = ss.undoCloseTab(gChromeWin, closedTabData);
  yield promiseBrowserLoaded(browser);

  
  is(getInputValue(browser, {id: "txt"}), OUTER_VALUE, "outer value restored correctly");
  is(getInputValue(browser, {id: "txt", frame: 0}), INNER_VALUE, "inner value restored correctly");

  
  gBrowserApp.closeTab(gBrowserApp.getTabForBrowser(browser));
});

run_next_test();
