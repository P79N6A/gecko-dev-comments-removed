



"use strict";

this.EXPORTED_SYMBOLS = [
  "Translation",
  "TranslationProvider",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

const TRANSLATION_PREF_SHOWUI = "browser.translation.ui.show";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Metrics.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);

const DAILY_COUNTER_FIELD = {type: Metrics.Storage.FIELD_DAILY_COUNTER};
const DAILY_LAST_TEXT_FIELD = {type: Metrics.Storage.FIELD_DAILY_LAST_TEXT};
const DAILY_LAST_NUMERIC_FIELD = {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC};


this.Translation = {
  STATE_OFFER: 0,
  STATE_TRANSLATING: 1,
  STATE_TRANSLATED: 2,
  STATE_ERROR: 3,
  STATE_UNAVAILABLE: 4,

  serviceUnavailable: false,

  supportedSourceLanguages: ["de", "en", "es", "fr", "ja", "ko", "pt", "ru", "zh"],
  supportedTargetLanguages: ["de", "en", "es", "fr", "ja", "ko", "pl", "pt", "ru", "tr", "vi", "zh"],

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

  documentStateReceived: function(aBrowser, aData) {
    if (aData.state == this.STATE_OFFER) {
      if (aData.detectedLanguage == this.defaultTargetLanguage) {
        
        return;
      }

      if (this.supportedSourceLanguages.indexOf(aData.detectedLanguage) == -1) {
        
        TranslationHealthReport.recordMissedTranslationOpportunity(aData.detectedLanguage);
        return;
      }

      TranslationHealthReport.recordTranslationOpportunity(aData.detectedLanguage);
    }

    if (!Services.prefs.getBoolPref(TRANSLATION_PREF_SHOWUI))
      return;

    if (!aBrowser.translationUI)
      aBrowser.translationUI = new TranslationUI(aBrowser);
    let trUI = aBrowser.translationUI;

    
    trUI._state = Translation.serviceUnavailable ? Translation.STATE_UNAVAILABLE
                                                 : aData.state;
    trUI.detectedLanguage = aData.detectedLanguage;
    trUI.translatedFrom = aData.translatedFrom;
    trUI.translatedTo = aData.translatedTo;
    trUI.originalShown = aData.originalShown;

    trUI.showURLBarIcon();

    if (trUI.shouldShowInfoBar(aBrowser.currentURI))
      trUI.showTranslationInfoBar();
  },

  openProviderAttribution: function() {
    Cu.import("resource:///modules/RecentWindow.jsm");
    RecentWindow.getMostRecentBrowserWindow().openUILinkIn(
      "http://aka.ms/MicrosoftTranslatorAttribution", "tab");
  }
};
















function TranslationUI(aBrowser) {
  this.browser = aBrowser;
}

TranslationUI.prototype = {
  get browser() this._browser,
  set browser(aBrowser) {
    if (this._browser)
      this._browser.messageManager.removeMessageListener("Translation:Finished", this);
    aBrowser.messageManager.addMessageListener("Translation:Finished", this);
    this._browser = aBrowser;
  },
  translate: function(aFrom, aTo) {
    if (aFrom == aTo ||
        (this.state == Translation.STATE_TRANSLATED &&
         this.translatedFrom == aFrom && this.translatedTo == aTo)) {
      
      return;
    }

    if (this.state == Translation.STATE_OFFER) {
      if (this.detectedLanguage != aFrom)
        TranslationHealthReport.recordDetectedLanguageChange(true);
    } else {
      if (this.translatedFrom != aFrom)
        TranslationHealthReport.recordDetectedLanguageChange(false);
      if (this.translatedTo != aTo)
        TranslationHealthReport.recordTargetLanguageChange();
    }

    this.state = Translation.STATE_TRANSLATING;
    this.translatedFrom = aFrom;
    this.translatedTo = aTo;

    this.browser.messageManager.sendAsyncMessage(
      "Translation:TranslateDocument",
      { from: aFrom, to: aTo }
    );
  },

  showURLBarIcon: function() {
    let chromeWin = this.browser.ownerGlobal;
    let PopupNotifications = chromeWin.PopupNotifications;
    let removeId = this.originalShown ? "translated" : "translate";
    let notification =
      PopupNotifications.getNotification(removeId, this.browser);
    if (notification)
      PopupNotifications.remove(notification);

    let callback = (aTopic, aNewBrowser) => {
      if (aTopic == "swapping") {
        let infoBarVisible =
          this.notificationBox.getNotificationWithValue("translation");
        aNewBrowser.translationUI = this;
        this.browser = aNewBrowser;
        if (infoBarVisible)
          this.showTranslationInfoBar();
        return true;
      }

      if (aTopic != "showing")
        return false;
      let notification = this.notificationBox.getNotificationWithValue("translation");
      if (notification)
        notification.close();
      else
        this.showTranslationInfoBar();
      return true;
    };

    let addId = this.originalShown ? "translate" : "translated";
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
    this.originalShown = true;
    this.showURLBarIcon();
    this.browser.messageManager.sendAsyncMessage("Translation:ShowOriginal");
    TranslationHealthReport.recordShowOriginalContent();
  },

  showTranslatedContent: function() {
    this.originalShown = false;
    this.showURLBarIcon();
    this.browser.messageManager.sendAsyncMessage("Translation:ShowTranslation");
  },

  get notificationBox() this.browser.ownerGlobal.gBrowser.getNotificationBox(this.browser),

  showTranslationInfoBar: function() {
    let notificationBox = this.notificationBox;
    let notif = notificationBox.appendNotification("", "translation", null,
                                                   notificationBox.PRIORITY_INFO_HIGH);
    notif.init(this);
    return notif;
  },

  shouldShowInfoBar: function(aURI) {
    
    
    if (Translation.serviceUnavailable)
      return false;

    
    let neverForLangs =
      Services.prefs.getCharPref("browser.translation.neverForLanguages");
    if (neverForLangs.split(",").indexOf(this.detectedLanguage) != -1) {
      TranslationHealthReport.recordAutoRejectedTranslationOffer();
      return false;
    }

    
    let perms = Services.perms;
    if (perms.testExactPermission(aURI, "translate") ==  perms.DENY_ACTION) {
      TranslationHealthReport.recordAutoRejectedTranslationOffer();
      return false;
    }

    return true;
  },

  receiveMessage: function(msg) {
    switch (msg.name) {
      case "Translation:Finished":
        if (msg.data.success) {
          this.originalShown = false;
          this.state = Translation.STATE_TRANSLATED;
          this.showURLBarIcon();

          
          TranslationHealthReport.recordTranslation(msg.data.from, msg.data.to,
                                                    msg.data.characterCount);
        } else if (msg.data.unavailable) {
          Translation.serviceUnavailable = true;
          this.state = Translation.STATE_UNAVAILABLE;
        } else {
          this.state = Translation.STATE_ERROR;
        }
        break;
    }
  },

  infobarClosed: function() {
    if (this.state == Translation.STATE_OFFER)
      TranslationHealthReport.recordDeniedTranslationOffer();
  }
};




let TranslationHealthReport = {
  




  recordTranslationOpportunity: function (language) {
    this._withProvider(provider => provider.recordTranslationOpportunity(language));
   },

  






  recordMissedTranslationOpportunity: function (language) {
    this._withProvider(provider => provider.recordMissedTranslationOpportunity(language));
  },

  









  recordAutoRejectedTranslationOffer: function () {
    this._withProvider(provider => provider.recordAutoRejectedTranslationOffer());
  },

   








  recordTranslation: function (langFrom, langTo, numCharacters) {
    this._withProvider(provider => provider.recordTranslation(langFrom, langTo, numCharacters));
  },

  











  recordDetectedLanguageChange: function (beforeFirstTranslation) {
    this._withProvider(provider => provider.recordDetectedLanguageChange(beforeFirstTranslation));
  },

  




  recordTargetLanguageChange: function () {
    this._withProvider(provider => provider.recordTargetLanguageChange());
  },

  


  recordDeniedTranslationOffer: function () {
    this._withProvider(provider => provider.recordDeniedTranslationOffer());
  },

  


  recordShowOriginalContent: function () {
    this._withProvider(provider => provider.recordShowOriginalContent());
  },

  





  _withProvider: function (callback) {
    try {
      let reporter = Cc["@mozilla.org/datareporting/service;1"]
                        .getService().wrappedJSObject.healthReporter;

      if (reporter) {
        reporter.onInit().then(function () {
          callback(reporter.getProvider("org.mozilla.translation"));
        }, Cu.reportError);
      } else {
        callback(null);
      }
    } catch (ex) {
      Cu.reportError(ex);
    }
  }
};









function TranslationMeasurement1() {
  Metrics.Measurement.call(this);

  this._serializers[this.SERIALIZE_JSON].singular =
    this._wrapJSONSerializer(this._serializers[this.SERIALIZE_JSON].singular);

  this._serializers[this.SERIALIZE_JSON].daily =
    this._wrapJSONSerializer(this._serializers[this.SERIALIZE_JSON].daily);
}

TranslationMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "translation",
  version: 1,

  fields: {
    translationOpportunityCount: DAILY_COUNTER_FIELD,
    missedTranslationOpportunityCount: DAILY_COUNTER_FIELD,
    pageTranslatedCount: DAILY_COUNTER_FIELD,
    charactersTranslatedCount: DAILY_COUNTER_FIELD,
    translationOpportunityCountsByLanguage: DAILY_LAST_TEXT_FIELD,
    missedTranslationOpportunityCountsByLanguage: DAILY_LAST_TEXT_FIELD,
    pageTranslatedCountsByLanguage: DAILY_LAST_TEXT_FIELD,
    detectedLanguageChangedBefore: DAILY_COUNTER_FIELD,
    detectedLanguageChangedAfter: DAILY_COUNTER_FIELD,
    targetLanguageChanged: DAILY_COUNTER_FIELD,
    deniedTranslationOffer: DAILY_COUNTER_FIELD,
    showOriginalContent: DAILY_COUNTER_FIELD,
    detectLanguageEnabled: DAILY_LAST_NUMERIC_FIELD,
    showTranslationUI: DAILY_LAST_NUMERIC_FIELD,
    autoRejectedTranslationOffer: DAILY_COUNTER_FIELD,
  },

  shouldIncludeField: function (field) {
    if (!Services.prefs.getBoolPref("toolkit.telemetry.enabled")) {
      
      
      return false;
    }

    return field in this._fields;
  },

  _getDailyLastTextFieldAsJSON: function(name, date) {
    let id = this.fieldID(name);

    return this.storage.getDailyLastTextFromFieldID(id, date).then((data) => {
      if (data.hasDay(date)) {
        data = JSON.parse(data.getDay(date));
      } else {
        data = {};
      }

      return data;
    });
  },

  _wrapJSONSerializer: function (serializer) {
    let _parseInPlace = function(o, k) {
      if (k in o) {
        o[k] = JSON.parse(o[k]);
      }
    };

    return function (data) {
      let result = serializer(data);

      
      
      _parseInPlace(result, "translationOpportunityCountsByLanguage");
      _parseInPlace(result, "missedTranslationOpportunityCountsByLanguage");
      _parseInPlace(result, "pageTranslatedCountsByLanguage");

      return result;
    }
  }
});

this.TranslationProvider = function () {
  Metrics.Provider.call(this);
}

TranslationProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.translation",

  measurementTypes: [
    TranslationMeasurement1,
  ],

  recordTranslationOpportunity: function (language, date=new Date()) {
    let m = this.getMeasurement(TranslationMeasurement1.prototype.name,
                                TranslationMeasurement1.prototype.version);

    return this._enqueueTelemetryStorageTask(function* recordTask() {
      yield m.incrementDailyCounter("translationOpportunityCount", date);

      let langCounts = yield m._getDailyLastTextFieldAsJSON(
        "translationOpportunityCountsByLanguage", date);

      langCounts[language] = (langCounts[language] || 0) + 1;
      langCounts = JSON.stringify(langCounts);

      yield m.setDailyLastText("translationOpportunityCountsByLanguage",
                               langCounts, date);

    }.bind(this));
  },

  recordMissedTranslationOpportunity: function (language, date=new Date()) {
    let m = this.getMeasurement(TranslationMeasurement1.prototype.name,
                                TranslationMeasurement1.prototype.version);

    return this._enqueueTelemetryStorageTask(function* recordTask() {
      yield m.incrementDailyCounter("missedTranslationOpportunityCount", date);

      let langCounts = yield m._getDailyLastTextFieldAsJSON(
        "missedTranslationOpportunityCountsByLanguage", date);

      langCounts[language] = (langCounts[language] || 0) + 1;
      langCounts = JSON.stringify(langCounts);

      yield m.setDailyLastText("missedTranslationOpportunityCountsByLanguage",
                               langCounts, date);

    }.bind(this));
  },

  recordAutoRejectedTranslationOffer: function (date=new Date()) {
    let m = this.getMeasurement(TranslationMeasurement1.prototype.name,
                                TranslationMeasurement1.prototype.version);

    return this._enqueueTelemetryStorageTask(function* recordTask() {
      yield m.incrementDailyCounter("autoRejectedTranslationOffer", date);
    }.bind(this));
  },

  recordTranslation: function (langFrom, langTo, numCharacters, date=new Date()) {
    let m = this.getMeasurement(TranslationMeasurement1.prototype.name,
                                TranslationMeasurement1.prototype.version);

    return this._enqueueTelemetryStorageTask(function* recordTask() {
      yield m.incrementDailyCounter("pageTranslatedCount", date);
      yield m.incrementDailyCounter("charactersTranslatedCount", date,
                                    numCharacters);

      let langCounts = yield m._getDailyLastTextFieldAsJSON(
        "pageTranslatedCountsByLanguage", date);

      let counts = langCounts[langFrom] || {};
      counts["total"] = (counts["total"] || 0) + 1;
      counts[langTo] = (counts[langTo] || 0) + 1;
      langCounts[langFrom] = counts;
      langCounts = JSON.stringify(langCounts);

      yield m.setDailyLastText("pageTranslatedCountsByLanguage",
                               langCounts, date);
    }.bind(this));
  },

  recordDetectedLanguageChange: function (beforeFirstTranslation) {
    let m = this.getMeasurement(TranslationMeasurement1.prototype.name,
                                TranslationMeasurement1.prototype.version);

    return this._enqueueTelemetryStorageTask(function* recordTask() {
      if (beforeFirstTranslation) {
          yield m.incrementDailyCounter("detectedLanguageChangedBefore");
        } else {
          yield m.incrementDailyCounter("detectedLanguageChangedAfter");
        }
    }.bind(this));
  },

  recordTargetLanguageChange: function () {
    let m = this.getMeasurement(TranslationMeasurement1.prototype.name,
                                TranslationMeasurement1.prototype.version);

    return this._enqueueTelemetryStorageTask(function* recordTask() {
      yield m.incrementDailyCounter("targetLanguageChanged");
    }.bind(this));
  },

  recordDeniedTranslationOffer: function () {
    let m = this.getMeasurement(TranslationMeasurement1.prototype.name,
                                TranslationMeasurement1.prototype.version);

    return this._enqueueTelemetryStorageTask(function* recordTask() {
      yield m.incrementDailyCounter("deniedTranslationOffer");
    }.bind(this));
  },

  recordShowOriginalContent: function () {
    let m = this.getMeasurement(TranslationMeasurement1.prototype.name,
                                TranslationMeasurement1.prototype.version);

    return this._enqueueTelemetryStorageTask(function* recordTask() {
      yield m.incrementDailyCounter("showOriginalContent");
    }.bind(this));
  },

  collectDailyData: function () {
    let m = this.getMeasurement(TranslationMeasurement1.prototype.name,
                                TranslationMeasurement1.prototype.version);

    return this._enqueueTelemetryStorageTask(function* recordTask() {
      let detectLanguageEnabled = Services.prefs.getBoolPref("browser.translation.detectLanguage");
      yield m.setDailyLastNumeric("detectLanguageEnabled", detectLanguageEnabled ? 1 : 0);

      let showTranslationUI = Services.prefs.getBoolPref("browser.translation.ui.show");
      yield m.setDailyLastNumeric("showTranslationUI", showTranslationUI ? 1 : 0);
    }.bind(this));
  },

  _enqueueTelemetryStorageTask: function (task) {
    if (!Services.prefs.getBoolPref("toolkit.telemetry.enabled")) {
      
      
      return Promise.resolve(null);
    }

    return this.enqueueStorageOperation(() => {
      return Task.spawn(task);
    });
  }
});
