Components.utils.import("resource://gre/modules/FileUtils.jsm");        

function loadIntoWindow(window) {}
function unloadFromWindow(window) {}

function setDefaultPrefs() {
    
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                getService(Components.interfaces.nsIPrefService);
    var branch = prefs.getDefaultBranch("");

#include reftest-preferences.js
}

var windowListener = {
    onOpenWindow: function(aWindow) {
        let domWindow = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindowInternal || Components.interfaces.nsIDOMWindow);
        domWindow.addEventListener("load", function() {
            domWindow.removeEventListener("load", arguments.callee, false);

            let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);

            
            let enumerator = wm.getEnumerator("navigator:browser");
            while (enumerator.hasMoreElements()) {
                let win = enumerator.getNext().QueryInterface(Components.interfaces.nsIDOMWindow);
                setDefaultPrefs();
                Components.utils.import("chrome://reftest/content/reftest.jsm");
                win.addEventListener("pageshow", function() {
                    win.removeEventListener("pageshow", arguments.callee); 
                    
                    win.setTimeout(function () {OnRefTestLoad(win);}, 0);
                });
                break;
            }
        }, false);
   },
   onCloseWindow: function(aWindow){ },
   onWindowTitleChange: function(){ },
};

function startup(aData, aReason) {
    let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].
             getService (Components.interfaces.nsIWindowMediator);

    Components.manager.addBootstrappedManifestLocation(aData.installPath);

    
    wm.addListener(windowListener);
}

function shutdown(aData, aReason) {
    
    if (aReason == APP_SHUTDOWN)
        return;

    let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].
             getService(Components.interfaces.nsIWindowMediator);

    
    wm.removeListener(windowListener);

    
    let enumerator = wm.getEnumerator("navigator:browser");
    while (enumerator.hasMoreElements()) {
        let win = enumerator.getNext().QueryInterface(Components.interfaces.nsIDOMWindow);
        unloadFromWindow(win);
    }
}

function install(aData, aReason) { }
function uninstall(aData, aReason) { }

