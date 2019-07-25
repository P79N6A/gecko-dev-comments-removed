



































const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
}





var testSecurityInfoViaPadlock = function()
{
  
  controller.open("https://www.verisign.com/");
  controller.waitForPageLoad();

  
  var secUI = controller.window.getBrowser().mCurrentBrowser.securityUI;
  var cert = secUI.QueryInterface(Ci.nsISSLStatusProvider).SSLStatus.serverCert;

  
  controller.click(new elementslib.ID(controller.window.document, "security-button"));

  
  controller.sleep(500);
  var window = mozmill.wm.getMostRecentWindow('Browser:page-info');
  var pageInfoController = new mozmill.controller.MozMillController(window);

  
  var securityTab = new elementslib.ID(pageInfoController.window.document, "securityTab");
  pageInfoController.assertProperty(securityTab, "selected", "true");

  
  var webIDDomainLabel = new elementslib.ID(pageInfoController.window.document, "security-identity-domain-value");
  pageInfoController.waitForEval("subject.domainLabel.indexOf(subject.CName) != -1", gTimeout, 100,
                                 {domainLabel: webIDDomainLabel.getNode().value, CName: cert.commonName});


  
  var webIDOwnerLabel = new elementslib.ID(pageInfoController.window.document, "security-identity-owner-value");
  pageInfoController.assertValue(webIDOwnerLabel, cert.organization);

  
  var webIDVerifierLabel = new elementslib.ID(pageInfoController.window.document, "security-identity-verifier-value");
  pageInfoController.assertValue(webIDVerifierLabel, cert.issuerOrganization);

  
  pageInfoController.keypress(null, 'VK_ESCAPE', {});

  
  controller.sleep(200);
}





