




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI'];  

var gDelay = 2000;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
}




var testUntrustedPageGetMeOutOfHereButton = function()
{
  
  controller.open("https://mozilla.org");
  controller.sleep(gDelay);
  
  
  var getMeOutOfHereButton = new elementslib.ID(controller.tabs.activeTab, 
                                                "getMeOutOfHereButton");
  controller.assertNode(getMeOutOfHereButton);
  
  
  controller.click(getMeOutOfHereButton);
  
  
  controller.waitForPageLoad();
  
  
  var defaultHomePage = UtilsAPI.getProperty("resource:/browserconfig.properties", 
                                             "browser.startup.homepage");
  
  UtilsAPI.assertLoadedUrlEqual(controller, defaultHomePage);
  
}





