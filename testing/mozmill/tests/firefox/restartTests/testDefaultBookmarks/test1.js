





































var RELATIVE_ROOT = '../../../shared-modules';
var MODULE_REQUIRES = ['ModalDialogAPI', 'PlacesAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
  module.bs = PlacesAPI.bookmarksService;
  module.hs = PlacesAPI.historyService;
  module.ls = PlacesAPI.livemarkService;
}

var testVerifyDefaultBookmarks = function() {
  var toolbarElemString = "/*[name()='window']/*[name()='deck'][1]" +
                          "/*[name()='vbox'][1]/*[name()='toolbox'][1]" +
                          "/*[name()='toolbar'][3]";
  var elemString = toolbarElemString + "/*[name()='toolbaritem'][1]" +
                   "/*[name()='hbox'][1]/*[name()='hbox'][1]" +
                   "/*[name()='scrollbox'][1]/*[name()='toolbarbutton'][%1]";

  
  var toolbar = new elementslib.XPath(controller.window.document, toolbarElemString);
  controller.assertJSProperty(toolbar, "collapsed", true);

  
  var bookmarksButton = new elementslib.ID(controller.window.document, "bookmarks-menu-button");
  controller.click(bookmarksButton);
  
  var bookmarkBarItem = new elementslib.ID(controller.window.document, "BMB_viewBookmarksToolbar");
  controller.mouseDown(bookmarkBarItem);
  controller.mouseUp(bookmarkBarItem);
  
  
  
  
  
  
  
  
  controller.waitForEval("subject.collapsed == false", gTimeout, 100,
                         toolbar.getNode());

  
  var toolbarNodes = getBookmarkToolbarItems();
  toolbarNodes.containerOpen = true;

  
  controller.assertJS("subject.toolbarItemCount == 3",
                      {toolbarItemCount: toolbarNodes.childCount});

  
  var mostVisited = new elementslib.XPath(controller.window.document,
                                          elemString.replace("%1", "1"));
  controller.assertJSProperty(mostVisited, "label", toolbarNodes.getChild(0).title);

  
  var gettingStarted = new elementslib.XPath(controller.window.document,
                                             elemString.replace("%1", "2"));
  controller.assertJSProperty(gettingStarted, "label", toolbarNodes.getChild(1).title);

  var locationBar = new elementslib.ID(controller.window.document, "urlbar");
  controller.click(gettingStarted);
  controller.waitForPageLoad();

  
  var uriSource = UtilsAPI.createURI(toolbarNodes.getChild(1).uri, null, null);
  var uriTarget = UtilsAPI.createURI(locationBar.getNode().value, null, null);
  controller.assertJS("subject.source.path == subject.target.path",
                      {source: uriSource, target: uriTarget});

  
  var RSS = new elementslib.XPath(controller.window.document, elemString.replace("%1", "3"));
  controller.assertJSProperty(RSS, "label", toolbarNodes.getChild(2).title);

  
  toolbarNodes.containerOpen = false;

  
  var md = new ModalDialogAPI.modalDialog(feedHandler);
  md.start();

  
  controller.rightClick(RSS);
  controller.sleep(100);
  controller.click(new elementslib.ID(controller.window.document, "placesContext_show:info"));
}






function feedHandler(controller) {
  try {
    
    var toolbarNodes = getBookmarkToolbarItems();
    toolbarNodes.containerOpen = true;
    var child = toolbarNodes.getChild(2);

    
	controller.assertJS("subject.isLivemark == true",
	                    {isLivemark: ls.isLivemark(child.itemId)});

    
    var siteLocation = new elementslib.ID(controller.window.document, "editBMPanel_siteLocationField");
    controller.assertValue(siteLocation, ls.getSiteURI(child.itemId).spec);

    var feedLocation = new elementslib.ID(controller.window.document, "editBMPanel_feedLocationField");
    controller.assertValue(feedLocation, ls.getFeedURI(child.itemId).spec);

    
    toolbarNodes.containerOpen = false;
  } catch(e) {
  }

  controller.keypress(null, "VK_ESCAPE", {});
}




function getBookmarkToolbarItems() {
  var options = hs.getNewQueryOptions();
  var query = hs.getNewQuery();

  query.setFolders([bs.toolbarFolder], 1);

  return hs.executeQuery(query, options).root;
}





