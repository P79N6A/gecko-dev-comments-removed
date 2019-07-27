



let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let {TargetFactory, require} = devtools;
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let {Services} = Cu.import("resource://gre/modules/Services.jsm", {});
const {DOMHelpers} = Cu.import("resource:///modules/devtools/DOMHelpers.jsm", {});
const {Hosts} = require("devtools/framework/toolbox-hosts");

let oldCanRecord = Services.telemetry.canRecordExtended;

gDevTools.testing = true;
registerCleanupFunction(() => {
  _stopTelemetry();
  gDevTools.testing = false;

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }

  console = undefined;
});

const TEST_URI_ROOT = "http://example.com/browser/browser/devtools/shared/test/";
const OPTIONS_VIEW_URL = TEST_URI_ROOT + "doc_options-view.xul";




function addTab(aURL, aCallback)
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  content.location = aURL;

  let tab = gBrowser.selectedTab;
  let browser = gBrowser.getBrowserForTab(tab);

  function onTabLoad() {
    browser.removeEventListener("load", onTabLoad, true);
    aCallback(browser, tab, browser.contentDocument);
  }

  browser.addEventListener("load", onTabLoad, true);
}

function promiseTab(aURL) {
  return new Promise(resolve =>
    addTab(aURL, resolve));
}

function catchFail(func) {
  return function() {
    try {
      return func.apply(null, arguments);
    }
    catch (ex) {
      ok(false, ex);
      console.error(ex);
      finish();
      throw ex;
    }
  };
}






























function waitForValue(aOptions)
{
  let start = Date.now();
  let timeout = aOptions.timeout || 5000;
  let lastValue;

  function wait(validatorFn, successFn, failureFn)
  {
    if ((Date.now() - start) > timeout) {
      
      ok(false, "Timed out while waiting for: " + aOptions.name);
      let expected = "value" in aOptions ?
                     "'" + aOptions.value + "'" :
                     "a trueish value";
      info("timeout info :: got '" + lastValue + "', expected " + expected);
      failureFn(aOptions, lastValue);
      return;
    }

    lastValue = validatorFn(aOptions, lastValue);
    let successful = "value" in aOptions ?
                      lastValue == aOptions.value :
                      lastValue;
    if (successful) {
      ok(true, aOptions.name);
      successFn(aOptions, lastValue);
    }
    else {
      setTimeout(function() wait(validatorFn, successFn, failureFn), 100);
    }
  }

  wait(aOptions.validator, aOptions.success, aOptions.failure);
}

function oneTimeObserve(name, callback) {
  var func = function() {
    Services.obs.removeObserver(func, name);
    callback();
  };
  Services.obs.addObserver(func, name, false);
}

let createHost = Task.async(function*(type = "bottom", src = "data:text/html;charset=utf-8,") {
  let host = new Hosts[type](gBrowser.selectedTab);
  let iframe = yield host.create();

  yield new Promise(resolve => {
    let domHelper = new DOMHelpers(iframe.contentWindow);
    iframe.setAttribute("src", src);
    domHelper.onceDOMReady(resolve);
  });

  return [host, iframe.contentWindow, iframe.contentDocument];
});

function reportError(error) {
  let stack = "    " + error.stack.replace(/\n?.*?@/g, "\n    JS frame :: ");

  ok(false, "ERROR: " + error + " at " + error.fileName + ":" +
            error.lineNumber + "\n\nStack trace:" + stack);

  if (finishUp) {
    finishUp();
  }
}

function startTelemetry() {
  Services.telemetry.canRecordExtended = true;
}




function _stopTelemetry() {
  let Telemetry = devtools.require("devtools/shared/telemetry");
  let telemetry = new Telemetry();

  telemetry.clearToolsOpenedPref();

  Services.telemetry.canRecordExtended = oldCanRecord;

  
  for (let histId in Services.telemetry.histogramSnapshots) {
    try {
      let histogram = Services.telemetry.getHistogramById(histId);
      histogram.clear();
    } catch(e) {
      
    }
  }
}













function checkTelemetry(histId, expected, checkType="array") {
  let actual = Services.telemetry.getHistogramById(histId).snapshot().counts;

  switch (checkType) {
    case "array":
      is(JSON.stringify(actual), JSON.stringify(expected), histId + " correct.");
    break;
    case "hasentries":
      let hasEntry = actual.some(num => num > 0);
      ok(hasEntry, histId + " has at least one entry.");
    break;
  }
}









function generateTelemetryTests(prefix="") {
  dump("=".repeat(80) + "\n");
  for (let histId in Services.telemetry.histogramSnapshots) {
    if (!histId.startsWith(prefix)) {
      continue;
    }

    let snapshot = Services.telemetry.histogramSnapshots[histId];
    let actual = snapshot.counts;

    switch (snapshot.histogram_type) {
      case Services.telemetry.HISTOGRAM_EXPONENTIAL:
      case Services.telemetry.HISTOGRAM_LINEAR:
        let total = 0;
        for (let val of actual) {
          total += val;
        }

        if (histId.endsWith("_ENUMERATED")) {
          if (total > 0) {
            dump("checkTelemetry(\"" + histId + "\", " + JSON.stringify(actual) + ");\n");
          }
          continue;
        }

        dump("checkTelemetry(\"" + histId + "\", null, \"hasentries\");\n");
      break;
      case Services.telemetry.HISTOGRAM_BOOLEAN:
        actual = JSON.stringify(actual);

        if (actual !== "[0,0,0]") {
          dump("checkTelemetry(\"" + histId + "\", " + actual + ");\n");
        }
      break;
      case Services.telemetry.HISTOGRAM_FLAG:
        actual = JSON.stringify(actual);

        if (actual !== "[1,0,0]") {
          dump("checkTelemetry(\"" + histId + "\", " + actual + ");\n");
        }
      break;
      case Services.telemetry.HISTOGRAM_COUNT:
        dump("checkTelemetry(\"" + histId + "\", " + actual + ");\n");
      break;
    }
  }
  dump("=".repeat(80) + "\n");
}








function* openAndCloseToolbox(nbOfTimes, usageTime, toolId) {
  for (let i = 0; i < nbOfTimes; i ++) {
    info("Opening toolbox " + (i + 1));
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    yield gDevTools.showToolbox(target, toolId);

    
    yield new Promise(resolve => setTimeout(resolve, usageTime));

    info("Closing toolbox " + (i + 1));
    yield gDevTools.closeToolbox(target);
  }
}
