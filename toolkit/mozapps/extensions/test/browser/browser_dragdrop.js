











var gManagerWindow;
var chromeUtils = {};
this._scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                     getService(Ci.mozIJSSubScriptLoader);
this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/ChromeUtils.js", chromeUtils);




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

function test() {
  waitForExplicitFinish();

  open_manager("addons://list/extension", function(aWindow) {
    gManagerWindow = aWindow;
    run_next_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, function() {
    finish();
  });
}

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


add_test(function() {
  var url = TESTROOT + "addons/browser_dragdrop1.xpi";

  new WindowOpenListener(INSTALL_URI, function(aWindow) {
    test_confirmation(aWindow, [url]);
  }, run_next_test);

  var viewContainer = gManagerWindow.document.getElementById("view-port");
  var effect = chromeUtils.synthesizeDrop(viewContainer, viewContainer,
               [[{type: "text/x-moz-url", data: url}]],
               "copy", gManagerWindow, EventUtils);
  is(effect, "copy", "Drag should be accepted");
});


add_test(function() {
  var fileurl = get_addon_file_url("browser_dragdrop1.xpi");

  new WindowOpenListener(INSTALL_URI, function(aWindow) {
    test_confirmation(aWindow, [fileurl.spec]);
  }, run_next_test);

  var viewContainer = gManagerWindow.document.getElementById("view-port");
  var effect = chromeUtils.synthesizeDrop(viewContainer, viewContainer,
               [[{type: "application/x-moz-file", data: fileurl.file}]],
               "copy", gManagerWindow, EventUtils);
  is(effect, "copy", "Drag should be accepted");
});


add_test(function() {
  var url1 = TESTROOT + "addons/browser_dragdrop1.xpi";
  var url2 = TESTROOT2 + "addons/browser_dragdrop2.xpi";

  new WindowOpenListener(INSTALL_URI, function(aWindow) {
    test_confirmation(aWindow, [url1, url2]);
  }, run_next_test);

  var viewContainer = gManagerWindow.document.getElementById("view-port");
  var effect = chromeUtils.synthesizeDrop(viewContainer, viewContainer,
               [[{type: "text/x-moz-url", data: url1}],
                [{type: "text/x-moz-url", data: url2}]],
               "copy", gManagerWindow, EventUtils);
  is(effect, "copy", "Drag should be accepted");
});


add_test(function() {
  var fileurl1 = get_addon_file_url("browser_dragdrop1.xpi");
  var fileurl2 = get_addon_file_url("browser_dragdrop2.xpi");

  new WindowOpenListener(INSTALL_URI, function(aWindow) {
    test_confirmation(aWindow, [fileurl1.spec, fileurl2.spec]);
  }, run_next_test);

  var viewContainer = gManagerWindow.document.getElementById("view-port");
  var effect = chromeUtils.synthesizeDrop(viewContainer, viewContainer,
               [[{type: "application/x-moz-file", data: fileurl1.file}],
                [{type: "application/x-moz-file", data: fileurl2.file}]],
               "copy", gManagerWindow, EventUtils);
  is(effect, "copy", "Drag should be accepted");
});


add_test(function() {
  var url = TESTROOT + "addons/browser_dragdrop1.xpi";
  var fileurl = get_addon_file_url("browser_dragdrop2.xpi");

  new WindowOpenListener(INSTALL_URI, function(aWindow) {
    test_confirmation(aWindow, [url, fileurl.spec]);
  }, run_next_test);

  var viewContainer = gManagerWindow.document.getElementById("view-port");
  var effect = chromeUtils.synthesizeDrop(viewContainer, viewContainer,
               [[{type: "text/x-moz-url", data: url}],
                [{type: "application/x-moz-file", data: fileurl.file}]],
               "copy", gManagerWindow, EventUtils);
  is(effect, "copy", "Drag should be accepted");
});
