
var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

function loadIntoWindow(window) {}
function unloadFromWindow(window) {}

function _sendMessageToJava (aMsg) {
  return Services.androidBridge.handleGeckoMessage(aMsg);
};




var windowListener = {
  onOpenWindow: function(aWindow) {
    
    let domWindow = aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowInternal || Ci.nsIDOMWindow);
    domWindow.addEventListener("load", function() {
      domWindow.removeEventListener("load", arguments.callee, false);
      if (domWindow) {
        domWindow.addEventListener("scroll", function(e) {
          let message = {
            type: 'robocop:scroll',
            y: XPCNativeWrapper.unwrap(e.target).documentElement.scrollTop,
            height: XPCNativeWrapper.unwrap(e.target).documentElement.scrollHeight,
            cheight: XPCNativeWrapper.unwrap(e.target).documentElement.clientHeight,
          };
          _sendMessageToJava(message);
        });
      }
    }, false);
  },
  onCloseWindow: function(aWindow) { },
  onWindowTitleChange: function(aWindow, aTitle) { }
};

function startup(aData, aReason) {
  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);

  
  wm.addListener(windowListener);
  Services.obs.addObserver(function observe(aSubject, aTopic, aData) {
      dump("Robocop:Quit received -- requesting quit");
      let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
      appStartup.quit(Ci.nsIAppStartup.eForceQuit);
  }, "Robocop:Quit", false);
}

function shutdown(aData, aReason) {
  
  if (aReason == APP_SHUTDOWN) return;

  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);

  
  wm.removeListener(windowListener);
}

function install(aData, aReason) { }
function uninstall(aData, aReason) { }

