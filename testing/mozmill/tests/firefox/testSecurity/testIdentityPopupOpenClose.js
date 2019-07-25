









































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI'];

var gDelay = 0;
var gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}




var testIdentityPopupOpenClose = function() {
  
  var identityBox = new elementslib.ID(controller.window.document, "identity-box");
  controller.click(identityBox);

  
  var popup = new elementslib.ID(controller.window.document, "identity-popup");
  controller.waitForEval("subject.state == 'open'", gTimeout, 100, popup.getNode());
  controller.sleep(gDelay);

  
  var button = new elementslib.ID(controller.window.document, "identity-popup-more-info-button");
  UtilsAPI.assertElementVisible(controller, button, true);

  
  var contentArea = new elementslib.XPath(controller.tabs.activeTab, "/html/body");
  controller.click(contentArea);

  
  controller.waitForEval("subject.state == 'closed'", gTimeout, 100, popup.getNode());
}





