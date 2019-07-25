





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PlacesAPI', 'PrefsAPI', 'ToolbarAPI'];

const gTimeout = 5000;
const gDelay = 200;

const testSite = {url : 'http://www.google.com/', string: 'google'};

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();
  module.locationBar =  new ToolbarAPI.locationBar(controller);

  
  try {
    PlacesAPI.historyService.removeAllPages();
  } catch (ex) {}
}





var testFaviconInAutoComplete = function()
{
  
  PrefsAPI.openPreferencesDialog(prefDialogSuggestsCallback);

  
  locationBar.loadURL(testSite.url);
  controller.waitForPageLoad();

  
  var locationBarFaviconUrl = locationBar.getElement({type:"favicon"}).getNode().getAttribute('src');

  
  controller.sleep(4000);

  
  locationBar.clear();

  
  for each (letter in testSite.string) {
    locationBar.type(letter);
    controller.sleep(gDelay);
  }

  
  var richlistItem = locationBar.autoCompleteResults.getResult(0);

  
  controller.waitForEval('subject.isOpened == true', 3000, 100, locationBar.autoCompleteResults);

  
  var listFaviconUrl = richlistItem.getNode().boxObject.firstChild.childNodes[0].getAttribute('src');

  
  controller.assertJS("subject.isSameFavicon == true",
                      {isSameFavicon: richlistItem.getNode().image.indexOf(locationBarFaviconUrl) != -1});
}







var prefDialogSuggestsCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'panePrivacy';

  var suggests = new elementslib.ID(controller.window.document, "locationBarSuggestion");
  controller.waitForElement(suggests);
  controller.select(suggests, null, null, 1);
  controller.sleep(gDelay);

  prefDialog.close(true);
}
