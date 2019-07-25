




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}




var testDefaultPhishingEnabled = function() {
  PrefsAPI.openPreferencesDialog(prefPaneSetCallback);
}







var prefPaneSetCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneSecurity';

  
  var attackElem = new elementslib.ID(controller.window.document, "blockAttackSites");
  var forgeryElem = new elementslib.ID(controller.window.document, "blockWebForgeries");

  
  controller.waitForElement(attackElem, gTimeout);
  controller.assertChecked(attackElem);
  controller.assertChecked(forgeryElem);

  prefDialog.close();
}





