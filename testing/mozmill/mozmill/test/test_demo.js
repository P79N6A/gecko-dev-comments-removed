var elementslib = {}; Components.utils.import('resource://mozmill/modules/elementslib.js', elementslib);
var mozmill = {}; Components.utils.import('resource://mozmill/modules/mozmill.js', mozmill);
var controller = {};  Components.utils.import('resource://mozmill/modules/controller.js', controller);

var test_PrefsContentTab = function() {
  
  var controller = new controller.MozMillController(mozmill.utils.getWindowByType("Browser:Preferences"));

  
  controller.click(new elementslib.Elem( controller.tabs.Content.button ));
  controller.sleep(1000);
  
  e = new elementslib.ID(controller.window.document, 'popupPolicy')
  controller.waitForElement(e);
  
  controller.click(e);
  controller.sleep(1000);
  
  controller.click(new elementslib.ID(controller.window.document, 'loadImages'));
  controller.sleep(1000);
  
  controller.click(new elementslib.ID(controller.window.document, 'enableJavaScript'));
  controller.sleep(1000);
  
  controller.click(new elementslib.ID(controller.window.document, 'enableJava'));
  controller.sleep(1000);
}

var test_GoogleDotCom = function () {
  
  var controller = mozmill.getBrowserController();
  controller.window.focus();
  controller.open('http://www.google.com');
  controller.sleep(2000);
  controller.type(new elementslib.Name(controller.window.content.document, 'q'), 'Mozilla');
  controller.assertValue(new elementslib.Name(controller.window.content.document, 'q'), 'Mozilla');
  controller.sleep(2000);
  controller.click(new elementslib.Name(controller.window.content.document, 'btnG'));
  controller.sleep(2000);
}

var test_mozillaorg = function () {
  
  var controller = mozmill.getBrowserController();
  controller.window.focus();
  controller.open('http://www.mozilla.org');
  controller.sleep(5000);
  controller.type(new elementslib.Name(controller.window.content.document, 'q'), 'QA');
  controller.waitForElement(new elementslib.Name(controller.window.content.document, 'q'));
  controller.click(new elementslib.ID(controller.window.content.document, 'submit'));
  controller.sleep(3000);
  controller.click(new elementslib.Link(controller.window.content.document, 'Mozilla'));
  controller.sleep(2000);
  controller.waitForElement(new elementslib.Link(controller.window.content.document, 'Tools'));
  controller.click(new elementslib.Link(controller.window.content.document, 'Tools'));
  controller.waitForElement(new elementslib.ID(controller.window.content.document, 'searchInput'));
  controller.type(new elementslib.ID(controller.window.content.document, 'searchInput'), 'MozMill');
  controller.sleep(1000);
  controller.click(new elementslib.Name(controller.window.content.document, 'fulltext'));
  controller.sleep(3000);
  controller.open('http://code.google.com/p/mozmill/');
  controller.sleep(3000);
  controller.waitForElement(new elementslib.Link(controller.window.content.document, 'Downloads'));
  controller.click(new elementslib.Link(controller.window.content.document, 'Downloads'));
  controller.sleep(3000);
  controller.click(new elementslib.Link(controller.window.content.document, 'Source'));
  controller.sleep(3000);
  controller.open('http://www.mozilla.org');
}

test_mozillaorg.EXCLUDED_PLATFORMS = ['darwin'];
