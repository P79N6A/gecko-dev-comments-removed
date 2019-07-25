





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PlacesAPI', 'ToolbarAPI'];

const gTimeout = 5000;
const gDelay = 100;

const websites = ['http://www.google.com/',
                  'http://www.mozilla.org/',
                  'http://www.getpersonas.com/',
                  'about:blank'];

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  locationBar = new ToolbarAPI.locationBar(controller);

  
  try {
    PlacesAPI.historyService.removeAllPages();
  } catch (ex) {}
}




var testAccessLocationBarHistory = function()
{
  
  
  for each (website in websites) {
    locationBar.loadURL(website);
    controller.waitForPageLoad();
  }

  
  controller.sleep(4000);

  
  locationBar.clear();

  
  
  controller.keypress(locationBar.urlbar, "VK_DOWN", {});
  controller.sleep(gDelay);
  controller.keypress(locationBar.urlbar, "VK_DOWN", {});
  controller.sleep(gDelay);

  
  controller.waitForEval("subject.selectedIndex == 0",
                         gTimeout, 100, locationBar.autoCompleteResults);
  locationBar.contains("getpersonas");
  controller.keypress(null, "VK_RETURN", {});
  controller.waitForPageLoad();

  
  
  var personasImage = new elementslib.XPath(controller.tabs.activeTab, "/html/body/div[@id='outer-wrapper']/div[@id='inner-wrapper']/div[@id='nav']/h1/a/img");
  controller.waitForElement(personasImage, gTimeout, 100);

  
  locationBar.contains("getpersonas");
}





