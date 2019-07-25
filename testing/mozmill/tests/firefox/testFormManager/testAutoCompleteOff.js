




































const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}

var testAutoCompleteOff = function() {
  var url = "http://www.google.com/webhp?complete=1&hl=en";
  var searchTerm = "mozillazine";

  
  controller.open(url);
  controller.waitForPageLoad();

  
  var searchField = new elementslib.Name(controller.tabs.activeTab, "q");
  var submitButton = new elementslib.Name(controller.tabs.activeTab, "btnG");

  controller.waitForElement(searchField);
  controller.type(searchField, searchTerm);
  controller.click(submitButton);

  
  controller.open(url);
  controller.waitForPageLoad();

  
  controller.waitForElement(searchField, gTimeout);
  controller.type(searchField, searchTerm.substring(0, 3));
  controller.sleep(500);

  
  var popupAutoCompList = new elementslib.ID(controller.window.document, "PopupAutoComplete");
  controller.assertProperty(popupAutoCompList, "popupOpen", false);
}





