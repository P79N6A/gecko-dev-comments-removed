





































const RELATIVE_ROOT = '../../shared-modules';
const MODULE_REQUIRES = ['PrivateBrowsingAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const TIMEOUT = 5000;

const LOCAL_TEST_FOLDER = collector.addHttpResource('../test-files/');
const LOCAL_TEST_PAGES = [
  {url: LOCAL_TEST_FOLDER + 'layout/mozilla.html', name: 'community'},
  {url: LOCAL_TEST_FOLDER + 'layout/mozilla_mission.html', name: 'mission'}
];

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  pb = new PrivateBrowsingAPI.privateBrowsing(controller);
  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var teardownModule = function(module) {
  pb.reset();
}




var testCloseWindow = function() {
  
  
  if (!mozmill.isMac)
    return;

  var windowCount = mozmill.utils.getWindows().length;

  
  pb.enabled = false;
  pb.showPrompt = false;

  
  var newTab = new elementslib.Elem(controller.menus['file-menu'].menu_newNavigatorTab);
  
  for each (var page in LOCAL_TEST_PAGES) {
    controller.open(page.url);
    controller.click(newTab);
  }

  
  for (var i = 0; i < LOCAL_TEST_PAGES.length; i++) {
    var elem = new elementslib.Name(controller.tabs.getTab(i), LOCAL_TEST_PAGES[i].name);
     controller.waitForElement(elem, TIMEOUT); 
  }

  
  pb.start();

  
  var cmdKey = UtilsAPI.getEntity(tabBrowser.getDtds(), "closeCmd.key");
  controller.keypress(null, cmdKey, {accelKey: true});
  
  controller.waitForEval("subject.utils.getWindows().length == subject.expectedCount",
                         TIMEOUT, 100,
                         {utils: mozmill.utils, expectedCount: (windowCount - 1)});

  
  
  pb.enabled = false;
  controller.waitForEval("subject.utils.getWindows().length == subject.expectedCount",
                         TIMEOUT, 100,
                         {utils: mozmill.utils, expectedCount: windowCount});

  UtilsAPI.handleWindow("type", "navigator:browser", checkWindowOpen, true);
}

function checkWindowOpen(controller) {
  
  controller.assertJS("subject.tabs.length == subject.expectedCount",
                      {tabs: controller.tabs, expectedCount: (websites.length + 1)});

  
  for (var i = 0; i < LOCAL_TEST_PAGES.length; i++) {
    var tab = controller.tabs.getTab(i);
    var elem = new elementslib.Name(tab, LOCAL_TEST_PAGES[i].name);

    controller.waitForPageLoad(tab);
    controller.waitForElement(elem, TIMEOUT);
  }
}





