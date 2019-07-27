



let testPage1 = "<html id='html1'><body id='body1'><button id='button1'>Tab 1</button></body></html>";
let testPage2 = "<html id='html2'><body id='body2'><button id='button2'>Tab 2</button></body></html>";
let testPage3 = "<html id='html3'><body id='body3'><button id='button3'>Tab 3</button></body></html>";

const fm = Services.focus;

let tab1 = null;
let tab2 = null;
let browser1 = null;
let browser2 = null;
let _browser_tabfocus_test_lastfocus;
let _browser_tabfocus_test_lastfocuswindow = null;
let actualEvents = [];
let expectedEvents = [];
let currentTestName = "";
let _expectedElement = null;
let _expectedWindow = null;

let currentPromiseResolver = null;

function* getFocusedElementForBrowser(browser, dontCheckExtraFocus = false)
{
  if (gMultiProcessBrowser) {
    return new Promise((resolve, reject) => {
      messageManager.addMessageListener("Browser:GetCurrentFocus", function getCurrentFocus(message) {
        messageManager.removeMessageListener("Browser:GetCurrentFocus", getCurrentFocus);
        resolve(message.data.details);
      });

      
      
      
      browser.messageManager.sendAsyncMessage("Browser:GetFocusedElement",
        { dontCheckExtraFocus : dontCheckExtraFocus });
    });
  }
  else {
    var focusedWindow = {};
    var node = fm.getFocusedElementForWindow(browser.contentWindow, false, focusedWindow);
    return "Focus is " + (node ? node.id : "<none>");
  }
}

function focusInChild()
{
  var fm = Components.classes["@mozilla.org/focus-manager;1"].
                      getService(Components.interfaces.nsIFocusManager);

  function eventListener(event) {
    var id;
    if (event.target instanceof Components.interfaces.nsIDOMWindow)
      id = event.originalTarget.document.documentElement.id + "-window";
    else if (event.target instanceof Components.interfaces.nsIDOMDocument)
      id = event.originalTarget.documentElement.id + "-document";
    else
      id = event.originalTarget.id;
    sendSyncMessage("Browser:FocusChanged", { details : event.type + ": " + id });
  }

  addEventListener("focus", eventListener, true);
  addEventListener("blur", eventListener, true);

  addMessageListener("Browser:GetFocusedElement", function getFocusedElement(message) {
    var focusedWindow = {};
    var node = fm.getFocusedElementForWindow(content, false, focusedWindow);
    var details = "Focus is " + (node ? node.id : "<none>");

    

    let doc = content.document;
    if (!message.data.dontCheckExtraFocus) {
      if (fm.focusedElement != node) {
        details += "<ERROR: focusedElement doesn't match>";
      }
      if (fm.focusedWindow && fm.focusedWindow != content) {
        details += "<ERROR: focusedWindow doesn't match>";
      }
      if ((fm.focusedWindow == content) != doc.hasFocus()) {
        details += "<ERROR: child hasFocus() is not correct>";
      }
      if ((fm.focusedElement && doc.activeElement != fm.focusedElement) ||
          (!fm.focusedElement && doc.activeElement != doc.body)) {
        details += "<ERROR: child activeElement is not correct>";
      }
    }

    sendSyncMessage("Browser:GetCurrentFocus", { details : details });
  });
}

add_task(function*() {
  tab1 = gBrowser.addTab();
  browser1 = gBrowser.getBrowserForTab(tab1);

  tab2 = gBrowser.addTab();
  browser2 = gBrowser.getBrowserForTab(tab2);

  yield promiseTabLoadEvent(tab1, "data:text/html," + escape(testPage1));
  yield promiseTabLoadEvent(tab2, "data:text/html," + escape(testPage2));

  var childFocusScript = "data:,(" + focusInChild.toString() + ")();";
  browser1.messageManager.loadFrameScript(childFocusScript, true);
  browser2.messageManager.loadFrameScript(childFocusScript, true);

  gURLBar.focus();
  yield SimpleTest.promiseFocus();

  var messages = "";
  if (gMultiProcessBrowser) {
    messageManager.addMessageListener("Browser:FocusChanged", message => {
      actualEvents.push(message.data.details);
      compareFocusResults();
    });
  }

  _browser_tabfocus_test_lastfocus = gURLBar;
  _browser_tabfocus_test_lastfocuswindow = window;

  window.addEventListener("focus", _browser_tabfocus_test_eventOccured, true);
  window.addEventListener("blur", _browser_tabfocus_test_eventOccured, true);

  
  var fm = Services.focus;
  var focusedWindow = {};

  let focused = yield getFocusedElementForBrowser(browser1);
  is(focused, "Focus is <none>", "initial focus in tab 1");

  focused = yield getFocusedElementForBrowser(browser2);
  is(focused, "Focus is <none>", "initial focus in tab 2");

  is(document.activeElement, gURLBar.inputField, "focus after loading two tabs");

  yield expectFocusShift(function () gBrowser.selectedTab = tab2,
                         browser2.contentWindow, null, true,
                         "after tab change, focus in new tab");

  focused = yield getFocusedElementForBrowser(browser2);
  is(focused, "Focus is <none>", "focusedElement after tab change, focus in new tab");

  
  
  yield expectFocusShift(function () gBrowser.selectedTab = tab1,
                         browser1.contentWindow, null, true,
                         "after tab change, focus in original tab");

  focused = yield getFocusedElementForBrowser(browser1);
  is(focused, "Focus is <none>", "focusedElement after tab change, focus in original tab");

  
  var button1 = browser1.contentDocument.getElementById("button1");
  yield expectFocusShift(function () button1.focus(),
                         browser1.contentWindow, button1, true,
                         "after button focused");

  focused = yield getFocusedElementForBrowser(browser1);
  is(focused, "Focus is button1", "focusedElement in first browser after button focused");

  
  
  
  var button2 = browser2.contentDocument.getElementById("button2");
  yield expectFocusShift(function () button2.focus(),
                         browser1.contentWindow, button1, false,
                         "after button focus in unfocused tab");

  focused = yield getFocusedElementForBrowser(browser1, false);
  is(focused, "Focus is button1", "focusedElement in first browser after button focus in unfocused tab");
  focused = yield getFocusedElementForBrowser(browser2, true);
  is(focused, "Focus is button2", "focusedElement in second browser after button focus in unfocused tab");

  
  yield expectFocusShift(function () gBrowser.selectedTab = tab2,
                         browser2.contentWindow, button2, true,
                         "after tab change with button focused");

  
  
  yield expectFocusShift(function () button1.blur(),
                         browser2.contentWindow, button2, false,
                         "focusedWindow after blur in unfocused tab");

  focused = yield getFocusedElementForBrowser(browser1, true);
  is(focused, "Focus is <none>", "focusedElement in first browser after focus in unfocused tab");
  focused = yield getFocusedElementForBrowser(browser2, false);
  is(focused, "Focus is button2", "focusedElement in second browser after focus in unfocused tab");

  
  yield expectFocusShift(function () gBrowser.selectedTab.focus(),
                         window, gBrowser.selectedTab, true,
                         "focusing tab element");
  yield expectFocusShift(function () gBrowser.selectedTab = tab1,
                         window, tab1, true,
                         "tab change when selected tab element was focused");

  let paintWaiter;
  if (gMultiProcessBrowser) {
    paintWaiter = new Promise((resolve, reject) => {
      browser2.addEventListener("MozAfterRemotePaint", function paintListener() {
        browser2.removeEventListener("MozAfterRemotePaint", paintListener, false);
        executeSoon(resolve);
      }, false);
      browser2.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader.requestNotifyAfterRemotePaint();
    });
  }

  yield expectFocusShift(function () gBrowser.selectedTab = tab2,
                         window, tab2, true,
                         "another tab change when selected tab element was focused");

  
  
  
  
  if (gMultiProcessBrowser) {
    yield paintWaiter;
  }

  yield expectFocusShift(function () gBrowser.selectedTab.blur(),
                         window, null, true,
                         "blurring tab element");

  
  
  button1.focus();
  yield expectFocusShift(function () gURLBar.focus(),
                         window, gURLBar.inputField, true,
                         "focusedWindow after url field focused");
  focused = yield getFocusedElementForBrowser(browser1, true);
  is(focused, "Focus is button1", "focusedElement after url field focused, first browser");
  focused = yield getFocusedElementForBrowser(browser2, true);
  is(focused, "Focus is button2", "focusedElement after url field focused, second browser");

  yield expectFocusShift(function () gURLBar.blur(),
                         window, null, true,
                         "blurring url field");

  
  
  yield expectFocusShift(function () gBrowser.selectedTab = tab1,
                         browser1.contentWindow, button1, true,
                         "after tab change, focus in url field, button focused in new tab");

  focused = yield getFocusedElementForBrowser(browser1, false);
  is(focused, "Focus is button1", "after switch tab, focus in unfocused tab, first browser");
  focused = yield getFocusedElementForBrowser(browser2, true);
  is(focused, "Focus is button2", "after switch tab, focus in unfocused tab, second browser");

  
  yield expectFocusShift(function () button1.blur(),
                         browser1.contentWindow, null, true,
                         "after blur in focused tab");

  focused = yield getFocusedElementForBrowser(browser1, false);
  is(focused, "Focus is <none>", "focusedWindow after blur in focused tab, child");
  focusedWindow = {};
  is(fm.getFocusedElementForWindow(window, false, focusedWindow), browser1, "focusedElement after blur in focused tab, parent");

  
  yield expectFocusShift(function () gURLBar.blur(),
                         browser1.contentWindow, null, false,
                         "after blur in unfocused url field");

  focusedWindow = {};
  is(fm.getFocusedElementForWindow(window, false, focusedWindow), browser1, "focusedElement after blur in unfocused url field");

  
  yield expectFocusShift(function () gBrowser.selectedTab = tab2,
                         browser2.contentWindow, button2, true,
                         "after switch from unfocused to focused tab");
  focused = yield getFocusedElementForBrowser(browser2, true);
  is(focused, "Focus is button2", "focusedElement after switch from unfocused to focused tab");

  
  
  yield expectFocusShift(function () fm.clearFocus(window),
                         window, null, true,
                         "after switch to chrome with no focused element");

  focusedWindow = {};
  is(fm.getFocusedElementForWindow(window, false, focusedWindow), null, "focusedElement after switch to chrome with no focused element");

  
  yield expectFocusShift(function () gBrowser.selectedTab = tab1,
                         browser1.contentWindow, null, true,
                         "focusedWindow after tab switch from no focus to no focus");

  focused = yield getFocusedElementForBrowser(browser1, false);
  is(focused, "Focus is <none>", "after tab switch from no focus to no focus, first browser");
  focused = yield getFocusedElementForBrowser(browser2, true);
  is(focused, "Focus is button2", "after tab switch from no focus to no focus, second browser");

  
  
  yield expectFocusShift(function () button1.focus(),
                         browser1.contentWindow, button1, true,
                         "focus button");

  yield promiseTabLoadEvent(tab1, "data:text/html," + escape(testPage3));

  
  gURLBar.focus();

  let contentwin = browser1.contentWindow;
  yield new Promise((resolve, reject) => {
    window.addEventListener("pageshow", function navigationOccured(event) {
      window.removeEventListener("pageshow", navigationOccured, true);
      resolve();
    }, true);
    document.getElementById('Browser:Back').doCommand();
  });

  is(window.document.activeElement, gURLBar.inputField, "urlbar still focused after navigating back");

  
  if (!gMultiProcessBrowser) {
    gURLBar.focus();
    actualEvents = [];
    _browser_tabfocus_test_lastfocus = gURLBar;
    _browser_tabfocus_test_lastfocuswindow = window;

    yield expectFocusShift(function () EventUtils.synthesizeKey("VK_F6", { }),
                           browser1.contentWindow, browser1.contentDocument.documentElement,
                           true, "switch document forward with f6");

    EventUtils.synthesizeKey("VK_F6", { });
    is(fm.focusedWindow, window, "switch document forward again with f6");

    browser1.style.MozUserFocus = "ignore";
    browser1.clientWidth;
    EventUtils.synthesizeKey("VK_F6", { });
    is(fm.focusedWindow, window, "switch document forward again with f6 when browser non-focusable");

    browser1.style.MozUserFocus = "normal";
    browser1.clientWidth;
  }

  window.removeEventListener("focus", _browser_tabfocus_test_eventOccured, true);
  window.removeEventListener("blur", _browser_tabfocus_test_eventOccured, true);

  gBrowser.removeCurrentTab();
  gBrowser.removeCurrentTab();
  finish();
});

function _browser_tabfocus_test_eventOccured(event)
{
  var id;

  
  if (Cu.isCrossProcessWrapper(event.originalTarget))
    return;

  if (event.target instanceof Window)
    id = event.originalTarget.document.documentElement.id + "-window";
  else if (event.target instanceof Document)
    id = event.originalTarget.documentElement.id + "-document";
  else if (event.target.id == "urlbar" && event.originalTarget.localName == "input")
    id = "urlbar";
  else if (event.originalTarget.localName == "browser")
    id = (event.originalTarget == browser1) ? "browser1" : "browser2";
  else if (event.originalTarget.localName == "tab")
    id = (event.originalTarget == tab1) ? "tab1" : "tab2";
  else
    id = event.originalTarget.id;

  actualEvents.push(event.type + ": " + id);
  compareFocusResults();
}

function getId(element)
{
  if (element.localName == "browser") {
    return element == browser1 ? "browser1" : "browser2";
  }

  if (element.localName == "tab") {
    return element == tab1 ? "tab1" : "tab2";
  }

  return (element.localName == "input") ? "urlbar" : element.id;
}

function compareFocusResults()
{
  if (!currentPromiseResolver)
    return;

  if (actualEvents.length < expectedEvents.length)
    return;

  for (let e = 0; e < expectedEvents.length; e++) {
    
    
    if (gMultiProcessBrowser && actualEvents[e] != expectedEvents[e] &&
        actualEvents[e].startsWith("focus: browser")) {

      
      
      let foundLaterIndex = expectedEvents.indexOf(actualEvents[e], e + 1);
      if (foundLaterIndex > e) {
        expectedEvents.splice(e, 0, expectedEvents.splice(foundLaterIndex, 1)[0]);
      }
    }

    is(actualEvents[e], expectedEvents[e], currentTestName + " events [event " + e + "]");
  }
  actualEvents = [];

  
  executeSoon(() => {
    var focusedElement = fm.focusedElement;
    is(focusedElement ? getId(focusedElement) : "none",
       _expectedElement ? getId(_expectedElement) : "none", currentTestName + " focusedElement");
    is(fm.focusedWindow, _expectedWindow, currentTestName + " focusedWindow");
    var focusedWindow = {};
    is(fm.getFocusedElementForWindow(_expectedWindow, false, focusedWindow),
       _expectedElement, currentTestName + " getFocusedElementForWindow");
    is(focusedWindow.value, _expectedWindow, currentTestName + " getFocusedElementForWindow frame");
    is(_expectedWindow.document.hasFocus(), true, currentTestName + " hasFocus");
    var expectedActive = _expectedElement;
    if (!expectedActive) {
      expectedActive = _expectedWindow.document instanceof XULDocument ?
                       _expectedWindow.document.documentElement : _expectedWindow.document.body;
    }
    is(_expectedWindow.document.activeElement, expectedActive, currentTestName + " activeElement");

    currentPromiseResolver();
    currentPromiseResolver = null;
  });
}

function* expectFocusShift(callback, expectedWindow, expectedElement, focusChanged, testid)
{
  currentPromiseResolver = null;
  currentTestName = testid;

  expectedEvents = [];

  if (focusChanged) {
    _expectedElement = expectedElement;
    _expectedWindow = expectedWindow;

    
    
    if (gMultiProcessBrowser) {
      if (_expectedWindow == browser1.contentWindow) {
        _expectedElement = browser1;
      }
      else if (_expectedWindow == browser2.contentWindow) {
        _expectedElement = browser2;
      }
      _expectedWindow = window;
    }

    if (gMultiProcessBrowser && _browser_tabfocus_test_lastfocuswindow != window &&
        _browser_tabfocus_test_lastfocuswindow != expectedWindow) {
      let browserid = _browser_tabfocus_test_lastfocuswindow == browser1.contentWindow ? "browser1" : "browser2";
      expectedEvents.push("blur: " + browserid);
    }

    var newElementIsFocused = (expectedElement && expectedElement != expectedElement.ownerDocument.documentElement);
    if (newElementIsFocused && gMultiProcessBrowser &&
        _browser_tabfocus_test_lastfocuswindow != window &&
        expectedWindow == window) {
      
      expectedEvents.push("focus: " + getId(expectedElement));
      newElementIsFocused = false;
    }

    if (_browser_tabfocus_test_lastfocus && _browser_tabfocus_test_lastfocus != _expectedElement)
      expectedEvents.push("blur: " + getId(_browser_tabfocus_test_lastfocus));

    if (_browser_tabfocus_test_lastfocuswindow &&
        _browser_tabfocus_test_lastfocuswindow != expectedWindow) {

      if (!gMultiProcessBrowser || _browser_tabfocus_test_lastfocuswindow != window) {
        let windowid = _browser_tabfocus_test_lastfocuswindow.document.documentElement.id;
        expectedEvents.push("blur: " + windowid + "-document");
        expectedEvents.push("blur: " + windowid + "-window");
      }
    }

    if (expectedWindow && _browser_tabfocus_test_lastfocuswindow != expectedWindow) {
      if (gMultiProcessBrowser && expectedWindow != window) {
        let browserid = expectedWindow == browser1.contentWindow ? "browser1" : "browser2";
        expectedEvents.push("focus: " + browserid);
      }

      if (!gMultiProcessBrowser || expectedWindow != window) {
        var windowid = expectedWindow.document.documentElement.id;
        expectedEvents.push("focus: " + windowid + "-document");
        expectedEvents.push("focus: " + windowid + "-window");
      }
    }

    if (newElementIsFocused) {
      expectedEvents.push("focus: " + getId(expectedElement));
    }

    _browser_tabfocus_test_lastfocus = expectedElement;
    _browser_tabfocus_test_lastfocuswindow = expectedWindow;
  }

  return new Promise((resolve, reject) => {
    currentPromiseResolver = resolve;
    callback();

    
    if (!expectedEvents.length) {
      currentPromiseResolver();
      currentPromiseResolver = null;
    }
  });
}

