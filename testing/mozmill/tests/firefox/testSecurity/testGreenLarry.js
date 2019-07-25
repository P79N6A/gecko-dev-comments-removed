





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI'];

var gTimeout = 5000;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();

  
  module.gETLDService = Cc["@mozilla.org/network/effective-tld-service;1"]
                           .getService(Ci.nsIEffectiveTLDService);
}




var testLarryGreen = function()
{
  
  controller.open("https://www.verisign.com/");
  controller.waitForPageLoad();

  
  var securityUI = controller.window.getBrowser().mCurrentBrowser.securityUI;
  var cert = securityUI.QueryInterface(Ci.nsISSLStatusProvider).SSLStatus.serverCert;

  
  
  var identLabel = new elementslib.ID(controller.window.document, "identity-icon-label");
  var country = cert.subjectName.substring(cert.subjectName.indexOf("C=")+2,
                                           cert.subjectName.indexOf(",serialNumber="));
  var certIdent = cert.organization + ' (' + country + ')';
  controller.assertValue(identLabel, certIdent);

  
  var favicon = new elementslib.ID(controller.window.document, "page-proxy-favicon");
  controller.assertProperty(favicon, "src" ,"https://www.verisign.com/favicon.ico");

  
  var identityBox = new elementslib.ID(controller.window.document, "identity-box");
  controller.assertProperty(identityBox, "className", "verifiedIdentity");

  
  controller.click(identityBox);

  
  var doorhanger = new elementslib.ID(controller.window.document, "identity-popup");
  controller.waitForEval("subject.state == 'open'", 2000, 100, doorhanger.getNode());

  
  controller.assertProperty(doorhanger, "className", "verifiedIdentity");

  
  var lockIcon = new elementslib.ID(controller.window.document, "identity-popup-encryption-icon");
  var cssInfoLockImage = controller.window.getComputedStyle(lockIcon.getNode(), "");
  controller.assertJS("subject.getPropertyValue('list-style-image') != 'none'", cssInfoLockImage);

  
  
  
  var host = new elementslib.ID(controller.window.document, "identity-popup-content-host");
  controller.assertProperty(host, "textContent", gETLDService.getBaseDomainFromHost(cert.commonName));

  
  var owner = new elementslib.ID(controller.window.document, "identity-popup-content-owner");
  controller.assertProperty(owner, "textContent", cert.organization);

  
  
  
  var city = cert.subjectName.substring(cert.subjectName.indexOf("L=")+2,
                                        cert.subjectName.indexOf(",ST="));
  var state = cert.subjectName.substring(cert.subjectName.indexOf("ST=")+3,
                                         cert.subjectName.indexOf(",postalCode="));
  var country = cert.subjectName.substring(cert.subjectName.indexOf("C=")+2,
                                           cert.subjectName.indexOf(",serialNumber="));
  var location = city + '\n' + state + ', ' + country;
  var ownerLocation = new elementslib.ID(controller.window.document,
                                         "identity-popup-content-supplemental");
  controller.assertProperty(ownerLocation, "textContent", location);

  
  var l10nVerifierLabel = UtilsAPI.getProperty("chrome://browser/locale/browser.properties",
                                               "identity.identified.verifier");
  l10nVerifierLabel = l10nVerifierLabel.replace("%S", cert.issuerOrganization);
  var verifier = new elementslib.ID(controller.window.document,
                                    "identity-popup-content-verifier");
  controller.assertProperty(verifier, "textContent", l10nVerifierLabel);

  
  var l10nEncryptionLabel = UtilsAPI.getProperty("chrome://browser/locale/browser.properties",
                                                 "identity.encrypted");
  var label = new elementslib.ID(controller.window.document,
                                 "identity-popup-encryption-label");
  controller.assertProperty(label, "textContent", l10nEncryptionLabel);

  
  var moreInfoButton = new elementslib.ID(controller.window.document,
                                          "identity-popup-more-info-button");
  controller.click(moreInfoButton);
  controller.sleep(500);

  
  var window = mozmill.wm.getMostRecentWindow('Browser:page-info');
  var pageInfoController = new mozmill.controller.MozMillController(window);

  
  var securityTab = new elementslib.ID(pageInfoController.window.document, "securityTab");
  pageInfoController.assertProperty(securityTab, "selected", "true");

  
  var webIDDomainLabel = new elementslib.ID(pageInfoController.window.document, "security-identity-domain-value");
  pageInfoController.waitForEval("subject.domainLabel.indexOf(subject.CName) != -1", gTimeout, 100,
                                 {domainLabel: webIDDomainLabel.getNode().value, CName: cert.commonName});

  
  var webIDOwnerLabel = new elementslib.ID(pageInfoController.window.document,
                                           "security-identity-owner-value");
  pageInfoController.assertValue(webIDOwnerLabel, cert.organization);

  
  var webIDVerifierLabel = new elementslib.ID(pageInfoController.window.document,
                                              "security-identity-verifier-value");
  pageInfoController.assertValue(webIDVerifierLabel, cert.issuerOrganization);

  
  pageInfoController.keypress(null, 'VK_ESCAPE', {});

  
  controller.sleep(200);
}





