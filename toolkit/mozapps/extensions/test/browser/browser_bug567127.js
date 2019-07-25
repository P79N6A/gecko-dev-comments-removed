





var MockFilePicker = SpecialPowers.MockFilePicker;
MockFilePicker.reset();

var gManagerWindow;
var gSawInstallNotification = false;




function WindowOpenListener(url, opencallback, closecallback) {
  this.url = url;
  this.opencallback = opencallback;
  this.closecallback = closecallback;

  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  wm.addListener(this);
}

WindowOpenListener.prototype = {
  url: null,
  opencallback: null,
  closecallback: null,
  window: null,
  domwindow: null,

  handleEvent: function(event) {
    is(this.domwindow.document.location.href, this.url, "Should have opened the correct window");

    this.domwindow.removeEventListener("load", this, false);
    
    var self = this;
    executeSoon(function() { self.opencallback(self.domwindow); } );
  },

  onWindowTitleChange: function(window, title) {
  },

  onOpenWindow: function(window) {
    if (this.window)
      return;

    this.window = window;
    this.domwindow = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                           .getInterface(Components.interfaces.nsIDOMWindow);
    this.domwindow.addEventListener("load", this, false);
  },

  onCloseWindow: function(window) {
    if (this.window != window)
      return;

    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    wm.removeListener(this);
    this.opencallback = null;
    this.window = null;
    this.domwindow = null;

    
    executeSoon(this.closecallback);
    this.closecallback = null;
  }
};


var gInstallNotificationObserver = {
  observe: function(aSubject, aTopic, aData) {
    var installInfo = aSubject.QueryInterface(Ci.amIWebInstallInfo);
    isnot(installInfo.originatingWindow, null, "Notification should have non-null originatingWindow");
    gSawInstallNotification = true;
    Services.obs.removeObserver(this, "addon-install-started");
  }
};


function test_confirmation(aWindow, aExpectedURLs) {
  var list = aWindow.document.getElementById("itemList");
  is(list.childNodes.length, aExpectedURLs.length, "Should be the right number of installs");

  aExpectedURLs.forEach(function(aURL) {
    var node = list.firstChild;
    while (node) {
      if (node.url == aURL) {
        ok(true, "Should have seen " + aURL + " in the list");
        return;
      }
      node = node.nextSibling;
    }
    ok(false, "Should have seen " + aURL + " in the list");
  });

  aWindow.document.documentElement.cancelDialog();
}


function test() {
  waitForExplicitFinish();
  
  open_manager("addons://list/extension", function(aWindow) {
    gManagerWindow = aWindow;
    run_next_test();
  });
}

function end_test() {
  is(gSawInstallNotification, true, "Should have seen addon-install-started notification.");

  MockFilePicker.reset();
  close_manager(gManagerWindow, function() {
    finish();
  });
}


add_test(function() {
  var filePaths = [
                   get_addon_file_url("browser_bug567127_1.xpi"),
                   get_addon_file_url("browser_bug567127_2.xpi")
                  ];
  MockFilePicker.returnFiles = filePaths.map(function(aPath) aPath.file);
  
  Services.obs.addObserver(gInstallNotificationObserver,
                           "addon-install-started", false);

  new WindowOpenListener(INSTALL_URI, function(aWindow) {
    test_confirmation(aWindow, filePaths.map(function(aPath) aPath.spec));
  }, run_next_test);

  gManagerWindow.gViewController.doCommand("cmd_installFromFile");
});
