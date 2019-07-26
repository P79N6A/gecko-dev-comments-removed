



"use strict";

this.EXPORTED_SYMBOLS = ["Translation"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

const TRANSLATION_PREF_SHOWUI = "browser.translation.ui.show";

Cu.import("resource://gre/modules/Services.jsm");

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
    if (!Services.prefs.getBoolPref(TRANSLATION_PREF_SHOWUI))
      return;

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
  aBrowser.messageManager.addMessageListener("Translation:Finished", this);
}

TranslationUI.prototype = {
  STATE_OFFER: 0,
  STATE_TRANSLATING: 1,
  STATE_TRANSLATED: 2,
  STATE_ERROR: 3,

  translate: function(aFrom, aTo) {
    this.state = this.STATE_TRANSLATING;
    this.translatedFrom = aFrom;
    this.translatedTo = aTo;

    this.browser.messageManager.sendAsyncMessage(
      "Translation:TranslateDocument",
      { from: aFrom, to: aTo }
    );
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
      let notification = this.notificationBox.getNotificationWithValue("translation");
      if (notification)
        notification.close();
      else
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
    this.browser.messageManager.sendAsyncMessage("Translation:ShowOriginal");
  },

  showTranslatedContent: function() {
    this.showURLBarIcon(true);
    this.originalShown = false;
    this.browser.messageManager.sendAsyncMessage("Translation:ShowTranslation");
  },

  get notificationBox() this.browser.ownerGlobal.gBrowser.getNotificationBox(),

  showTranslationInfoBar: function() {
    let notificationBox = this.notificationBox;
    let notif = notificationBox.appendNotification("", "translation", null,
                                                   notificationBox.PRIORITY_INFO_HIGH);
    notif.init(this);
    return notif;
  },

  shouldShowInfoBar: function(aURI, aDetectedLanguage) {
    
    let neverForLangs =
      Services.prefs.getCharPref("browser.translation.neverForLanguages");
    if (neverForLangs.split(",").indexOf(aDetectedLanguage) != -1)
      return false;

    
    let perms = Services.perms;
    return perms.testExactPermission(aURI, "translate") != perms.DENY_ACTION;
  },

  showTranslationUI: function(aDetectedLanguage) {
    this.detectedLanguage = aDetectedLanguage;

    
    this.state = 0;
    this.translatedFrom = "";
    this.translatedTo = "";
    this.originalShown = true;

    this.showURLBarIcon();

    if (!this.shouldShowInfoBar(this.browser.currentURI, aDetectedLanguage))
      return null;

    return this.showTranslationInfoBar();
  },

  receiveMessage: function(msg) {
    switch (msg.name) {
      case "Translation:Finished":
        if (msg.data.success) {
          this.state = this.STATE_TRANSLATED;
          this.showURLBarIcon(true);
          this.originalShown = false;
        } else {
          this.state = this.STATE_ERROR;
        }
        break;
    }
  }
};
