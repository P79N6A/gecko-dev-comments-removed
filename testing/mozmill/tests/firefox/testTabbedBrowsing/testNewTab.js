





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);
  tabBrowser.closeAllTabs();
}

var testNewTab = function()
{
  
  var section = new elementslib.ID(controller.tabs.activeTab, "sub");

  controller.open('http://www.mozilla.org');
  controller.waitForPageLoad();
  controller.waitForElement(section, gTimeout);

  
  checkOpenTab({type: "menu"});
  checkOpenTab({type: "shortcut"});
  checkOpenTab({type: "tabStrip"});
  checkOpenTab({type: "newTabButton"});
}







var checkOpenTab = function(event)
{
  
  tabBrowser.openTab(event);
  controller.waitForEval("subject.length == 2", gTimeout, 100, controller.tabs);
  controller.assertJS("subject.activeTab.location == 'about:blank'",
                      controller.tabs);

  
  var title = UtilsAPI.getProperty("chrome://browser/locale/tabbrowser.properties",
                                   "tabs.untitled");
  var tab = tabBrowser.getTab();
  controller.assertJS("subject.label == '" + title + "'", tab.getNode());

  
  tabBrowser.closeTab({type: "shortcut"});
  controller.waitForEval("subject.length == 1", gTimeout, 100, controller.tabs);
}





