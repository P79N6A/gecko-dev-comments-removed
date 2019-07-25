









































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI'];

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
}




var testLarryGrey = function()
{
  
  controller.open("http://www.mozilla.org");
  controller.waitForPageLoad();

  
  var favicon = new elementslib.ID(controller.window.document, "page-proxy-favicon");
  controller.assertProperty(favicon, "src" ,"http://www.mozilla.org/favicon.ico");

  
  controller.assertValue(new elementslib.ID(controller.window.document, "identity-icon-label"), "");

  
  controller.click(new elementslib.ID(controller.window.document, "identity-box"));

  
  var doorhanger = new elementslib.ID(controller.window.document, "identity-popup");
  controller.waitForEval("subject.state == 'open'", 2000, 100, doorhanger.getNode());

  
  controller.assertProperty(doorhanger, "className", "unknownIdentity");

  
  var moreInfoButton = new elementslib.ID(controller.window.document, "identity-popup-more-info-button");
  controller.click(moreInfoButton);
  controller.sleep(500);

  
  var window = mozmill.wm.getMostRecentWindow('Browser:page-info');
  var pageInfoController = new mozmill.controller.MozMillController(window);

  
  var securityTab = new elementslib.ID(pageInfoController.window.document, "securityTab");
  pageInfoController.assertProperty(securityTab, "selected", "true");

  
  var webIDDomainLabel = new elementslib.ID(pageInfoController.window.document, "security-identity-domain-value");
  pageInfoController.assertValue(webIDDomainLabel, "www.mozilla.org");

  
  var webIDOwnerLabel = new elementslib.ID(pageInfoController.window.document, "security-identity-owner-value");
  var securityOwner = UtilsAPI.getProperty("chrome://browser/locale/pageInfo.properties", "securityNoOwner");
  pageInfoController.assertValue(webIDOwnerLabel, securityOwner);

  
  var webIDVerifierLabel = new elementslib.ID(pageInfoController.window.document, "security-identity-verifier-value");
  var securityIdentifier = UtilsAPI.getProperty("chrome://browser/locale/pageInfo.properties", "notset");
  pageInfoController.assertValue(webIDVerifierLabel, securityIdentifier);

  
  pageInfoController.keypress(null, 'VK_ESCAPE', {});

  
  controller.sleep(200);
}





