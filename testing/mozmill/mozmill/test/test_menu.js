var setupTest = function() {
  controller = mozmill.getBrowserController();

  
  contextMenu = controller.getMenu("#contentAreaContextMenu");
}

var testMenuAPI = function() {
  
  controller.mainMenu.click("#menu_newNavigatorTab");

  controller.open("http://www.mozilla.org");
  controller.waitForPageLoad();

  
  var search = new elementslib.ID(controller.tabs.activeTab, "q");
  controller.type(search, "mozmill");
  contextMenu.select("#context-selectall", search);

  
  contextMenu.open(search);
  var state = contextMenu.getItem("#context-viewimage");
  controller.assert(function() {
    return state.getNode().hidden;
  }, "Context menu entry 'View Image' is not visible");

  
  contextMenu.keypress("VK_DOWN", {});
  contextMenu.keypress("VK_ENTER", {});
  contextMenu.close();

  controller.assert(function() {
    return search.getNode().value == "";
  }, "Text field has been emptied.");
}

