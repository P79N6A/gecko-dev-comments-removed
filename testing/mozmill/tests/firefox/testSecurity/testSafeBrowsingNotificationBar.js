






































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

var testNotificationBar = function() {
  var badSites = ['http://www.mozilla.com/firefox/its-a-trap.html',
                  'http://www.mozilla.com/firefox/its-an-attack.html'];

  for (var i = 0; i < badSites.length; i++ ) {
    
    controller.open(badSites[i]);
    controller.waitForPageLoad(1000);

    
    checkIgnoreWarningButton(badSites[i]);
    checkNoPhishingButton(badSites[i]);

    
    controller.goBack();
    controller.waitForPageLoad(1000);
    checkIgnoreWarningButton(badSites[i]);

    
    checkGetMeOutOfHereButton();

    
    controller.goBack();
    controller.waitForPageLoad(1000);
    checkIgnoreWarningButton(badSites[i]);

    
    checkXButton();
  }
}






var checkIgnoreWarningButton = function(badUrl) {
  
  var ignoreWarningButton = new elementslib.ID(controller.tabs.activeTab, "ignoreWarningButton");
  controller.waitThenClick(ignoreWarningButton, gTimeout);
  controller.waitForPageLoad();

  
  var locationBar = new elementslib.ID(controller.window.document, "urlbar");

  controller.assertValue(locationBar, badUrl);
  controller.assertNodeNotExist(ignoreWarningButton);
  controller.assertNode(new elementslib.ID(controller.tabs.activeTab, "main-feature"));
}






var checkNoPhishingButton = function(badUrl) {
  if (badUrl == 'http://www.mozilla.com/firefox/its-a-trap.html' ) {
    
    var label = UtilsAPI.getProperty("chrome://browser/locale/browser.properties",
                                     "safebrowsing.notAForgeryButton.label");
    var button = tabBrowser.getTabPanelElement(tabBrowser.selectedIndex,
                                               '/{"value":"blocked-badware-page"}/{"label":"' + label + '"}');
    controller.waitThenClick(button, gTimeout);
    controller.waitForPageLoad(controller.tabs.getTab(1));

    
    var urlField = new elementslib.ID(controller.tabs.activeTab, "url");
    controller.waitForElement(urlField, gTimeout);
    controller.assertValue(urlField, 'http://www.mozilla.com/firefox/its-a-trap.html');

  } else if (badUrl == 'http://www.mozilla.com/firefox/its-an-attack.html' ) {
    
    var label = UtilsAPI.getProperty("chrome://browser/locale/browser.properties",
                                     "safebrowsing.notAnAttackButton.label");
    var button = tabBrowser.getTabPanelElement(tabBrowser.selectedIndex,
                                               '/{"value":"blocked-badware-page"}/{"label":"' + label + '"}');
    controller.waitThenClick(button, gTimeout);
    controller.waitForPageLoad(controller.tabs.getTab(1));

    
    var locationBar = new elementslib.ID(controller.window.document, "urlbar");
	controller.assertJS("subject.urlbar.indexOf('http://www.stopbadware.org/') != -1",
	                    {urlbar: locationBar.getNode().value});
  }

  TabbedBrowsingAPI.closeAllTabs(controller);
}




var checkGetMeOutOfHereButton = function() {
  
  var label = UtilsAPI.getProperty("chrome://browser/locale/browser.properties",
                                   "safebrowsing.getMeOutOfHereButton.label");
  var button = tabBrowser.getTabPanelElement(tabBrowser.selectedIndex,
                                             '/{"value":"blocked-badware-page"}/{"label":"' + label + '"}');
  controller.waitThenClick(button, gTimeout);

  
  controller.waitForPageLoad();

  var defaultHomePage = UtilsAPI.getProperty("resource:/browserconfig.properties",
                                             "browser.startup.homepage");
  UtilsAPI.assertLoadedUrlEqual(controller, defaultHomePage);  
}




var checkXButton = function() {
  
  var button = tabBrowser.getTabPanelElement(tabBrowser.selectedIndex,
                                             '/{"value":"blocked-badware-page"}/anon({"type":"critical"})' +
                                             '/{"class":"messageCloseButton tabbable"}');
  controller.waitThenClick(button, gTimeout);
  controller.sleep(1000);
  controller.assertNodeNotExist(button);
}





