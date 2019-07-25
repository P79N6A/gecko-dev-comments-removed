







































const RELATIVE_ROOT = '../../shared-modules';
const MODULE_REQUIRES = ['PlacesAPI', 'PrefsAPI', 'ToolbarAPI'];

const LOCAL_TEST_FOLDER = collector.addHttpResource('../test-files/');
const LOCAL_TEST_PAGE = { 
  url: LOCAL_TEST_FOLDER + 'layout/mozilla.html',
  string: "mozilla" 
};

var setupModule = function() {
  controller = mozmill.getBrowserController();
  locationBar =  new ToolbarAPI.locationBar(controller);

  
  PlacesAPI.removeAllHistory();
}





var testFaviconInAutoComplete = function() {
  
  PrefsAPI.openPreferencesDialog(prefDialogSuggestsCallback);

  
  locationBar.loadURL(LOCAL_TEST_PAGE.url);
  controller.waitForPageLoad();

  
  var locationBarFaviconUrl = locationBar.getElement({type:"favicon"}).getNode().getAttribute('src');

  
  controller.sleep(4000);

  
  locationBar.clear();

  
  for each (var letter in LOCAL_TEST_PAGE.string) {
    locationBar.type(letter);
    controller.sleep(200);
  }

  
  var richlistItem = locationBar.autoCompleteResults.getResult(0);

  
  controller.waitForEval('subject.isOpened == true', 3000, 100, locationBar.autoCompleteResults);

  
  var listFaviconUrl = richlistItem.getNode().boxObject.firstChild.childNodes[0].getAttribute('src');

  
  controller.assertJS("subject.isSameFavicon == true",
                      {isSameFavicon: richlistItem.getNode().image.indexOf(locationBarFaviconUrl) != -1});
}







var prefDialogSuggestsCallback = function(controller) {
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'panePrivacy';

  var suggests = new elementslib.ID(controller.window.document, "locationBarSuggestion");
  controller.waitForElement(suggests);
  controller.select(suggests, null, null, 1);
  controller.sleep(200);

  prefDialog.close(true);
}
