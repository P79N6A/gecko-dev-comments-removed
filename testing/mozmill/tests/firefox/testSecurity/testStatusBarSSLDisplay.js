



































var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
}




var testSSLDomainLabelInStatusBar = function()
{
  
  controller.open("https://addons.mozilla.org");
  controller.waitForPageLoad();
  
  
  var padlockIcon = new elementslib.ID(controller.window.document, 
                                       "security-button");
  controller.assertNode(padlockIcon);                                      

  
  var lookupPath = '/id("main-window")' +
                   '/id("browser-bottombox")' +
                   '/id("status-bar")' +
                   '/id("security-button")' +
                   '/anon({"class":"statusbarpanel-text"})';
  var domainText = new elementslib.Lookup(controller.window.document, lookupPath);
  controller.assertNodeNotExist(domainText);
}





