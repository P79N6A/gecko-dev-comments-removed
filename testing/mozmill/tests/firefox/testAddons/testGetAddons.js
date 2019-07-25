





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI', 'PrefsAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;
const gSearchTimeout = 30000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  addonsManager = new AddonsAPI.addonsManager();

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var teardownModule = function(module)
{
  addonsManager.close(true);
}




var testLaunchAddonsManager = function()
{
  
  addonsManager.open(controller);

  
  for each (pane in ["search", "extensions", "themes", "plugins"]) {
    addonsManager.paneId = pane;

    
    if (pane == "extensions" || pane == "themes") {
      var updatesButton = addonsManager.getElement({type: "button_findUpdates"});
      UtilsAPI.assertElementVisible(addonsManager.controller, updatesButton, true);
    }
  }

  addonsManager.close();
}




var testGetAddonsTab = function()
{
  addonsManager.open(controller);

  
  addonsManager.paneId = "search";

  var searchField = addonsManager.getElement({type: "search_field"});
  addonsManager.controller.assertProperty(searchField, "hidden", "false");

  var browseAllAddons = addonsManager.getElement({type: "link_browseAddons"});
  addonsManager.controller.assertProperty(browseAllAddons, "hidden", "false");

  var footer = addonsManager.getElement({type: "search_status", subtype: "footer"});
  addonsManager.controller.waitForElement(footer, gSearchTimeout);
  addonsManager.controller.assertProperty(footer, "hidden", false);

  
  var maxResults = PrefsAPI.preferences.getPref("extensions.getAddons.maxResults", -1);
  var listBox = addonsManager.getElement({type: "listbox"});

  addonsManager.controller.assertJS("subject.numSearchResults > 0",
                                    {numSearchResults: listBox.getNode().itemCount});
  addonsManager.controller.assertJS("subject.numSearchResults <= subject.maxResults",
                                    {numSearchResults: listBox.getNode().itemCount,
                                     maxResults: maxResults}
                                   );

  
  
  
  var footerLabel = addonsManager.getElement({type: "search_statusLabel", value: footer});
  var recommendedUrl = UtilsAPI.formatUrlPref("extensions.getAddons.recommended.browseURL");
  addonsManager.controller.assertJS("subject.correctRecommendedURL",
                                    {correctRecommendedURL:
                                     footerLabel.getNode().getAttribute('recommendedURL') == recommendedUrl}
                                   );

  
  var browseAddonUrl = UtilsAPI.formatUrlPref("extensions.getAddons.browseAddons");
  addonsManager.controller.waitThenClick(browseAllAddons, gTimeout);

  
  controller.waitForEval("subject.tabs.length == 2", gTimeout, 100, controller);
  controller.waitForPageLoad();
  UtilsAPI.assertLoadedUrlEqual(controller, browseAddonUrl);
}






