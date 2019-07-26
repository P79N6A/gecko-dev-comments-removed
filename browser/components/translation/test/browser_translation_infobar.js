





let tmp = {};
Cu.import("resource:///modules/translation/Translation.jsm", tmp);
let {Translation} = tmp;

const kShowUIPref = "browser.translation.ui.show";

function waitForCondition(condition, nextTest, errorMsg) {
  var tries = 0;
  var interval = setInterval(function() {
    if (tries >= 30) {
      ok(false, errorMsg);
      moveOn();
    }
    var conditionPassed;
    try {
      conditionPassed = condition();
    } catch (e) {
      ok(false, e + "\n" + e.stack);
      conditionPassed = false;
    }
    if (conditionPassed) {
      moveOn();
    }
    tries++;
  }, 100);
  var moveOn = function() { clearInterval(interval); nextTest(); };
}

var TranslationStub = {
  translate: function(aFrom, aTo) {
    this.state = this.STATE_TRANSLATING;
    this.translatedFrom = aFrom;
    this.translatedTo = aTo;
  },

  _reset: function() {
    this.translatedFrom = "";
    this.translatedTo = "";
  },

  failTranslation: function() {
    this.state = this.STATE_ERROR;
    this._reset();
  },

  finishTranslation: function() {
    this.showTranslatedContent();
    this.state = this.STATE_TRANSLATED;
    this._reset();
  }
};

function showTranslationUI(aDetectedLanguage) {
  let browser = gBrowser.selectedBrowser;
  Translation.languageDetected(browser, aDetectedLanguage);
  let ui = browser.translationUI;
  for (let name of ["translate", "_reset", "failTranslation", "finishTranslation"])
    ui[name] = TranslationStub[name];
  return ui.notificationBox.getNotificationWithValue("translation");
}

function hasTranslationInfoBar() {
  return !!gBrowser.getNotificationBox().getNotificationWithValue("translation");
}

function test() {
  waitForExplicitFinish();

  Services.prefs.setBoolPref(kShowUIPref, true);
  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;
  tab.linkedBrowser.addEventListener("load", function onload() {
    tab.linkedBrowser.removeEventListener("load", onload, true);
    TranslationStub.browser = gBrowser.selectedBrowser;
    registerCleanupFunction(function () {
      gBrowser.removeTab(tab);
      Services.prefs.clearUserPref(kShowUIPref);
    });
    run_tests(() => {
      finish();
    });
  }, true);

  content.location = "data:text/plain,test page";
}

function checkURLBarIcon(aExpectTranslated = false) {
  is(!PopupNotifications.getNotification("translate"), aExpectTranslated,
     "translate icon " + (aExpectTranslated ? "not " : "") + "shown");
  is(!!PopupNotifications.getNotification("translated"), aExpectTranslated,
     "translated icon " + (aExpectTranslated ? "" : "not ") + "shown");
}

function run_tests(aFinishCallback) {
  info("Show an info bar saying the current page is in French");
  let notif = showTranslationUI("fr");
  is(notif.state, notif.translation.STATE_OFFER, "the infobar is offering translation");
  is(notif._getAnonElt("detectedLanguage").value, "fr", "The detected language is displayed");
  checkURLBarIcon();

  info("Click the 'Translate' button");
  notif._getAnonElt("translate").click();
  is(notif.state, notif.translation.STATE_TRANSLATING, "the infobar is in the translating state");
  ok(!!notif.translation.translatedFrom, "Translation.translate has been called");
  is(notif.translation.translatedFrom, "fr", "from language correct");
  is(notif.translation.translatedTo, Translation.defaultTargetLanguage, "from language correct");
  checkURLBarIcon();

  info("Make the translation fail and check we are in the error state.");
  notif.translation.failTranslation();
  is(notif.state, notif.translation.STATE_ERROR, "infobar in the error state");
  checkURLBarIcon();

  info("Click the try again button");
  notif._getAnonElt("tryAgain").click();
  is(notif.state, notif.translation.STATE_TRANSLATING, "infobar in the translating state");
  ok(!!notif.translation.translatedFrom, "Translation.translate has been called");
  is(notif.translation.translatedFrom, "fr", "from language correct");
  is(notif.translation.translatedTo, Translation.defaultTargetLanguage, "from language correct");
  checkURLBarIcon();

  info("Make the translation succeed and check we are in the 'translated' state.");
  notif.translation.finishTranslation();
  is(notif.state, notif.translation.STATE_TRANSLATED, "infobar in the translated state");
  checkURLBarIcon(true);

  info("Test 'Show original' / 'Show Translation' buttons.");
  
  ok(!notif._getAnonElt("showOriginal").hidden, "'Show Original' button visible");
  ok(notif._getAnonElt("showTranslation").hidden, "'Show Translation' button hidden");
  
  notif._getAnonElt("showOriginal").click();
  
  checkURLBarIcon();
  
  ok(notif._getAnonElt("showOriginal").hidden, "'Show Original' button hidden");
  ok(!notif._getAnonElt("showTranslation").hidden, "'Show Translation' button visible");
  
  notif._getAnonElt("showTranslation").click();
  
  checkURLBarIcon(true);
  
  ok(!notif._getAnonElt("showOriginal").hidden, "'Show Original' button visible");
  ok(notif._getAnonElt("showTranslation").hidden, "'Show Translation' button hidden");

  info("Check that changing the source language causes a re-translation");
  let from = notif._getAnonElt("fromLanguage");
  from.value = "es";
  from.doCommand();
  is(notif.state, notif.translation.STATE_TRANSLATING, "infobar in the translating state");
  ok(!!notif.translation.translatedFrom, "Translation.translate has been called");
  is(notif.translation.translatedFrom, "es", "from language correct");
  is(notif.translation.translatedTo, Translation.defaultTargetLanguage, "to language correct");
  
  
  checkURLBarIcon(true);
  notif.translation.finishTranslation();
  checkURLBarIcon(true);

  info("Check that changing the target language causes a re-translation");
  let to = notif._getAnonElt("toLanguage");
  to.value = "pl";
  to.doCommand();
  is(notif.state, notif.translation.STATE_TRANSLATING, "infobar in the translating state");
  ok(!!notif.translation.translatedFrom, "Translation.translate has been called");
  is(notif.translation.translatedFrom, "es", "from language correct");
  is(notif.translation.translatedTo, "pl", "to language correct");
  checkURLBarIcon(true);
  notif.translation.finishTranslation();
  checkURLBarIcon(true);

  
  notif.close();

  info("Reopen the info bar to check that it's possible to override the detected language.");
  notif = showTranslationUI("fr");
  is(notif.state, notif.translation.STATE_OFFER, "the infobar is offering translation");
  is(notif._getAnonElt("detectedLanguage").value, "fr", "The detected language is displayed");
  
  notif._getAnonElt("detectedLanguage").value = "ja";
  notif._getAnonElt("translate").click();
  is(notif.state, notif.translation.STATE_TRANSLATING, "the infobar is in the translating state");
  ok(!!notif.translation.translatedFrom, "Translation.translate has been called");
  is(notif.translation.translatedFrom, "ja", "from language correct");
  notif.close();

  info("Reopen to check the 'Not Now' button closes the notification.");
  notif = showTranslationUI("fr");
  let notificationBox = gBrowser.getNotificationBox();
  is(hasTranslationInfoBar(), true, "there's a 'translate' notification");
  notif._getAnonElt("notNow").click();
  is(hasTranslationInfoBar(), false, "no 'translate' notification after clicking 'not now'");

  info("Reopen to check the url bar icon closes the notification.");
  notif = showTranslationUI("fr");
  is(hasTranslationInfoBar(), true, "there's a 'translate' notification");
  PopupNotifications.getNotification("translate").anchorElement.click();
  is(hasTranslationInfoBar(), false, "no 'translate' notification after clicking the url bar icon");

  info("Check that clicking the url bar icon reopens the info bar");
  checkURLBarIcon();
  
  
  PopupNotifications.getNotification("translate").anchorElement.click();
  waitForCondition(hasTranslationInfoBar, () => {
    ok(hasTranslationInfoBar(), "there's a 'translate' notification");
    aFinishCallback();
  }, "timeout waiting for the info bar to reappear");
}
