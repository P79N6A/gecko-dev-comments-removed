





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}

var teardownModule = function(module) {
  
  PrefsAPI.preferences.clearUserPref("security.enable_ssl3");
  PrefsAPI.preferences.clearUserPref("security.enable_tls");
}





var testDisableSSL = function() {
  
  controller.open("about:blank");
  controller.waitForPageLoad();

  PrefsAPI.openPreferencesDialog(prefDialogCallback);

  controller.open("https://www.verisign.com");
  controller.waitForPageLoad(1000);

  
  var title = new elementslib.ID(controller.tabs.activeTab, "errorTitleText");
  controller.waitForElement(title, gTimeout);
  controller.assertJS("subject.errorTitle == 'Secure Connection Failed'",
                      {errorTitle: title.getNode().textContent});

  
  controller.assertNode(new elementslib.ID(controller.tabs.activeTab, "errorTryAgain"));

  
  var text = new elementslib.ID(controller.tabs.activeTab, "errorShortDescText");
  controller.waitForElement(text, gTimeout);
  controller.assertJS("subject.errorMessage.indexOf('ssl_error_ssl_disabled') != -1",
                      {errorMessage: text.getNode().textContent});

  controller.assertJS("subject.errorMessage.indexOf('www.verisign.com') != -1",
                      {errorMessage: text.getNode().textContent});

  controller.assertJS("subject.errorMessage.indexOf('SSL protocol has been disabled') != -1",
                      {errorMessage: text.getNode().textContent});
}







var prefDialogCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneAdvanced';

  
  var encryption = new elementslib.ID(controller.window.document, "encryptionTab");
  controller.waitThenClick(encryption, gTimeout);
  controller.sleep(gDelay);

  
  var sslPref = new elementslib.ID(controller.window.document, "useSSL3");
  controller.waitForElement(sslPref, gTimeout);
  controller.check(sslPref, false);

  
  var tlsPref = new elementslib.ID(controller.window.document, "useTLS1");
  controller.waitForElement(tlsPref, gTimeout);
  controller.check(tlsPref, false);

  prefDialog.close(true);
}





