





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}




var testStopAndReload = function()
{
  var url = "http://www.mozilla.com/en-US/";

  
  controller.open("about:blank");
  controller.waitForPageLoad();

  
  controller.open(url);
  controller.sleep(100);
  controller.click(new elementslib.ID(controller.window.document, "stop-button"));

  
  
  var elem = new elementslib.ID(controller.tabs.activeTab, "query");
  controller.assertNodeNotExist(elem);
  controller.sleep(gDelay);

  
  controller.open(url);
  controller.waitForPageLoad();

  controller.waitForElement(elem, gTimeout);
}





