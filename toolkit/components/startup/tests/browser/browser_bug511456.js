




































const Cc = Components.classes;
const Ci = Components.interfaces;

const PROMPT_URL = "chrome://global/content/commonDialog.xul";
const TEST_URL = "http://example.com/browser/toolkit/components/startup/tests/browser/beforeunload.html";

var Watcher = {
  seen: false,
  allowClose: false,

  
  windowLoad: function(win) {
    
    var self = this;
    executeSoon(function() { self.windowReady(win); } );
  },

  windowReady: function(win) {
    if (win.document.location.href != PROMPT_URL)
      return;
    this.seen = true;
    if (this.allowClose)
      win.document.documentElement.acceptDialog();
    else
      win.document.documentElement.cancelDialog();
  },

  

  onWindowTitleChange: function(win, title) {
  },

  onOpenWindow: function(win) {
    var domwindow = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                       .getInterface(Components.interfaces.nsIDOMWindowInternal);
    var self = this;
    domwindow.addEventListener("load", function() {
      self.windowLoad(domwindow);
    }, false);
  },

  onCloseWindow: function(win) {
  },

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIWindowMediatorListener) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

function test() {
  waitForExplicitFinish();

  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  wm.addListener(Watcher);

  var win2 = OpenBrowserWindow();
  win2.addEventListener("load", function() {
    win2.removeEventListener("load", arguments.callee, false);
    gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
    gBrowser.addEventListener("load", function() {
      if (window.content.location.href != TEST_URL)
        return;
      gBrowser.removeEventListener("load", arguments.callee, false);
      Watcher.seen = false;
      var appStartup = Cc['@mozilla.org/toolkit/app-startup;1'].
                       getService(Ci.nsIAppStartup);
      appStartup.quit(Ci.nsIAppStartup.eAttemptQuit);
      Watcher.allowClose = true;
      ok(Watcher.seen, "Should have seen a prompt dialog");
      ok(!win2.closed, "Shouldn't have closed the additional window");
      win2.close();
      gBrowser.removeTab(gBrowser.selectedTab);
      executeSoon(finish_test);
    }, false);
  }, false);
}

function finish_test() {
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  wm.removeListener(Watcher);
  finish();
}
