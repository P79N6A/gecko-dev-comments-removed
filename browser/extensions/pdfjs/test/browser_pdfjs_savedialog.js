


const RELATIVE_DIR = "browser/extensions/pdfjs/test/";
const TESTROOT = "http://example.com/browser/" + RELATIVE_DIR;

function test() {
  var oldAction = changeMimeHandler(Ci.nsIHandlerInfo.useSystemDefault, true);
  var tab = gBrowser.addTab(TESTROOT + "file_pdfjs_test.pdf");
  
  
  
  
  addWindowListener('chrome://mozapps/content/downloads/unknownContentType.xul', finish);

  waitForExplicitFinish();
  registerCleanupFunction(function() {
    changeMimeHandler(oldAction[0], oldAction[1]);
    gBrowser.removeTab(tab);
  });
}

function changeMimeHandler(preferredAction, alwaysAskBeforeHandling) {
  let handlerService = Cc["@mozilla.org/uriloader/handler-service;1"].getService(Ci.nsIHandlerService);
  let mimeService = Cc["@mozilla.org/mime;1"].getService(Ci.nsIMIMEService);
  let handlerInfo = mimeService.getFromTypeAndExtension('application/pdf', 'pdf');
  var oldAction = [handlerInfo.preferredAction, handlerInfo.alwaysAskBeforeHandling];

  
  handlerInfo.alwaysAskBeforeHandling = alwaysAskBeforeHandling;
  handlerInfo.preferredAction = preferredAction;
  handlerService.store(handlerInfo);

  Services.obs.notifyObservers(null, 'pdfjs:handlerChanged', null);

  
  handlerInfo = mimeService.getFromTypeAndExtension('application/pdf', 'pdf');

  
  
  
  is(handlerInfo.alwaysAskBeforeHandling, alwaysAskBeforeHandling, 'always-ask prompt change successful');
  is(handlerInfo.preferredAction, preferredAction, 'mime handler change successful');

  return oldAction;
}

function addWindowListener(aURL, aCallback) {
  Services.wm.addListener({
    onOpenWindow: function(aXULWindow) {
      info("window opened, waiting for focus");
      Services.wm.removeListener(this);

      var domwindow = aXULWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDOMWindow);
      waitForFocus(function() {
        is(domwindow.document.location.href, aURL, "should have seen the right window open");
        domwindow.close();
        aCallback();
      }, domwindow);
    },
    onCloseWindow: function(aXULWindow) { },
    onWindowTitleChange: function(aXULWindow, aNewTitle) { }
  });
}
