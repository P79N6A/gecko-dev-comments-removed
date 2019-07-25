






































const TEST_URI = "http://example.com/browser/dom/tests/browser/test-console-api.html";

var gWindow;

function test() {
  waitForExplicitFinish();

  var tab = gBrowser.addTab(TEST_URI);
  gBrowser.selectedTab = tab;
  var browser = gBrowser.selectedBrowser;

  registerCleanupFunction(function () {
    gBrowser.removeTab(tab);
  });

  ConsoleObserver.init();

  browser.addEventListener("DOMContentLoaded", function onLoad(event) {
    browser.removeEventListener("DOMContentLoaded", onLoad, false);
    executeSoon(function test_executeSoon() {
      gWindow = browser.contentWindow;
      consoleAPISanityTest();
      observeConsoleTest();
    });

  }, false);
}

var gWindow;

function testConsoleData(aMessageObject) {
  let messageWindow = getWindowByWindowId(aMessageObject.ID);
  is(messageWindow, gWindow, "found correct window by window ID");

  is(aMessageObject.level, gLevel, "expected level received");
  ok(aMessageObject.arguments, "we have arguments");
  is(aMessageObject.arguments.length, gArgs.length, "arguments.length matches");

  if (gLevel == "trace") {
    is(aMessageObject.arguments.toSource(), gArgs.toSource(),
       "stack trace is correct");

    
    ConsoleObserver.destroy();
    finish();
  }
  else {
    gArgs.forEach(function (a, i) {
      is(aMessageObject.arguments[i], a, "correct arg " + i);
    });
  }

  if (aMessageObject.level == "error") {
    
    startTraceTest();
  }
}

function startTraceTest() {
  gLevel = "trace";
  gArgs = [
    {filename: TEST_URI, lineNumber: 6, functionName: null, language: 2},
    {filename: TEST_URI, lineNumber: 11, functionName: "foobar585956b", language: 2},
    {filename: TEST_URI, lineNumber: 15, functionName: "foobar585956a", language: 2},
    {filename: TEST_URI, lineNumber: 1, functionName: "onclick", language: 2}
  ];

  let button = gWindow.document.getElementById("test-trace");
  ok(button, "found #test-trace button");
  EventUtils.synthesizeMouse(button, 2, 2, {}, gWindow);
}

var gLevel, gArgs;
function expect(level) {
  gLevel = level;
  gArgs = Array.slice(arguments, 1);
}

function observeConsoleTest() {
  let win = XPCNativeWrapper.unwrap(gWindow);
  expect("log", "arg");
  win.console.log("arg");

  expect("info", "arg", "extra arg");
  win.console.info("arg", "extra arg");

  expect("warn", "arg", "extra arg", 1);
  win.console.warn("arg", "extra arg", 1);

  expect("error", "arg");
  win.console.error("arg");
}

function consoleAPISanityTest() {
  let win = XPCNativeWrapper.unwrap(gWindow);
  ok(win.console, "we have a console attached");
  ok(win.console, "we have a console attached, 2nd attempt");

  ok(win.console.log, "console.log is here");
  ok(win.console.info, "console.info is here");
  ok(win.console.warn, "console.warn is here");
  ok(win.console.error, "console.error is here");
  ok(win.console.trace, "console.trace is here");
}

var ConsoleObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  init: function CO_init() {
    Services.obs.addObserver(this, "console-api-log-event", false);
  },

  destroy: function CO_destroy() {
    Services.obs.removeObserver(this, "console-api-log-event");
  },

  observe: function CO_observe(aSubject, aTopic, aData) {
    try {
      testConsoleData(aSubject.wrappedJSObject);
    } catch (ex) {
      
      
      ok(false, "Exception thrown in CO_observe: " + ex);
    }
  }
};

function getWindowId(aWindow)
{
  return aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                .getInterface(Ci.nsIDOMWindowUtils)
                .outerWindowID;
}

function getWindowByWindowId(aId) {
  let someWindow = Services.wm.getMostRecentWindow("navigator:browser");
  if (someWindow) {
    let windowUtils = someWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDOMWindowUtils);
    return windowUtils.getOuterWindowWithId(aId);
  }
  return null;
}
