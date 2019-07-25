





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ToolbarAPI'];

const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
  module.locationBar =  new ToolbarAPI.locationBar(controller);
}




var testLocationBarSearches = function ()
{
  controller.open("about:blank");
  controller.waitForPageLoad();

  


  var randomTestString = "oau45rtdgsh34nft";

  
  locationBar.loadURL(randomTestString);
  controller.waitForPageLoad();
  controller.assertJS("subject.contains('" + randomTestString + "') == true", locationBar);

  
  var yourSearchString = new elementslib.XPath(controller.tabs.activeTab,
                                               "/html/body[@id='gsr']/div[@id='cnt']/div[@id='res']/div/p[1]/b");
  controller.assertText(yourSearchString, randomTestString);

  controller.open("about:blank");
  controller.waitForPageLoad();

  



  
  locationBar.loadURL("personas");
  controller.waitForPageLoad();
  controller.assertJS("subject.contains('getpersonas') == true", locationBar);

  
  var personasImage = new elementslib.XPath(controller.tabs.activeTab,
                                            "/html/body/div[@id='outer-wrapper']/div[@id='inner-wrapper']/div[@id='nav']/h1/a/img");
  controller.waitForElement(personasImage, gTimeout);

  controller.open("about:blank");
  controller.waitForPageLoad();

  


  var resultsTestString = "lotr";

  
  locationBar.loadURL(resultsTestString);
  controller.waitForPageLoad();
  controller.assertJS("subject.contains('" + resultsTestString + "') == true",
                      locationBar);

  
  
  var resultsStringCheck = new elementslib.XPath(controller.tabs.activeTab,
                                                 "/html/body[@id='gsr']/div[@id='cnt']/div[@id='ssb']/p/b[4]");
  controller.assertText(resultsStringCheck, resultsTestString);
}





