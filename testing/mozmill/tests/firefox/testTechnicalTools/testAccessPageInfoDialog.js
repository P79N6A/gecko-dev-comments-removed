




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
}

var teardownModule = function(module)
{
  UtilsAPI.closeContentAreaContextMenu(controller);
}

var testAccessPageInfo = function ()
{
  
  var panes = [
               {button: 'generalTab', panel: 'generalPanel'},
               {button: 'mediaTab', panel: 'mediaPanel'},
               {button: 'feedTab', panel: 'feedListbox'},
               {button: 'permTab', panel: 'permPanel'},
               {button: 'securityTab', panel: 'securityPanel'}
              ];

  
  controller.open('http://www.cnn.com');
  controller.waitForPageLoad();

  
  controller.rightclick(new elementslib.XPath(controller.tabs.activeTab, "/html"));
  controller.sleep(gDelay);
  controller.click(new elementslib.ID(controller.window.document, "context-viewinfo"));
  controller.sleep(500);

  
  var window = mozmill.wm.getMostRecentWindow('Browser:page-info');
  var piController = new mozmill.controller.MozMillController(window);

  
  for each (pane in panes) {
    piController.click(new elementslib.ID(piController.window.document, pane.button));
    piController.sleep(gDelay);

    
    var node = new elementslib.ID(piController.window.document, pane.panel);
    piController.waitForElement(node, gTimeout);
  }

  
  piController.keypress(null, 'VK_ESCAPE', {});

  
  controller.sleep(200);
}





