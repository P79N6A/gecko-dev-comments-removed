




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var testWarningPages = function() {
  var urls = ['http://www.mozilla.com/firefox/its-a-trap.html',
              'http://www.mozilla.com/firefox/its-an-attack.html'];

  for (var i = 0; i < urls.length; i++ ) {
    
    controller.open(urls[i]);
    controller.waitForPageLoad(1000);

    
    checkGetMeOutOfHereButton();

    
    controller.open(urls[i]);
    controller.waitForPageLoad(1000);

    
    checkReportButton(i, urls[i]);

    
    controller.open(urls[i]);
    controller.waitForPageLoad(1000);

    
    checkIgnoreWarningButton(urls[i]);
  }
}




var checkGetMeOutOfHereButton = function()
{
  var getMeOutOfHereButton = new elementslib.ID(controller.tabs.activeTab, "getMeOutButton");

  
  controller.waitThenClick(getMeOutOfHereButton, gTimeout);
  controller.waitForPageLoad();

  
  var defaultHomePage = UtilsAPI.getProperty("resource:/browserconfig.properties", "browser.startup.homepage");
  UtilsAPI.assertLoadedUrlEqual(controller, defaultHomePage);
}









var checkReportButton = function(type, badUrl) {
  var reportButton = new elementslib.ID(controller.tabs.activeTab, "reportButton");

  
  controller.waitThenClick(reportButton, gTimeout);
  controller.waitForPageLoad();

  var locale = PrefsAPI.preferences.getPref("general.useragent.locale", "");
  var url = "";

  if (type == 0) {
    
    url = UtilsAPI.formatUrlPref("browser.safebrowsing.warning.infoURL");

    var phishingElement = new elementslib.XPath(controller.tabs.activeTab, "/html/body[@id='phishing-protection']")
    controller.assertNode(phishingElement);

  } else if (type == 1) {
    
    url = UtilsAPI.formatUrlPref("browser.safebrowsing.malware.reportURL") + badUrl;

    var malwareElement = new elementslib.ID(controller.tabs.activeTab, "date");
    controller.assertNode(malwareElement);
  }

  UtilsAPI.assertLoadedUrlEqual(controller, url);
}







var checkIgnoreWarningButton = function(url) {
  var ignoreWarningButton = new elementslib.ID(controller.tabs.activeTab, "ignoreWarningButton");

  
  controller.waitThenClick(ignoreWarningButton, gTimeout);
  controller.waitForPageLoad();

  
  var locationBar = new elementslib.ID(controller.window.document, "urlbar");

  controller.assertValue(locationBar, url);
  controller.assertNodeNotExist(ignoreWarningButton);
  controller.assertNode(new elementslib.ID(controller.tabs.activeTab, "main-feature"));
}





