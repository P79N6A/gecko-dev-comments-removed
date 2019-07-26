

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
let { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
let { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
let { gDevTools } = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let TargetFactory = devtools.TargetFactory;
let Toolbox = devtools.Toolbox;

const EXAMPLE_URL = "http://example.com/browser/browser/devtools/netmonitor/test/";

const SIMPLE_URL = EXAMPLE_URL + "html_simple-test-page.html";
const NAVIGATE_URL = EXAMPLE_URL + "html_navigate-test-page.html";
const CONTENT_TYPE_URL = EXAMPLE_URL + "html_content-type-test-page.html";
const CONTENT_TYPE_WITHOUT_CACHE_URL = EXAMPLE_URL + "html_content-type-without-cache-test-page.html";
const CYRILLIC_URL = EXAMPLE_URL + "html_cyrillic-test-page.html";
const STATUS_CODES_URL = EXAMPLE_URL + "html_status-codes-test-page.html";
const POST_DATA_URL = EXAMPLE_URL + "html_post-data-test-page.html";
const POST_RAW_URL = EXAMPLE_URL + "html_post-raw-test-page.html";
const POST_RAW_WITH_HEADERS_URL = EXAMPLE_URL + "html_post-raw-with-headers-test-page.html";
const PARAMS_URL = EXAMPLE_URL + "html_params-test-page.html";
const JSONP_URL = EXAMPLE_URL + "html_jsonp-test-page.html";
const JSON_LONG_URL = EXAMPLE_URL + "html_json-long-test-page.html";
const JSON_MALFORMED_URL = EXAMPLE_URL + "html_json-malformed-test-page.html";
const JSON_CUSTOM_MIME_URL = EXAMPLE_URL + "html_json-custom-mime-test-page.html";
const JSON_TEXT_MIME_URL = EXAMPLE_URL + "html_json-text-mime-test-page.html";
const SORTING_URL = EXAMPLE_URL + "html_sorting-test-page.html";
const FILTERING_URL = EXAMPLE_URL + "html_filter-test-page.html";
const INFINITE_GET_URL = EXAMPLE_URL + "html_infinite-get-page.html";
const CUSTOM_GET_URL = EXAMPLE_URL + "html_custom-get-page.html";
const STATISTICS_URL = EXAMPLE_URL + "html_statistics-test-page.html";

const SIMPLE_SJS = EXAMPLE_URL + "sjs_simple-test-server.sjs";
const CONTENT_TYPE_SJS = EXAMPLE_URL + "sjs_content-type-test-server.sjs";
const STATUS_CODES_SJS = EXAMPLE_URL + "sjs_status-codes-test-server.sjs";
const SORTING_SJS = EXAMPLE_URL + "sjs_sorting-test-server.sjs";

const TEST_IMAGE = EXAMPLE_URL + "test-image.png";
const TEST_IMAGE_DATA_URI = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABGdBTUEAAK/INwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAHWSURBVHjaYvz//z8DJQAggJiQOe/fv2fv7Oz8rays/N+VkfG/iYnJfyD/1+rVq7ffu3dPFpsBAAHEAHIBCJ85c8bN2Nj4vwsDw/8zQLwKiO8CcRoQu0DxqlWrdsHUwzBAAIGJmTNnPgYa9j8UqhFElwPxf2MIDeIrKSn9FwSJoRkAEEAM0DD4DzMAyPi/G+QKY4hh5WAXGf8PDQ0FGwJ22d27CjADAAIIrLmjo+MXA9R2kAHvGBA2wwx6B8W7od6CeQcggKCmCEL8bgwxYCbUIGTDVkHDBia+CuotgACCueD3TDQN75D4xmAvCoK9ARMHBzAw0AECiBHkAlC0Mdy7x9ABNA3obAZXIAa6iKEcGlMVQHwWyjYuL2d4v2cPg8vZswx7gHyAAAK7AOif7SAbOqCmn4Ha3AHFsIDtgPq/vLz8P4MSkJ2W9h8ggBjevXvHDo4FQUQg/kdypqCg4H8lUIACnQ/SOBMYI8bAsAJFPcj1AAEEjwVQqLpAbXmH5BJjqI0gi9DTAAgDBBCcAVLkgmQ7yKCZxpCQxqUZhAECCJ4XgMl493ug21ZD+aDAXH0WLM4A9MZPXJkJIIAwTAR5pQMalaCABQUULttBGCCAGCnNzgABBgAMJ5THwGvJLAAAAABJRU5ErkJggg==";


waitForExplicitFinish();


const gEnableLogging = Services.prefs.getBoolPref("devtools.debugger.log");
Services.prefs.setBoolPref("devtools.debugger.log", true);


const gDefaultFilters = Services.prefs.getCharPref("devtools.netmonitor.filters");

registerCleanupFunction(() => {
  info("finish() was called, cleaning up...");
  Services.prefs.setBoolPref("devtools.debugger.log", gEnableLogging);
  Services.prefs.setCharPref("devtools.netmonitor.filters", gDefaultFilters);
});

function addTab(aUrl, aWindow) {
  info("Adding tab: " + aUrl);

  let deferred = promise.defer();
  let targetWindow = aWindow || window;
  let targetBrowser = targetWindow.gBrowser;

  targetWindow.focus();
  let tab = targetBrowser.selectedTab = targetBrowser.addTab(aUrl);
  let browser = tab.linkedBrowser;

  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    deferred.resolve(tab);
  }, true);

  return deferred.promise;
}

function removeTab(aTab, aWindow) {
  info("Removing tab.");

  let targetWindow = aWindow || window;
  let targetBrowser = targetWindow.gBrowser;

  targetBrowser.removeTab(aTab);
}

function initNetMonitor(aUrl, aWindow) {
  info("Initializing a network monitor pane.");

  return addTab(aUrl).then((aTab) => {
    info("Net tab added successfully: " + aUrl);

    let deferred = promise.defer();
    let debuggee = aTab.linkedBrowser.contentWindow.wrappedJSObject;
    let target = TargetFactory.forTab(aTab);

    gDevTools.showToolbox(target, "netmonitor").then((aToolbox) => {
      info("Netork monitor pane shown successfully.");

      let monitor = aToolbox.getCurrentPanel();
      deferred.resolve([aTab, debuggee, monitor]);
    });

    return deferred.promise;
  });
}

function restartNetMonitor(aMonitor, aNewUrl) {
  info("Restarting the specified network monitor.");

  let deferred = promise.defer();
  let tab = aMonitor.target.tab;
  let url = aNewUrl || tab.linkedBrowser.contentWindow.wrappedJSObject.location.href;

  aMonitor.once("destroyed", () => initNetMonitor(url).then(deferred.resolve));
  removeTab(tab);

  return deferred.promise;
}

function teardown(aMonitor) {
  info("Destroying the specified network monitor.");

  let deferred = promise.defer();
  let tab = aMonitor.target.tab;

  aMonitor.once("destroyed", deferred.resolve);
  removeTab(tab);

  return deferred.promise;
}

function waitForNetworkEvents(aMonitor, aGetRequests, aPostRequests = 0) {
  let deferred = promise.defer();

  let panel = aMonitor.panelWin;
  let genericEvents = 0;
  let postEvents = 0;

  function onGenericEvent() {
    genericEvents++;
    maybeResolve();
  }

  function onPostEvent() {
    postEvents++;
    maybeResolve();
  }

  function maybeResolve() {
    info("> Network events progress: " +
      genericEvents + "/" + ((aGetRequests + aPostRequests) * 13) + ", " +
      postEvents + "/" + (aPostRequests * 2));

    
    
    if (genericEvents == (aGetRequests + aPostRequests) * 13 &&
        postEvents == aPostRequests * 2) {

      panel.off(panel.EVENTS.UPDATING_REQUEST_HEADERS, onGenericEvent);
      panel.off(panel.EVENTS.RECEIVED_REQUEST_HEADERS, onGenericEvent);
      panel.off(panel.EVENTS.UPDATING_REQUEST_COOKIES, onGenericEvent);
      panel.off(panel.EVENTS.RECEIVED_REQUEST_COOKIES, onGenericEvent);
      panel.off(panel.EVENTS.UPDATING_REQUEST_POST_DATA, onPostEvent);
      panel.off(panel.EVENTS.RECEIVED_REQUEST_POST_DATA, onPostEvent);
      panel.off(panel.EVENTS.UPDATING_RESPONSE_HEADERS, onGenericEvent);
      panel.off(panel.EVENTS.RECEIVED_RESPONSE_HEADERS, onGenericEvent);
      panel.off(panel.EVENTS.UPDATING_RESPONSE_COOKIES, onGenericEvent);
      panel.off(panel.EVENTS.RECEIVED_RESPONSE_COOKIES, onGenericEvent);
      panel.off(panel.EVENTS.STARTED_RECEIVING_RESPONSE, onGenericEvent);
      panel.off(panel.EVENTS.UPDATING_RESPONSE_CONTENT, onGenericEvent);
      panel.off(panel.EVENTS.RECEIVED_RESPONSE_CONTENT, onGenericEvent);
      panel.off(panel.EVENTS.UPDATING_EVENT_TIMINGS, onGenericEvent);
      panel.off(panel.EVENTS.RECEIVED_EVENT_TIMINGS, onGenericEvent);

      executeSoon(deferred.resolve);
    }
  }

  panel.on(panel.EVENTS.UPDATING_REQUEST_HEADERS, onGenericEvent);
  panel.on(panel.EVENTS.RECEIVED_REQUEST_HEADERS, onGenericEvent);
  panel.on(panel.EVENTS.UPDATING_REQUEST_COOKIES, onGenericEvent);
  panel.on(panel.EVENTS.RECEIVED_REQUEST_COOKIES, onGenericEvent);
  panel.on(panel.EVENTS.UPDATING_REQUEST_POST_DATA, onPostEvent);
  panel.on(panel.EVENTS.RECEIVED_REQUEST_POST_DATA, onPostEvent);
  panel.on(panel.EVENTS.UPDATING_RESPONSE_HEADERS, onGenericEvent);
  panel.on(panel.EVENTS.RECEIVED_RESPONSE_HEADERS, onGenericEvent);
  panel.on(panel.EVENTS.UPDATING_RESPONSE_COOKIES, onGenericEvent);
  panel.on(panel.EVENTS.RECEIVED_RESPONSE_COOKIES, onGenericEvent);
  panel.on(panel.EVENTS.STARTED_RECEIVING_RESPONSE, onGenericEvent);
  panel.on(panel.EVENTS.UPDATING_RESPONSE_CONTENT, onGenericEvent);
  panel.on(panel.EVENTS.RECEIVED_RESPONSE_CONTENT, onGenericEvent);
  panel.on(panel.EVENTS.UPDATING_EVENT_TIMINGS, onGenericEvent);
  panel.on(panel.EVENTS.RECEIVED_EVENT_TIMINGS, onGenericEvent);

  return deferred.promise;
}

function verifyRequestItemTarget(aRequestItem, aMethod, aUrl, aData = {}) {
  info("> Verifying: " + aMethod + " " + aUrl + " " + aData.toSource());
  info("> Request: " + aRequestItem.attachment.toSource());

  let requestsMenu = aRequestItem.ownerView;
  let widgetIndex = requestsMenu.indexOfItem(aRequestItem);
  let visibleIndex = requestsMenu.visibleItems.indexOf(aRequestItem);

  info("Widget index of item: " + widgetIndex);
  info("Visible index of item: " + visibleIndex);

  let { fuzzyUrl, status, statusText, type, fullMimeType, size, time } = aData;
  let { attachment, target } = aRequestItem

  let uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
  let name = uri.fileName || "/";
  let query = uri.query;
  let hostPort = uri.hostPort;

  if (fuzzyUrl) {
    ok(attachment.method.startsWith(aMethod), "The attached method is incorrect.");
    ok(attachment.url.startsWith(aUrl), "The attached url is incorrect.");
  } else {
    is(attachment.method, aMethod, "The attached method is incorrect.");
    is(attachment.url, aUrl, "The attached url is incorrect.");
  }

  is(target.querySelector(".requests-menu-method").getAttribute("value"),
    aMethod, "The displayed method is incorrect.");

  if (fuzzyUrl) {
    ok(target.querySelector(".requests-menu-file").getAttribute("value").startsWith(
      name + (query ? "?" + query : "")), "The displayed file is incorrect.");
    ok(target.querySelector(".requests-menu-file").getAttribute("tooltiptext").startsWith(
      name + (query ? "?" + query : "")), "The tooltip file is incorrect.");
  } else {
    is(target.querySelector(".requests-menu-file").getAttribute("value"),
      name + (query ? "?" + query : ""), "The displayed file is incorrect.");
    is(target.querySelector(".requests-menu-file").getAttribute("tooltiptext"),
      name + (query ? "?" + query : ""), "The tooltip file is incorrect.");
  }

  is(target.querySelector(".requests-menu-domain").getAttribute("value"),
    hostPort, "The displayed domain is incorrect.");
  is(target.querySelector(".requests-menu-domain").getAttribute("tooltiptext"),
    hostPort, "The tooltip domain is incorrect.");

  if (status !== undefined) {
    let value = target.querySelector(".requests-menu-status").getAttribute("code");
    let codeValue = target.querySelector(".requests-menu-status-code").getAttribute("value");
    let tooltip = target.querySelector(".requests-menu-status-and-method").getAttribute("tooltiptext");
    info("Displayed status: " + value);
    info("Displayed code: " + codeValue);
    info("Tooltip status: " + tooltip);
    is(value, status, "The displayed status is incorrect.");
    is(codeValue, status, "The displayed status code is incorrect.");
    is(tooltip, status + " " + statusText, "The tooltip status is incorrect.");
  }
  if (type !== undefined) {
    let value = target.querySelector(".requests-menu-type").getAttribute("value");
    let tooltip = target.querySelector(".requests-menu-type").getAttribute("tooltiptext");
    info("Displayed type: " + value);
    info("Tooltip type: " + tooltip);
    is(value, type, "The displayed type is incorrect.");
    is(tooltip, fullMimeType, "The tooltip type is incorrect.");
  }
  if (size !== undefined) {
    let value = target.querySelector(".requests-menu-size").getAttribute("value");
    let tooltip = target.querySelector(".requests-menu-size").getAttribute("tooltiptext");
    info("Displayed size: " + value);
    info("Tooltip size: " + tooltip);
    is(value, size, "The displayed size is incorrect.");
    is(tooltip, size, "The tooltip size is incorrect.");
  }
  if (time !== undefined) {
    let value = target.querySelector(".requests-menu-timings-total").getAttribute("value");
    let tooltip = target.querySelector(".requests-menu-timings-total").getAttribute("tooltiptext");
    info("Displayed time: " + value);
    info("Tooltip time: " + tooltip);
    ok(~~(value.match(/[0-9]+/)) >= 0, "The displayed time is incorrect.");
    ok(~~(tooltip.match(/[0-9]+/)) >= 0, "The tooltip time is incorrect.");
  }

  if (visibleIndex != -1) {
    if (visibleIndex % 2 == 0) {
      ok(aRequestItem.target.hasAttribute("even"),
        "Unexpected 'even' attribute for " + aRequestItem.value);
      ok(!aRequestItem.target.hasAttribute("odd"),
        "Unexpected 'odd' attribute for " + aRequestItem.value);
    } else {
      ok(!aRequestItem.target.hasAttribute("even"),
        "Unexpected 'even' attribute for " + aRequestItem.value);
      ok(aRequestItem.target.hasAttribute("odd"),
        "Unexpected 'odd' attribute for " + aRequestItem.value);
    }
  }
}












function waitFor (subject, eventName) {
  let deferred = promise.defer();
  subject.once(eventName, deferred.resolve);
  return deferred.promise;
}







function testFilterButtons(aMonitor, aFilterType) {
  let doc = aMonitor.panelWin.document;
  let target = doc.querySelector("#requests-menu-filter-" + aFilterType + "-button");
  let buttons = doc.querySelectorAll(".requests-menu-footer-button");

  
  let checkStatus = [(button == target) ? 1 : 0 for (button of buttons)]
  testFilterButtonsCustom(aMonitor, checkStatus);
}









function testFilterButtonsCustom(aMonitor, aIsChecked) {
  let doc = aMonitor.panelWin.document;
  let buttons = doc.querySelectorAll(".requests-menu-footer-button");
  for (let i = 0; i < aIsChecked.length; i++) {
    let button = buttons[i];
    if (aIsChecked[i]) {
      is(button.hasAttribute("checked"), true,
        "The " + button.id + " button should have a 'checked' attribute.");
    } else {
      is(button.hasAttribute("checked"), false,
        "The " + button.id + " button should not have a 'checked' attribute.");
    }
  }
}
