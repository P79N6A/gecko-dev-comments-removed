





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI'];

const gDelay = 0;
const gTimeout = 200;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();

  
  var formHistory = Cc["@mozilla.org/satchel/form-history;1"]
                       .getService(Ci.nsIFormHistory2);
  formHistory.removeAllEntries();
}

var teardownModule = function(module) {
  PrefsAPI.preferences.clearUserPref("browser.formfill.enable");
}

var testToggleFormManager = function() {
  
  PrefsAPI.openPreferencesDialog(prefDialogFormCallback);

  var url = "http://www-archive.mozilla.org/wallet/samples/sample9.html";

  
  controller.open(url);
  controller.waitForPageLoad();

  var firstName = new elementslib.Name(controller.tabs.activeTab, "ship_fname");
  var fname = "John";
  var lastName = new elementslib.Name(controller.tabs.activeTab, "ship_lname");
  var lname = "Smith";

  controller.type(firstName, fname);
  controller.type(lastName, lname);

  controller.click(new elementslib.Name(controller.tabs.activeTab, "SubmitButton"));
  controller.waitForPageLoad();
  controller.waitForElement(firstName, gTimeout);

  
  var popDownAutoCompList = new elementslib.Lookup(controller.window.content.document, '/id("main-window")/id("mainPopupSet")/id("PopupAutoComplete")/anon({"anonid":"tree"})/{"class":"autocomplete-treebody"}');

  controller.type(firstName, fname.substring(0,2));
  controller.sleep(gTimeout);
  controller.assertNodeNotExist(popDownAutoCompList);
  controller.assertValue(firstName, fname.substring(0,2));

  controller.type(lastName, lname.substring(0,2));
  controller.sleep(gTimeout);
  controller.assertNodeNotExist(popDownAutoCompList);
  controller.assertValue(lastName, lname.substring(0,2));
}







var prefDialogFormCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'panePrivacy';

  
  var historyMode = new elementslib.ID(controller.window.document, "historyMode");
  controller.waitForElement(historyMode);
  controller.select(historyMode, null, null, "custom");

  var rememberForms = new elementslib.ID(controller.window.document, "rememberForms");
  controller.waitThenClick(rememberForms);
  controller.sleep(gDelay);

  prefDialog.close(true);
}





