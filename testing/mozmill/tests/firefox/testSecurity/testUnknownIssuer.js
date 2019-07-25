



































const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}





var testUnknownIssuer = function() {
  
  controller.open('https://mur.at');
  controller.waitForPageLoad(1000);

  
  var link = new elementslib.ID(controller.tabs.activeTab, "cert_domain_link");
  controller.waitForElement(link, gTimeout);
  controller.assertProperty(link, "textContent", "secure.mur.at");

  
  controller.assertNode(new elementslib.ID(controller.tabs.activeTab, "getMeOutOfHereButton"));

  
  controller.assertNode(new elementslib.ID(controller.tabs.activeTab, "exceptionDialogButton"));

  
  var text = new elementslib.ID(controller.tabs.activeTab, "technicalContentText");
  controller.waitForElement(text, gTimeout);
  controller.assertJS("subject.errorMessage.indexOf('sec_error_unknown_issuer') != -1",
                      {errorMessage: text.getNode().textContent});
}





