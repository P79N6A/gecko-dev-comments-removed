





var Translation = {
  supportedSourceLanguages: ["en", "zh", "ja", "es", "de", "fr", "ru", "ar", "ko", "pt"],
  supportedTargetLanguages: ["en", "pl", "tr", "vi"],
  defaultTargetLanguage: "en",

  _translateFrom: "",
  _translateTo: "",
  _deferred: null,
  translate: function(aFrom, aTo) {
    this._translateFrom = aFrom;
    this._translateTo = aTo;
    this._deferred = Promise.defer();
    return this._deferred.promise;
  },

  _reset: function() {
    this._translateFrom = "";
    this._translateTo = "";
    this._deferred = null;
  },

  failTranslation: function() {
    this._deferred.reject();
    this._reset();
  },

  finishTranslation: function() {
    this._deferred.resolve();
    this._reset();
  },

  _showOriginalCalled: false,
  showOriginalContent: function() {
    this._showOriginalCalled = true;
  },

  _showTranslationCalled: false,
  showTranslatedContent: function() {
    this._showTranslationCalled = true;
  },

  showTranslationUI: function(aLanguage) {
    let notificationBox = gBrowser.getNotificationBox();
    let notif = notificationBox.appendNotification("", "translation", null,
                                                   notificationBox.PRIORITY_INFO_HIGH);
    notif.init(this);
    notif.detectedLanguage = aLanguage;
    return notif;
  }
};

function test() {
  waitForExplicitFinish();

  
  let notif = Translation.showTranslationUI("fr");
  is(notif.state, notif.STATE_OFFER, "the infobar is offering translation");
  is(notif._getAnonElt("detectedLanguage").value, "fr", "The detected language is displayed");

  
  notif._getAnonElt("translate").click();
  is(notif.state, notif.STATE_TRANSLATING, "the infobar is in the translating state");
  ok(!!Translation._deferred, "Translation.translate has been called");
  is(Translation._translateFrom, "fr", "from language correct");
  is(Translation._translateTo, Translation.defaultTargetLanguage, "from language correct");

  
  Translation.failTranslation();
  is(notif.state, notif.STATE_ERROR, "infobar in the error state");

  
  notif._getAnonElt("tryAgain").click();
  is(notif.state, notif.STATE_TRANSLATING, "infobar in the translating state");
  ok(!!Translation._deferred, "Translation.translate has been called");
  is(Translation._translateFrom, "fr", "from language correct");
  is(Translation._translateTo, Translation.defaultTargetLanguage, "to language correct");

  
  Translation.finishTranslation();
  is(notif.state, notif.STATE_TRANSLATED, "infobar in the translated state");

  
  
  ok(!notif._getAnonElt("showOriginal").hidden, "'Show Original' button visible");
  ok(notif._getAnonElt("showTranslation").hidden, "'Show Translation' button hidden");
  
  notif._getAnonElt("showOriginal").click();
  
  ok(Translation._showOriginalCalled, "'Translation.showOriginalContent' called")
  ok(!Translation._showTranslationCalled, "'Translation.showTranslatedContent' not called")
  Translation._showOriginalCalled = false;
  
  ok(notif._getAnonElt("showOriginal").hidden, "'Show Original' button hidden");
  ok(!notif._getAnonElt("showTranslation").hidden, "'Show Translation' button visible");
  
  notif._getAnonElt("showTranslation").click();
  
  ok(!Translation._showOriginalCalled, "'Translation.showOriginalContent' not called")
  ok(Translation._showTranslationCalled, "'Translation.showTranslatedContent' called")
  Translation._showTranslationCalled = false;
  
  ok(!notif._getAnonElt("showOriginal").hidden, "'Show Original' button visible");
  ok(notif._getAnonElt("showTranslation").hidden, "'Show Translation' button hidden");

  
  let from = notif._getAnonElt("fromLanguage");
  from.value = "es";
  from.doCommand();
  is(notif.state, notif.STATE_TRANSLATING, "infobar in the translating state");
  ok(!!Translation._deferred, "Translation.translate has been called");
  is(Translation._translateFrom, "es", "from language correct");
  is(Translation._translateTo, Translation.defaultTargetLanguage, "to language correct");
  Translation.finishTranslation();

  
  let to = notif._getAnonElt("toLanguage");
  to.value = "pl";
  to.doCommand();
  is(notif.state, notif.STATE_TRANSLATING, "infobar in the translating state");
  ok(!!Translation._deferred, "Translation.translate has been called");
  is(Translation._translateFrom, "es", "from language correct");
  is(Translation._translateTo, "pl", "to language correct");
  Translation.finishTranslation();

  
  notif.close();

  
  notif = Translation.showTranslationUI("fr");
  is(notif.state, notif.STATE_OFFER, "the infobar is offering translation");
  is(notif._getAnonElt("detectedLanguage").value, "fr", "The detected language is displayed");
  
  notif._getAnonElt("detectedLanguage").value = "ja";
  notif._getAnonElt("translate").click();
  is(notif.state, notif.STATE_TRANSLATING, "the infobar is in the translating state");
  ok(!!Translation._deferred, "Translation.translate has been called");
  is(Translation._translateFrom, "ja", "from language correct");
  notif.close();

  
  notif = Translation.showTranslationUI("fr");

  let notificationBox = gBrowser.getNotificationBox();
  ok(!!notificationBox.getNotificationWithValue("translation"), "there's a 'translate' notification");
  notif._getAnonElt("notNow").click();
  ok(!notificationBox.getNotificationWithValue("translation"), "no 'translate' notification after clicking 'not now'");

  finish();
}
