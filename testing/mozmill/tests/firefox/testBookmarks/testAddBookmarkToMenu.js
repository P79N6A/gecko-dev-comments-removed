




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PlacesAPI', 'UtilsAPI'];

const gDelay = 0;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}

var teardownModule = function(module) {
  PlacesAPI.restoreDefaultBookmarks();
}

var testAddBookmarkToBookmarksMenu = function() {
  var uri = UtilsAPI.createURI("http://www.mozilla.org");

  
  controller.assertJS("subject.isBookmarked == false",
                      {isBookmarked: PlacesAPI.bookmarksService.isBookmarked(uri)});

  
  controller.open(uri.spec);
  controller.waitForPageLoad();

  
  controller.click(new elementslib.Elem(controller.menus.bookmarksMenu.menu_bookmarkThisPage));

  
  controller.waitForEval("subject._overlayLoaded == true", 2000, 100, controller.window.top.StarUI);

  
  
  var nameField = new elementslib.ID(controller.window.document, "editBMPanel_namePicker");
  var doneButton = new elementslib.ID(controller.window.document, "editBookmarkPanelDoneButton");

  controller.type(nameField, "Mozilla");
  controller.sleep(gDelay);
  controller.click(doneButton);

  
  
  controller.assertJS("subject.isBookmarkInBookmarksMenu == true",
                      {isBookmarkInBookmarksMenu: PlacesAPI.isBookmarkInFolder(uri, PlacesAPI.bookmarksService.bookmarksMenuFolder)});
}





