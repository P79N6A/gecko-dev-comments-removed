



let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let {TargetFactory, require} = devtools;
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
const {DOMHelpers} = Cu.import("resource:///modules/devtools/DOMHelpers.jsm", {});
const {Hosts} = require("devtools/framework/toolbox-hosts");

gDevTools.testing = true;
SimpleTest.registerCleanupFunction(() => {
  gDevTools.testing = false;
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

registerCleanupFunction(function tearDown() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }

  console = undefined;
});

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







function loadTelemetryAndRecordLogs() {
  info("Mock the Telemetry log function to record logged information");

  let Telemetry = require("devtools/shared/telemetry");
  Telemetry.prototype.telemetryInfo = {};
  Telemetry.prototype._oldlog = Telemetry.prototype.log;
  Telemetry.prototype.log = function(histogramId, value) {
    if (!this.telemetryInfo) {
      
      return;
    }
    if (histogramId) {
      if (!this.telemetryInfo[histogramId]) {
        this.telemetryInfo[histogramId] = [];
      }

      this.telemetryInfo[histogramId].push(value);
    }
  };

  return Telemetry;
}




function stopRecordingTelemetryLogs(Telemetry) {
  Telemetry.prototype.log = Telemetry.prototype._oldlog;
  delete Telemetry.prototype._oldlog;
  delete Telemetry.prototype.telemetryInfo;
}





function checkTelemetryResults(Telemetry) {
  let result = Telemetry.prototype.telemetryInfo;

  for (let [histId, value] of Iterator(result)) {
    if (histId.endsWith("OPENED_PER_USER_FLAG")) {
      ok(value.length === 1 && value[0] === true,
         "Per user value " + histId + " has a single value of true");
    } else if (histId.endsWith("OPENED_BOOLEAN")) {
      ok(value.length > 1, histId + " has more than one entry");

      let okay = value.every(function(element) {
        return element === true;
      });

      ok(okay, "All " + histId + " entries are === true");
    } else if (histId.endsWith("TIME_ACTIVE_SECONDS")) {
      ok(value.length > 1, histId + " has more than one entry");

      let okay = value.every(function(element) {
        return element > 0;
      });

      ok(okay, "All " + histId + " entries have time > 0");
    }
  }
}








function* openAndCloseToolbox(nbOfTimes, usageTime, toolId) {
  for (let i = 0; i < nbOfTimes; i ++) {
    info("Opening toolbox " + (i + 1));
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    yield gDevTools.showToolbox(target, toolId)

    
    yield new Promise(resolve => setTimeout(resolve, usageTime));

    info("Closing toolbox " + (i + 1));
    yield gDevTools.closeToolbox(target);
  }
}
