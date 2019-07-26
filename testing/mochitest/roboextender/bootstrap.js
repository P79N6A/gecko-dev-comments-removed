
var Cc = Components.classes;
var Ci = Components.interfaces;

function loadIntoWindow(window) {}
function unloadFromWindow(window) {}

function _sendMessageToJava (aMsg) {
  let bridge = Cc["@mozilla.org/android/bridge;1"].getService(Ci.nsIAndroidBridge);
  return bridge.handleGeckoMessage(JSON.stringify(aMsg));
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
          let retVal = _sendMessageToJava(message);
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
}

function shutdown(aData, aReason) {
  
  if (aReason == APP_SHUTDOWN) return;

  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
  let obs = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);

  
  wm.removeListener(windowListener);
}

function install(aData, aReason) { }
function uninstall(aData, aReason) { }
