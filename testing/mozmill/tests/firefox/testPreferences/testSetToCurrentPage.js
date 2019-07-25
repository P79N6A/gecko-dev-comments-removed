





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'TabbedBrowsingAPI'];

const gDelay = 0;
const gTimeout = 5000;

const homepage = 'http://www.mozilla.org/';

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var teardownModule = function(module) {
  PrefsAPI.preferences.clearUserPref("browser.startup.homepage");
}




var testSetHomePage = function() {
  
  controller.open(homepage);
  controller.waitForPageLoad();

  var link = new elementslib.Link(controller.tabs.activeTab, "Mozilla");
  controller.assertNode(link);

  
  PrefsAPI.openPreferencesDialog(prefDialogHomePageCallback);
}




var testHomeButton = function()
{
  
  controller.open('http://www.yahoo.com/');
  controller.waitForPageLoad();

  
  controller.click(new elementslib.ID(controller.window.document, "home-button"));
  controller.waitForPageLoad();

  
  var locationBar = new elementslib.ID(controller.window.document, "urlbar");
  controller.assertValue(locationBar, homepage);
}







var prefDialogHomePageCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneMain';

  
  var useCurrent = new elementslib.ID(controller.window.document, "useCurrent");
  controller.click(useCurrent);

  prefDialog.close(true);
}






