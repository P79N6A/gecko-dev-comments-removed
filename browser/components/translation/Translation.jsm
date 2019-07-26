



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
    if (this.supportedSourceLanguages.indexOf(aDetectedLanguage) == -1 ||
        aDetectedLanguage == this.defaultTargetLanguage)
      return;

    TranslationHealthReport.recordTranslationOpportunity(aDetectedLanguage);

    if (!Services.prefs.getBoolPref(TRANSLATION_PREF_SHOWUI))
      return;

    if (!aBrowser.translationUI)
      aBrowser.translationUI = new TranslationUI(aBrowser);


    aBrowser.translationUI.showTranslationUI(aDetectedLanguage);
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
    if (aFrom == aTo ||
        (this.state == this.STATE_TRANSLATED &&
         this.translatedFrom == aFrom && this.translatedTo == aTo)) {
      
      return;
    }

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

  get notificationBox() this.browser.ownerGlobal.gBrowser.getNotificationBox(this.browser),

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




let TranslationHealthReport = {
  




  recordTranslationOpportunity: function (language) {
    this._withProvider(provider => provider.recordTranslationOpportunity(language));
   },

   








  recordTranslation: function (langFrom, langTo, numCharacters) {
    this._withProvider(provider => provider.recordTranslation(langFrom, langTo, numCharacters));
  },

  











  recordLanguageChange: function (beforeFirstTranslation) {
    this._withProvider(provider => provider.recordLanguageChange(beforeFirstTranslation));
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
    pageTranslatedCount: DAILY_COUNTER_FIELD,
    charactersTranslatedCount: DAILY_COUNTER_FIELD,
    translationOpportunityCountsByLanguage: DAILY_LAST_TEXT_FIELD,
    pageTranslatedCountsByLanguage: DAILY_LAST_TEXT_FIELD,
    detectedLanguageChangedBefore: DAILY_COUNTER_FIELD,
    detectedLanguageChangedAfter: DAILY_COUNTER_FIELD,
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

  recordLanguageChange: function (beforeFirstTranslation) {
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

  _enqueueTelemetryStorageTask: function (task) {
    if (!Services.prefs.getBoolPref("toolkit.telemetry.enabled")) {
      
      
      return Promise.resolve(null);
    }

    return this.enqueueStorageOperation(() => {
      return Task.spawn(task);
    });
  }
});
