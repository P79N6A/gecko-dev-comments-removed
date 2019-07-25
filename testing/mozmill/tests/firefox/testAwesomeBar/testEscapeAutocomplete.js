




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PlacesAPI', 'ToolbarAPI'];

const gTimeout = 1000;
const gDelay = 100;

var setupModule = function(module) 
{
  module.controller = mozmill.getBrowserController();
  module.locationBar =  new ToolbarAPI.locationBar(controller);
  
  
  try {
    PlacesAPI.historyService.removeAllPages();
  } catch (ex) {}
}




var testEscape = function()
{
  var websites = ['http://www.google.com/', 'http://www.mozilla.org'];

  
  for each (website in websites) {
    locationBar.loadURL(website);
    controller.waitForPageLoad();
  }

  
  controller.sleep(4000);

  var testString = "google";
  
  
  locationBar.clear();

  
  for (var i = 0; i < testString.length; i++) {
    locationBar.type(testString[i]);
    controller.sleep(gDelay);
  }

  
  controller.assertJS("subject.contains('" + testString + "') == true", locationBar);
  controller.assertJS("subject.autoCompleteResults.isOpened == true", locationBar);

  
  controller.keypress(locationBar.urlbar, 'VK_ESCAPE', {});
  controller.assertJS("subject.contains('" + testString + "') == true", locationBar);
  controller.assertJS("subject.autoCompleteResults.isOpened == false", locationBar);
  
  
  controller.keypress(locationBar.urlbar, 'VK_ESCAPE', {});
  controller.assertJS("subject.contains('" + websites[1] + "') == true", locationBar);
}





