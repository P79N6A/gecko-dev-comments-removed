





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI'];

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();

  
  module.gETLDService = Cc["@mozilla.org/network/effective-tld-service;1"]
                           .getService(Ci.nsIEffectiveTLDService);
}




var testLarryBlue = function()
{
  
  controller.open("https://bugzilla.mozilla.org/");
  controller.waitForPageLoad();

  
  var securityUI = controller.window.getBrowser().mCurrentBrowser.securityUI;
  var cert = securityUI.QueryInterface(Ci.nsISSLStatusProvider).SSLStatus.serverCert;

  
  
  var identLabel = new elementslib.ID(controller.window.document, "identity-icon-label");
  controller.assertValue(identLabel, gETLDService.getBaseDomainFromHost(cert.commonName));

  
  var favicon = new elementslib.ID(controller.window.document, "page-proxy-favicon");
  controller.assertProperty(favicon, "src" ,"https://bugzilla.mozilla.org/skins/custom/images/bugzilla.png");

  
  var identityBox = new elementslib.ID(controller.window.document, "identity-box");
  controller.assertProperty(identityBox, "className", "verifiedDomain");

  
  controller.click(identityBox);

  
  var doorhanger = new elementslib.ID(controller.window.document, "identity-popup");
  controller.waitForEval("subject.state == 'open'", 2000, 100, doorhanger.getNode());

  
  controller.assertProperty(doorhanger, "className", "verifiedDomain");

  
  var lockIcon = new elementslib.ID(controller.window.document, "identity-popup-encryption-icon");
  var cssInfoLockImage = controller.window.getComputedStyle(lockIcon.getNode(), "");
  controller.assertJS("subject.getPropertyValue('list-style-image') != 'none'", cssInfoLockImage);

  
  
  
  var host = new elementslib.ID(controller.window.document, "identity-popup-content-host");
  controller.assertProperty(host, "textContent", gETLDService.getBaseDomainFromHost(cert.commonName));

  
  var owner = new elementslib.ID(controller.window.document, "identity-popup-content-owner");
  var property = UtilsAPI.getProperty("chrome://browser/locale/browser.properties",
                                      "identity.ownerUnknown2");
  controller.assertProperty(owner, "textContent", property);

  
  var l10nVerifierLabel = UtilsAPI.getProperty("chrome://browser/locale/browser.properties",
                                               "identity.identified.verifier");
  l10nVerifierLabel = l10nVerifierLabel.replace("%S", cert.issuerOrganization);
  var verifier = new elementslib.ID(controller.window.document, "identity-popup-content-verifier");
  controller.assertProperty(verifier, "textContent", l10nVerifierLabel);

  
  var l10nEncryptionLabel = UtilsAPI.getProperty("chrome://browser/locale/browser.properties",
                                                 "identity.encrypted");
  var encryptionLabel = new elementslib.ID(controller.window.document, "identity-popup-encryption-label");
  controller.assertProperty(encryptionLabel, "textContent", l10nEncryptionLabel);

  
  var moreInfoButton = new elementslib.ID(controller.window.document, "identity-popup-more-info-button");
  controller.click(moreInfoButton);
  controller.sleep(500);

  
  var window = mozmill.wm.getMostRecentWindow('Browser:page-info');
  var pageInfoController = new mozmill.controller.MozMillController(window);

  
  var securityTab = new elementslib.ID(pageInfoController.window.document, "securityTab");
  pageInfoController.assertProperty(securityTab, "selected", "true");

  
  
  
  var webIDDomainLabel = new elementslib.ID(pageInfoController.window.document,
                                            "security-identity-domain-value");
  pageInfoController.assertValue(webIDDomainLabel, cert.commonName.replace("*", "bugzilla"));

  
  var webIDOwnerLabel = new elementslib.ID(pageInfoController.window.document, "security-identity-owner-value");
  var securityOwner = UtilsAPI.getProperty("chrome://browser/locale/pageInfo.properties", "securityNoOwner");
  pageInfoController.assertValue(webIDOwnerLabel, securityOwner);

  
  var webIDVerifierLabel = new elementslib.ID(pageInfoController.window.document, "security-identity-verifier-value");
  pageInfoController.assertValue(webIDVerifierLabel, cert.issuerOrganization);

  
  pageInfoController.keypress(null, 'VK_ESCAPE', {});

  
  controller.sleep(200);
}





