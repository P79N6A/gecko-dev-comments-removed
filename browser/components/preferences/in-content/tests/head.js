


Components.utils.import("resource://gre/modules/Promise.jsm");

const kDefaultWait = 2000;

function is_hidden(aElement) {
  var style = aElement.ownerDocument.defaultView.getComputedStyle(aElement, "");
  if (style.display == "none")
    return true;
  if (style.visibility != "visible")
    return true;

  
  if (aElement.parentNode != aElement.ownerDocument)
    return is_hidden(aElement.parentNode);

  return false;
}

function is_element_visible(aElement, aMsg) {
  isnot(aElement, null, "Element should not be null, when checking visibility");
  ok(!is_hidden(aElement), aMsg);
}

function is_element_hidden(aElement, aMsg) {
  isnot(aElement, null, "Element should not be null, when checking visibility");
  ok(is_hidden(aElement), aMsg);
}

function open_preferences(aCallback) {
  gBrowser.selectedTab = gBrowser.addTab("about:preferences");
  let newTabBrowser = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  newTabBrowser.addEventListener("Initialized", function () {
    newTabBrowser.removeEventListener("Initialized", arguments.callee, true);
    aCallback(gBrowser.contentWindow);
  }, true);
}

function openAndLoadSubDialog(aURL, aFeatures = null, aParams = null, aClosingCallback = null) {
  let dialog = content.gSubDialog.open(aURL, aFeatures, aParams, aClosingCallback);
  let deferred = Promise.defer();

  content.gSubDialog._frame.addEventListener("load", function load(aEvent) {
    if (aEvent.target.contentWindow.location == "about:blank")
      return;
    content.gSubDialog._frame.removeEventListener("load", load);

    ise(content.gSubDialog._frame.contentWindow.location.toString(), aURL,
        "Check the proper URL is loaded");

    
    is_element_visible(content.gSubDialog._overlay, "Overlay is visible");

    
    let expectedStyleSheetURLs = content.gSubDialog._injectedStyleSheets.slice(0);
    for (let styleSheet of content.gSubDialog._frame.contentDocument.styleSheets) {
      let i = expectedStyleSheetURLs.indexOf(styleSheet.href);
      if (i >= 0) {
        info("found " + styleSheet.href);
        expectedStyleSheetURLs.splice(i, 1);
      }
    }
    ise(expectedStyleSheetURLs.length, 0, "All expectedStyleSheetURLs should have been found");

    deferred.resolve(dialog);
  });

  return deferred.promise;
}





















function waitForEvent(aSubject, aEventName, aTimeoutMs, aTarget) {
  let eventDeferred = Promise.defer();
  let timeoutMs = aTimeoutMs || kDefaultWait;
  let stack = new Error().stack;
  let timerID = setTimeout(function wfe_canceller() {
    aSubject.removeEventListener(aEventName, listener);
    eventDeferred.reject(new Error(aEventName + " event timeout at " + stack));
  }, timeoutMs);

  var listener = function (aEvent) {
    if (aTarget && aTarget !== aEvent.target)
        return;

    
    clearTimeout(timerID);
    eventDeferred.resolve(aEvent);
  };

  function cleanup(aEventOrError) {
    
    aSubject.removeEventListener(aEventName, listener);
    return aEventOrError;
  }
  aSubject.addEventListener(aEventName, listener, false);
  return eventDeferred.promise.then(cleanup, cleanup);
}

