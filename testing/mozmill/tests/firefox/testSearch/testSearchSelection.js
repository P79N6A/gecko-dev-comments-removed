




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'SearchAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  search = new SearchAPI.searchBar(controller);
  tabs = new TabbedBrowsingAPI.tabBrowser(controller);
  tabs.closeAllTabs();
}

var teardownModule = function(module)
{
  search.clear();

  PrefsAPI.preferences.clearUserPref("browser.tabs.loadInBackground");
}




var testSearchSelectionViaContextMenu = function()
{
  var engines = search.visibleEngines;
  var engineName = engines[engines.length - 1].name;

  
  search.selectedEngine = engineName;

  
  var textElem = new elementslib.XPath(controller.tabs.activeTab, "//div[@id='openweb_promo']/p");

  controller.open("http://www.mozilla.org/causes/");
  controller.waitForPageLoad();

  
  startSearch(textElem, engineName, true);

  
  startSearch(textElem, engineName, false);
}











var startSearch = function(element, engineName, loadInBackground)
{
  var tabCount = tabs.length;
  var tabIndex = tabs.selectedIndex;

  PrefsAPI.preferences.setPref("browser.tabs.loadInBackground", loadInBackground);

  
  controller.doubleClick(element);
  var selection = controller.tabs.activeTabWindow.getSelection().toString().trim();

  
  controller.rightClick(element);

  var contextEntry = new elementslib.ID(controller.window.document, "context-searchselect");
  var contextLabel = contextEntry.getNode().getAttribute('label');
  controller.assertJS("subject.isEngineNameInContextMenu == true",
                      {isEngineNameInContextMenu: contextLabel.indexOf(engineName) != -1});
  controller.click(contextEntry);
  UtilsAPI.closeContentAreaContextMenu(controller);

  
  controller.waitForEval("subject.hasNewTabOpened == true", gTimeout, 100,
                         {hasNewTabOpened: tabs.length == (tabCount + 1)});

  if (loadInBackground) {
    controller.waitForEval("subject.isNewBackgroundTab == true", gTimeout, 100,
                           {isNewBackgroundTab: tabs.selectedIndex == tabIndex});
    tabs.selectedIndex = tabs.selectedIndex + 1;
  } else {
    controller.waitForEval("subject.isNewForegroundTab == true", gTimeout, 100,
                           {isNewForegroundTab: tabs.selectedIndex == tabIndex + 1});
  }

  controller.waitForPageLoad();

  
  search.checkSearchResultPage(selection);

  tabs.closeTab({type: "shortcut"});
  tabs.selectedIndex = tabIndex;
}
