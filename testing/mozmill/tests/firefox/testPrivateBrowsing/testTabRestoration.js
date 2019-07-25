





































const RELATIVE_ROOT = '../../shared-modules';
const MODULE_REQUIRES = ['PrivateBrowsingAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const TIMEOUT = 5000;

const LOCAL_TEST_FOLDER = collector.addHttpResource('../test-files/');
const LOCAL_TEST_PAGES = [
  {url: LOCAL_TEST_FOLDER + 'layout/mozilla_contribute.html', id: 'localization'},
  {url: LOCAL_TEST_FOLDER + 'layout/mozilla_community.html', id: 'history'}
];

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  pb = new PrivateBrowsingAPI.privateBrowsing(controller);

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var teardownModule = function(module) {
  pb.reset();
}




var testTabRestoration = function() {
  
  pb.enabled = false;
  pb.showPrompt = false;

  
  var newTab = new elementslib.Elem(controller.menus['file-menu'].menu_newNavigatorTab);

  for each (var page in LOCAL_TEST_PAGES) {
    controller.open(page.url);
    controller.click(newTab);
  }

  
  for (var i = 0; i < LOCAL_TEST_PAGES.length; i++) {
    var elem = new elementslib.ID(controller.tabs.getTab(i), LOCAL_TEST_PAGES[i].id);
    controller.waitForElement(elem, TIMEOUT);
  }

  
  pb.start();

  
  pb.stop();

  
  controller.assertJS("subject.tabCountActual == subject.tabCountExpected",
                      {tabCountActual: controller.tabs.length,
                       tabCountExpected: LOCAL_TEST_PAGES.length + 1});

  
  for (var i = 0; i < LOCAL_TEST_PAGES.length; i++) {
    var elem = new elementslib.ID(controller.tabs.getTab(i), LOCAL_TEST_PAGES[i].id);
    controller.waitForElement(elem, TIMEOUT);
  }
}





