





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI', 'PrefsAPI'];

const gDelay = 0;
const gTimeout = 5000;
const gSearchTimeout = 30000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  addonsManager = new AddonsAPI.addonsManager();
}

var teardownModule = function(module)
{
  addonsManager.close();
}




var testSearchForAddons = function() 
{
  addonsManager.open(controller);
  controller = addonsManager.controller;

  addonsManager.search("rss");

  
  var footer = addonsManager.getElement({type: "search_status", subtype: "footer"});
  controller.waitForElement(footer, gSearchTimeout);
  controller.assertProperty(footer, "hidden", false);

  
  var searchButton = addonsManager.getElement({type: "search_fieldButton"});
  var buttonPanel = searchButton.getNode().selectedPanel;
  controller.assertJS("subject.isClearButtonShown == true",
                      {isClearButtonShown: buttonPanel.getAttribute('class') == 'textbox-search-clear'});

  
  var maxResults = PrefsAPI.preferences.getPref("extensions.getAddons.maxResults", -1);
  var listBox = addonsManager.getElement({type: "listbox"});

  addonsManager.controller.assertJS("subject.numSearchResults > 0",
                                    {numSearchResults: listBox.getNode().itemCount});
  addonsManager.controller.assertJS("subject.numSearchResults <= subject.maxResults",
                                    {numSearchResults: listBox.getNode().itemCount,
                                     maxResults: maxResults}
                                   );

  
  var searchField = addonsManager.getElement({type: "search_field"});
  controller.keypress(searchField, "VK_ESCAPE", {});

  buttonPanel = searchButton.getNode().selectedPanel;
  controller.assertJS("subject.isClearButtonShown == true",
                      {isClearButtonShown: buttonPanel.getAttribute('class') != 'textbox-search-clear'});
  controller.assertValue(searchField, "");

  
  controller.waitForElement(footer, gSearchTimeout);
  controller.assertProperty(footer, "hidden", false);

  
  addonsManager.controller.assertJS("subject.numSearchResults > 0",
                                    {numSearchResults: listBox.getNode().itemCount});
  addonsManager.controller.assertJS("subject.numSearchResults <= subject.maxResults",
                                    {numSearchResults: listBox.getNode().itemCount,
                                     maxResults: maxResults}
                                   );
}





