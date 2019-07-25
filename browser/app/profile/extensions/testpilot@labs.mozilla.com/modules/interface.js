












































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
    
    return (this._prefs.getCharPref(UPDATE_CHANNEL_PREF) == "beta");
  },

  buildCorrectInterface: function(window) {
    let firefoxnav = window.document.getElementById("nav-bar");
    

    if (!firefoxnav) {
      return;
    }

    

    if (this.isBetaChannel()) {
      let self = this;
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