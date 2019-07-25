












































EXPORTED_SYMBOLS = ["TestPilotUIBuilder"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const UPDATE_CHANNEL_PREF = "app.update.channel";

var TestPilotUIBuilder = {
  __prefs: null,
  get _prefs() {
    this.__prefs = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefBranch);
    return this.__prefs;
  },

  buildTestPilotInterface: function(window) {
    
    let feedbackButton = window.document.getElementById("feedback-menu-button");
    if (!feedbackButton) {
      let toolbox = window.document.getElementById("navigator-toolbox");
      let palette = toolbox.palette;
      feedbackButton = palette.getElementsByAttribute("id", "feedback-menu-button").item(0);
    }
    feedbackButton.parentNode.removeChild(feedbackButton);
  },

  buildFeedbackInterface: function(window) {
    



    


    let firefoxnav = window.document.getElementById("nav-bar");
    let pref = "extensions.testpilot.alreadyCustomizedToolbar";
    let alreadyCustomized = this._prefs.getBoolPref(pref);
    let curSet = firefoxnav.currentSet;

    if (!alreadyCustomized && (-1 == curSet.indexOf("feedback-menu-button"))) {
      
      let newSet = curSet + ",feedback-menu-button";
      firefoxnav.setAttribute("currentset", newSet);
      firefoxnav.currentSet = newSet;
      window.document.persist("nav-bar", "currentset");
      this._prefs.setBoolPref(pref, true);
      
      try {
        window.BrowserToolboxCustomizeDone(true);
      } catch (e) {
      }
    }
  },

  isBetaChannel: function() {
    
    let channel = this._prefs.getCharPref(UPDATE_CHANNEL_PREF);
    return (channel == "beta") || (channel == "betatest") || (channel == "aurora");
  },

  appVersionIsFinal: function() {
    
    let appInfo = Cc["@mozilla.org/xre/app-info;1"]
      .getService(Ci.nsIXULAppInfo);
    let version = appInfo.version;
    let versionChecker = Components.classes["@mozilla.org/xpcom/version-comparator;1"]
      .getService(Components.interfaces.nsIVersionComparator);
    if (versionChecker.compare(version, "4.0") >= 0) {
      if (version.indexOf("b") == -1 && version.indexOf("rc") == -1) {
        return true;
      }
    }
    return false;
  },

  buildCorrectInterface: function(window) {
    let firefoxnav = window.document.getElementById("nav-bar");
    

    if (!firefoxnav) {
      return;
    }

    

    let self = this;
    if (this.isBetaChannel()) {
      window.document.loadOverlay("chrome://testpilot/content/feedback-browser.xul",
                                  {observe: function(subject, topic, data) {
                                     if (topic == "xul-overlay-merged") {
                                       self.buildFeedbackInterface(window);
                                     }
                                   }});
    } else {
      window.document.loadOverlay("chrome://testpilot/content/tp-browser.xul",
                                  {observe: function(subject, topic, data) {
                                     if (topic == "xul-overlay-merged") {
                                       self.buildTestPilotInterface(window);
                                     }
                                  }});
    }
  }
};