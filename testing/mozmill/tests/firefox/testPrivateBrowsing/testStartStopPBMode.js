






































const RELATIVE_ROOT = '../../shared-modules';
const MODULE_REQUIRES = ['PrivateBrowsingAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const TIMEOUT = 5000;

const LOCAL_TEST_FOLDER = collector.addHttpResource('../test-files/');
const LOCAL_TEST_PAGES = [
  {url: LOCAL_TEST_FOLDER + 'layout/mozilla.html', id: 'community'},
  {url: 'about:', id: 'aboutPageList'}
];

var setupModule = function() {
  controller = mozmill.getBrowserController();
  modifier = controller.window.document.documentElement.
             getAttribute("titlemodifier_privatebrowsing");

  
  pb = new PrivateBrowsingAPI.privateBrowsing(controller);
  pb.handler = pbStartHandler;

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var teardownModule = function() {
  pb.reset();
}




var testEnablePrivateBrowsingMode = function() {
  
  pb.enabled = false;
  pb.showPrompt = true;

  
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

  
  controller.assertJS("subject.isOnlyOneTab == true", 
                      {isOnlyOneTab: controller.tabs.length == 1});

  
  controller.assertJS("subject.hasTitleModifier == true",
                      {hasTitleModifier: controller.window.document.
                                         title.indexOf(modifier) != -1});

  
  var description = UtilsAPI.getEntity(pb.getDtds(), "privatebrowsingpage.description");
  var learnMore = UtilsAPI.getEntity(pb.getDtds(), "privatebrowsingpage.learnMore");
  var longDescElem = new elementslib.ID(controller.tabs.activeTab, "errorLongDescText");
  var moreInfoElem = new elementslib.ID(controller.tabs.activeTab, "moreInfoLink");
  controller.waitForElement(longDescElem, TIMEOUT);  
  controller.assertText(longDescElem, description);
  controller.assertText(moreInfoElem, learnMore);
}




var testStopPrivateBrowsingMode = function() {
  
  pb.enabled = true;

  
  pb.stop();

  
  controller.assertJS("subject.allTabsRestored == true",
                      {allTabsRestored: controller.tabs.length == LOCAL_TEST_PAGES.length + 1});

  for (var i = 0; i < LOCAL_TEST_PAGES.length; i++) {
    var elem = new elementslib.ID(controller.tabs.getTab(i), LOCAL_TEST_PAGES[i].id);
    controller.waitForElement(elem, TIMEOUT);
  }

  
  controller.assertJS("subject.noTitleModifier == true",
                      {noTitleModifier: controller.window.document.
                                        title.indexOf(modifier) == -1});
}




var testKeyboardShortcut = function() {
  
  pb.enabled = false;
  pb.showPrompt = true;

  
  pb.start(true);

  
  pb.stop(true);
}







var pbStartHandler = function(controller) {
  
  var checkbox = new elementslib.ID(controller.window.document, 'checkbox');
  controller.waitThenClick(checkbox, TIMEOUT);

  var okButton = new elementslib.Lookup(controller.window.document, 
                                        '/id("commonDialog")' +
                                        '/anon({"anonid":"buttons"})' +
                                        '/{"dlgtype":"accept"}');
  controller.click(okButton);
}







