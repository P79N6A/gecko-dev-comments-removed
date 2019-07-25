



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrivateBrowsingAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();

  
  module.pb = new PrivateBrowsingAPI.privateBrowsing(controller);
}

var teardownModule = function(module)
{
  pb.reset();
}




var testCheckAboutPrivateBrowsing = function()
{
  
  pb.enabled = false;
  pb.showPrompt = false;

  pb.start();

  
  var importItem = new elementslib.ID(controller.window.document, "menu_import");
  controller.assertProperty(importItem, "disabled", true);

  
  if (mozmill.isMac) {
    var libraryItem = new elementslib.ID(controller.window.document, "bookmarksShowAll");
    controller.click(libraryItem);
    controller.sleep(500);

    
    var window = mozmill.wm.getMostRecentWindow('Places:Organizer');
    var libController = new mozmill.controller.MozMillController(window);

    
    var importItem = new elementslib.ID(libController.window.document, "menu_import");
    libController.assertProperty(importItem, "disabled", true);

    
    var importHTML = new elementslib.ID(libController.window.document, "fileImport");
    libController.assertPropertyNotExist(importHTML, "disabled");

    libController.keypress(null, "w", {accelKey: true});
    libController.sleep(200);
  }
}





