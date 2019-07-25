




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PlacesAPI', 'PrefsAPI','ToolbarAPI'];

const gTimeout = 5000;
const gDelay = 200;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
  module.locationBar =  new ToolbarAPI.locationBar(controller);

  
  try {
    var historyService = Cc["@mozilla.org/browser/nav-history-service;1"].
                     getService(Ci.nsINavHistoryService);
    historyService.removeAllPages();
  }
  catch (ex) {}
}




var testCheckItemHighlight = function()
{
  
  PrefsAPI.openPreferencesDialog(prefDialogSuggestsCallback);

  var websites = ['http://www.google.com/', 'about:blank'];

  
  for (var k = 0; k < websites.length; k++) {
    locationBar.loadURL(websites[k]);
    controller.waitForPageLoad();
  }

  
  controller.sleep(4000);

  var testString = "google";

  
  locationBar.clear();

  
  for (var i = 0; i < testString.length; i++) {
    locationBar.type(testString[i]);
    controller.sleep(gDelay);
  }

  
  var richlistItem = locationBar.autoCompleteResults.getResult(0);

  
  controller.waitForEval('subject.isOpened == true', 3000, 100, locationBar.autoCompleteResults);
  
  var entries = locationBar.autoCompleteResults.getUnderlinedText(richlistItem, "title");
  controller.assertJS("subject.underlinedTextCount == 1",
                      {underlinedTextCount: entries.length})
  for each (entry in entries) {
    controller.assertJS("subject.enteredTitle == subject.underlinedTitle",
                        {enteredTitle: testString, underlinedTitle: entry.toLowerCase()});
  }

  
  entries = locationBar.autoCompleteResults.getUnderlinedText(richlistItem, "url");
  controller.assertJS("subject.underlinedUrlCount == 1",
                      {underlinedUrlCount: entries.length})
  for each (entry in entries) {
    controller.assertJS("subject.enteredUrl == subject.underlinedUrl",
                        {enteredUrl: testString, underlinedUrl: entry.toLowerCase()});
  }
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





