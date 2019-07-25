





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}




var testDefaultSecurityPreferences = function() {
  PrefsAPI.openPreferencesDialog(prefDialogCallback);
}




var prefDialogCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneAdvanced';

  
  var encryption = new elementslib.ID(controller.window.document, "encryptionTab");
  controller.waitThenClick(encryption, gTimeout);
  controller.sleep(gDelay);

  
  var sslPref = new elementslib.ID(controller.window.document, "useSSL3");
  var tlsPref = new elementslib.ID(controller.window.document, "useTLS1");
  controller.waitForElement(sslPref, gTimeout);
  controller.assertChecked(sslPref);
  controller.assertChecked(tlsPref);

  prefDialog.close();
}





