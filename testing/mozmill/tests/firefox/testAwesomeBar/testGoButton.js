






































const RELATIVE_ROOT = '../../shared-modules';
const MODULE_REQUIRES = ['ToolbarAPI', 'UtilsAPI'];

const LOCAL_TEST_FOLDER = collector.addHttpResource('../test-files/');
const LOCAL_TEST_PAGES = [
  LOCAL_TEST_FOLDER + 'layout/mozilla.html',
  LOCAL_TEST_FOLDER + 'layout/mozilla_mission.html'
];

var setupModule = function() {
  controller = mozmill.getBrowserController();
  locationBar = new ToolbarAPI.locationBar(controller);

  goButton = locationBar.getElement({type: "goButton"});
}




var testGoButtonOnTypeOnly = function() {
  
  controller.open(LOCAL_TEST_PAGES[0]);
  controller.waitForPageLoad();

  
  UtilsAPI.assertElementVisible(controller, goButton, false);

  
  locationBar.focus({type: "shortcut"});
  locationBar.type("w");
  UtilsAPI.assertElementVisible(controller, goButton, true);

  
  locationBar.clear();
  controller.keypress(locationBar.urlbar, "VK_ESCAPE", {});
  UtilsAPI.assertElementVisible(controller, goButton, false);
}




var testClickLocationBarAndGo = function()
{

  
  controller.open(LOCAL_TEST_PAGES[0]);
  controller.waitForPageLoad();

  
  locationBar.focus({type: "shortcut"});
  locationBar.type(LOCAL_TEST_PAGES[1]);

  
  controller.click(goButton);
  controller.waitForPageLoad();

  
  var pageElement = new elementslib.ID(controller.tabs.activeTab, "organization");
  controller.assertNode(pageElement);
  UtilsAPI.assertElementVisible(controller, goButton, false);

  
  controller.assertValue(locationBar.urlbar, LOCAL_TEST_PAGES[1]);
}





