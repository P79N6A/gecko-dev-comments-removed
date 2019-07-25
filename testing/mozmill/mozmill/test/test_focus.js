



































var gDelay = 500;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
}

var checkKeypressFunction = function(element) {
  element.getNode().value = "";

  
  controller.keypress(null, "F", {});
  controller.sleep(gDelay);
  controller.assertValue(element, "");

  
  controller.keypress(element, "M", {});
  controller.sleep(gDelay);
  controller.assertValue(element, "M");

  
  controller.keypress(element, "F", {});
  controller.sleep(gDelay);
  controller.assertValue(element, "MF");
}

var checkTypeFunction = function(element) {
  element.getNode().value = "";

  
  controller.type(null, "Firefox");
  controller.sleep(gDelay);
  controller.assertValue(element, "");

  
  controller.type(element, "Mozilla");
  controller.sleep(gDelay);
  controller.assertValue(element, "Mozilla");

  
  controller.type(element, " Firefox");
  controller.sleep(gDelay);
  controller.assertValue(element, "Mozilla Firefox");
}

var testContentTextboxFocus = function() {
  controller.open("http://www.mozilla.org");
  controller.waitForPageLoad(controller.tabs.activeTab);

  var searchField = new elementslib.ID(controller.tabs.activeTab, "q");
  controller.waitForElement(searchField, 5000);
  controller.sleep(gDelay);

  checkKeypressFunction(searchField);
  checkTypeFunction(searchField);
}

var testChromeTextboxFocus = function() {
  var searchBar = new elementslib.ID(controller.window.document, "searchbar");

  checkKeypressFunction(searchBar);

  
  controller.keypress(null, "l", {accelKey: true});
  checkTypeFunction(searchBar);
}