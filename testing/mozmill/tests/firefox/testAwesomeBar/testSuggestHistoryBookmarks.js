





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PlacesAPI', 'PrefsAPI', 'ToolbarAPI'];

var testSite = {url : 'https://litmus.mozilla.org/', string: 'litmus'};

const gTimeout = 5000;
const gDelay = 200;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
  module.locationBar =  new ToolbarAPI.locationBar(controller);

  
  try {
    PlacesAPI.historyService.removeAllPages();
  } catch (ex) {}
}

var teardownModule = function(module)
{
  PlacesAPI.restoreDefaultBookmarks();
}




var testSuggestHistoryAndBookmarks = function()
{
  
  PrefsAPI.openPreferencesDialog(prefDialogSuggestsCallback);

  
  locationBar.loadURL(testSite.url);
  controller.waitForPageLoad();

  
  controller.sleep(4000);

  
  locationBar.clear();

  
  for each (letter in testSite.string) {
    locationBar.type(letter);
    controller.sleep(gDelay);
  }

  
  var richlistItem = locationBar.autoCompleteResults.getResult(0);

  
  controller.waitForEval('subject.isOpened == true', 3000, 100, locationBar.autoCompleteResults);

  var autoCompleteResultsList = locationBar.autoCompleteResults.getElement({type:"results"});
  controller.assertJS("subject.getNumberOfVisibleRows() == 1", autoCompleteResultsList.getNode());

  
  var entries = locationBar.autoCompleteResults.getUnderlinedText(richlistItem, "title");
  for each (entry in entries) {
    controller.assertJS("subject.enteredTitle == subject.underlinedTitle",
                        {enteredTitle: testSite.string, underlinedTitle: entry.toLowerCase()});
  }
}




var testStarInAutocomplete = function()
{
  
  controller.click(new elementslib.Elem(controller.menus.bookmarksMenu.menu_bookmarkThisPage));

  
  controller.waitForEval("subject._overlayLoaded == true", gTimeout, gDelay, controller.window.top.StarUI);
  var doneButton = new elementslib.ID(controller.window.document, "editBookmarkPanelDoneButton");
  controller.click(doneButton);

  
  var richlistItem = locationBar.autoCompleteResults.getResult(0);

  
  try {
    PlacesAPI.historyService.removeAllPages();
  } catch (ex) {}

  
  locationBar.clear();

  
  for each (letter in testSite.string) {
    locationBar.type(letter);
    controller.sleep(gDelay);
  }

  
  controller.waitForEval('subject.isOpened == true', 3000, 100, locationBar.autoCompleteResults);

  var entries = locationBar.autoCompleteResults.getUnderlinedText(richlistItem, "title");
  for each (entry in entries) {
    controller.assertJS("subject.enteredTitle == subject.underlinedTitle",
                        {enteredTitle: testSite.string, underlinedTitle: entry.toLowerCase()});
  }

  
  controller.assertJS("subject.isItemBookmarked == true",
                      {isItemBookmarked: richlistItem.getNode().getAttribute('type') == 'bookmark'});
}







var prefDialogSuggestsCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'panePrivacy';

  var suggests = new elementslib.ID(controller.window.document, "locationBarSuggestion");
  controller.waitForElement(suggests);
  controller.select(suggests, null, null, 0);
  controller.sleep(gDelay);

  prefDialog.close(true);
}
