





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI', 'PrefsAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}

var teardownModule = function(module) {
  PrefsAPI.preferences.clearUserPref("intl.accept_languages");
}




var testSetLanguages = function () {
  controller.open("about:blank");

  
  PrefsAPI.openPreferencesDialog(prefDialogCallback);

  
  controller.open('http://www.google.com/');
  controller.waitForPageLoad();

  
  controller.assertNode(new elementslib.Link(controller.tabs.activeTab, "Accedi"));
  controller.assertNode(new elementslib.Link(controller.tabs.activeTab, "Gruppi"));
  controller.assertNode(new elementslib.Link(controller.tabs.activeTab, "Ricerca avanzata"));
}







var prefDialogCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneContent';

  
  var md = new ModalDialogAPI.modalDialog(langHandler);
  md.start(200);

  var language = new elementslib.ID(controller.window.document, "chooseLanguage");
  controller.waitThenClick(language, gTimeout);

  prefDialog.close(true);
}







var langHandler = function(controller) {
  
  var langDropDown = new elementslib.ID(controller.window.document, "availableLanguages");
  controller.waitForElement(langDropDown, gTimeout);

  controller.keypress(langDropDown, "i", {});
  controller.sleep(100);
  controller.keypress(langDropDown, "t", {});
  controller.sleep(100);
  controller.keypress(langDropDown, "a", {});
  controller.sleep(100);
  controller.keypress(langDropDown, "l", {});
  controller.sleep(100);

  
  var addButton = new elementslib.ID(controller.window.document, "addButton");
  controller.waitForEval("subject.disabled == false", gTimeout, 100, addButton.getNode());
  controller.click(addButton);

  
  var upButton = new elementslib.ID(controller.window.document, "up");
  controller.click(upButton);
  controller.sleep(gDelay);
  controller.click(upButton);
  controller.sleep(gDelay);

  
  controller.click(new elementslib.Lookup(controller.window.document, '/id("LanguagesDialog")/anon({"anonid":"dlg-buttons"})/{"dlgtype":"accept"}'));
}





