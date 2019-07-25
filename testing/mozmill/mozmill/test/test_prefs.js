var elementslib = {}; Components.utils.import('resource://mozmill/modules/elementslib.js', elementslib);
var mozmill = {}; Components.utils.import('resource://mozmill/modules/mozmill.js', mozmill);
var controller = {};  Components.utils.import('resource://mozmill/modules/controller.js', controller);

var setupModule = function(module) {
  module.controller = new controller.MozMillController(mozmill.utils.getWindowByType("Browser:Preferences"));
}

var test_TabsTab = function() {
  
  controller.click(new elementslib.Elem( controller.tabs.Tabs.button ));
  
  var warnElem = new elementslib.ID(controller.window.document, 'warnCloseMultiple');
  controller.waitForElement(warnElem);
  controller.click(warnElem);
  
  controller.click(new elementslib.ID(controller.window.document, 'warnOpenMany'));
  
  controller.click(new elementslib.ID(controller.window.document, 'showTabBar'));
  
  controller.click(new elementslib.ID(controller.window.document, 'switchToNewTabs'));
  controller.sleep(1000);
}

var test_ContentTab = function() {
  
  controller.click(new elementslib.Elem( controller.tabs.Content.button ));
  controller.sleep(1000);
  
  
  controller.click(new elementslib.ID(controller.window.document, 'popupPolicy'));
  
  controller.click(new elementslib.ID(controller.window.document, 'loadImages'));
  
  controller.click(new elementslib.ID(controller.window.document, 'enableJavaScript'));
  
  controller.click(new elementslib.ID(controller.window.document, 'enableJava'));

  
  
  
  
}

var test_ApplicationsTab = function() {
  e = new elementslib.Elem( controller.tabs.Applications.button );
  controller.click(e);  
  controller.sleep(500);
  
  
  
  
  
  
  
  
  
  
  
  
  
}

var test_PrivacyTab = function() {
  controller.click(new elementslib.Elem( controller.tabs.Privacy.button ));
  controller.sleep(500);
  controller.click(new elementslib.ID(controller.window.document, 'rememberHistoryDays'));
  controller.click(new elementslib.ID(controller.window.document, 'rememberForms'));
  controller.click(new elementslib.ID(controller.window.document, 'rememberDownloads'));
  controller.click(new elementslib.ID(controller.window.document, 'acceptThirdParty'));
  controller.click(new elementslib.ID(controller.window.document, 'acceptCookies'));
  controller.click(new elementslib.ID(controller.window.document, 'alwaysClear'));
  controller.click(new elementslib.ID(controller.window.document, 'askBeforeClear'));
}
