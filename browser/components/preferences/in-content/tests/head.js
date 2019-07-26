


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
