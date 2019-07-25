



































EXPORTED_SYMBOLS = ["FeedbackManager"];

const Cc = Components.classes;
const Ci = Components.interfaces;

let Application = Cc["@mozilla.org/fuel/application;1"]
                  .getService(Ci.fuelIApplication);

var FeedbackManager = {
  _lastVisitedUrl: null,

  _happyUrl: null,
  get happyUrl() {
    if (!this._happyUrl) {
      this._happyUrl = Application.prefs.getValue("extensions.input.happyURL", "");
    }
    return this._happyUrl;
  },

  _sadUrl: null,
  get sadUrl() {
    if (!this._sadUrl) {
      this._sadUrl = Application.prefs.getValue("extensions.input.sadURL", "");
    }
    return this._sadUrl;
  },

  setCurrUrl: function FeedbackManager_setCurrUrl(url) {
    this._lastVisitedUrl = url;
  },

  fillInFeedbackPage: function FeedbackManager_fifp(url, window) {
    



    if (url == this.happyUrl || url == this.sadUrl) {
      let tabbrowser = window.getBrowser();
      let currentBrowser = tabbrowser.selectedBrowser;
      let document = currentBrowser.contentDocument;
      let field = document.getElementById("id_url");
      if (field && this._lastVisitedUrl) {
        field.value = this._lastVisitedUrl;
      }
    }
  }
};