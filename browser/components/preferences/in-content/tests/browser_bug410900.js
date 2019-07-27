


Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");

function test() {
  waitForExplicitFinish();
  Services.prefs.setBoolPref("browser.preferences.inContent", true);
  registerCleanupFunction(() => Services.prefs.clearUserPref("browser.preferences.inContent"));

  
  var handler = Cc["@mozilla.org/uriloader/web-handler-app;1"].
                createInstance(Ci.nsIWebHandlerApp);
  handler.name = "App pane alive test";
  handler.uriTemplate = "http://test.mozilla.org/%s";

  var extps = Cc["@mozilla.org/uriloader/external-protocol-service;1"].
              getService(Ci.nsIExternalProtocolService);
  var info = extps.getProtocolHandlerInfo("apppanetest");
  info.possibleApplicationHandlers.appendElement(handler, false);

  var hserv = Cc["@mozilla.org/uriloader/handler-service;1"].
              getService(Ci.nsIHandlerService);
  hserv.store(info);

  openPreferencesViaOpenPreferencesAPI("applications", null, {leaveOpen: true}).then(
      () => runTest(gBrowser.selectedBrowser.contentWindow)
  );
}

function runTest(win) {
  var rbox = win.document.getElementById("handlersView");
  ok(rbox, "handlersView is present");

  var items = rbox && rbox.getElementsByTagName("richlistitem");
  ok(items && items.length > 0, "App handler list populated");

  var handlerAdded = false;
  for (let i = 0; i < items.length; i++) {
    if (items[i].getAttribute('type') == "apppanetest")
      handlerAdded = true;
  }
  ok(handlerAdded, "apppanetest protocol handler was successfully added");

  gBrowser.removeCurrentTab();
  finish();
}
