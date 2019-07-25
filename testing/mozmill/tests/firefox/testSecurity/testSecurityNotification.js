






































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI', 'PrefsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}




var testSecNotification = function() {
  
  controller.open("https://addons.mozilla.org/");
  controller.waitForPageLoad();

  var query = new elementslib.ID(controller.tabs.activeTab, "query");
  controller.assertNode(query);

  
  var identLabel = new elementslib.ID(controller.window.document, "identity-icon-label");
  controller.assertValue(identLabel, 'Mozilla Corporation (US)');

  
  var securityButton = controller.window.document.getElementById("security-button");
  var cssSecButton = controller.window.getComputedStyle(securityButton, "");
  controller.assertJS("subject.getPropertyValue('list-style-image') != 'none'", cssSecButton);

  
  var identityBox = new elementslib.ID(controller.window.document, "identity-box");
  controller.assertProperty(identityBox, "className", "verifiedIdentity");

  
  controller.open("http://www.mozilla.org/");
  controller.waitForPageLoad();

  var projects = new elementslib.Link(controller.tabs.activeTab, "Our Projects");
  controller.assertNode(projects);

  
  controller.assertJS("subject.getPropertyValue('list-style-image') == 'none'", cssSecButton);

  
  controller.assertProperty(identityBox, "className", "unknownIdentity");

  
  controller.open("https://mozilla.org/");
  controller.waitForPageLoad(1000);

  
  var link = new elementslib.ID(controller.tabs.activeTab, "cert_domain_link");
  controller.waitForElement(link, gTimeout);
  controller.assertProperty(link, "textContent", "*.mozilla.org");

  
  controller.assertNode(new elementslib.ID(controller.tabs.activeTab, "getMeOutOfHereButton"));

  
  controller.assertNode(new elementslib.ID(controller.tabs.activeTab, "exceptionDialogButton"));

  
  var text = new elementslib.ID(controller.tabs.activeTab, "technicalContentText");
  controller.waitForElement(text, gTimeout);
  controller.assertJS("subject.textContent.indexOf('ssl_error_bad_cert_domain') != -1", text.getNode());
}





