






































const RELATIVE_ROOT = '../../shared-modules';
const MODULE_REQUIRES = ['PrefsAPI', 'TabbedBrowsingAPI', 'ToolbarAPI'];

const LOCAL_TEST_FOLDER = collector.addHttpResource('../test-files/');
const LOCAL_TEST_PAGES = [
  {url: LOCAL_TEST_FOLDER + 'layout/mozilla.html'},
  {url: LOCAL_TEST_FOLDER + 'layout/mozilla_mission.html'}
];

var setupModule = function() {
  controller = mozmill.getBrowserController();
  locationBar = new ToolbarAPI.locationBar(controller);

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var teardownModule = function(module) {
  PrefsAPI.preferences.clearUserPref("browser.startup.homepage");
}




var testSetHomePage = function() {
  
  controller.open(LOCAL_TEST_PAGES[0].url);
  controller.waitForPageLoad();

  var link = new elementslib.Link(controller.tabs.activeTab, "Community");
  controller.assertNode(link);

  
  PrefsAPI.openPreferencesDialog(prefDialogHomePageCallback);
}




var testHomeButton = function()
{
  
  controller.open(LOCAL_TEST_PAGES[1].url);
  controller.waitForPageLoad();

  
  controller.click(new elementslib.ID(controller.window.document, "home-button"));
  controller.waitForPageLoad();

  
  controller.assertValue(locationBar.urlbar, LOCAL_TEST_PAGES[0].url);
}







var prefDialogHomePageCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneMain';

  
  var useCurrent = new elementslib.ID(controller.window.document, "useCurrent");
  controller.click(useCurrent);

  prefDialog.close(true);
}






