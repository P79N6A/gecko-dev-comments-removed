




































const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}




var testBackandForward = function()
{
  var pageElements = ['guser', 'sb_form_q', 'i'];
  var websites = ['http://www.google.com/webhp?hl=en&complete=1',
                  'http://www.bing.com/',
                  'http://www.wolframalpha.com/'];

  
  for (var k = 0; k < websites.length; k++) {
    controller.open(websites[k]);
    controller.waitForPageLoad();

    var element = new elementslib.ID(controller.tabs.activeTab, pageElements[k]);
    controller.waitForElement(element, gTimeout);
  }

  
  for (var i = websites.length - 2; i >= 0; i--) {
    controller.goBack();
    controller.waitForPageLoad();

    var element = new elementslib.ID(controller.tabs.activeTab, pageElements[i]);
    controller.waitForElement(element, gTimeout);
  }

  
  for (var j = 1; j < websites.length; j++) {
    controller.goForward();
    controller.waitForPageLoad();

    var element = new elementslib.ID(controller.tabs.activeTab, pageElements[j]);
    controller.waitForElement(element, gTimeout);
  }
}





