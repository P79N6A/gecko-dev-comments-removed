












EXPORTED_SYMBOLS = ["TestPilotUIBuilder"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const UPDATE_CHANNEL_PREF = "app.update.channel";
const POPUP_SHOW_ON_NEW = "extensions.testpilot.popup.showOnNewStudy";
const POPUP_CHECK_INTERVAL = "extensions.testpilot.popup.delayAfterStartup";

var TestPilotUIBuilder = {
  get _prefs() {
    delete this._prefs;
    return this._prefs = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefBranch);
  },

  get _prefDefaultBranch() {
    delete this._prefDefaultBranch;
    return this._prefDefaultBranch = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefService).getDefaultBranch("");
  },

  get _comparator() {
    delete this._comparator;
    return this._comparator = Cc["@mozilla.org/xpcom/version-comparator;1"]
      .getService(Ci.nsIVersionComparator);
  },

  get _appVersion() {
    delete this._appVersion;
    return this._appVersion = Cc["@mozilla.org/xre/app-info;1"]
      .getService(Ci.nsIXULAppInfo).version;
  },

  buildTestPilotInterface: function(window) {
    
    let feedbackButton = window.document.getElementById("feedback-menu-button");
    if (!feedbackButton) {
      let toolbox = window.document.getElementById("navigator-toolbox");
      let palette = toolbox.palette;
      feedbackButton = palette.getElementsByAttribute("id", "feedback-menu-button").item(0);
    }
    feedbackButton.parentNode.removeChild(feedbackButton);

    


    this._prefDefaultBranch.setBoolPref(POPUP_SHOW_ON_NEW, false);
    this._prefDefaultBranch.setIntPref(POPUP_CHECK_INTERVAL, 180000);
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

    


    this._prefDefaultBranch.setBoolPref(POPUP_SHOW_ON_NEW, true);
    this._prefDefaultBranch.setIntPref(POPUP_CHECK_INTERVAL, 600000);
  },

  channelUsesFeedback: function() {
    
    let channel = this._prefDefaultBranch.getCharPref(UPDATE_CHANNEL_PREF);
    return (channel == "beta") || (channel == "betatest") || (channel == "aurora");
  },

  appVersionIsFinal: function() {
    
    if (this._comparator.compare(this._appVersion, "4.0") >= 0) {
      if (this._appVersion.indexOf("b") == -1 && this._appVersion.indexOf("rc") == -1) {
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

    

    if (this.channelUsesFeedback()) {
      window.document.loadOverlay("chrome://testpilot/content/feedback-browser.xul", null);
      this.buildFeedbackInterface(window);
    } else {
      window.document.loadOverlay("chrome://testpilot/content/tp-browser.xul", null);
      this.buildTestPilotInterface(window);
    }
  }
};