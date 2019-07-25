





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var teardownModule = function(module) {
  PrefsAPI.preferences.clearUserPref("browser.startup.homepage");
}




var testRestoreHomeToDefault = function() {
  
  controller.open('http://www.mozilla.org/');
  controller.waitForPageLoad();

  var link = new elementslib.Link(controller.tabs.activeTab, "Mozilla");
  controller.assertNode(link);

  
  PrefsAPI.openPreferencesDialog(prefDialogHomePageCallback);

  
  controller.click(new elementslib.ID(controller.window.document, "home-button"));
  controller.waitForPageLoad();
  controller.assertNode(link);

  
  PrefsAPI.openPreferencesDialog(prefDialogDefHomePageCallback);
}







var prefDialogHomePageCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneMain';

  
  var useCurrent = new elementslib.ID(controller.window.document, "useCurrent");
  controller.waitThenClick(useCurrent);
  controller.sleep(gDelay);

  prefDialog.close(true);
}

var prefDialogDefHomePageCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);

  
  var useDefault = new elementslib.ID(controller.window.document, "restoreDefaultHomePage");
  controller.waitForElement(useDefault, gTimeout);
  controller.click(useDefault);

  
  var defaultHomePage = UtilsAPI.getProperty("resource:/browserconfig.properties", "browser.startup.homepage");
  var browserHomePage = new elementslib.ID(controller.window.document, "browserHomePage");
  controller.assertValue(browserHomePage, defaultHomePage);

  prefDialog.close();
}





