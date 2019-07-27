








var EXPORTED_SYMBOLS = ["goQuitApplication"];

Components.utils.import("resource://gre/modules/Services.jsm");

function canQuitApplication() {
  try {
    var cancelQuit = Components.classes["@mozilla.org/supports-PRBool;1"]
                     .createInstance(Components.interfaces.nsISupportsPRBool);
    Services.obs.notifyObservers(cancelQuit, "quit-application-requested", null);

    
    if (cancelQuit.data) {
      return false;
    }
  }
  catch (ex) {}

  return true;
}

function goQuitApplication() {
  if (!canQuitApplication()) {
    return false;
  }

  const kAppStartup = '@mozilla.org/toolkit/app-startup;1';
  const kAppShell   = '@mozilla.org/appshell/appShellService;1';
  var   appService;
  var   forceQuit;

  if (kAppStartup in Components.classes) {
    appService = Components.classes[kAppStartup]
                 .getService(Components.interfaces.nsIAppStartup);
    forceQuit  = Components.interfaces.nsIAppStartup.eForceQuit;
  }
  else if (kAppShell in Components.classes) {
    appService = Components.classes[kAppShell].
      getService(Components.interfaces.nsIAppShellService);
    forceQuit = Components.interfaces.nsIAppShellService.eForceQuit;
  }
  else {
    throw new Error('goQuitApplication: no AppStartup/appShell');
  }

  try {
    appService.quit(forceQuit);
  }
  catch(ex) {
    throw new Error('goQuitApplication: ' + ex);
  }

  return true;
}

