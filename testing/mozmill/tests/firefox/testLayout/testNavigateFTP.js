




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI'];

const gTimeout = 5000;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
}

var testNavigateFTP = function () {
  
  controller.open('ftp://ftp.mozilla.org/pub/');
  controller.waitForPageLoad();

  var firefox = new elementslib.Link(controller.tabs.activeTab, 'firefox');
  controller.waitThenClick(firefox, gTimeout);
  controller.waitForPageLoad();

  var nightly = new elementslib.Link(controller.tabs.activeTab, 'nightly');
  controller.waitThenClick(nightly, gTimeout);
  controller.waitForPageLoad();

  var latestLink = new elementslib.Link(controller.tabs.activeTab, 'latest-trunk');
  controller.waitForElement(latestLink, gTimeout);
}





