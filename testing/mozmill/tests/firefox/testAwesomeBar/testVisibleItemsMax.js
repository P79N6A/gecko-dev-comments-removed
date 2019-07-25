




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PlacesAPI', 'PrefsAPI','ToolbarAPI'];

const gTimeout = 5000;
const gDelay = 100;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
  module.locationBar =  new ToolbarAPI.locationBar(controller);

  
  try {
    PlacesAPI.historyService.removeAllPages();
  } catch (ex) {}
}




var testVisibleItemsMax = function()
{
  
  PrefsAPI.openPreferencesDialog(prefDialogSuggestsCallback);

  
  var websites = [
                  'http://www.google.com/',
                  'http://www.mozilla.org',
                  'http://www.mozilla.org/projects/',
                  'http://www.mozilla.org/about/history.html',
                  'http://www.mozilla.org/contribute/',
                  'http://www.mozilla.org/causes/',
                  'http://www.mozilla.org/community/',
                  'http://www.mozilla.org/about/'
                 ];

  
  for each (website in websites) {
    locationBar.loadURL(website);
    controller.waitForPageLoad();
  }

  
  controller.sleep(4000);

  var testString = 'll';

  
  locationBar.clear();

  
  for each (letter in testString) {
    locationBar.type(letter);
    controller.sleep(gDelay);
  }

  
  var autoCompleteResultsList = locationBar.autoCompleteResults.getElement({type:"results"});
  controller.assertJS("subject.getNumberOfVisibleRows() == 6", autoCompleteResultsList.getNode());
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
