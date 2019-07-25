



































const Ci = Components.interfaces;
const Cu = Components.utils;
const Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");






function BlocklistPrompt() { }

BlocklistPrompt.prototype = {
  prompt: function(aAddons, aCount) {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    if (win.ExtensionsView.visible) {
      win.ExtensionsView.showRestart("blocked");
    } else {
      let bundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
      let notifyBox = win.getNotificationBox();
      let restartCallback = function(aNotification, aDescription) {
        
        var cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
        Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");
  
        
        if (cancelQuit.data == false) {
          let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
          appStartup.quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
        }
      };
      
      let buttons = [{accessKey: null,
                      label: bundle.GetStringFromName("notificationRestart.button"),
                      callback: restartCallback}];
      notifyBox.appendNotification(bundle.GetStringFromName("notificationRestart.blocked"),
                                   "blocked-add-on",
                                   "",
                                   "PRIORITY_CRITICAL_HIGH",
                                   buttons);
    }
    
    for (let i = 0; i < aAddons.length; i++) {
      if (aAddons[i].item instanceof Ci.nsIPluginTag)
        addonList[i].item.disabled = true;
      else
        aAddons[i].item.userDisabled = true;
    }
  },
  classID: Components.ID("{4e6ea350-b09a-11df-94e2-0800200c9a66}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBlocklistPrompt])
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([BlocklistPrompt]);

