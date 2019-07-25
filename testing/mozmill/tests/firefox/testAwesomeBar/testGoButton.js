





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['ToolbarAPI', 'UtilsAPI'];

const gDelay = 0;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
  module.locationBar = new ToolbarAPI.locationBar(controller);

  module.goButton = locationBar.getElement({type: "goButton"});
}




var testGoButtonOnTypeOnly = function() {
  
  controller.open("http://www.mozilla.org");
  controller.waitForPageLoad();

  
  UtilsAPI.assertElementVisible(controller, goButton, false);

  
  locationBar.focus({type: "shortcut"});
  locationBar.type("w");
  UtilsAPI.assertElementVisible(controller, goButton, true);

  
  locationBar.clear();
  controller.keypress(locationBar.urlbar, "VK_ESCAPE", {});
  UtilsAPI.assertElementVisible(controller, goButton, false);
}




var testClickLocationBarAndGo = function()
{
  var url = "http://www.google.com/webhp?complete=1&hl=en";

  
  controller.open("http://www.mozilla.org");
  controller.waitForPageLoad();

  
  locationBar.focus({type: "shortcut"});
  locationBar.type(url);

  
  controller.click(goButton);
  controller.waitForPageLoad();

  
  controller.assertNode(new elementslib.Name(controller.tabs.activeTab, "q"));
  UtilsAPI.assertElementVisible(controller, goButton, false);

  
  controller.assertValue(locationBar.urlbar, url);
}





