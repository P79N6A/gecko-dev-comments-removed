




































var Feedback = {
  _prefs: [],

  init: function(aEvent) {
    let appInfo = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
    document.getElementById("feedback-about").setAttribute("desc", appInfo.version);

    
    messageManager.loadFrameScript("data:,addMessageListener('Feedback:InitPage', function(m) { content.document.getElementById('id_url').value = m.json.referrer; });", true);

    let feedbackPrefs = document.getElementById("feedback-tools").childNodes;
    for (let i = 0; i < feedbackPrefs.length; i++) {
      let pref = feedbackPrefs[i].getAttribute("pref");
      if (!pref)
        continue;

      let value = Services.prefs.getPrefType(pref) == Ci.nsIPrefBranch.PREF_INVALID ? false : Services.prefs.getBoolPref(pref);
      Feedback._prefs.push({ "name": pref, "value": value });
    }

    
    window.addEventListener("UIReadyDelayed", function(aEvent) {
      window.removeEventListener(aEvent.type, arguments.callee, false);
      document.getElementById("feedback-container").hidden = false;
    }, false);
  },

  openFeedback: function(aURL) {
    let currentURL = Browser.selectedBrowser.currentURI.spec;
    let newTab = BrowserUI.newTab(aURL);

    
    newTab.browser.messageManager.addMessageListener("DOMContentLoaded", function() {
      newTab.browser.messageManager.removeMessageListener("DOMContentLoaded", arguments.callee, true);
      newTab.browser.messageManager.sendAsyncMessage("Feedback:InitPage", { referrer: currentURL });
    });
  },

  openReadme: function() {
    let formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].getService(Ci.nsIURLFormatter);
    let url = formatter.formatURLPref("app.releaseNotesURL");
    BrowserUI.newTab(url);
  },

  updateRestart: function updateRestart() {
    let msg = document.getElementById("feedback-messages");
    if (msg) {
      let strings = Elements.browserBundle;

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

      let buttons = [ {
        label: strings.getString("notificationRestart.button"),
        accessKey: "",
        callback: restartCallback
      } ];
  
      let message = strings.getString("notificationRestart.normal");
      msg.appendNotification(message, value, "", msg.PRIORITY_WARNING_LOW, buttons);
    }
  }
};

window.addEventListener("load", Feedback.init, false);
