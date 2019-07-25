







































const RELATIVE_ROOT = '../../shared-modules';
const MODULE_REQUIRES = ['PrefsAPI', 'SessionStoreAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const LOCAL_TEST_FOLDER = collector.addHttpResource('../test-files/');
const LOCAL_TEST_PAGE = LOCAL_TEST_FOLDER + 
                        "tabbedbrowsing/openinnewtab_target.html?id=";

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);

  tabBrowser.closeAllTabs();
  SessionStoreAPI.resetRecentlyClosedTabs();
}

var teardownModule = function(module) {
  UtilsAPI.closeContentAreaContextMenu(controller);
  SessionStoreAPI.resetRecentlyClosedTabs();
  tabBrowser.closeAllTabs();
}

var testUndoTabFromContextMenu = function() {
  
  var tabBar = tabBrowser.getElement({type: 'tabs'});
  controller.rightClick(tabBar);

  
  var contextMenuItem = new elementslib.ID(controller.window.document, 'context_undoCloseTab');
  controller.assertProperty(contextMenuItem, 'disabled', true);
  UtilsAPI.closeContentAreaContextMenu(controller);

  
  controller.assertJS("subject.closedTabCount == 0", 
                     {closedTabCount: SessionStoreAPI.getClosedTabCount(controller)});

  
  for (var i = 0; i < 3; i++) {
   controller.open(LOCAL_TEST_PAGE + i);
   controller.waitForPageLoad();
   tabBrowser.openTab({type: 'menu'});
  }

  
  tabBrowser.selectedIndex = 1;
  tabBrowser.closeTab({type: 'menu'});

  
  var linkId = new elementslib.ID(controller.tabs.activeTab, "id");
  controller.assertText(linkId, "2")

  
  controller.assertJS("subject.closedTabCount == 1", 
                      {closedTabCount: SessionStoreAPI.getClosedTabCount(controller)});

  
  controller.rightClick(tabBar);
  controller.assertProperty(contextMenuItem, 'disabled', false);

  
  controller.click(contextMenuItem);
  controller.waitForPageLoad();
  UtilsAPI.closeContentAreaContextMenu(controller);

  
  linkId = new elementslib.ID(controller.tabs.activeTab, "id");
  controller.assertText(linkId, "1");

  
  controller.assertJS("subject.closedTabCount == 0", 
                      {closedTabCount: SessionStoreAPI.getClosedTabCount(controller)});

  
  controller.rightClick(tabBar);
  controller.assertProperty(contextMenuItem, 'disabled', true);
  UtilsAPI.closeContentAreaContextMenu(controller);
}
