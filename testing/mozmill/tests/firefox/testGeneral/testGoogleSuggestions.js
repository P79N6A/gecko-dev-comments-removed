





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI'];

const gTimeout = 5000;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
}

var testGoogleSuggestedTerms = function() {
  
  controller.open("http://www.google.com/webhp?complete=1&hl=en");
  controller.waitForPageLoad();

  
  var searchField = new elementslib.Name(controller.tabs.activeTab, "q");
  controller.type(searchField, "area");

  
  
  if (UtilsAPI.appInfo.platformVersion.indexOf("pre") == -1) {
    var autoComplete = new elementslib.XPath(controller.tabs.activeTab, "/html/body/span[@id='main']/center/span[@id='body']/center/form/table[2]/tbody/tr[2]/td");
  } else {
    var autoComplete = new elementslib.XPath(controller.tabs.activeTab, "/html/body/center/form/table[1]/tbody/tr/td[2]");
  }

  
  controller.waitThenClick(autoComplete, gTimeout);

  
  controller.click(new elementslib.Name(controller.tabs.activeTab, "btnG"));
  controller.waitForPageLoad();

  
  var nextField = new elementslib.Link(controller.tabs.activeTab, "Next");

  controller.waitForElement(searchField, gTimeout);
  controller.waitForElement(nextField, gTimeout);
}





