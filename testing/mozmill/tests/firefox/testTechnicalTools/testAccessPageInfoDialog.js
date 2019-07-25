





































const RELATIVE_ROOT = '../../shared-modules';
const MODULE_REQUIRES = ['UtilsAPI'];

const TIMEOUT = 5000;

const LOCAL_TEST_FOLDER = collector.addHttpResource('../test-files/');
const LOCAL_TEST_PAGE = LOCAL_TEST_FOLDER + 'layout/mozilla.html';

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}

var teardownModule = function(module) {
  UtilsAPI.closeContentAreaContextMenu(controller);
}

var testAccessPageInfo = function () {
  
  controller.open(LOCAL_TEST_PAGE);
  controller.waitForPageLoad();

  
  controller.rightclick(new elementslib.XPath(controller.tabs.activeTab, "/html"));
  controller.click(new elementslib.ID(controller.window.document, "context-viewinfo"));

  UtilsAPI.handleWindow("type", "Browser:page-info", checkPageInfoWindow);
}

function checkPageInfoWindow(controller) {
  
  var panes = [
               {button: 'generalTab', panel: 'generalPanel'},
               {button: 'mediaTab', panel: 'mediaPanel'},
               {button: 'feedTab', panel: 'feedListbox'},
               {button: 'permTab', panel: 'permPanel'},
               {button: 'securityTab', panel: 'securityPanel'}
              ];

  
  for each (var pane in panes) {
    var paneButton = new elementslib.ID(controller.window.document, pane.button);
    controller.click(paneButton);

    
    var node = new elementslib.ID(controller.window.document, pane.panel);
    controller.waitForElement(node, TIMEOUT);
  }

  
  controller.keypress(null, 'VK_ESCAPE', {});
}





