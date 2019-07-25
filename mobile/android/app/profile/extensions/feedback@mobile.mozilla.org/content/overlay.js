




































var Feedback = {
  _prefs: [],
  _device: "",
  _manufacturer: "",

  init: function(aEvent) {
    
    let panel = document.getElementById("feedback-container");
    panel.addEventListener("ToolPanelShown", function delayedInit(aEvent) {
      panel.removeEventListener("ToolPanelShown", delayedInit, false);

      
      messageManager.loadFrameScript("chrome://feedback/content/content.js", true);

      let setting = document.getElementById("feedback-checkCompatibility");
      setting.setAttribute("pref", Feedback.compatibilityPref);
      setting.preferenceChanged();

      document.getElementById("feedback-container").hidden = false;

      let feedbackPrefs = document.getElementById("feedback-tools").childNodes;
      for (let i = 0; i < feedbackPrefs.length; i++) {
        let pref = feedbackPrefs[i].getAttribute("pref");
        if (!pref)
          continue;
  
        let value = Services.prefs.getPrefType(pref) == Ci.nsIPrefBranch.PREF_INVALID ? false : Services.prefs.getBoolPref(pref);
        Feedback._prefs.push({ "name": pref, "value": value });
      }

      let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
      Feedback._device = sysInfo.get("device");
      Feedback._manufacturer = sysInfo.get("manufacturer");
    }, false);
  },

  get compatibilityPref() {
    let result = "extensions.checkCompatibility.";
    let channel = Services.prefs.getCharPref("app.update.channel");
    if (channel == "nightly") {
      result += "nightly";
    } else {
      
      const BRANCH_REGEXP = /^([^\.]+\.[0-9]+[a-z]*).*/gi;
      result += Services.appinfo.version.replace(BRANCH_REGEXP, "$1");
    }
    delete this.compatibilityPref;
    return this.compatibilityPref = result;
  },

  openFeedback: function(aName) {
    let pref = "extensions.feedback.url." + aName;
    let url = Services.prefs.getPrefType(pref) == Ci.nsIPrefBranch.PREF_INVALID ? "" : Services.prefs.getCharPref(pref);
    if (!url)
      return;

    let currentURL = Browser.selectedBrowser.currentURI.spec;
    let newTab = BrowserUI.newTab(url, Browser.selectedTab);

    
    newTab.browser.messageManager.addMessageListener("DOMContentLoaded", function() {
      newTab.browser.messageManager.removeMessageListener("DOMContentLoaded", arguments.callee, true);
      newTab.browser.messageManager.sendAsyncMessage("Feedback:InitPage", { referrer: currentURL, device: Feedback._device, manufacturer: Feedback._manufacturer });
    });
  },

  updateRestart: function updateRestart() {
    let msg = document.getElementById("feedback-messages");
    if (msg) {
      let value = "restart-app";
      let notification = msg.getNotificationWithValue(value);
      if (notification) {
        
        
        for each (let pref in this._prefs) {
          let value = Services.prefs.getPrefType(pref.name) == Ci.nsIPrefBranch.PREF_INVALID ? false : Services.prefs.getBoolPref(pref.name);
          if (value != pref.value)
            return;
        }

        notification.close();
        return;
      }
  
      let restartCallback = function(aNotification, aDescription) {
        
        let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
        Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");
  
        
        if (cancelQuit.data == false) {
          let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
          appStartup.quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
        }
      };

      let strings = Strings.browser;

      let buttons = [ {
        label: strings.GetStringFromName("notificationRestart.button"),
        accessKey: "",
        callback: restartCallback
      } ];
  
      let message = strings.GetStringFromName("notificationRestart.normal");
      msg.appendNotification(message, value, "", msg.PRIORITY_WARNING_LOW, buttons);
    }
  }
};

window.addEventListener("load", Feedback.init, false);
