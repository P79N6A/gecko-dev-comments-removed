



"use strict";

this.EXPORTED_SYMBOLS = ["Translation"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.Translation = {
  supportedSourceLanguages: ["en", "zh", "ja", "es", "de", "fr", "ru", "ar", "ko", "pt"],
  supportedTargetLanguages: ["en", "pl", "tr", "vi"],

  _defaultTargetLanguage: "",
  get defaultTargetLanguage() {
    if (!this._defaultTargetLanguage) {
      this._defaultTargetLanguage = Cc["@mozilla.org/chrome/chrome-registry;1"]
                                      .getService(Ci.nsIXULChromeRegistry)
                                      .getSelectedLocale("global")
                                      .split("-")[0];
    }
    return this._defaultTargetLanguage;
  },

  languageDetected: function(aBrowser, aDetectedLanguage) {
    if (this.supportedSourceLanguages.indexOf(aDetectedLanguage) != -1 &&
        aDetectedLanguage != this.defaultTargetLanguage) {
      if (!aBrowser.translationUI)
        aBrowser.translationUI = new TranslationUI(aBrowser);

      aBrowser.translationUI.showTranslationUI(aDetectedLanguage);
    }
  }
};

















function TranslationUI(aBrowser) {
  this.browser = aBrowser;
}

TranslationUI.prototype = {
  STATE_OFFER: 0,
  STATE_TRANSLATING: 1,
  STATE_TRANSLATED: 2,
  STATE_ERROR: 3,

  get doc() this.browser.contentDocument,

  translate: function(aFrom, aTo) {
    this.state = this.STATE_TRANSLATING;
    this.translatedFrom = aFrom;
    this.translatedTo = aTo;
  },

  showURLBarIcon: function(aTranslated) {
    let chromeWin = this.browser.ownerGlobal;
    let PopupNotifications = chromeWin.PopupNotifications;
    let removeId = aTranslated ? "translate" : "translated";
    let notification =
      PopupNotifications.getNotification(removeId, this.browser);
    if (notification)
      PopupNotifications.remove(notification);

    let callback = aTopic => {
      if (aTopic != "showing")
        return false;
      if (!this.notificationBox.getNotificationWithValue("translation"))
        this.showTranslationInfoBar();
      return true;
    };

    let addId = aTranslated ? "translated" : "translate";
    PopupNotifications.show(this.browser, addId, null,
                            addId + "-notification-icon", null, null,
                            {dismissed: true, eventCallback: callback});
  },

  _state: 0,
  get state() this._state,
  set state(val) {
    let notif = this.notificationBox.getNotificationWithValue("translation");
    if (notif)
      notif.state = val;
    this._state = val;
  },

  originalShown: true,
  showOriginalContent: function() {
    this.showURLBarIcon();
    this.originalShown = true;
  },

  showTranslatedContent: function() {
    this.showURLBarIcon(true);
    this.originalShown = false;
  },

  get notificationBox() this.browser.ownerGlobal.gBrowser.getNotificationBox(),

  showTranslationInfoBar: function() {
    let notificationBox = this.notificationBox;
    let notif = notificationBox.appendNotification("", "translation", null,
                                                   notificationBox.PRIORITY_INFO_HIGH);
    notif.init(this);
    return notif;
  },

  showTranslationUI: function(aDetectedLanguage) {
    this.detectedLanguage = aDetectedLanguage;

    
    this.state = 0;
    this.translatedFrom = "";
    this.translatedTo = "";
    this.originalShown = true;

    this.showURLBarIcon();
    return this.showTranslationInfoBar();
  }
};
