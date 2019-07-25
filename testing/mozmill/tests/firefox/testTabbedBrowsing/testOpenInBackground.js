






































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const localTestFolder = collector.addHttpResource('./files');

const gDelay = 0;
const gTimeout = 5000;

var gTabOrder = [
  {index: 1, linkid: 2},
  {index: 2, linkid: 3},
  {index: 3, linkid: 1}
];

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);
  tabBrowser.closeAllTabs();
}

var teardownModule = function()
{
  PrefsAPI.preferences.clearUserPref("browser.tabs.loadInBackground");
  UtilsAPI.closeContentAreaContextMenu(controller);
}

var testOpenInBackgroundTab = function()
{
  PrefsAPI.openPreferencesDialog(prefDialogCallback);

  
  controller.open(localTestFolder + "openinnewtab.html");
  controller.waitForPageLoad();

  for(var i = 0; i < gTabOrder.length; i++) {
    
    var currentLink = new elementslib.Name(controller.tabs.activeTab, "link_" + (i + 1));
    var contextMenuItem = new elementslib.ID(controller.window.document, "context-openlinkintab");
    
    if(i == 2) {
      
      controller.middleClick(currentLink);
    } else {
      
      controller.rightClick(currentLink);
      controller.click(contextMenuItem);
      UtilsAPI.closeContentAreaContextMenu(controller);
    }

    
    controller.waitForEval("subject.length == " + (i + 2), gTimeout, 100, tabBrowser);
    controller.waitForEval("subject.selectedIndex == 0", gTimeout, 100, tabBrowser);
    
    if(i == 0) {
      
      tabBrowser.selectedIndex = 1;
      tabBrowser.selectedIndex = 0;
    }
  }

  
  for each(tab in gTabOrder) {
    var linkId = new elementslib.ID(controller.tabs.getTab(tab.index), "id");
    controller.waitForElement(linkId);
    controller.assertText(linkId, tab.linkid);
  }

  
  tabBrowser.selectedIndex = 3;
  tabBrowser.closeTab({type: "closeButton"});

  
  controller.waitForEval("subject.length == 3", gTimeout, 100, tabBrowser);
  controller.waitForEval("subject.selectedIndex == 2", gTimeout, 100, tabBrowser);
}

var prefDialogCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneTabs';

  
  var switchToTabsPref = new elementslib.ID(controller.window.document, "switchToNewTabs");
  controller.waitForElement(switchToTabsPref, gTimeout);
  controller.check(switchToTabsPref, false);

  prefDialog.close(true);
}





