






































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'TabbedBrowsingAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);
  tabBrowser.closeAllTabs();
}

var testCloseTab = function()
{
  
  for(var i = 0; i < 4; i++) {
    tabBrowser.openTab({type: "menu"});
  }

  controller.waitForEval("subject.length == 5",  gTimeout,   100, tabBrowser);

  
  tabBrowser.closeTab({type: "shortcut"});
  controller.waitForEval("subject.length == 4", gTimeout, 100, tabBrowser);

  
  tabBrowser.closeTab({type: "menu"});
  controller.waitForEval("subject.length == 3", gTimeout, 100, tabBrowser);

  
  tabBrowser.closeTab({type: "middleClick", index: 0});
  controller.waitForEval("subject.length == 2", gTimeout, 100, tabBrowser);

  
  tabBrowser.closeTab({type: "closeButton"});
  controller.waitForEval("subject.length == 1", gTimeout, 100, tabBrowser);
}





