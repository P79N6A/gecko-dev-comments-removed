




































const gDelay = 0;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();

  try {
    
    var formHistory = Cc["@mozilla.org/satchel/form-history;1"].
                        getService(Ci.nsIFormHistory2);
    formHistory.removeAllEntries();
  } catch (ex) {
  }
}

var testFormCompletion = function() {
  var url = 'http://www.mozilla.org/';
  var searchText = 'mozillazine';

  
  controller.open(url);
  controller.waitForPageLoad();

  var searchField = new elementslib.ID(controller.tabs.activeTab, "q");
  controller.assertNode(searchField);

  
  controller.type(searchField, searchText);
  controller.sleep(gDelay);

  controller.click(new elementslib.ID(controller.tabs.activeTab, "quick-search-btn"));
  controller.waitForPageLoad();

  
  controller.open('http://www.yahoo.com/');
  controller.waitForPageLoad();

  
  controller.open(url);
  controller.waitForPageLoad();

  
  controller.type(searchField, "mozilla");

  
  var popDownAutoCompList = new elementslib.Lookup(controller.window.document, '/id("main-window")/id("mainPopupSet")/id("PopupAutoComplete")/anon({"anonid":"tree"})/{"class":"autocomplete-treebody"}');

  controller.keypress(searchField, "VK_DOWN", {});
  controller.sleep(1000);
  controller.click(popDownAutoCompList);

  
  controller.assertValue(searchField, searchText);
}





